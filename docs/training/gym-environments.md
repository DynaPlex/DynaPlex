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

## Park mode: collectors that reset environments themselves

Some RL libraries do not want autoreset. Their collector notices `done`,
keeps the terminal observation, and resets the finished environments itself.
`autoreset=False` gives them that:

```python
env = dp.gym.VectorEnv(mdp, features=LostSalesFeaturizer, num_envs=256,
                       autoreset=False)
obs, infos = env.reset(seed=42)
obs, rewards, terminated, truncated, infos = env.step(actions)

parked = np.flatnonzero(~infos["live"])          # episodes that ended this step
if parked.size:
    obs_new, infos_new = env.reset(env_id=parked)  # only those rows come back
```

- The returned observation **is** the terminal one — there is no
  `final_observation` channel in this mode, because the regular row already
  carries the end state.
- A parked environment is skipped by `step`: it pays reward 0 and **holds** its
  terminal flags, so the row keeps saying *done, reset me*. Stepping one is a
  no-op, not an error.
- `reset(env_id=...)` revives exactly those environments at their stream's next
  episode, and returns only those rows, in the order you asked for.
- `step(actions, ready_env_ids)` likewise addresses a subset: pass one action
  per id (or a full-width array), and every return covers just those rows.

Because episode streams are seeded per (environment, episode index), parking
and immediately reviving reproduces the autoreset run **bit for bit** — the two
modes differ only in where the terminal observation surfaces.

## Tianshou

[Tianshou](https://tianshou.org)'s vector-env contract differs from
Gymnasium's in ways one object cannot satisfy at once: `reset(env_id)` puts
environment ids where Gymnasium's `reset(seed)` puts the seed, observations
and infos are object arrays of per-environment dicts, and the spaces are
per-environment lists. `dynaplex.gym.TianshouVectorEnv` is the adapter, over a
park-mode env:

```python
from dynaplex.gym import TianshouVectorEnv
from tianshou.data import Collector, VectorReplayBuffer

env = TianshouVectorEnv(mdp, features=LostSalesFeaturizer, num_envs=64)
collector = Collector(algorithm, env, VectorReplayBuffer(50_000, len(env)))
```

It takes the same arguments as `VectorEnv` apart from `autoreset`. The
action-validity mask rides **inside the observation** under the key `"mask"`,
which is where Tianshou's discrete algorithms look for it (`batch.obs.mask`) —
so a featurizer may not declare a tensor called `mask`. Off-policy and
on-policy algorithms both work; DQN and Reinforce are the ones under test.

Requires `pip install "dynaplex[tianshou]"`.

!!! warning "Write your own network module"
    Tianshou's stock networks (`tianshou.utils.net.common.Net` and friends)
    assume the observation is one flat array and call `torch.as_tensor(obs)` on
    it. A DynaPlex observation is a **bundle** — the featurizer's named tensors
    plus `"mask"` — so those will fail. Write a small module that reads the spec
    tensors by name instead, as the example below does; it is a handful of
    lines, and it is the same module a [`Net` factory](ppo.md) builds, so the
    trained weights stay usable from DynaPlex afterwards. A featurizer with more
    than one tensor (object/token writers) needs such a factory anyway —
    `dynaplex.MLP()` only builds for a single flat tensor.

!!! note "What it costs"
    Observations stay **batched** across the boundary — they go out as a
    Tianshou `Batch` of `[num_envs, ...]` arrays, which is what the collector
    converts to anyway. Only the infos are per environment, because Tianshou
    requires an object array there. So the adapter keeps scaling, at roughly
    a constant factor behind the raw env (same MDP, random-valid actions,
    Apple M4 Pro):

    | num_envs | `VectorEnv` | `TianshouVectorEnv` |
    |---:|---:|---:|
    | 64 | 1.4M steps/s | 1.2M steps/s |
    | 256 | 7.5M steps/s | 4.0M steps/s |
    | 1024 | 17.4M steps/s | 6.4M steps/s |
    | 4096 | 34.1M steps/s | 8.4M steps/s |

    Either way this is well ahead of stepping `num_envs` Python environments
    one at a time, which is what a stock vector env does.

### Worked example

[:material-download: tianshou_dqn_example.py](../downloads/tianshou_dqn_example.py){ .md-button }

Trains a DQN agent with Tianshou and then scores it with the DynaPlex
[comparer](policy-comparison.md) against a domain heuristic on common random
numbers — the round trip that makes the number mean something:

```python title="tianshou_dqn_example.py"
--8<-- "docs/downloads/tianshou_dqn_example.py"
```

The network is built by a DynaPlex `Net` factory and wrapped in a small adapter
for Tianshou's `(obs, state) -> (logits, state)` calling convention, so one set
of weights serves both sides and the trained module drops straight into an
[`NNAgent`](../reference/api/training.md).
