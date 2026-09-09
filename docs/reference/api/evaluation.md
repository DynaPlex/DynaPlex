# Evaluating policies

Comparing policies and assessing their performance — see the
[policy comparison guide](../../training/policy-comparison.md) for usage;
the [exact solver](exact-solver.md) has its own page.
Everything on this page is imported from the top-level `dynaplex`
package.

::: dynaplex.PolicyComparer

::: dynaplex.PolicyAssessment

::: dynaplex.Comparison

## Model check

Runs the MDP under a policy in plain Python — no compile — and fails with a
hint naming the method when the decision structure is degenerate (a category
that never changes, time that never advances, an unwritten validity mask, no
real choice at any decision, hidden state outside the `State` object). Run it
before `rehearse`. See *Promises → Checking the model dynamically*.

::: dynaplex.check_mdp

::: dynaplex.MdpCheckReport

## Rehearsal

The model checker: the checked pass every algorithm runs by default
(`checks="rehearsal"`), as a standalone call. See *Language reference →
Runtime checks and fast mode*.

::: dynaplex.rehearse

::: dynaplex.RehearsalReport
