# API reference — `dynaplex`

The top-level package: sample collection and training, policy evaluation,
featurizers, and random number generation. The model-authoring vocabulary —
including `DiscreteDist` and the samplers — lives in
[`dynaplex.modelling`](modelling.md); the `Engine` and other advanced
primitives for custom algorithm harnesses live in
[`dynaplex.runtime`](runtime.md).

## Sample collection and training

::: dynaplex.dcl.dcl

::: dynaplex.PPOTrainer

::: dynaplex.PPOTrainerConfig

## Policy evaluation

::: dynaplex.PolicyComparer

::: dynaplex.PolicyAssessment

::: dynaplex.Comparison

## Random number generation

For *event distributions*, prefer building a
[`DiscreteDist`](modelling.md#dynaplex.modelling.DiscreteDist) (plus a
precomputed `dist.alias_sampler()`) over rolling your own from raw draws.
The generators below are the underlying source of randomness.

::: dynaplex.default_rng

::: dynaplex.Generator

## Featurizers

::: dynaplex.featurizer

::: dynaplex.GlobalStateWriter

## Errors

::: dynaplex.DynaPlexError
