"""
This demonstrates the definition of an MDP and a policy in DynaPlex:
1. Defining an MDP in DynaML
2. Defining a policy in DynaML
3. Validating the MDP and policy
4. Training a policy with PPO
"""

from __future__ import annotations

from dataclasses import dataclass

import numpy as np
from numpy.typing import NDArray

from dynaplex import PolicyComparer, PPOTrainerConfig, PPOTrainer, default_rng
from dynaplex.modelling import (
    Validity,
    HorizonType,
    StateCategory,
    TrajectoryContext,
    assert_mdp,
    assert_policy_for_mdp,
    featurizer,
    GlobalStateWriter,
)


# ============================================================================
# MDP Definition
# ============================================================================

@dataclass(slots=True)
class State:
    """
    State representation for the airplane MDP.
    """
    remaining_days: int
    remaining_seats: int
    price_offered_per_seat: int
    # this member must always be defined on any dynaplex MDP state:
    category: StateCategory = StateCategory.AWAIT_EVENT
    

@dataclass(init=False, slots=True)
class AirplaneMDP:
    """
    Airplane ticket selling MDP.
    
    Actions:
        0: Reject customer
        1: Accept customer (sell seat)
    """    
    # MDP configuration (instance attributes, no defaults)
    initial_days: int
    initial_seats: int
    prices_per_customer_type: list[int]
    average_price: float
    customer_type_probs: list[float]
    num_actions: int
    horizon_type: HorizonType
    
    def __init__(
        self,
        initial_days: int,
        initial_seats: int,
        prices_per_customer_type: list[int],
        customer_type_probs: list[float],
    ):
        """
        Initialize the Airplane MDP with validation.
        
        Args:
            initial_days: Number of days in selling period (must be > 0)
            initial_seats: Flight capacity (must be > 0)
            prices_per_customer_type: List of prices for each customer type (all must be > 0)
            customer_type_probs: Probability distribution over customer types (must sum to 1.0)
            
        Raises:
            ValueError: If any validation checks fail
        """
        # NOTE:  __init__ on the MDP class itself is never called by the dynaplex compiler, so we can use any valid cpython code here
        #  but the class must be a dataclass, and other functions need to be valid  DynaML code.  
        
        # Validating parameters
        assert initial_days > 0 and initial_seats > 0
        assert prices_per_customer_type and all(price > 0 for price in prices_per_customer_type)
        assert customer_type_probs and len(customer_type_probs) == len(prices_per_customer_type)
        assert all(prob >= 0 for prob in customer_type_probs) and np.isclose(sum(customer_type_probs), 1.0, atol=1e-6)
        
        # Set attributes
        #NOTE: ensure all attributes are set that are part of the annotation!
        self.initial_days = initial_days
        self.initial_seats = initial_seats
        self.prices_per_customer_type = prices_per_customer_type
        self.average_price = sum(prices_per_customer_type) / len(prices_per_customer_type)
        self.customer_type_probs = customer_type_probs

        # number of actions in the MDP that are potentially valid. 
        self.num_actions = 2  # 0: Reject, 1: Accept
        self.horizon_type = HorizonType.FINITE

        # Discover the number of features; should be called last in __init__. 
    
    def get_initial_state(self, context: TrajectoryContext) -> State:
        """
        Generates and returns an initial state of the MDP.
        
        Args:
            context.rng: NumPy random generator to support random initial state.
        """
        # NOTE: function get_initial_state and any functions that it calls must be valid DynaML code.
        return State(
            remaining_days=self.initial_days,
            remaining_seats=self.initial_seats,
            price_offered_per_seat=0,
            category=StateCategory.AWAIT_EVENT,
        )
    
    def modify_state_with_event(self, state: State, context: TrajectoryContext) -> None:
        """
        Generate a (customer arrival) event and modify state in place.
       
        Args:
            state: Current state (modified in place)
            context: Trajectory context containing rng and cumulative_cost
        """
        # NOTE: function modify_state_with_event and any functions that it calls must be valid DynaML code.

        # rng Generator -> modern/recommended approach to generate random numbers in numpy. 
        # rng.choice == np.random.choice:
        state.price_offered_per_seat = context.rng.choice(
           self.prices_per_customer_type,
           p=self.customer_type_probs,
        )
        
        # Next, the agent must decide whether to accept or reject the customer.
        state.category = StateCategory.AWAIT_ACTION        
        # time elapsed increases by 1 (do this in every modify_state_with_event unless you know what you are doing):
        context.time_elapsed += 1
    
    def modify_state_with_action(self, state: State, context: TrajectoryContext, action: int) -> None:
        """
        Apply an action to the state (modify in place).
        
        Args:
            state: Current state (modified in place)
            context: Trajectory context containing cumulative_cost (updated in place)
            action: Action to apply to the state
        """
        # NOTE: this function (and any functions that it calls) must be valid DynaML code.

        # NOTE: do _not_ attempt to generate random numbers here. Any random transitions must happen 
        # in modify_state_with_event, using the rng parameter passed in there. 
        
        assert state.remaining_days > 0 
        state.remaining_days -= 1

       
        if action == 0:
            # Reject customer
            # No cost, so cumulative_cost remains unchanged
            state.price_offered_per_seat = 0
        
        elif action == 1:
            assert state.remaining_seats > 0, "Cannot accept customer: no seats available"
            # Accept customer ; sell the seat:
            state.remaining_seats -= 1            
            # Use a cost-based formulation (cost = -reward),    
            # hence we should update cumulative cost as follows:
            context.cumulative_cost -= state.price_offered_per_seat
            # Reset the price offered per seat to 0, awaiting the next event. 
            state.price_offered_per_seat = 0
        
        else:
            assert False, f"Invalid action: Must be 0 (reject) or 1 (accept)"

         # After processing action, we await the next event - customer arrival. 
        if state.remaining_days == 0:
            state.category = StateCategory.FINAL
        else:
            state.category = StateCategory.AWAIT_EVENT
    
    
    def write_action_validity(self, state: State, valid: Validity) -> None:
        """
        Write action validity: valid[i] = True  if action i is allowed in the current state
                               valid[i] = False otherwise. 
        
        Args:
            state: Current state
            valid: Boolean array of length num_actions to write the validity mask to
        """
        # NOTE: function write_action_validity must be valid DynaML code.
        valid.set(0, True)                       # Reject is always allowed. 
        valid.set(1, state.remaining_seats > 0)  # Can accept only if seats available
        
        # at least one action must be valid. When in doubt, consider adding:
        # assert np.any(valid), "No valid actions"


# ============================================================================
# Policy Definition
# ============================================================================

@dataclass(slots=True)
class SimplePolicy:
    """
    Simple rule-based policy for the airplane MDP. This policy adheres to the DynaPlex DSL. 
    
    This policy uses threshold-based rules to decide when to accept or reject customers.   
 
    """
    mdp: AirplaneMDP
    seat_threshold: int = 5
    days_threshold: int = 9
    min_price_low_days: int = 2000
    min_price_high_days: int = 3000
    
    def get_action(self, state: State) -> int:
        """
        Determine which action to take given the current state. Simple heuristic policy.


        # NOTE: this function must be valid DynaML code.
        Args:
            state: Current state
            
        Returns:
            Action (0=reject, 1=accept)
        """
        if state.remaining_seats == 0:
            return 0
        
        # Rule 1: More than seat_threshold seats left
        elif state.remaining_seats > self.seat_threshold:
            return 1
        
        # Rule 2: 1-seat_threshold seats and <= days_threshold remaining
        elif state.remaining_days <= self.days_threshold and state.price_offered_per_seat >= self.min_price_low_days:
            return 1
        
        # Rule 3: 1-seat_threshold seats and > days_threshold remaining
        elif state.remaining_days > self.days_threshold and state.price_offered_per_seat >= self.min_price_high_days:
            return 1
        else:
            return 0


# ============================================================================
# Featurizer — state representation OUTSIDE the MDP
# ============================================================================

@featurizer
@dataclass(slots=True)
class AirplaneFeaturizer:
    """Featurizer: writer fields declare the representation, write_features fills one
    batch row through them. @featurizer derives the FeatureHolder class (attached as
    AirplaneFeaturizer.Holder) and synthesizes the install/reset/finish field-walks —
    hand-writing them remains possible."""
    mdp: AirplaneMDP
    v: GlobalStateWriter

    def write_features(self, state: State) -> None:
        # NOTE: must be valid DynaML code.
        self.v.append(state.remaining_days / self.mdp.initial_days)
        self.v.append(state.remaining_seats / self.mdp.initial_seats)
        self.v.append(state.price_offered_per_seat / self.mdp.average_price)


# ============================================================================
# Validation: Manual Simulation
# ============================================================================

def simulate_episode(mdp: AirplaneMDP, policy: SimplePolicy, *, seed: int = 42) -> None:
    """
    Simulate a single episode to validate MDP implementation.
    
    Useful for debugging and validating your MDP before training.
    """     
    context = TrajectoryContext(rng=default_rng(seed))
    state = mdp.get_initial_state(context)
    
    step = 0
    print("=" * 80)
    print("DETAILED SIMULATION (Single Episode for MDP & policy validation)")
    print(f"Initial state: {state}")
    print("-" * 80)
    
    while state.category != StateCategory.FINAL:
        if state.category == StateCategory.AWAIT_EVENT:
            mdp.modify_state_with_event(state, context)
            print(f"  State after event: {state}")
            
        elif state.category == StateCategory.AWAIT_ACTION:
            # Apply policy and set action on context
            action = policy.get_action(state)
            mdp.modify_state_with_action(state, context, action)
            print(f"Step {step}: ACTION {action} -> State after action: {state}")
            step += 1
        
        else:
            raise RuntimeError(f"Unexpected state category: {state.category}")
    
    print("-" * 80)
    print(f"Episode finished: {step} steps, total revenue: €{-context.cumulative_cost:.0f}")
    
  

def main() -> None:
    """Run airplane MDP simulation example."""
    # Create MDP with standard configuration
    mdp = AirplaneMDP(
        initial_days=25,
        initial_seats=10,
        prices_per_customer_type=[3000, 2000, 1000],
        customer_type_probs=[0.4, 0.3, 0.3],
    )
    
    # Create policy with default parameters
    policy = SimplePolicy(mdp=mdp)	


    # no-op functionthat makes pyright verify that MDP satisfies the MDPProtocol interface.
    assert_mdp(mdp)
    assert_policy_for_mdp(mdp, policy)
    # Run single simulation with detailed output
    simulate_episode(mdp, policy, seed=42)
    
    
    
    num_simulations = 10000
    print("\n" + "=" * 80)
    print(f"PERFORMANCE EVALUATION ({num_simulations} Episodes)")
    print("=" * 80)

    # PolicyComparer runs the episodes on the compiled engine (falling back to
    # plain Python automatically if a policy cannot be compiled).
    comparer = PolicyComparer(mdp, number_of_trajectories=num_simulations, seed=0)
    assessment = comparer.assess(policy)

    # Costs are negative revenues, so profit = -cost.
    print(f"Number of simulations: {num_simulations}")
    print(f"Average profit: €{-assessment.mean:.2f}")
    print(f"Standard error of the mean: €{assessment.error:.2f}")
    print(f"Min profit: €{-np.max(assessment.returns):.2f}")
    print(f"Max profit: €{-np.min(assessment.returns):.2f}")
    print("=" * 80)


# ============================================================================
# PPO Training Example
# ============================================================================

def train_ppo_airplane() -> None:
    """Train a PPO policy for the airplane MDP."""
    # Create MDP
    initial_days = 25
    mdp = AirplaneMDP(
        initial_days=initial_days,
        initial_seats=10,
        prices_per_customer_type=[3000, 2000, 1000],
        customer_type_probs=[0.4, 0.3, 0.3],
    )
    
    # Create baseline policy for comparison
    policy = SimplePolicy(mdp=mdp)
    
    # Configure PPO trainer
    config = PPOTrainerConfig(
        seed=42,
        device="cpu",
        hidden_sizes=(128, 128),
        num_envs=100,
        total_timesteps=100000,
        num_steps=2 * initial_days,
        minibatch_size=64,
        lr=2.5e-4,
        logdir=None,  # Defaults to log/<MDP_class_name>/ppo
    )
    
    # Train policy
    ppo_trainer = PPOTrainer(mdp=mdp, config=config, features=AirplaneFeaturizer)
    load_policy = False
    if load_policy:
        print("Loading previously trained policy...")
        trained_policy = ppo_trainer.load_trained_policy()
        print("Policy loaded successfully!")
    else:
        print("Training policy...")
        trained_policy = ppo_trainer.train()
        print("Training completed!")
    
    # Compare with baseline
    print("=" * 80)
    print("POLICY COMPARISON (Note: PPO does not easily beat the simple policy in this example)")
    print("=" * 80)
    # Both policies see the same random streams (common random numbers), so the
    # delta columns are paired differences with tight standard errors.
    comparer = PolicyComparer(mdp, number_of_trajectories=1000, seed=0)
    results = comparer.compare({"simple rule": policy, "trained (PPO)": trained_policy})
    print(results)
    print("=" * 80)



if __name__ == "__main__":
    main()
    # once a model is validated, you could consider training a policy:
    train_ppo_airplane()
