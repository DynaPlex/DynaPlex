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
from dp import dynaplex
from scripts.networks.actor_critic import CriticMLP, ActorMLP
from dp.gym.base_env import BaseEnv
sys.path.remove(parent_directory)



# This script assumes the desired mdp characteristics are specified in a file with a name of type mdp_config_{MDP_VERSION_NUMBER}.json
folder_name = "lost_sales" # the name of the folder where the json file is located
mdp_version_number = 2
path_to_json = dynaplex.filepath("mdp_config_examples",folder_name,f"mdp_config_{mdp_version_number}.json")

# Global variables used to initialize the experiment (notice the parsed json file should not contain any commented line)
try:
    with open( path_to_json , "r" ) as input_file:
        vars = json.load(input_file)
except FileNotFoundError:
    raise FileNotFoundError(f"File {path_to_json} not found. Please make sure the file exists and try again.")
except:
    raise Exception("Something went wrong when loading the json file. Have you checked the json file does not contain any comment?")

#json is optional, we can also initiate vars with something like
#vars = {"id": "lost_sales", "p": 9.0, "h": 1.0, "leadtime": 3, "demand_dist":{"type":"poisson","mean":3.0} }


mdp = dp.get_mdp(**vars)

# Training parameters
train_args = {"hidden_dim": 64,
              "lr": 1e-5,
              "discount_factor": 0.999,
              "batch_size": 64,
              "max_batch_size": 0,  # 0 means step_per_collect amount
              "nr_train_envs": 1,
              "nr_test_envs": 1,
              "max_epoch": 2,
              "step_per_collect": 100,
              "step_per_epoch": 20,
              "repeat_per_collect": 1,
              "replay_buffer_size": 200,
              "max_batchsize": 64,
              "num_actions_until_done": 50, # train environments can be either infinite or finite horizon mdp. 0 means infinite horizon
              "num_steps_per_test_episode": 100 # in order to use test environments, episodes should be guaranteed to get to terminations
              }


def policy_path():
    path = os.path.normpath(dp.filepath(mdp.identifier(), "ppo_policy"))
    return path

def save_best_fn(policy):
    save_path = policy_path()
    dp.save_policy(policy.actor,
                   {'num_inputs': mdp.num_flat_features(), 'num_outputs': mdp.num_valid_actions()},
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
        # obs_with_tensors = [
        #     {"obs": torch.from_numpy(obs['obs']).to(device=device),
        #      "mask": torch.from_numpy(obs['mask']).to(torch.bool).to(device=device)}
        #     for obs in kwargs["obs"]]
        # kwargs["obs"] = obs_with_tensors
        kwargs["obs"] = torch.from_numpy(kwargs["obs"])
    if "obs_next" in kwargs:
        # obs_with_tensors = [
        #     torch.cat((torch.from_numpy(obs['mask']).to(device=device),
        #                torch.from_numpy(obs['obs']).to(device=device)))
        #     for obs in kwargs["obs_next"]]
        # kwargs["obs_next"] = obs_with_tensors
        kwargs["obs_next"] = torch.from_numpy(kwargs["obs_next"])
    return kwargs


if __name__ == '__main__':
    model_name = "ppo_model.pt" #used for tensorboard logging
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
    policy = ts.policy.PPOPolicy(actor_net, critic_net, optim,
                                 discount_factor=train_args["discount_factor"],
                                 max_batchsize=train_args["max_batchsize"], # max batch size for GAE estimation, default to 256
                                 value_clip=True,
                                 dist_fn=torch.distributions.categorical.Categorical,
                                 deterministic_eval=True,
                                 lr_scheduler=scheduler,
                                 reward_normalization=False
                                 )
    policy.action_type = "discrete"

    # a tensorboard logger is available to monitor training results
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
        episode_per_test=100, batch_size=train_args["batch_size"],
        repeat_per_collect=train_args["repeat_per_collect"],
        logger=logger, test_in_train=True,
        save_best_fn=save_best_fn)

    # for epoch, epoch_stat, info in trainer:
    #     print("Epoch:", epoch)
    #     print(epoch_stat)
    #     print(info)

    result = trainer.run()
    print(f'Finished training!')

    policies = []

    #load base stock policy
    base_policy = mdp.get_policy("base_stock")
    policies.append(base_policy)

    #load ppo
    ppo_load_path = policy_path()
    ppo_policy = dp.load_policy(mdp, ppo_load_path)
    policies.append(ppo_policy)

    comparer = dp.get_comparer(mdp)
    comparison = comparer.compare(policies)
    result = [(item['mean'], item['error']) for item in comparison]

    print(result)
