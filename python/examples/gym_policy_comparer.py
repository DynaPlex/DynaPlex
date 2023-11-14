import torch
import os
import json
import numpy as np
import sys

from torch.optim.lr_scheduler import ExponentialLR, LinearLR

parent_directory = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.append(parent_directory)
# noinspection PyUnresolvedReferences
from dp.loader import DynaPlex as dp
sys.path.remove(parent_directory)

# This script assumes the desired mdp characteristics are specified in a file with a name of type mdp_config_{MDP_VERSION_NUMBER}.json
folder_name = "lost_sales" # the name of the folder where the json file is located
mdp_version_number = 2

path_to_json = os.path.join(os.path.dirname(__file__), "..", "..", "src", "lib", "models", "models", folder_name, f"mdp_config_{mdp_version_number}.json")

# Global variables used to initialize the experiment (notice the parsed json file should not contain any commented line)
try:
    with open( path_to_json , "r" ) as input_file:
        vars = json.load(input_file) #vars can be initialized manually with something like 
except FileNotFoundError:
    raise FileNotFoundError(f"File {path_to_json} not found. Please make sure the file exists and try again.")
except:
    raise Exception("Something went wrong when loading the json file. Have you checked the json file does not contain any comment?")

mdp = dp.get_mdp(**vars)

dcl_path = os.path.normpath(f"{dp.io_path()}/dcl/{mdp.identifier()}")
ppo_path = os.path.normpath(f"{dp.io_path()}/ppo")
dcl_filename = 'best_lost_sales_nn'
ppo_filename = 'ppo'
gen_n = 1
policies = []

#load dcl
dcl_load_path = os.path.normpath(f'{dcl_path}/{dcl_filename}_{gen_n}')
dcl_policy = dp.load_policy(mdp, dcl_load_path)
policies.append(dcl_policy)

#load ppo
ppo_load_path = os.path.normpath(f'{ppo_path}/{ppo_filename}')
ppo_policy = dp.load_policy(mdp, ppo_load_path)
policies.append(ppo_policy)

comparer = dp.get_comparer(mdp, number_of_trajectories=100, periods_per_trajectory=1000)
comparison = comparer.compare(policies)
result = [(item['mean'], item['error']) for item in comparison]

print(result)
