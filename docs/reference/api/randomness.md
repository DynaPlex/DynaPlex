# Distributions and randomness

How random events are modelled and drawn. For *event distributions*,
prefer building a [`DiscreteDist`][dynaplex.modelling.DiscreteDist] (plus
a precomputed `dist.alias_sampler()`) over rolling your own from raw
draws; the generators further down are the underlying source of
randomness. The distribution classes live in `dynaplex.modelling`; the
generators are imported from the top-level `dynaplex` package. For a
high-level tour of what `DiscreteDist` can do — the supported families,
convolution and mixing, the three ways to draw, and the tail truncation to
be aware of — see [Discrete distributions](../discrete-distributions.md).

## Distributions and samplers

::: dynaplex.modelling.DiscreteDist
    options:
      filters: ["!^_", "!^cdf_sampler$"]

::: dynaplex.modelling.AliasSampler

## Random number generation

::: dynaplex.default_rng

::: dynaplex.Generator
