"""
DynaPlex extension for Python
"""
from __future__ import annotations
from dp import save_policy
import numpy
import typing
__all__ = ['MDP', 'Policy', 'PolicyComparer', 'State', 'dcl', 'demonstrator', 'filepath', 'get_comparer', 'get_dcl', 'get_demonstrator', 'get_gym_emulator', 'get_mdp', 'get_sample_generator', 'get_trajectory', 'gym_emulator', 'io_path', 'list_mdps', 'load_policy', 'sample_generator', 'save_policy', 'trajectory']class MDP:
    def allowed_actions(self, trajectory: trajectory) -> list:
        """
        	Returns list of allowed actions for the current state of trajectory.
        """
    def deep_copy(self, trajectory: trajectory) -> trajectory:
        """
        	Returns a deep copy of the trajectory, copying all trajectory variables and cloning the underlying state.
        """
    def deep_copy_and_reinitiate(self, trajectory: trajectory) -> trajectory:
        """
        	Returns a deep copy of the trajectory, copying all trajectory variables and cloning the underlying state, while reinitiating any hidden state variables using the initiation RNG.
        """
    def discount_factor(self) -> float:
        ...
    def get_features(self, trajectory: trajectory) -> numpy.ndarray[numpy.float32]:
        """
        	returns the features corresponding to a trajectory as a 1*N numpy array, with N being the number of features.
        """
    def get_mask(self, trajectory: trajectory) -> numpy.ndarray[bool]:
        """
        	returns the mask corresponding to a trajectory as a 1*M numpy array, with M being the number of valid actions for the MDP.
        """
    @typing.overload
    def get_policy(self, **kwargs) -> Policy:
        """
        Get a policy; supports keyword arguments.
        """
    @typing.overload
    def get_policy(self, id: str) -> Policy:
        """
        Convenience function that calls GetPolicy with the parameter id.
        """
    def get_state_category(self, traj: trajectory) -> dict:
        """
        	Returns state category for a particular state.
        """
    def get_static_info(self) -> dict:
        """
        Gets dictionary representing static information for this MDP, i.e. MDP properties.
        """
    def identifier(self) -> str:
        ...
    @typing.overload
    def incorporate_action(self, trajectory: trajectory) -> None:
        """
        	Incorporates the NextAction into the trajectories. Trajectory
        	must have Category.IsAwaitsAction; throws DynaPlex::Exception otherwise.
        	Note: Assumes trajectory.NextAction is allowed for corresponding trajectory.GetState().
        """
    @typing.overload
    def incorporate_action(self, trajectory: trajectory, policy: Policy) -> None:
        """
        	Incorporates the action selected by the policy into the trajectory. Trajectory in span/vector
        	must have Category.IsAwaitsAction; throws DynaPlex::Exception otherwise.
        	Note: Immediately after the call, NextAction for the trajectory still contains the action taken.
        """
    def incorporate_until_action(self, trajectory: trajectory, max_period_count: int = 9223372036854775807) -> bool:
        """
        	Incorporates events in the provided trajectory until at least one of the following holds:
        	1. Category IsFinal()
        	2. trajectory.PeriodCount>=MaxPeriodCount, and Category.IsAwaitEvent();
        	3. Category.IsAwaitAction()
        	returns true if trajectory is in category 3, false otherwise.
        """
    def incorporate_until_nontrivial_action(self, trajectory: trajectory, max_period_count: int = 9223372036854775807) -> bool:
        """
        	Incorporates events in the provided trajectory until at least one of the following holds:
        	1. Category IsFinal()
        	2. trajectory.PeriodCount>=MaxPeriodCount, and Category.IsAwaitEvent();
        	3. Category.IsAwaitAction(), and there is more than a single action allowed
        	returns true if trajectory is in category 3, false otherwise.
        """
    @typing.overload
    def initiate_state(self, trajectory: trajectory) -> None:
        """
        	Initiates the state in the trajectory. Uses random initial state (GetInitialState(RNG&)) if available, otherwise uses deterministic state (GetInitialState()).
        	Updates the Category in the trajectory to reflect the initial state, and re-initiates PeriodCount, CumulativeReturn, and EffectiveDiscountFactor.
        """
    @typing.overload
    def initiate_state(self, trajectory: trajectory, initial_state_trajectory: trajectory) -> None:
        """
        	Sets the state in the trajectory to a specific state value deep-copied from the initial_state_trajectory, while reinitiating any hidden state variables.
        	Updates the Category in the trajectory, and re-initiates PeriodCount, CumulativeReturn, and EffectiveDiscountFactor.
        """
    @typing.overload
    def initiate_state(self, trajectory: trajectory, state_as_dict: dict) -> None:
        """
        	Sets the state in the trajectory to a specific state value converted from state_as_dict, while reinitiating any hidden state variables.
        	Updates the Category in the trajectory, and re-initiates PeriodCount, CumulativeReturn, and EffectiveDiscountFactor.
        """
    def is_infinite_horizon(self) -> bool:
        """
        indicates whether the MDP is infinite or finite horizon
        """
    def list_policies(self) -> dict:
        """
        Lists key-value pairs (id,description) that represent the available build-in policies for this MDP.
        """
    def num_flat_features(self) -> int:
        ...
    def num_valid_actions(self) -> int:
        ...
    def provides_flat_features(self) -> bool:
        ...
    def run_until_period_count(self, policy: Policy, trajectory: trajectory, max_period_count: int) -> None:
        """
        	The trajectory (which has a current period_count and a cumulative_return) is continued until period_count equals max_period_count, while updating cumulative_return
        """
    def type_identifier(self) -> str:
        ...
class Policy:
    def get_config(self) -> dict:
        """
        Gets dictionary representing static information for this policy, i.e. the parameters it was configured with.
        """
    def set_action(self, trajectory: trajectory) -> None:
        """
        uses policy to set the next_action for the trajectory
        """
    def type_identifier(self) -> str:
        ...
class PolicyComparer:
    def assess(self, arg0: Policy) -> dict:
        ...
    @typing.overload
    def compare(self, first: Policy, second: Policy, index: int = -1) -> list:
        ...
    @typing.overload
    def compare(self, policies: list[Policy], index: int = -1) -> list:
        ...
class State:
    pass
class dcl:
    def get_policies(self) -> list[Policy]:
        ...
    def get_policy(self, generation: int = -1) -> Policy:
        ...
    def train_policy(self) -> None:
        ...
class demonstrator:
    def get_trace(self, mdp: MDP, policy: Policy = None) -> list[dict]:
        """
        gets a trace for demonstration and rendering purposes.
        """
class gym_emulator:
    def action_space_size(self) -> int:
        ...
    def close(self) -> None:
        ...
    def current_state_as_object(self) -> dict:
        """
        returns the current state of the emulator as a dictionary.
        """
    def mdp_identifier(self) -> str:
        ...
    def observation_space_size(self) -> int:
        ...
    def reset(self, **kwargs) -> tuple[tuple[list[float], list[int]], dict]:
        ...
    def step(self, action: int) -> tuple[tuple[list[float], list[int]], float, bool, bool, dict]:
        ...
class sample_generator:
    def generate_samples(self, policy: Policy, file_path: str) -> None:
        """
        Generates samples using policy (default:random) as rollout policy and stores them in a file at file_path.
        """
class trajectory:
    next_action: int
    def awaits_action(self) -> bool:
        ...
    def awaits_event(self) -> bool:
        ...
    def category_index(self) -> int:
        ...
    def is_final(self) -> bool:
        ...
    def seed_rngprovider(self, sample_seed: int = 1073741823, trajectory_seed: int = 4194303) -> None:
        ...
    def state_as_dict(self) -> dict:
        """
        Converts the state underlying this trajectory to a python dictionary. Note: relatively expensive operation.
        """
    @property
    def cumulative_return(self) -> float:
        ...
    @property
    def effective_discount_factor(self) -> float:
        ...
    @property
    def external_index(self) -> int:
        ...
    @property
    def period_count(self) -> int:
        ...
def filepath(*args) -> str:
    """
    Constructs a file path from a list of subdirectories and a filename, which is assumed to be the last element of the list. Creates the directory if not existent, but does not verify or require the existence of the file.
    """
def get_comparer(mdp: MDP, **kwargs) -> PolicyComparer:
    """
    Gets comparer based on MDP and keyword arguments.
    """
def get_dcl(mdp: MDP, policy: Policy = None, **kwargs) -> dcl:
    """
    Returns a class that can be used to run dcl algorithm based on mdp, policy and keyword arguments.
    """
def get_demonstrator(**kwargs) -> demonstrator:
    """
    Gets comparer based on keyword arguments; may provide max_period_count and rng_seed.
    """
def get_gym_emulator(mdp: MDP, **kwargs) -> gym_emulator:
    """
    Gets gym emulator based on MDP; also accepts key word arguments.
    """
def get_mdp(**kwargs) -> MDP:
    """
    Gets MDP based on keyword arguments.
    """
def get_sample_generator(mdp: MDP, **kwargs) -> sample_generator:
    """
    Returns a class that can be used to generate roll-out samples for a specific mdp.
    """
def get_trajectory(externalIndex: int = 0) -> trajectory:
    """
    gets trajectory
    """
def io_path() -> str:
    """
    Gets the path of the dynaplex IO directory.
    """
def list_mdps() -> dict:
    """
    Lists available MDPs
    """
def load_policy(mdp: MDP, path: str) -> Policy:
    """
    loads policy for mdp from path
    """
