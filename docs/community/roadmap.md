# Planned features

DynaPlex is a ground-up rewrite of the original DynaPlex, and not everything
from the original library has been ported yet. This page records what is
planned, so you know what is coming and what is deliberately absent today.
Once the public issue tracker is live, individual items will be tracked
there — if one of these matters to you, let us know in
[Discussions](https://github.com/DynaPlex/DynaPlex/discussions); it helps us
prioritize.

## Exact solvers

The original DynaPlex included exact methods based on policy iteration and
value iteration. These are not yet available in the rewrite; porting them is
planned. Until then, DynaPlex covers deep reinforcement learning (DCL) and
classical parameterized policies.

## Cluster / HPC job scripts

DCL parallelizes well beyond a single workstation. We plan to provide
ready-to-adapt job scripts for HPC clusters (SLURM-based, with Snellius —
the Dutch national supercomputer — as the worked example), covering
installation into a cluster environment and running larger training jobs.

## Beyond-MLP neural networks in DCL

The worked DCL examples currently use multi-layer perceptrons over a flat
feature vector. DynaPlex already lets you plug in **custom network
architectures** — you supply the network factory — and the observation surface
now supports **multi-tensor feature bundles**, the substrate needed for
architectures that read structured state rather than a single flat vector.

We are building on this in stages:

- **Convolutional networks** over spatially-structured states — a worked example
  is planned.
- **Attention / pointer networks** over object-token features (variable-length
  sets of entities) — the [featurizer](../training/featurizers.md) support for
  this is landing and still stabilizing; a worked example will follow.
- **Graph neural networks** over multi-tensor features — under development.

## Ports of models from the original DynaPlex

Models written for the original C++ DynaPlex do not run on the new engine,
but porting them is generally straightforward — the new modelling surface is
richer than the old one. We port selected models from the literature as
examples; if you depend on a specific model from the original library, open
an issue.
