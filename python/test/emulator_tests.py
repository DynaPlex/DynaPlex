import os
import sys
import numpy as np
import pytest

# Assuming the dp module and DynaPlex are available in the parent directory
parent_directory = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.append(parent_directory)

# noinspection PyUnresolvedReferences
from dp.loader import DynaPlex as dp
sys.path.remove(parent_directory)

@pytest.fixture
def mdp():
    return dp.get_mdp(id="lost_sales", p=9.0, h=1.0, leadtime=3, demand_dist={"type": "poisson", "mean": 3.0})

@pytest.fixture
def emulator(mdp):
    return dp.get_gym_emulator(mdp, num_actions_until_done=10, seed=12)

def test_action_space_size(emulator):
    assert emulator.action_space_size() > 0, "Action space size should be greater than 0."

def test_observation_space_size(emulator):
    assert emulator.observation_space_size() > 0, "Observation space size should be greater than 0."

def test_reset(emulator):
    obs = emulator.reset(seed=0)
    assert isinstance(obs, tuple), "Reset should return a tuple."
    assert len(obs) == 2, "Reset should return a tuple of length 2."
    feats, action_mask = obs
    assert isinstance(feats, list), "Features should be a list."
    assert isinstance(action_mask, list), "Action mask should be a list."
    assert all(isinstance(action, int) for action in action_mask), "Action mask should contain integers."

def test_step(emulator):
    obs = emulator.reset(seed=0)
    feats, action_mask = obs
    done = False
    total_reward = 0

    while not done:
        valid_actions = [i for i, valid in enumerate(action_mask) if valid == 1]
        assert valid_actions, "There should be at least one valid action."
        action = np.random.choice(valid_actions)
        obs, reward, done, info = emulator.step(action)
        feats, action_mask = obs
        total_reward += reward
        assert isinstance(reward, float), "Reward should be a float."
        assert isinstance(done, bool), "Done should be a boolean."
        assert isinstance(info, dict), "Info should be a dictionary."
        assert isinstance(feats, list), "Features should be a list."
        assert isinstance(action_mask, list), "Action mask should be a list."

        # Assuming that we want to test if the emulator can handle multiple steps
        if not done:
            assert action_mask, "Action mask should not be empty after steps."
