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
    n=4000,                           # labeled samples per generation
    m=200,                            # rollouts per candidate action
    h=40,                             # rollout horizon
    workers=8, slots=512,             # execution: worker threads x rollout slots
    network=dp.MLP(hidden=[128, 128]),
    train=dict(loss="ce", epochs=30, batch_size=64, lr=1e-3),
)
agents = d.run(generations=2)         # agent_gen1, agent_gen2 — both NNAgents
```

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

Sample collection is **bit-reproducible** given the seed, and invariant to
`workers`/`slots` (pure throughput knobs). Network training is seeded but
follows torch's determinism caveats.

## Training options

- `network=` accepts a constructible network spec: `dp.MLP(hidden=[...])`, or
  `dp.Net("your_module.your_factory", **kwargs)` for a custom architecture.
  Specs are recorded in the agent artifact, so agents rebuild from importable
  code on any machine — live torch modules are rejected by design.
- `train=` is passed to `train_network`: `loss` (`"ce"` hard labels,
  `"soft_ce"` AlphaZero-style soft targets over the finalists, `"count_ce"`
  visit-count targets), `epochs`, `batch_size`, `lr`, early stopping via
  `patience`/`val_fraction`.

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
