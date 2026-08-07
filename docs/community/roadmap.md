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

The DCL examples currently use multi-layer perceptrons. DynaPlex supports
custom network architectures, and we plan worked examples showing them —
e.g. convolutional networks over spatially-structured states. Further
architectures (graph neural networks over multi-tensor features) are under
development.

## Ports of models from the original DynaPlex

Models written for the original C++ DynaPlex do not run on the new engine,
but porting them is generally straightforward — the new modelling surface is
richer than the old one. We port selected models from the literature as
examples; if you depend on a specific model from the original library, open
an issue.
