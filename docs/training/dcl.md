# Deep Controlled Learning

Deep Controlled Learning (DCL) is DynaPlex's flagship training algorithm: an
approximate policy iteration method that repeatedly improves a policy by
simulating, for many sampled states, the consequences of each candidate action
and training a neural network to reproduce the best decisions. It has proven remarkably effective for the highly stochastic sequential decision making problems that arise in operations management (OM), and that DynaPlex focuses on;
see the
[published paper](https://www.sciencedirect.com/science/article/pii/S0377221725000463)
for the algorithm and benchmarks.

!!! warning "Coming from 1.14: the featurizer moved"
    In 2.0, `DCL(mdp, policy, ...)` no longer takes `features`, `network` or `train`.
    A script written for 1.14 stops on that line with

    ```text
    TypeError: DCL.__init__() got an unexpected keyword argument 'features'
    ```

    Give the three arguments to `dcl.run(generations, ...)` or `dcl.train_policy(...)`
    instead, as in the example below. The [changelog](../changelog.md#200) lists what
    else changed.

```python
import dynaplex as dp
from dynaplex.modelling import DiscreteDist
from dynaplex.models.lost_sales import BaseStockPolicy, LostSalesFeaturizer, LostSalesMDP

mdp = LostSalesMDP(p=9.0, h=1.0, leadtime=4, demand_dist=DiscreteDist.poisson(5.0))
base_stock = BaseStockPolicy(mdp)

# What is collected: an MDP model, the policy that rolls out, and how much to simulate.
dcl = dp.DCL(
    mdp, base_stock,                  # the initial (generation-0) rollout policy
    n=4000,                           # labeled samples per generation (default 5000)
    m=200,                            # rollouts per candidate action (default 1000)
    h=40,                             # rollout horizon (default: 40 finite, 256 infinite horizon)
)

# What is trained on it: a featurizer and a network.
agents = dcl.run(
    1,                                # generations to train: here one, agent_gen1
    features=LostSalesFeaturizer,     # state representation for the network
    network=dp.MLP(hidden=[128, 128]),
)

# Does the trained policy beat the policy it learned from?
comparer = dp.PolicyComparer(mdp, number_of_trajectories=4096, seed=1)
print(comparer.compare({"base-stock": base_stock, "agent_gen1": agents[0]}))
```

```text
policy                mean       error    delta_mean  delta_error
base-stock *       8.26649    0.004231             0            0
agent_gen1         6.93424    0.004548      -1.33226     0.003395
(* = benchmark; delta = policy - benchmark, paired on common random numbers)
```

This takes about half a minute on a laptop. The costs are per period, lower is better:
after one generation the agent is 1.33 below the base-stock policy it learned from.

!!! tip "Start with one generation"
    The example trains one generation, and that is the usual first step on a new
    MDP model: collect under your own policy, train once, and check with the comparer
    whether the agent beats that policy. If it does, the pieces work together: the
    MDP model, the rollout settings, the featurizer and the network. More generations
    typically improve the policy further; ask for them with `dcl.run(3, ...)` in the
    same script, and what is already on disk is loaded, not redone.

    If it does not, or not by much, the featurizer or the network is often the place
    to look. Generation 0 does not depend on either, so you can try several on the
    samples you already have, without simulating again:
    [Trying featurizers and networks](dcl-prototyping.md).

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
3. The lowest-cost action becomes the **label**. The state and its statistics
   (the label, the valid actions, the cost estimate of every action) are written
   to disk as the collection goes; together they are the generation's samples.
4. **Make the features** of the stored states with your featurizer, and
   **train the network** on them (masked cross-entropy; see the `train=`
   options below) — the result is `agent_gen{g+1}`.

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

Two settings shape *which* states get labeled. Both are settings of the
collection, so changing one gives another collection:

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

## What is on disk

```text
dynaplex_runs/
    LostSalesMDP_dcl_d4b7f148/                    the collection
        dcl.json                                  what was collected: MDP model, policy, settings
        gen0/                                     generation 0: the state and the statistics of every sample
        LostSalesFeaturizer_mlp_128x128_c387fd/   an experiment: featurizer, network, training settings
            exp.json
            agent_gen1/                           a trained NNAgent (weights.pt + agent.json)
            gen1/  agent_gen2/  ...               later generations, when you ask for them
```

A `DCL` describes a **collection**: the MDP model, the rollout policy and the collection
settings (`n`, `m`, `h`, `seed`, ...). A featurizer, a network and the training
settings together are an **experiment** on it. Generation 0 belongs to the collection
and is shared by all its experiments. From generation 1 on the agent itself rolls
out, and it needs its featurizer to act, so those generations belong to the
experiment.

The names say what is in a directory: the class of the MDP model, the featurizer, the
network, and then a short hash of the settings. Runs are stored under `root=` when you
give it, else under the directory in the environment variable `DYNAPLEX_RUNS_DIR`, else
in `dynaplex_runs` where Python was started. `DCL` prints the full path.

### Labels

A label takes the place of a hash:

```python
dcl = dp.DCL(mdp, base_stock, n=4000, m=200, h=40, gen0_label="small")
agents = dcl.run(1, features=LostSalesFeaturizer, network=dp.MLP(hidden=[128, 128]),
                 training_label="first")
# dynaplex_runs/LostSalesMDP_dcl_small/LostSalesFeaturizer_mlp_128x128_first/
```

Without labels you never have to think about directories: every distinct setting gets
its own, and nothing is ever refused. With a label you say where a run lives, and
DynaPlex holds you to it: if the directory holds a run with other settings, the run
stops with a [`WorkdirMismatchError`][dynaplex.WorkdirMismatchError] that lists what
differs. Use another label, or delete the directory. Nothing on disk is overwritten in
either case.

## When a run is reused

Everything is **idempotent**: what is on disk is loaded, not computed again. Run the
same script again to resume after an interruption, or ask for more generations and run
it again to extend the loop. A second run prints

```text
[gen 1/1] agent_gen1 exists — skipping
```

and returns the stored agent. What counts as "the same run" is therefore worth knowing:

- **A collection is the same** when the MDP model, the rollout policy and the collection
  settings are the same. For the MDP model and the policy that means their parameter
  values *and their code*: after an edit of `modify_state_with_event`, or of the
  policy's `get_action`, the same script collects again, in a new directory. Which file
  the code is in does not matter, so an MDP model that moved to another module is still
  the same MDP model.
- **An experiment is the same** when the featurizer has the same name, the network the
  same name and parameters, and the training settings are the same.
- **The machine does not count.** A run that moves to another computer, or is started
  with another `machine=`, finds its directory and goes on.

!!! warning "An edited featurizer or network is not noticed"
    The code of a featurizer and of a network factory is not compared. If you change
    the body of `write_features`, or of your network, and run the script again, the
    agent that was trained before the change is loaded, with the `exists — skipping`
    line above and no error. After such an edit, delete the experiment's directory or
    give the run another `training_label`.

Sample collection is seeded: the same script on the same machine collects the same
samples. Another machine, or another `machine=`, divides the work differently and
samples other states of the same MDP model. Network training is seeded but follows
torch's determinism caveats.

## Training options

These go to `run`, `train_policy` or `experiment`:

- `network=` accepts a constructible network spec: `dp.MLP(hidden=[...])`, or
  `dp.Net("your_module.your_factory", **kwargs)` for a custom architecture.
  Specs are recorded in the agent artifact, so agents rebuild from importable
  code on any machine — live torch modules are rejected by design.
  Writing your own network is on
  [Trying featurizers and networks](dcl-prototyping.md#your-own-network).
- `train=` is passed to `train_network`: `loss` (`"soft_ce"`, the default:
  AlphaZero-style soft targets over the final-round survivors, each weighted by
  its cost gap to the best; `"ce"` hard labels on the winner; `"count_ce"`
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
agent = agents[-1]                      # newest generation
agent([some_state], mdp=mdp)            # -> action, straight from Python
results = dp.PolicyComparer(mdp).compare(base_stock, agent)
```

The comparer evaluates agents through its batched NN harness on
[common random numbers](policy-comparison.md), so training progress across
generations is directly measurable against any benchmark policy.
