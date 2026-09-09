"""Train a DQN agent with Tianshou on a DynaPlex MDP, then score it with the
DynaPlex comparer.

    python -m dynaplex.examples.tianshou_dqn_example

Needs the optional extras:  pip install "dynaplex[tianshou]"

The point of this example is the round trip. Tianshou brings the RL algorithm
and the training loop; DynaPlex brings the MDP (stepped batched in a compiled
kernel, not one Python env at a time), the featurizer, and — at the end — an
apples-to-apples comparison against a domain heuristic on common random
numbers, which is what actually tells you whether the learned policy is any
good.

Three pieces are worth reading closely:

1. `dynaplex.gym.TianshouVectorEnv` presents Tianshou's vector-env contract
   (per-env dicts, subset `step(action, id)` / `reset(env_id)`) over the
   batched engine env. The action mask rides in the observation under "mask",
   which is where Tianshou's discrete algorithms look for it.
2. The network is built by a DynaPlex `Net` factory — a dict-forward module
   over the featurizer's declared spec. `_TianshouQ` is a thin adapter that
   gives that same module Tianshou's `(obs, state) -> (logits, state)` shape,
   so ONE set of weights serves both sides.
3. Because of (2), the trained module drops straight into an `NNAgent` and
   into `PolicyComparer` next to `BaseStockPolicy`.
"""
from __future__ import annotations

from typing import Any, cast

import numpy as np
import torch
import torch.nn as nn

import dynaplex
from dynaplex.gym import TianshouVectorEnv
from dynaplex.modelling import DiscreteDist
from dynaplex.models.lost_sales import BaseStockPolicy, LostSalesFeaturizer, LostSalesMDP

FEATURIZER = "dynaplex.models.lost_sales.LostSalesFeaturizer"

NUM_ENVS = 64          # kernel slots: envs are nearly free, the cost is per STEP
MAX_EPISODE_STEPS = 50
EPOCHS = 5
SEED = 0


class _TianshouQ(nn.Module):
    """Tianshou's `(obs, state) -> (logits, state)` shape over a DynaPlex
    dict-forward module. Tianshou hands `obs` as a Batch holding the
    featurizer's tensors plus "mask"; we forward only the spec tensors, and
    Tianshou applies the mask to the Q-values itself."""

    def __init__(self, module: nn.Module, keys: tuple[str, ...]):
        super().__init__()
        self.module = module
        self.keys = keys

    def forward(self, obs, state=None, info=None, **kwargs):
        batch = {k: torch.as_tensor(np.asarray(obs[k]), dtype=torch.float32)
                 for k in self.keys}
        return self.module(batch), state


def main() -> None:
    from tianshou.algorithm.modelfree.dqn import DQN, DiscreteQLearningPolicy
    from tianshou.algorithm.optim import AdamOptimizerFactory
    from tianshou.data import Collector, VectorReplayBuffer
    from tianshou.trainer import OffPolicyTrainerParams

    mdp = LostSalesMDP(p=9.0, h=1.0, leadtime=3,
                       demand_dist=DiscreteDist.poisson(3.0))

    def make_env() -> TianshouVectorEnv:
        return TianshouVectorEnv(mdp, features=LostSalesFeaturizer, num_envs=NUM_ENVS,
                                 max_episode_steps=MAX_EPISODE_STEPS)

    train_env, test_env = make_env(), make_env()
    train_env.seed(SEED)
    test_env.seed(SEED + 1)          # disjoint streams from training

    # One network, two shapes: the DynaPlex Net spec is the source of truth
    # (it is what an NNAgent stores and rebuilds from), the adapter is Tianshou's.
    spec = train_env.env.spec
    net_spec = dynaplex.MLP(hidden=(128, 128))
    module = net_spec.build(spec, mdp.num_actions)

    policy = DiscreteQLearningPolicy(
        model=_TianshouQ(module, tuple(spec)),
        action_space=train_env.action_space[0],
        observation_space=train_env.observation_space[0],
        eps_training=0.2, eps_inference=0.0)
    algorithm = DQN(policy=policy, optim=AdamOptimizerFactory(lr=1e-3),
                    gamma=0.99, target_update_freq=200)

    print(f"training DQN on {type(mdp).__name__} "
          f"({NUM_ENVS} envs, backend={train_env.env.backend})")
    # Tianshou annotates its collectors against its own BaseVectorEnv; our env is
    # duck-typed to that contract rather than derived from it, so tell the checker.
    train_venv, test_venv = cast(Any, train_env), cast(Any, test_env)
    result = algorithm.run_training(OffPolicyTrainerParams(
        training_collector=Collector(
            algorithm, train_venv,
            VectorReplayBuffer(50_000, len(train_env)), exploration_noise=True),
        test_collector=Collector(algorithm, test_venv),
        max_epochs=EPOCHS, epoch_num_steps=5_000,
        collection_step_num_env_steps=NUM_ENVS,
        test_step_num_episodes=NUM_ENVS, batch_size=256,
        update_step_num_gradient_steps_per_sample=0.1,
        show_progress=False, verbose=False))
    print(f"  tianshou best test return: {result.best_reward:.2f} "
          f"(episode reward over {MAX_EPISODE_STEPS} periods)")

    # The trained weights ARE a DynaPlex agent: same module, same spec, and the
    # greedy action is the masked argmax over Q-values that `act` already does.
    agent = dynaplex.NNAgent(module, spec, mdp.num_actions, FEATURIZER, net_spec)

    comparer = dynaplex.PolicyComparer(mdp, number_of_trajectories=256, seed=42)
    print("\ncost per period on common random numbers (lower is better):")
    for score in comparer.compare([agent, BaseStockPolicy(mdp)]):
        print(f"  {score.name:16s} {score.mean:8.4f} +/- {score.error:.4f}")


if __name__ == "__main__":
    main()
