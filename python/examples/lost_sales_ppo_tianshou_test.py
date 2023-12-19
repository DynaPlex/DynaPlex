import torch
import os
import json
import numpy as np
import sys
import tianshou

parent_directory = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.append(parent_directory)
# noinspection PyUnresolvedReferences
from dp.loader import DynaPlex as dp
from gym.base_env import BaseEnv
sys.path.remove(parent_directory)

load_mdp_from_file = False

if load_mdp_from_file:
    # This script assumes the desired mdp characteristics are specified in a file with a name of type mdp_config_{MDP_VERSION_NUMBER}.json
    folder_name = "lost_sales"  # the name of the folder where the json file is located
    mdp_version_number = 1
    # this returns path/to/IO_DynaPlex/mdp_config_examples/lost_sales/mdp_config_[..].json:
    path_to_json = dp.filepath("mdp_config_examples", folder_name, f"mdp_config_{mdp_version_number}.json")

    # Global variables used to initialize the experiment (notice the parsed json file should not contain any commented line)
    try:
        with open(path_to_json, "r") as input_file:
            vars = json.load(input_file)    # vars can be initialized manually with something like
    except FileNotFoundError:
        raise FileNotFoundError(f"File {path_to_json} not found. Please make sure the file exists and try again.")
    except:
        raise Exception("Something went wrong when loading the json file. Have you checked the json file does not contain any comment?")
else:
    # Of course, we can also just initiate using a (possibly nested) dict:
    vars = {
        "id": "lost_sales",
        "p": 4.0,
        "h": 1.0,
        "leadtime": 3,
        "discount_factor": 1.0,
        "demand_dist": {
            "type": "poisson",
            "mean": 4.0
        }
    }

mdp = dp.get_mdp(**vars)

device = 'cpu'

def preprocess(obs):
    """
    Observations contain the mask as part of a dictionary.
    This function ensures that the data gathered in training and testing are in the correct format.
    """
    return [{"obs": torch.from_numpy(obs['obs']).to(torch.float).to(device=device),
             "mask": torch.from_numpy(obs['mask']).to(torch.bool).to(device=device)}]


def policy_path():
    path = os.path.normpath(dp.filepath(mdp.identifier(), "ppo_policy"))
    return path


model = torch.jit.load(f"{policy_path()}.pth")

env = BaseEnv(mdp, 1000)

rewards = []

for step in range(100):
    observation, info = env.reset()
    tot_reward = 0
    terminated = truncated = False
    while not (terminated or truncated):

        observation = tianshou.data.Batch(preprocess(observation))

        action = model(observation)
        observation, reward, terminated, truncated, info = env.step(action.argmax(dim=1))

        # print(reward)
        # print(action)

        tot_reward += reward

    rewards.append(tot_reward)

print(f"{np.mean(rewards)} +- {np.std(rewards)}")

