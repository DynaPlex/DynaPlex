"""
Bin Packing MDP example in DynaML - the dynamic modelling language for DynaPlex.
Includes:
- BinPackingMDP: The MDP class
- LowestWeightPolicy: A simple heuristic policy that assigns the incoming weight to the bin with the lowest current weight
- FirstFitPolicy: A simple heuristic policy that assigns the incoming weight to the first bin that can accommodate it without overflow
- State: The state class
"""
from dataclasses import dataclass

import numpy as np
from numpy.typing import NDArray

from dynaplex.modelling import (
    AliasSampler,
    assert_mdp,
    assert_policy_for_mdp,
    const_dataclass,
    DiscreteDist,
    featurizer,
    GlobalStateWriter,
    HorizonType,
    StateCategory,
    TrajectoryContext,
    Validity,
)


@dataclass(slots=True)
class State:
    """
    State representation for the bin packing MDP.
    """
    weight_vector: NDArray[np.int64]  # Current weight in each bin
    upcoming_weight: int              # Weight to be assigned
    # This member must always be defined on any dynaplex MDP state:
    category: StateCategory = StateCategory.AWAIT_EVENT


@const_dataclass(init=False, slots=True)
class BinPackingMDP:
    """
    Bin Packing MDP is an infinite horizon online bin packing problem. 

    In this problem, weights are revealed one by one and must be assigned to one of several bins.
    When a bin exceeds max_bin_size, any overflow weight incurs a cost.
    The goal is to minimize total overflow cost across the episode.
    
    Actions:
        0 to number_of_bins-1: Assign weight to the corresponding bin
    """
    max_bin_size: int
    number_of_bins: int
    weight_dist: DiscreteDist         # Distribution of arriving weights
    weight_sampler: AliasSampler      # O(1) repeated-draw sampler over weight_dist
    num_actions: int
    horizon_type: HorizonType


    def __init__(
        self,
        max_bin_size: int,
        number_of_bins: int,
        weight_dist: DiscreteDist,
    ):
        """
        Initialize the Bin Packing MDP with validation.

        Args:
            max_bin_size: Maximum weight a bin can hold before overflow (must be > 0)
            number_of_bins: Number of bins available (must be > 0)
            weight_dist: Distribution of the arriving weights (non-negative support);
                         non-consecutive weight values are expressed as zero-probability
                         gaps in the PMF (e.g. DiscreteDist.custom(probs, offset=10))

        Raises:
            AssertionError: If any validation checks fail
        """
        # NOTE: __init__ on the MDP class itself is never called by the dynaplex compiler,
        # so we can use any valid cpython code here, but the class must be a dataclass,
        # and other functions need to be valid DynaML code.

        # Validating parameters
        assert max_bin_size > 0, "max_bin_size must be positive"
        assert number_of_bins > 0, "number_of_bins must be positive"
        assert weight_dist.min() >= 0, "weights must be non-negative"

        # Set attributes
        # NOTE: only set attributes that are part of the annotation.
        self.max_bin_size = max_bin_size
        self.number_of_bins = number_of_bins
        self.weight_dist = weight_dist
        self.weight_sampler = weight_dist.alias_sampler()
        # Number of actions in the MDP that are potentially valid
        self.num_actions = number_of_bins
        # Horizon type for this MDP
        self.horizon_type = HorizonType.INFINITE


    def get_initial_state(self, context: TrajectoryContext) -> State:
        """
        Generates and returns an initial state of the MDP.
        """
        return State(
            weight_vector=np.zeros(self.number_of_bins, dtype=np.int64),
            upcoming_weight=0,
            category=StateCategory.AWAIT_EVENT,
        )


    def modify_state_with_event(self, state: State, context: TrajectoryContext) -> None:
        """
        Generate a weight arrival event and modify state in place.
        
        Args:
            state: Current state (modified in place)
            context: Trajectory context containing rng and cumulative_cost
        """
        # Sample a weight from the distribution
        state.upcoming_weight = self.weight_sampler.sample(context.rng)


        # Next, the agent must decide which bin to assign the weight to
        state.category = StateCategory.AWAIT_ACTION
        # time elapsed increases by 1
        context.time_elapsed += 1


    def modify_state_with_action(self, state: State, context: TrajectoryContext, action: int) -> None:
        """
        Apply an action to the state (modify in place).
        
        Args:
            state: Current state (modified in place)
            context: Trajectory context containing cumulative_cost (updated in place)
            action: Bin index to assign the weight to (0 to number_of_bins-1)
        """
        # NOTE: do _not_ attempt to generate random numbers here; that is what modify_state_with_event is for.       
        
        assert action >= 0 and action < self.number_of_bins, f"Invalid action: {action}"
        # Assign weight to the selected bin
        state.weight_vector[action] += state.upcoming_weight
        
        # Calculate overflow amount
        diff = state.weight_vector[action] - self.max_bin_size
        
        if diff >= 0:
            # Bin overflows: incur cost and dispatch (empty) the bin
            context.cumulative_cost += float(diff)
            state.weight_vector[action] = 0
        
        # Reset upcoming weight
        state.upcoming_weight = 0
        
        # Transition back to await next event (infinite horizon - never reaches FINAL)
        state.category = StateCategory.AWAIT_EVENT


    def write_action_validity(self, state: State, valid: Validity) -> None:
        """
        Write action validity: valid[i] = True if action i is allowed in the current state
                               valid[i] = False otherwise.
        
        Args:
            state: Current state
            valid: Boolean array of length num_actions to write the validity mask to
        """
        # All bins are always valid actions in this problem. write_action_validity must write EVERY
        # entry (the contract is "valid[i] = True if allowed, False otherwise" — the caller's buffer
        # is not pre-initialised), so set them all explicitly rather than relying on a default.
        for i in range(self.number_of_bins):
            valid.set(i, True)


@const_dataclass(slots=True)
class LowestWeightPolicy:
    """
    Simple heuristic policy for the bin packing MDP.
    
    This policy always assigns the incoming weight to the bin with the lowest current weight.
    """
    mdp: BinPackingMDP
    
    def get_action(self, state: State) -> int:
        """
        Assigns weight to the bin with the lowest current weight.
        
        Args:
            state: Current state
            
        Returns:
            Action (bin index to assign weight to)
        """
        # NOTE: this function must be valid DynaML code.
        
        # Find bin with minimum weight
        min_weight = state.weight_vector[0]
        min_index = 0
        
        for i in range(1, len(state.weight_vector)):
            if state.weight_vector[i] < min_weight:
                min_weight = state.weight_vector[i]
                min_index = i
        
        return min_index


@const_dataclass(slots=True)
class FirstFitPolicy:
    """
    First-fit heuristic policy.
    
    Assigns weight to the first bin that can accommodate it without overflow.
    If no bin can accommodate it, assigns to the first bin.
    """
    mdp: BinPackingMDP
    
    def get_action(self, state: State) -> int:
        for i in range(len(state.weight_vector)):
            if state.weight_vector[i] + state.upcoming_weight <= self.mdp.max_bin_size:
                return i        
        # If no bin can accommodate, use first bin
        return 0

@featurizer
@dataclass(slots=True)
class BinPackingFeaturizer:
    """Default featurizer for BinPackingMDP (@featurizer derives the holder and
    synthesizes install/reset/finish — featurizers.md section 13)."""
    mdp: BinPackingMDP
    v: GlobalStateWriter

    def write_features(self, state: State) -> None:
        self.v.extend(state.weight_vector)
        self.v.append(state.upcoming_weight)


# Plumbing, safe to skip: @featurizer already attached the derived holder class as
# an attribute (<Featurizer>.Holder); this line just gives it an importable
# module-level name, so other files can `from ... import <Name>FeaturizerHolder`
# and use it in type annotations. dcl() never needs this — it finds the holder
# by itself. Only add such an alias when other code imports your holder by name.
BinPackingFeaturizerHolder = BinPackingFeaturizer.Holder
