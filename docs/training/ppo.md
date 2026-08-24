# PPO

`dynaplex.PPO` trains a policy with Proximal Policy Optimization — the
best-known general-purpose deep-RL algorithm — on the
[engine-backed vectorized environment](gym-environments.md). It is a masked,
CleanRL-style implementation with a few correctness upgrades, and its output
is the same [`NNAgent`](dcl.md#the-result-an-nnagent) format that
[DCL](dcl.md) produces, so evaluation, saving/loading, and deployment are
identical for both.

```python
import dynaplex as dp
from dynaplex.modelling import DiscreteDist
from dynaplex.models.lost_sales import LostSalesFeaturizer, LostSalesMDP

mdp = LostSalesMDP(p=9.0, h=1.0, leadtime=4, demand_dist=DiscreteDist.poisson(5.0))

ppo = dp.PPO(
    mdp,
    features=LostSalesFeaturizer,
    network=dp.MLP(hidden=[128, 128]),
    config=dp.PPOConfig(
        minibatch_size=4096,
        num_envs=256,            # parallel environments — hundreds are cheap
        num_steps=128,           # rollout length; batch = 256 x 128 = 32768
        total_timesteps=500_000,
    ),
)
agent = ppo.train()              # -> NNAgent
```

```text
[ppo] env backend engine: 256 envs x 128 steps -> batch 32768 (8 minibatches of 4096), 15 iterations, device cpu
[ppo] iter 10/15  step 327,680  SPS 178,984  eval cost 7.8176 ± 0.0168  *best*
...
[ppo] done in 3.1s — best eval cost 7.8176 (iteration 10); agents saved to dynaplex_runs/...
```

## What you get

`train()` returns the **best-evaluating agent** (see below), and stores under
`dynaplex_runs/{MDPClass}_{mdp_hash}/ppo_{experiment_hash}/`:

```text
ppo.json          # the experiment record
best_agent/       # NNAgent with the best evaluation during training
final_agent/      # NNAgent after the last update
checkpoint.pt     # actor + critic + optimizer, for manual resume
tensorboard/      # training curves (if tensorboard is installed)
```

Rerunning a completed workdir **loads** the stored agent instead of
retraining; changing any config value trains into a fresh workdir.

## Evaluation during training

Every `eval_interval` iterations (and once at the end), the current policy is
wrapped as an `NNAgent` and assessed with a
[`PolicyComparer`](policy-comparison.md) on **common random numbers** — the
same trajectories every time, so successive evaluations are paired and the
best-model selection is low-variance. Evaluation numbers are in the MDP's
native cost metric and are directly comparable to any other policy you assess
with the same settings, including DCL agents and hand-written benchmarks.

## The training cycle

PPO alternates **collection** and **update** phases. Each iteration steps all
`num_envs` environments for `num_steps` steps, producing a rollout batch of
`num_envs × num_steps` timesteps; it then makes `update_epochs` passes over
that batch in shuffled minibatches of `minibatch_size`. So per iteration there
are `update_epochs × batch ÷ minibatch_size` gradient steps, every collected
timestep is reused `update_epochs` times, and the total number of iterations
is `total_timesteps ÷ batch`. The startup line in the example output above
prints exactly this arithmetic — read it before waiting on a long run.

Two configurations that silently degenerate:

- `num_envs × num_steps` **greater than** `total_timesteps` → zero iterations;
  nothing is trained.
- `minibatch_size` greater than the rollout batch → a single, giant minibatch
  per epoch instead of shuffled minibatches.

## Tuning for large batches

The vectorized env makes environments nearly free — the per-step cost is one
engine round plus one batched network forward — so `num_envs` in the hundreds
to thousands is the intended regime. Three rules of thumb:

- **Don't shrink `num_steps`** to compensate for env count: with
  `gamma=0.99` the effective GAE horizon is ~100 steps, so 64–128 is the
  floor for dense-reward problems.
- At fixed `total_timesteps`, more envs means **fewer, larger updates**.
  Compensate with a higher `lr` and/or fewer `update_epochs` (the update
  pass, not the environment, dominates wall-clock at large batch sizes), and
  prefer raising `total_timesteps` over starving the update count.
- `minibatch_size` is the one required config field; a few thousand is a
  good starting point at these batch sizes.

All fields are documented on
[`PPOConfig`](../reference/api/training.md#dynaplex.PPOConfig).

## Correctness notes

- **Action masking** is end-to-end: invalid actions get zero probability
  during collection and are excluded from the policy loss.
- **Truncation is bootstrapped exactly.** When an episode is cut off by the
  time cap, the value of the *true* final state — delivered by the env's
  final-observation channel — completes the return. (Common PPO
  implementations bootstrap from the first observation of the *next* episode
  instead: a small, structural bias.)
- The environment is deterministic and seeded; training is seeded
  (numpy/torch/env) but not bit-guaranteed across devices or torch builds —
  bit-reproducibility is [DCL](dcl.md)'s contract, not PPO's.
