import os
import sys
import numpy as np

from dp import dynaplex

mdp = dynaplex.get_mdp(id="lost_sales",
                   p=9.0, h=1.0,
                   leadtime=3,
                   demand_dist={"type": "poisson", "mean": 3.0})

emulator = dynaplex.get_gym_emulator(mdp,num_actions_until_done=10, seed=12)

#these might be convenient.
emulator.action_space_size()
emulator.observation_space_size()

for tries in range(3):
    print("reset")
    obs, info = emulator.reset(seed=tries)
    feats, action_mask = obs
    done = False
    truncated = False
    total_reward = 0

    while not (done or truncated):
        valid_actions = [i for i, valid in enumerate(action_mask) if valid == 1]
        if not valid_actions:
            raise ValueError("No valid actions available!")
        action = np.random.choice(valid_actions)
        obs, reward, done, truncated, info = emulator.step(action)
        feats, action_mask = obs
        total_reward += reward
        print(obs)