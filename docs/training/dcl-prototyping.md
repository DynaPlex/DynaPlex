# Trying featurizers and networks

How to represent a state for a network, and which network to use, are the two
choices in [Deep Controlled Learning](dcl.md) that usually take a few attempts.
DynaPlex makes an attempt cheap. Generation 0 rolls out your own policy, which
reads states directly, so collecting it involves no featurizer and no network.
It is collected once and stores the state of every sample. Every featurizer and
network you try afterwards is trained on those same samples: an attempt costs
the training only, and the results differ only in what you changed.

```python
import dynaplex as dp
from dynaplex.modelling import DiscreteDist
from dynaplex.models.lost_sales import BaseStockPolicy, LostSalesFeaturizer, LostSalesMDP

mdp = LostSalesMDP(p=9.0, h=1.0, leadtime=4, demand_dist=DiscreteDist.poisson(5.0))
base_stock = BaseStockPolicy(mdp)

dcl = dp.DCL(mdp, base_stock, n=4000, m=200, h=40)
dcl.collect_gen0()                                   # once; a later call loads it

small = dcl.train_policy(features=LostSalesFeaturizer, network=dp.MLP(hidden=[32]))
wide = dcl.train_policy(features=LostSalesFeaturizer, network=dp.MLP(hidden=[128, 128]))
deep = dcl.train_policy(features=LostSalesFeaturizer, network=dp.MLP(hidden=[64, 64, 64]))

comparer = dp.PolicyComparer(mdp, number_of_trajectories=4096, seed=1)
print(comparer.compare({"base-stock": base_stock, "small": small, "wide": wide, "deep": deep}))
```

```text
policy                mean       error    delta_mean  delta_error
base-stock *       8.26649    0.004231             0            0
small              6.99651    0.004632      -1.26999     0.003236
wide               6.93424    0.004548      -1.33226     0.003395
deep                6.9084    0.004527      -1.35809     0.003508
(* = benchmark; delta = policy - benchmark, paired on common random numbers)
```

`train_policy` returns the generation-1 agent for that featurizer and network.
The comparer runs all of them on the same random numbers, so the `delta`
columns are paired differences and small gaps between two networks are visible.

On disk this is one collection with three experiments in it:

```text
dynaplex_runs/LostSalesMDP_dcl_d4b7f148/
    dcl.json
    gen0/
    LostSalesFeaturizer_mlp_32_3c24bd/         agent_gen1/
    LostSalesFeaturizer_mlp_128x128_c387fd/    agent_gen1/
    LostSalesFeaturizer_mlp_64x64x64_56a0a5/   agent_gen1/
```

Running the script again loads all three. Adding a fourth line trains the
fourth network only. To go on with the one you prefer, ask for more
generations with the same arguments: `dcl.run(3, features=..., network=...)`
finds its generation-1 agent and continues from there.

!!! warning "Edit a featurizer or a network: delete its directory"
    An experiment is found by the *name* of its featurizer and network and by
    its settings, not by their code. After changing the body of a featurizer
    or of a network factory, the script would load the agent trained before
    the change. Delete the experiment's directory, or pass another
    `training_label`. See [When a run is reused](dcl.md#when-a-run-is-reused).

## Another featurizer

A [featurizer](featurizers.md) is a class, and trying another one is passing
another class:

```python
mine = dcl.train_policy(features=MyOtherFeaturizer, network=dp.MLP(hidden=[128, 128]))
```

Features are never stored. They are made from the stored states when training
starts, which takes a fraction of a second for 100,000 lost-sales states.
[`dynaplex.featurize`][dynaplex.featurize] is that step on its own, for when
you want to look at what your featurizer writes:

```python
x = dp.featurize(mdp, dcl.records() + "/states", LostSalesFeaturizer)   # {tensor name: array}
```

## Your own network

`dp.MLP(hidden=[...])` is a network over one flat feature vector. For anything
else, write a **factory**: a function in an importable module that builds a
torch module from the feature spec and the number of actions.

```python
# my_nets.py
import torch.nn as nn
import dynaplex as dp

@dp.net_factory(label=lambda width, depth: f"resmlp_{depth}x{width}")
def resmlp_factory(spec, num_actions, width=64, depth=2):
    ...                               # build and return an nn.Module
```

```python
net = dp.Net("my_nets.resmlp_factory", width=128, depth=4)
agent = dcl.train_policy(features=LostSalesFeaturizer, network=net)
# .../LostSalesFeaturizer_resmlp_4x128_<hash>/
```

- The module's `forward` takes the batch of features, a dict from tensor name
  to tensor as the featurizer's spec describes it, and returns one score per
  action: a tensor of shape `[batch, num_actions]`. The scores are logits over
  the admissible actions. The network does not see which actions are
  admissible and does not choose: DynaPlex masks the scores and selects.
- `dp.Net("module.factory", **kwargs)` names the factory and its arguments. It
  is this description that is stored with an agent, so an agent can be rebuilt
  on any machine where the module can be imported. A live torch module is not
  accepted.
- [`@dp.net_factory(label=...)`][dynaplex.net_factory] gives the name used in
  the directory: a function of the factory's own parameters, or a fixed
  string. Without it the name is the function's name without a trailing
  `_factory`, whatever the arguments, and experiments that differ in the
  arguments are told apart by the hash only.

For complete factories to start from, see
[example-nn-architectures](https://github.com/DynaPlex/example-nn-architectures):
a flat network, a graph network and a network over tokens, each written as a
factory and trained on the same collection.

## The experiment as an object

`dcl.experiment(features=..., network=..., train=...)` gives the experiment
itself, a [`DCLExperiment`][dynaplex.DCLExperiment], when you want its pieces:
`run(g)`, `agent(g)`, `agents()`, `samples(g)`, and `featurized(g)` for the
samples with their features.

## Training several at once

The trainings above run one after the other. They do not depend on each other,
so they can also run side by side, each in its own process. One rule applies:
**collect generation 0 first**, in one process, as the example does. Processes
that start without it would each try to collect it, and all but one stop with a
[`CollectionInProgressError`][dynaplex.CollectionInProgressError]. With
generation 0 on disk, every process only reads it.

A worked example, with networks that differ in more than their width, is the
vehicle routing model in
[example-nn-architectures](https://github.com/DynaPlex/example-nn-architectures):
one collection, a flat network, a graph network and a network over tokens, and
one comparison table.
