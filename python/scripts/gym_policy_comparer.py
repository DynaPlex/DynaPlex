import torch
import os
import json
import numpy as np
import sys

from torch.optim.lr_scheduler import ExponentialLR, LinearLR

from dp import dynaplex

# This script assumes the desired mdp characteristics are specified in a file with a name of type mdp_config_{MDP_VERSION_NUMBER}.json
folder_name = "lost_sales" # the name of the folder where the json file is located
mdp_version_number = 2
path_to_json = dynaplex.filepath("mdp_config_examples", folder_name, f"mdp_config_{mdp_version_number}.json")

# Global variables used to initialize the experiment (notice the parsed json file should not contain any commented line)
try:
    with open( path_to_json , "r" ) as input_file:
        vars = json.load(input_file) #vars can be initialized manually with something like 
except FileNotFoundError:
    raise FileNotFoundError(f"File {path_to_json} not found. Please make sure the file exists and try again.")
except:
    raise Exception("Something went wrong when loading the json file. Have you checked the json file does not contain any comment?")

mdp = dynaplex.get_mdp(**vars)

def dcl_policy(gen):
    dcl_filename = f'dcl_python_{gen}'
    dcl_load_path = dynaplex.filepath(mdp.identifier(), dcl_filename)
    return dynaplex.load_policy(mdp, dcl_load_path)

def ppo_policy():
    ppo_filename = 'ppo_policy'
    ppo_load_path = dynaplex.filepath(mdp.identifier(), ppo_filename)
    return dynaplex.load_policy(mdp, ppo_load_path)

policies = []
policies.append(mdp.get_policy("base_stock"))
policies.append(ppo_policy())
policies.append(dcl_policy(1))

comparer = dynaplex.get_comparer(mdp)
comparison = comparer.compare(policies)
result = [(item['policy']['id'],item['mean'], item['error']) for item in comparison]

print(result)
