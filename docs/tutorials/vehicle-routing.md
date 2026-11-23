# Vehicle routing on a grid

In the [airplane](airplane-mdp.md) and [bin packing](binpacking-mdp.md)
models the state is a handful of numbers. Sometimes a model has a lot of
features: a whole grid, a list of orders, a fleet of vehicles. Then it becomes
interesting to look at other neural network architectures than the default
one. This page shows such a model. The model and everything to train neural
networks on it are in the repository
[example-nn-architectures](https://github.com/DynaPlex/example-nn-architectures).

## The model

A fleet of vehicles drives on a 10×10 grid. Demand arrives on random cells,
and every unit of demand costs money for every period it waits. Each period,
every vehicle decides where to go next: it stays where it is or moves to a
neighbouring cell. A vehicle picks up the demand on the cell it stands on, as
much as it can carry, and when it is full it drives back to the depot in the
corner to unload. The goal is a policy that keeps the waiting demand low.

The model is adapted from Akkerman, Mes & van Jaarsveld (2025), [*A comparison
of reinforcement learning policies for dynamic vehicle routing problems with
stochastic customer requests*](https://doi.org/10.1016/j.cie.2024.110747),
Computers & Industrial Engineering 200, 110747.

## Watch an episode

The player below shows one episode of 200 periods. Press play, or drag the
slider to jump to any period.

<iframe src="../vehicle-routing-episode.html"
        title="One episode of the vehicle routing model under the myopic policy"
        loading="lazy"
        style="width: 100%; height: 680px; border: 1px solid #ccc;"></iframe>

Vehicles are triangles labeled `index:capacity`. A vehicle turns orange when
it is full and drives back to the depot. Orders are red dots, with their
weight written in them when it is more than one.

What you see is the **myopic policy**, in which every vehicle steps toward
the nearest order. It is a sensible rule, but not a good one. Watch for
vehicles that drive to the same order, and for orders in a far corner that
wait a long time.

If the player does not show, open it
[as a page of its own](../vehicle-routing-episode.html).

## Training neural networks on it

The repository trains neural networks on this model with
[Deep Controlled Learning](../training/dcl.md), and the two best ones bring
the average cost per period from about 3.0 under the myopic policy to between
2.0 and 2.3. It does so with three network architectures, which differ in how
the state is presented to the network: as one flat vector, as a graph with
one node per cell, and as a set of tokens for the candidate moves. Its README
explains the model in detail, the three
[featurizers](../training/featurizers.md), the networks and the results. The
whole training script takes about 12 minutes on a laptop.

The player on this page is the file `episode.html` that the repository's
`visualize.py` writes. With the repository installed you can make one for a
different seed, or for a network you trained yourself.
