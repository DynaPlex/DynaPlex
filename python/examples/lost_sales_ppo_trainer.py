import torch
import os
import json
import sys
import tianshou as ts

from torch.utils.tensorboard import SummaryWriter
from tianshou.utils import TensorboardLogger

from torch.optim.lr_scheduler import ExponentialLR

parent_directory = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.append(parent_directory)
# noinspection PyUnresolvedReferences
from dp.loader import DynaPlex as dp
from networks.lost_sales_actor_critic_mlp import CriticMLP, ActorMLP
from utils.tianshou.network_wrapper import TianshouModuleWrapper
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

# Training parameters
train_args = {"hidden_dim": 64,
              "lr": 1e-3,
              "discount_factor": 0.99,
              # "discount_factor": 1.0,
              "batch_size": 64,
              "max_batch_size": 0,  # 0 means step_per_collect amount
              "nr_train_envs": 8,
              "nr_test_envs": 8,
              "max_epoch": 3,
              "step_per_collect": 500,
              "step_per_epoch": 1000,
              "repeat_per_collect": 2,
              "replay_buffer_size": 20000,
              "max_batchsize": 2048,
              "num_actions_until_done": 1000,   # train environments can be either infinite or finite horizon mdp. 0 means infinite horizon
              "num_steps_per_test_episode": 1000    # in order to use test environments, episodes should be guaranteed to get to terminations
              }


def policy_path():
    path = os.path.normpath(dp.filepath(mdp.identifier(), "ppo_policy"))
    return path


def save_best_fn(policy):
    save_path = policy_path()
    dp.save_policy(policy.actor.wrapped_module,
                   {'input_type': 'dict', 'num_inputs': mdp.num_flat_features(), 'num_outputs': mdp.num_valid_actions()},
                   save_path)


def get_env():
    return BaseEnv(mdp, train_args["num_actions_until_done"])


def get_test_env():    
    return BaseEnv(mdp, train_args["num_steps_per_test_episode"])


def preprocess_function(**kwargs):
    """
    Observations contain the mask as part of a dictionary.
    This function ensures that the data gathered in training and testing are in the correct format.
    """
    if "obs" in kwargs:
        obs_with_tensors = [
            {"obs": torch.from_numpy(obs['obs']).to(torch.float).to(device=device),
             "mask": torch.from_numpy(obs['mask']).to(torch.bool).to(device=device)}
            for obs in kwargs["obs"]]
        kwargs["obs"] = obs_with_tensors
    if "obs_next" in kwargs:
        obs_with_tensors = [
            {"obs": torch.from_numpy(obs['obs']).to(torch.float).to(device=device),
             "mask": torch.from_numpy(obs['mask']).to(torch.bool).to(device=device)}
            for obs in kwargs["obs_next"]]
        kwargs["obs_next"] = obs_with_tensors
    return kwargs


if __name__ == '__main__':

    train = True
    if train:
        model_name = "ppo_model_dict.pt"     #used for tensorboard logging
        device = "cuda" if torch.cuda.is_available() else "cpu"

        # define actor network structure
        actor_net = ActorMLP(
            input_dim=mdp.num_flat_features(),
            hidden_dim=train_args["hidden_dim"],
            output_dim=mdp.num_valid_actions(),
            min_val=torch.finfo(torch.float).min
        ).to(device)

        # define critic network structure
        critic_net = CriticMLP(
            input_dim=mdp.num_flat_features(),
            hidden_dim=train_args["hidden_dim"],
            output_dim=mdp.num_valid_actions(),
            min_val=torch.finfo(torch.float).min
        ).to(device).share_memory()

        # define optimizer
        optim = torch.optim.Adam(
            params=list(actor_net.parameters()) + list(critic_net.parameters()),
            lr=train_args["lr"]
        )

        # define scheduler
        scheduler = ExponentialLR(optim, 0.99)

        # define PPO policy
        policy = ts.policy.PPOPolicy(TianshouModuleWrapper(actor_net), critic_net, optim,
                                     discount_factor=train_args["discount_factor"],
                                     max_batchsize=train_args["max_batchsize"], # max batch size for GAE estimation, default to 256
                                     value_clip=True,
                                     dist_fn=torch.distributions.categorical.Categorical,
                                     deterministic_eval=True,
                                     lr_scheduler=scheduler,
                                     reward_normalization=False
                                     )
        policy.action_type = "discrete"

        # a tensorboard logger is available to monitor training results.
        # log in the directory where all mdp results are stored:
        log_path = dp.filepath(mdp.identifier(), "tensorboard_logs", model_name)
        writer = SummaryWriter(log_path)
        logger = TensorboardLogger(writer)

        # create nr_envs train environments
        train_envs = ts.env.DummyVectorEnv(
            [get_env for _ in range(train_args["nr_train_envs"])]
        )
        collector = ts.data.Collector(policy, train_envs, ts.data.VectorReplayBuffer(train_args["replay_buffer_size"], train_args["nr_train_envs"]), exploration_noise=True, preprocess_fn=preprocess_function)
        collector.reset()

        # create nr_envs test environments
        test_envs = ts.env.DummyVectorEnv(
            [get_test_env for _ in range(train_args["nr_test_envs"])]
        )
        test_collector = ts.data.Collector(policy, test_envs, exploration_noise=False, preprocess_fn=preprocess_function)
        test_collector.reset()


        # train the policy
        print("Starting training")
        policy.train()
        trainer = ts.trainer.OnpolicyTrainer(
            policy, collector, test_collector=test_collector,
            # policy, collector, test_collector=None,
            max_epoch=train_args["max_epoch"],
            step_per_epoch=train_args["step_per_epoch"],
            step_per_collect=train_args["step_per_collect"],
            episode_per_test=10, batch_size=train_args["batch_size"],
            repeat_per_collect=train_args["repeat_per_collect"],
            logger=logger, test_in_train=True,
            save_best_fn=save_best_fn)
        print(f'save location:{policy_path()}')
        result = trainer.run()
        print(f'Finished training!')

    policies = [dp.load_policy(mdp, policy_path()), mdp.get_policy("random")]
    comparer = dp.get_comparer(mdp, number_of_trajectories=256, periods_per_trajectory=10000, rng_seed=12)

    comparison = comparer.compare(policies)
    result = [(item['policy']['id'], item['mean']) for item in comparison]
    print(result)
