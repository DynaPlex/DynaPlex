"""
DynaPlex extension for Python
"""
from __future__ import annotations
from dp import save_policy
import typing
__all__ = ['MDP', 'Policy', 'PolicyComparer', 'dcl', 'filepath', 'get_comparer', 'get_dcl', 'get_gym_emulator', 'get_mdp', 'get_sample_generator', 'gym_emulator', 'io_path', 'list_mdps', 'load_policy', 'sample_generator', 'save_policy']
class MDP:
    def discount_factor(self) -> float:
        ...
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
    def get_static_info(self) -> VarGroup:
        ...
    def identifier(self) -> str:
        ...
    def is_infinite_horizon(self) -> bool:
        ...
    def list_policies(self) -> VarGroup:
        ...
    def num_flat_features(self) -> int:
        ...
    def num_valid_actions(self) -> int:
        ...
    def provides_flat_features(self) -> bool:
        ...
    def type_identifier(self) -> str:
        ...
class Policy:
    def get_config(self) -> VarGroup:
        ...
    def type_identifier(self) -> str:
        ...
class PolicyComparer:
    def assess(self, arg0: Policy) -> VarGroup:
        ...
    @typing.overload
    def compare(self, first: Policy, second: Policy, index: int = -1) -> list[VarGroup]:
        ...
    @typing.overload
    def compare(self, policies: list[Policy], index: int = -1) -> list[VarGroup]:
        ...
class dcl:
    def get_policies(self) -> list[Policy]:
        ...
    def get_policy(self, generation: int = -1) -> Policy:
        ...
    def train_policy(self) -> None:
        ...
class gym_emulator:
    def action_space_size(self) -> int:
        ...
    def close(self) -> None:
        ...
    def current_state_as_object(self) -> VarGroup:
        ...
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
def filepath(*args) -> str:
    """
    Constructs a file path from a list of subdirectories and a filename, which is assumed to be the last element of the list. Creates the directory if not existent, but does not verify or require the existence of the file.
    """
def get_comparer(mdp: MDP, **kwargs) -> PolicyComparer:
    """
    Gets comparer based on MDP and keyword arguments.
    """
def get_dcl(mdp: MDP, policy: Policy = None, **kwargs) -> ...:
    """
    Returns a class that can be used to run dcl algorithm based on mdp, policy and keyword arguments.
    """
def get_gym_emulator(mdp: MDP, **kwargs) -> ...:
    """
    Gets comparer based on MDP and keyword arguments.
    """
def get_mdp(**kwargs) -> MDP:
    """
    Gets MDP based on keyword arguments.
    """
def get_sample_generator(mdp: MDP, **kwargs) -> ...:
    """
    Returns a class that can be used to generate roll-out samples for a specific mdp.
    """
def io_path() -> str:
    """
    Gets the path of the dynaplex IO directory.
    """
def list_mdps() -> VarGroup:
    """
    Lists available MDPs
    """
def load_policy(mdp: MDP, path: str) -> Policy:
    """
    loads policy for mdp from path
    """