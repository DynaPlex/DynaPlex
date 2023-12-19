import os
import json
import sys


parent_directory = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.append(parent_directory)
# noinspection PyUnresolvedReferences
from dp.loader import DynaPlex as dp
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


def dcl_policy(gen):
    dcl_filename = f'dcl_python_{gen}'
    dcl_load_path = dp.filepath(mdp.identifier(), dcl_filename)
    return dp.load_policy(mdp, dcl_load_path)


def ppo_policy():
    ppo_filename = 'ppo_policy'
    ppo_load_path = dp.filepath(mdp.identifier(), ppo_filename)
    return dp.load_policy(mdp, ppo_load_path)


policies = []
policies.append(mdp.get_policy("base_stock"))
policies.append(ppo_policy())
policies.append(dcl_policy(1))

comparer = dp.get_comparer(mdp)
comparison = comparer.compare(policies)
result = [(item['policy']['id'], item['mean'], item['error']) for item in comparison]

print(result)
