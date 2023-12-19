import os
import sys

import torch
from torch.utils.tensorboard import SummaryWriter
from torch.optim.lr_scheduler import ExponentialLR

import tianshou as ts
from tianshou.utils import TensorboardLogger

parent_directory = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.append(parent_directory)
# noinspection PyUnresolvedReferences
from dp.loader import DynaPlex as dp
from gym.base_env import BaseEnv
from networks.order_picking_actor_critic_gnn import NAGNNActor, NAGNNCritic
from utils.tianshou.network_wrapper import TianshouModuleWrapper
sys.path.remove(parent_directory)

gridsize = 6
n_pickers = 2
max_orders_per_event = 1
mdp = dp.get_mdp(id="order_picking",
                 grid_size=gridsize,
                 n_pickers=n_pickers,
                 max_orders_per_event=max_orders_per_event)


# Training parameters
train_args = {"hidden_dim": 64,
              "lr": 5e-3,
              "discount_factor": 0.99,
              "batch_size": 64,
              "max_batch_size_ppo": 0,
              "nr_envs": 1,
              "max_epoch": 30,
              "step_per_collect": 500,
              "step_per_epoch": 1000,
              "repeat_per_collect": 2,
              "num_periods_until_done": 25,
              }


def save_best_fn(policy):
    path = dp.filepath(mdp.identifier(), 'ppo', f'ppo')
    dp.save_policy(policy.actor.wrapped_module,
                   {'input_type': 'dict_with_mask'},
                   path)


def get_env():    
    return BaseEnv(mdp, num_periods_until_done=train_args["num_periods_until_done"])


def get_test_env():    
    return BaseEnv(mdp, num_periods_until_done=train_args["num_periods_until_done"])


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
    model_name = "ppo_model.pt"     # used for tensorboard logging
    device = "cuda" if torch.cuda.is_available() else "cpu"

    dist_matrix = mdp.get_static_info()['distance_matrix']
    dist_mat_tensor = torch.FloatTensor([dist_vec[f'row_{idx}'] for idx, dist_vec in enumerate(dist_matrix)])

    # define actor network structure
    actor_net = NAGNNActor(
        input_dim=8,
        hidden_dim=train_args["hidden_dim"],
        n_layers=3,
        gridsize=gridsize,
        dist_matrix=dist_mat_tensor,
        min_val=torch.finfo(torch.float).min
    ).to(device)

    # define critic network structure
    critic_net = NAGNNCritic(
        input_dim=8,
        hidden_dim=train_args["hidden_dim"],
        n_layers=3,
        gridsize=gridsize,
        dist_matrix=dist_mat_tensor,
        min_val=torch.finfo(torch.float).min
    ).to(device).share_memory()

    # define optimizer
    optim = torch.optim.Adam(
        params=list(actor_net.parameters()) + list(critic_net.parameters()),
        lr=train_args["lr"]
    )

    # define scheduler
    scheduler = ExponentialLR(optim, 0.90)

    # define PPO policy
    policy = ts.policy.PPOPolicy(TianshouModuleWrapper(actor_net), critic_net, optim,
                                 discount_factor=train_args["discount_factor"],
                                 value_clip=True,
                                 dist_fn=torch.distributions.categorical.Categorical,
                                 deterministic_eval=True,
                                 lr_scheduler=scheduler,
                                 reward_normalization=False
                                 )
    policy.action_type = "discrete"

    # a tensorboard logger is available to monitor training results
    log_path = os.path.join("../dp/logs", model_name)
    writer = SummaryWriter(log_path)
    logger = TensorboardLogger(writer)
    
    # create nr_envs train environments
    train_envs = ts.env.DummyVectorEnv(
        [get_env for _ in range(train_args["nr_envs"])]
    )
    collector = ts.data.Collector(policy, train_envs, ts.data.VectorReplayBuffer(20000, train_args["nr_envs"]),
                                  exploration_noise=True, preprocess_fn=preprocess_function)
    collector.reset()

    # create nr_envs test environments
    test_envs = ts.env.DummyVectorEnv(
        [get_test_env for _ in range(train_args["nr_envs"])]
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

    result = trainer.run()
    print(f'Finished training!')
