# Exact solver

Exact policy evaluation and optimization of a finite-state MDP — the average
cost per period under an infinite horizon, the expected total cost under a
finite one — see the [exact solver guide](../../training/exact-solver.md) for
usage.
Everything on this page is imported from the top-level `dynaplex` package.

::: dynaplex.ExactSolver

## Results

::: dynaplex.ExactResult

## The exact policy

The action table as a compiled policy, returned by `ExactResult.get_policy()`:
`ExactPolicy` on the perfect (mixed-radix) path, `ExactTablePolicy` on the
table path. Both are normal policies for the
[comparer](../../training/policy-comparison.md) and [DCL](../../training/dcl.md).

::: dynaplex.ExactPolicy

::: dynaplex.ExactTablePolicy
