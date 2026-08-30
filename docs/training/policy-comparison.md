# Comparing policies

`PolicyComparer` is DynaPlex's evaluation tool: it estimates the performance
of any policy on an MDP, and compares several policies head-to-head on
**common random numbers**. A "policy" here is anything with a
`get_action(state)` or `get_action(state, context)` method (the
[two policy shapes](../reference/api/modelling.md#policies-the-two-shapes)) —
a hand-written benchmark rule, a classical parameterized policy, the built-in
[`RandomPolicy`](../reference/api/training.md#built-in-policies) — or a
trained [`NNAgent`](dcl.md#the-result-an-nnagent) produced by [DCL](dcl.md)
or [PPO](ppo.md).

```python
import dynaplex as dp
from dynaplex.modelling import DiscreteDist
from dynaplex.models.lost_sales import BaseStockPolicy, LostSalesMDP

mdp = LostSalesMDP(p=9.0, h=1.0, leadtime=4, demand_dist=DiscreteDist.poisson(5.0))

comparer = dp.PolicyComparer(mdp, number_of_trajectories=4096, seed=1)
results = comparer.compare({
    "base-stock": BaseStockPolicy(mdp),
    "agent": my_agent,          # e.g. an NNAgent from DCL or PPO
})
print(results)
```

```text
policy         mean       error    delta_mean  delta_error
base-stock *  8.26691   0.005968            0            0
agent         6.85786   0.006526     -1.40905     0.006226
(* = benchmark; delta = policy - benchmark, paired on common random numbers)
```

Use `comparer.assess(policy)` for a single policy; `compare(...)` accepts
positional policies, a list, or a dict (dict keys become the result names),
and `benchmark=` selects the benchmark by index or name.

## What the numbers mean

The metric is always the MDP's **native cost** (lower is better):

- **Infinite-horizon MDPs** report the long-run average **cost per unit
  time**. Each trajectory runs a warmup window, then a measurement window,
  both denominated in *time* (`context.time_elapsed`, which the MDP owns and
  advances) — not in action counts. Defaults: `warmup_time=128`,
  `horizon=1024`.
- **Finite-horizon MDPs** report the expected cumulative **episode cost**;
  each trajectory runs to its `FINAL` state.

`mean` and `error` (standard error) are absolute; `delta_mean`/`delta_error`
are the difference with the benchmark, **paired per trajectory**.

## Per-trajectory statistics

Every assessment also carries `stats`: the trajectory context's statistics
holder. For the vanilla `TrajectoryContext` it has two arrays,
`stats.cumulative_cost` and `stats.time_elapsed` (the raw window totals behind
`returns`, one row per trajectory).

!!! tip "Collect your own statistics"
    An MDP that declares its own trajectory context gets **every statistic it
    declares** back the same way — `stats.<field>`, with a leading trajectory
    axis — and with the guarantee that row `i` is trajectory `i` under common
    random numbers for every policy and backend, so rows pair across policies.
    Customers accepted per type, revenue per day, whether the flight sold out:
    a field on the context, updated inside the transition functions, is all it
    takes. See [Custom trajectory
    statistics](../advanced/airplane-statistics.md).

`stats` is typed when the context names its holder (`Stats = MyStats` in the
context body; see the tutorial): `PolicyComparer(mdp)` is then a
`PolicyComparer[MyStats]` and pyright knows `assessment.stats` is a `MyStats`.
For an MDP without a custom context it is a `TrajectoryStats`; for a custom
context that leaves the holder derived it is untyped (`Any`).

```python
res = comparer.compare({"rule": rule, "trained": agent})
d = res["trained"].stats.stockouts - res["rule"].stats.stockouts    # paired per trajectory
print(d.mean(), d.std(ddof=1) / np.sqrt(len(d)))
```

There is deliberately no summary API on top: the arrays are raw and you
compute whatever the study needs in numpy.

## Common random numbers

Every trajectory's random stream is derived from the comparer `seed` and the
trajectory index alone — so trajectory *i* sees identical randomness under
every policy, no matter which backend evaluates it or how many worker threads
are used. Paired differences therefore have far tighter standard errors than
the absolute means: two policies whose absolute costs overlap within error
bars can still be separated decisively by their paired delta.

The same property makes results **reproducible**: the same comparer settings
produce bit-identical per-trajectory returns across runs, worker counts, and
backends.

## Backends

- `backend="auto"` (default) compiles an evaluation kernel per policy type and
  runs it multi-threaded; policies outside the DynaML subset fall back to a
  pure-Python evaluation that is *bit-identical* to the compiled one, so CRN
  pairing survives even a mixed comparison.
- `NNAgent` policies run a dedicated **NN harness**: the engine advances
  thousands of trajectories in lock-step and the network runs one batched
  forward per step — evaluating a trained agent costs about as much as
  evaluating a simple heuristic.
- `workers` (default: CPU count − 2) is a pure throughput knob; results do
  not depend on it.

See the [API reference](../reference/api/evaluation.md) for all
options.
