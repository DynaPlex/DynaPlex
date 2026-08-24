# Training policies

Sample collection, training algorithms, featurizers, and the trained
agents and networks they produce. Everything on this page is imported
from the top-level `dynaplex` package, except the observation-spec
vocabulary (`TensorSpec`, `Dtype`, and friends), which lives in
`dynaplex.modelling`. For assessing the resulting policies, see
[evaluating policies](evaluation.md).

## Sample collection and training

::: dynaplex.DCL

::: dynaplex.PPO

::: dynaplex.PPOConfig

::: dynaplex.SampleSet

## Built-in policies

::: dynaplex.RandomPolicy

## Featurizers

A featurizer turns a decision state into the tensors a network consumes.
Its writer fields declare the representation and `write_features` fills one
batch row through them; `spec()` declares the **observation spec** — a plain
`dict[str, TensorSpec]`, one named tensor per writer field, batch axis
implicit. An observation batch is the matching dict of batched arrays
(everything downstream — network factories, [`SampleSet`][dynaplex.SampleSet]
slabs, [gym observations](../../training/gym-environments.md) — is keyed the
same way). `spec()` is optional on the class: when omitted, `@featurizer`
synthesizes one that sizes by counting on a probe state.

::: dynaplex.featurizer

::: dynaplex.GlobalStateWriter

::: dynaplex.modelling.FeaturizerProtocol
    options:
      members:
        - write_features

## The observation spec

::: dynaplex.modelling.TensorSpec

::: dynaplex.modelling.Dtype

::: dynaplex.modelling.feature_spec

::: dynaplex.modelling.GlobalStateCounter

::: dynaplex.modelling.probe_state

## Agents and networks

::: dynaplex.NNAgent

::: dynaplex.MLP

::: dynaplex.Net

::: dynaplex.train_network
