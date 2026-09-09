# Deep Controlled Learning

Deep Controlled Learning (DCL) is DynaPlex's flagship training algorithm: an
approximate policy iteration method that repeatedly improves a policy by
simulating, for many sampled states, the consequences of each candidate action
and training a neural network to reproduce the best decisions. It has proven remarkably effective for the highly stochastic sequential decision making problems that arise in operations management (OM), and that DynaPlex focuses on;
see the
[published paper](https://www.sciencedirect.com/science/article/pii/S0377221725000463)
for the algorithm and benchmarks.

```python
import dynaplex as dp
from dynaplex.modelling import DiscreteDist
from dynaplex.models.lost_sales import BaseStockPolicy, LostSalesFeaturizer, LostSalesMDP

mdp = LostSalesMDP(p=9.0, h=1.0, leadtime=4, demand_dist=DiscreteDist.poisson(5.0))

d = dp.DCL(
    mdp, BaseStockPolicy(mdp),        # the initial (generation-0) rollout policy
    features=LostSalesFeaturizer,     # state representation for the network
    n=4000,                           # labeled samples per generation (default 5000)
    m=200,                            # rollouts per candidate action (default 1000)
    h=40,                             # rollout horizon (default: 40 finite, 256 infinite horizon)
    warmup_time=100,                  # periods under the rollout policy before labeling starts
                                      #   (default: 100 infinite horizon, not applicable on finite)
    restart_after=50,                 # retire a trajectory after this many labels and restart it
                                      #   from a fresh warm state (default: 0 = never)
    network=dp.MLP(hidden=[128, 128]),  # or dp.Net("your_module.factory", ...) for custom nets
    train=dict(loss="ce", epochs=30, batch_size=64, lr=1e-3),
    machine="auto",                   # execution plan chosen for this machine (the default)
)
agents = d.run(generations=2)         # agent_gen1, agent_gen2 — both NNAgents
```

!!! note "Where it runs: you should not have to say"
    How many worker threads, engines and processes DCL uses, and where the
    network runs, are execution choices: they change how fast a generation
    runs, and have little effect on the samples that come out of it. So
    DynaPlex does not ask you to set them. With the default
    `machine="auto"` it detects the machine and selects a tuned execution plan
    for it: on a laptop or workstation a plan derived from the cores and the
    accelerator present, on a cluster one of the engineered node presets. The
    aim is that every machine you are likely to run on has a good plan, chosen
    for you. When detection finds an allocation no plan covers it says so,
    rather than run on an untuned shape and look slow. You always see what was
    chosen: each collection pass starts by printing the plan (processes,
    engines, workers, slots for that generation, the device the trained policy
    runs on, and why that plan was derived) and the machine it was verified
    against, and
    `python -m dynaplex.machine` shows the same detection without running
    anything. For experiments on the plans themselves, `gen0=` and `trained=`
    take a `dp.Shape` (processes, engines, workers, slots) that overlays the
    plan's shape for generation 0 and for the trained policy respectively, and
    `device=` moves the trained policy. The plan table, what is
    detected, and the SLURM lines for the cluster units are on
    [Machines and execution plans](machines.md).

## How a generation works

One generation turns a *rollout policy* into training data and a new agent:

1. **Sample states** by simulating the MDP.
2. For each sampled decision state, **compare the candidate actions**: take
   each admissible action once, then continue with the rollout policy for `h`
   time units, averaging cost over `m` replications. Weak candidates are
   eliminated early by **sequential halving** (`reduction_factor`,
   `final_field`), which concentrates the simulation budget on the actions
   that are still in contention.
3. The lowest-cost action becomes the **label**; `(features, label)` pairs
   form the generation's `SampleSet`.
4. **Train the network** on those samples (masked cross-entropy; see the
   `train=` options below) — the result is `agent_gen{g+1}`.

Generation 0 rolls out the initial policy you supplied; every later generation
rolls out the previous generation's agent. A few generations typically
converge to a policy that clearly beats the initial heuristic.

!!! tip "No heuristic at hand? Start from random."
    A sensible domain heuristic is still the best generation-0 policy — it
    concentrates sampling on relevant states from the start. When none is
    available, the built-in
    [`RandomPolicy`](../reference/api/training.md#built-in-policies) fills
    the gap: `dp.DCL(mdp, RandomPolicy(mdp), ...)` works for **any** MDP
    (it picks uniformly among the actions the MDP marks valid) and gives
    DCL a legitimate zero-knowledge starting point; the first trained
    generation then replaces it as the rollout policy.

## Where the sampled states come from

Two knobs shape *which* states get labeled, and both are part of the
experiment identity:

- **`warmup_time`** (the legacy `L`). On an infinite-horizon MDP the initial
  state is the modeller's artifact, not a state the system spends time in. So
  each trajectory first runs `warmup_time` periods under the generation's own
  rollout policy (the heuristic at generation 0, the trained agent after)
  before its decision states are labeled: the samples describe the policy's
  stationary behaviour, and follow the policy as it improves generation by
  generation. Default 100 on an infinite-horizon MDP; `warmup_time=0` turns it
  off. On a finite-horizon MDP the initial state is the honest start of an
  episode, so the warm-up does not apply and the default is 0 (setting it
  raises).
- **`restart_after`** (the legacy `reinitiate_counter`). After that many
  labels a trajectory is retired and its slot restarted from a fresh warm
  state (or the initial state when there is no warm-up), so that consecutive
  samples do not all come from one long trajectory sitting in one basin: a
  sample-diversity knob for slow-mixing systems. Counted in labels, not
  periods. Default 0 = never restart. It works on any horizon; on a finite
  horizon every episode restarts at its end anyway, so it is rarely needed
  there.

`DCL(sample_info=True)` records, per labeled sample, which controller and
trajectory produced it, the warm state it was started from, the label's index
along its trajectory and the time it was labeled at, as an `info` slab on the
`SampleSet`, so the origin of every training row can be reconstructed.

## Artifacts and resumability

Everything lands in a **workdir** derived from the *structure* of the
experiment:

```text
dynaplex_runs/{MDPClass}_{mdp_hash}/exp_{experiment_hash}/
    dcl.json          # the experiment record
    samples_gen{g}/   # SampleSets per generation
    agent_gen{g}/     # trained NNAgents (weights.pt + agent.json)
```

`run()` is **idempotent**: existing generations are loaded, not recomputed.
Rerun the same script to resume after an interruption, or bump
`generations` and rerun to extend the loop. Any change to the MDP parameters,
the sampling budget, the network, or the training options produces a new
workdir automatically — no results are ever silently mixed.

Sample collection is seeded, and the execution plan (`machine`, `workers`,
`slots`) is a throughput choice that has little effect on the samples that
come out. Network training is seeded but follows torch's determinism caveats.

## Training options

- `network=` accepts a constructible network spec: `dp.MLP(hidden=[...])`, or
  `dp.Net("your_module.your_factory", **kwargs)` for a custom architecture.
  Specs are recorded in the agent artifact, so agents rebuild from importable
  code on any machine — live torch modules are rejected by design.
- `train=` is passed to `train_network`: `loss` (`"ce"` hard labels,
  `"soft_ce"` AlphaZero-style soft targets over the finalists, `"count_ce"`
  visit-count targets), `epochs`, `batch_size`, `lr`, early stopping via
  `patience`/`val_fraction`/`val_every`. The regimen is the legacy DynaPlex one: 5 %
  of the samples held out, validated every 5 epochs, the best-validation weights
  restored, and training stopped once the epochs since the last improvement exceed
  the patience (10, so 15 epochs).

### Defaults

The defaults are those of the legacy DynaPlex DCL, so a legacy configuration ports
one to one: `n=5000` samples per generation, `m=1000` rollouts per candidate action,
`h=40` on a finite-horizon MDP and `h=256` on an infinite-horizon one, and on an
infinite-horizon MDP a warm-up of `warmup_time=100` periods under the rollout policy
before a trajectory's first labeled state (legacy `L`; off on finite horizon, where
the initial state is the honest start of an episode). `restart_after` is off unless
set. Pass any of them explicitly to override; `warmup_time=0` turns the warm-up off.

## The result: an NNAgent

Trained agents are [`NNAgent`](../reference/api/training.md#agents-and-networks)
bundles — featurizer reference, network spec, weights, and provenance — the
common output format of DCL and [PPO](ppo.md) alike:

```python
agent = d.agent()                       # newest generation
agent([some_state], mdp=mdp)            # -> action, straight from Python
results = dp.PolicyComparer(mdp).compare(BaseStockPolicy(mdp), agent)
```

The comparer evaluates agents through its batched NN harness on
[common random numbers](policy-comparison.md), so training progress across
generations is directly measurable against any benchmark policy.
