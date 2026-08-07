# API reference — `dynaplex.modelling`

The model-authoring vocabulary: everything you import when writing an MDP,
a State class, a policy, or a featurizer. (The `featurizer` decorator,
`GlobalStateWriter`, and the random generators are documented on the
[`dynaplex` page](core.md); the language rules for compiled code are in the
[language reference](../language-reference.md).)

## The MDP contract

::: dynaplex.modelling.MDPProtocol

::: dynaplex.modelling.PolicyProtocol

::: dynaplex.modelling.assert_mdp

::: dynaplex.modelling.assert_policy_for_mdp

::: dynaplex.modelling.assert_featurizer_for_mdp

## States and trajectories

::: dynaplex.modelling.StateCategory

::: dynaplex.modelling.HorizonType

::: dynaplex.modelling.TrajectoryContext

::: dynaplex.modelling.Validity

## Const classes and annotations

::: dynaplex.modelling.const_dataclass

::: dynaplex.modelling.Const

::: dynaplex.modelling.Rank

::: dynaplex.modelling.Array1D

::: dynaplex.modelling.Array2D

::: dynaplex.modelling.Array3D

::: dynaplex.modelling.ConstList

::: dynaplex.modelling.ConstArray1D

::: dynaplex.modelling.ConstArray2D

::: dynaplex.modelling.ConstArray3D

## Distributions and samplers

::: dynaplex.modelling.DiscreteDist
    options:
      filters: ["!^_", "!^cdf_sampler$"]

::: dynaplex.modelling.AliasSampler

## Copying state

::: dynaplex.modelling.clone
