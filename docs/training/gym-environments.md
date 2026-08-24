# Vectorized gym environments

`dynaplex.gym.VectorEnv` turns any DynaPlex MDP into a fast, vectorized,
[Gymnasium](https://gymnasium.farama.org)-compatible environment. It is the
substrate [PPO](ppo.md) trains on, and the bridge for using DynaPlex models
with external RL code.

Requires the optional `gymnasium` dependency: `pip install "dynaplex[gym]"`.

```python
import numpy as np
import dynaplex as dp
from dynaplex.modelling import DiscreteDist
from dynaplex.models.lost_sales import LostSalesFeaturizer, LostSalesMDP

mdp = LostSalesMDP(p=9.0, h=1.0, leadtime=4, demand_dist=DiscreteDist.poisson(5.0))
env = dp.gym.VectorEnv(mdp, features=LostSalesFeaturizer, num_envs=256)

obs, infos = env.reset(seed=42)                 # obs: {"v": float32 [256, ...]}
for _ in range(1000):
    mask = infos["action_mask"]                  # bool [256, num_actions]
    actions = mask.argmax(axis=1)                # your policy here
    obs, rewards, terminated, truncated, infos = env.step(actions)
```

Observations come from a [featurizer](featurizers.md) (the required `features=`
argument) and are **bundles**: the observation space is a `spaces.Dict` with one `Box`
per spec tensor, keyed by writer field name, and `reset`/`step` return the
matching dict of batched arrays (SB3's `CombinedExtractor` convention).
Actions are `Discrete(mdp.num_actions)`, rewards are the **negated costs**
incurred during the step, and the action-validity mask rides in
`infos["action_mask"]`.

## Why it is fast

In the engine, an environment is a *slot*, not a process: the `num_envs`
slots are sharded over worker threads inside one compiled kernel. Per policy
step the cost is **one kernel round** — not one Python round-trip per
environment — so environment count is nearly free until the CPU saturates.
Indicative throughput (lost-sales MDP, Apple M4 Pro, random-valid actions):

| num_envs | engine backend | pure-Python execution |
|---:|---:|---:|
| 256 | 4.5M steps/s | 330k steps/s |
| 1024 | 15.1M steps/s | 318k steps/s |
| 4096 | 34.7M steps/s | 319k steps/s |

This is why hundreds-to-thousands of parallel environments is the intended
regime, and why [PPO's defaults](ppo.md#tuning-for-large-batches) are sized
accordingly.

## Episode lifecycle

The env **autoresets on the same step**: when an episode ends during
`step()`, the returned observation and mask already belong to the *next*
episode, and the ended episode's last state surfaces through the infos —

```python
if "final_observation" in infos:
    which = infos["_final_observation"]          # bool [num_envs]
    final_obs = infos["final_observation"]       # object array; where `which`,
                                                 # each entry is an obs dict
```

- `terminated[i]` — the episode reached a `FINAL` state (finite-horizon MDPs).
- `truncated[i]` — the episode hit the time cap: the first decision point
  with `context.time_elapsed >= max_episode_steps`. The cap is denominated in
  **time units**, not action counts. Defaults: 2048 for infinite-horizon
  MDPs, never for finite-horizon ones.

The final observation is exactly what value-based algorithms need to
bootstrap truncated episodes correctly — [PPO](ppo.md#correctness-notes) uses
it for that.

!!! note "Featurizers meet FINAL states"
    With same-step autoreset, the featurizer runs on episode **end** states —
    including `FINAL` states of finite-horizon MDPs. Featurizers are plain
    field readers, so this normally just works, but it is part of the
    contract: your featurizer must tolerate `FINAL` states.
    `write_action_validity` is never called on them.

## Determinism

Episode streams are seeded per (environment, episode) from the reset seed:

- `reset(seed=s)` replays **byte-identically** — same observations, rewards,
  and flags, given the same actions.
- `reset()` without a seed *continues*: current episodes are abandoned and
  each slot starts its next episode with fresh draws (Gymnasium semantics).
- Results are invariant to the `workers` thread count and to the backend —
  both are pure execution knobs.

## Backends

`backend="auto"` (default) compiles the MDP into the engine kernel and falls
back to pure-Python execution if the model uses Python features outside the
DynaML subset; `"engine"` and `"python"` force one. The two backends are
**bit-identical** (the Python path executes the same kernel code), so you can
develop against `"python"` and switch on the speed later.

See [`dynaplex.gym.VectorEnv`](../reference/api/gym.md) for all constructor
options (`num_envs`, `max_episode_steps`, `workers`, `backend`, `jit`, ...).

## Current limits

`autoreset=False` (park mode, for external collectors that reset environments
themselves, e.g. Tianshou's) and per-environment subset resets are designed
but not yet implemented.
