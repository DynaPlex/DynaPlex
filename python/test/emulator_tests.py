import os
import sys
import numpy as np
import pytest

from dp import dynaplex


@pytest.fixture
def mdp():
    return dynaplex.get_mdp(id="lost_sales", p=9.0, h=1.0, leadtime=3, demand_dist={"type": "poisson", "mean": 3.0})


@pytest.fixture
def emulator(mdp):
    return dynaplex.get_gym_emulator(mdp, num_actions_until_done=10, seed=12)


def test_action_space_size(emulator):
    assert emulator.action_space_size() > 0, "Action space size should be greater than 0."


def test_observation_space_size(emulator):
    assert emulator.observation_space_size() > 0, "Observation space size should be greater than 0."


def test_reset(emulator):
    obs, info = emulator.reset(seed=0)
    assert isinstance(info, dict), "info should be a dictionary"
    assert isinstance(obs, tuple), "obs should be a tuple."
    assert len(obs) == 2, "Reset should return a tuple of length 2."
    feats, action_mask = obs
    assert isinstance(feats, list), "Features should be a list."
    assert isinstance(action_mask, list), "Action mask should be a list."
    assert all(isinstance(action, int) for action in action_mask), "Action mask should contain integers."


def test_step(emulator):
    obs, info = emulator.reset(seed=0)
    feats, action_mask = obs
    terminated = False
    truncated = False
    total_reward = 0
    total_steps = 0
    while not (terminated or truncated):
        total_steps += 1
        assert total_steps < 1000 , "Infinite loop"
        valid_actions = [i for i, valid in enumerate(action_mask) if valid == 1]
        assert valid_actions, "There should be at least one valid action."
        action = np.random.choice(valid_actions)
        obs, reward, terminated, truncated, info = emulator.step(action)
        assert isinstance(obs, tuple), "obs should be a tuple."

        feats, action_mask = obs
        total_reward += reward

        assert isinstance(reward, float), "Reward should be a float."
        assert isinstance(terminated, bool), "Terminated should be a boolean."
        assert isinstance(truncated, bool), "Truncated should be a boolean."
        assert isinstance(info, dict), "Info should be a dictionary."
        assert isinstance(feats, list), "Features should be a list."
        assert isinstance(action_mask, list), "Action mask should be a list."

        # Assuming that we want to test if the emulator can handle multiple steps
        if not terminated:
            assert action_mask, "Action mask should not be empty after steps."
