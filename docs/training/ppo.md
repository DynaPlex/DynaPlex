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

`train()` returns the **best-evaluating agent** (see below), and stores the run
in two levels, as [DCL does](dcl.md#what-is-on-disk):

```text
dynaplex_runs/
    LostSalesMDP_ppo_3f9a1c2e/                    the MDP model
        mdp.json
        LostSalesFeaturizer_mlp_128x128_9c22d4/   one training run
            ppo.json          # the record of the run
            best_agent/       # NNAgent with the best evaluation during training
            final_agent/      # NNAgent after the last update
            checkpoint.pt     # actor + critic + optimizer, for manual resume
            tensorboard/      # training curves (if tensorboard is installed)
```

The names say what is in a directory: the class of the MDP model, the
featurizer and the network, each followed by a short hash. The upper hash
covers the MDP model, its parameter values and its code. The lower one covers
the parameters of the network, the critic network and every field of
`PPOConfig`. Runs are stored under `root=` when you give it, else under the
directory in the environment variable `DYNAPLEX_RUNS_DIR`, else in
`dynaplex_runs` where Python was started.

Running a finished run again **loads** the stored agent instead of training
again; changing any config value trains into another directory. A run that was
interrupted starts over.

A label takes the place of a hash: `dp.PPO(..., mdp_label="base",
training_label="lr3")` stores the run in
`LostSalesMDP_ppo_base/LostSalesFeaturizer_mlp_128x128_lr3/`. With a label you
say where a run lives, and DynaPlex holds you to it: when the directory holds
another MDP model, or a run with other settings, the run stops with a
[`WorkdirMismatchError`][dynaplex.WorkdirMismatchError]. The same holds for
`workdir=`, which names the run's directory itself.

!!! warning "An edited featurizer or network is not noticed"
    As in DCL, the featurizer and the network are in the directory by *name*,
    and their code is not compared. After changing the body of a featurizer or
    of a network factory, the same script loads the agent that was trained
    before the change. Delete the run's directory, or give it another
    `training_label`.

## Evaluation during training

Every `eval_interval` iterations (and once at the end), the current policy is
wrapped as an `NNAgent` and assessed with a
[`PolicyComparer`](policy-comparison.md) on **common random numbers** — the
same trajectories every time, so successive evaluations are paired and the
choice of the best agent is low-variance. Evaluation numbers are in the MDP's
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
  final-observation channel (PPO runs the env in
  [`AutoresetMode.SAME_STEP`](gym-environments.md#episode-lifecycle)) —
  completes the return. (Common PPO
  implementations bootstrap from the first observation of the *next* episode
  instead: a small, structural bias.)
- **Discounting is over time, not over decisions.** A step that took `dt`
  periods in the MDP model (the env's
  [`infos["dt"]`](gym-environments.md#how-long-a-step-took)) is discounted by
  `gamma ** dt`, and a decision that takes no time is not discounted. The
  objective is therefore the cost per period, as the comparer measures it,
  also on an MDP model where the number of decisions per period depends on the
  policy. On an MDP model with one decision per period this is the usual
  discounting.
- **The environment is the compiled one.** `PPOConfig.backend` defaults to
  `"engine"`: an MDP model or featurizer that does not compile is an error.
  `backend="python"` trains on the Python environment, about ten times slower;
  `backend="auto"` falls back to it with a warning when the build fails.
- The environment is deterministic and seeded; training is seeded
  (numpy/torch/env) but not bit-guaranteed across devices or torch builds —
  bit-reproducibility is [DCL](dcl.md)'s contract, not PPO's.
