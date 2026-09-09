# Discrete distributions

`DiscreteDist` is the DynaPlex way of working with integer-valued random
quantities: demand per period, lead times, batch sizes, customer types,
item weights. It is a small immutable value that holds an explicit
probability mass function over a finite range of integers, and everything
you would want to do with such a distribution — build it, inspect it, combine
it with others, draw from it — is a method on it. This page is a high-level
inventory of what it can do; the signatures are in the
[API reference](api/randomness.md).

The short version: whenever your model has a discrete distribution, express
it as a `DiscreteDist` rather than as hand-written code over raw generator
draws. That is what the tutorials do, what the validation machinery
recognizes, and what keeps the model exact and reproducible.

```python
from dynaplex.modelling import DiscreteDist

demand = DiscreteDist.poisson(3.2)          # a distribution
demand.expectation(), demand.fractile(0.95)  # 3.2, 6

lead_time_demand = demand.add(demand).add(demand)   # convolution: 3 periods

sampler = demand.alias_sampler()             # build once, e.g. in the MDP's __init__ ...
x = sampler.sample(rng)                      # ... then draw very fast, every period
```

## Runs in plain Python and in compiled code

A `DiscreteDist` works in two places, with identical results:

- **In CPython.** Build and inspect distributions in an ordinary Python
  session, in the MDP's `__init__`, in a notebook, in tests. The factories
  call the same C++ kernel that compiled code uses, so a distribution built in
  Python holds exactly the bytes compiled code would produce.
- **In compiled DynaML.** Store the distribution (or a sampler built from it)
  in a field of the MDP and call its methods from `modify_state_with_event`,
  from a policy, from a featurizer. Every method on this page is callable
  from compiled code.

Distributions are deeply const: once built, neither the probabilities nor the
offset can change, in either mode. This is what lets them be shared freely
across workers and worlds.

## Supported distributions

Factories on the class build a distribution from parameters:

| Family | Factory |
| --- | --- |
| Point mass | `DiscreteDist.constant(value)` |
| Arbitrary PMF | `DiscreteDist.custom(probs, offset=0)` — `probs[i]` is the probability of `offset + i`; the offset may be negative |
| Poisson | `DiscreteDist.poisson(mean)` |
| Geometric on `{0, 1, ...}` | `DiscreteDist.geometric(mean)` or `DiscreteDist.geometric_from_prob(p)` |
| Binomial | `DiscreteDist.binomial(n, p)` |
| Negative binomial | `DiscreteDist.negative_binomial(r, p)` |
| Two-moment fit | `DiscreteDist.adan_eenige_resing(mean, stdev)` |

The two-moment fit deserves a word. In practice a demand process is often
specified only by a mean and a standard deviation, typically from a forecast.
`adan_eenige_resing` turns those two numbers into a concrete distribution
using the fit of Adan, van Eenige and Resing (1995): depending on the
squared coefficient of variation it selects a Poisson, a mixture of two
binomials, a mixture of two negative binomials, or a mixture of two
geometrics, so that the result has exactly the requested mean and standard
deviation. Not every pair is attainable by an integer distribution (a mean of
2.5 cannot have a standard deviation of 0.1); `least_variance_for_moment_fit(mean)`
tells you the floor.

`custom` validates its input: probabilities must be nonnegative and sum to
one within a small tolerance, in CPython and in compiled code alike.

## Inspecting a distribution

Every distribution answers the usual questions:

- `min()` and `max()` — the support;
- `probability_at(value)` — the PMF, zero outside the support;
- `expectation()`, `variance()`, `std()`, `entropy()`;
- `fractile(alpha)` — the smallest value whose cumulative probability
  reaches `alpha`, the building block of base-stock levels and service-level
  targets.

These are cheap, deterministic, and usable from compiled code, so a policy
can, for instance, compute an order-up-to level from the lead-time demand
distribution on the fly.

## Combining distributions

Distributions compose into new distributions. All of these return a fresh
`DiscreteDist`; the inputs are untouched.

- **Convolution, `a.add(b)`** — the distribution of the sum of two
  independent draws. Chain it to get the demand over a lead time, the total
  weight of a batch, the workload of a shift. This is exact, not a
  simulation.
- **Mixture, `a.mix(b, prob_of_other)`** — with probability `prob_of_other`
  draw from `b`, otherwise from `a`. Use it for regime switching, a
  fraction of lost or expedited orders, or a distribution with an atom at
  zero.
- **Censoring, `a.take_maximum_with(value)`** — the distribution of
  `max(X, value)`; combine with `invert()` (the distribution of `-X`) to get a
  `min`.
- **Negation, `a.invert()`** — the distribution of `-X`, so that subtraction
  is `a.add(b.invert())`.

Because these are ordinary values, you can build a whole family of derived
distributions in `__init__` — one per lead time, one per regime — and keep
them in a list that the state indexes.

## Drawing from a distribution

There are three ways to draw, for three situations.

**A static distribution, drawn many times.** This is the usual case: the
distribution is fixed for the whole model and sampled every period. Build it
once, then precompute a sampler and draw through that:

```python
self.demand = DiscreteDist.poisson(3.2)
self.demand_sampler = self.demand.alias_sampler()   # in __init__
...
x = self.demand_sampler.sample(context.rng)         # in the event step
```

`alias_sampler()` gives the fastest draws: a constant cost per draw, however
wide the support, at two uniforms per draw.

**A distribution that depends on the state.** If the distribution object
itself is chosen per state — say from a list indexed by the current regime —
you can still keep a precomputed sampler per distribution. If you cannot,
`dist.sample(rng)` draws directly from the PMF by a linear scan, with no
precomputation. `dist.conditional_sample(rng, minimum_value)` draws from the
distribution conditioned on being at least `minimum_value`, without
rejection.

**Parameters that change every draw.** Forecast-driven demand has a different
mean every period; building a distribution per period would be wasteful.
The one-shot class methods draw a single value from a family without
materializing anything: `DiscreteDist.poisson_sample(mean, rng)`,
`binomial_sample(n, p, rng)`, `negative_binomial_sample(r, p, rng)`,
`geometric_sample(mean, rng)`, `geometric_from_prob_sample(p, rng)` and
`adan_eenige_resing_sample(mean, stdev, rng)`. They cost a few tens of
nanoseconds for typical means and are exact draws from the family.

All draws accept either generator family: DynaPlex's own `dynaplex.Generator`
or NumPy's `Generator`. See
[random number generation](language-reference.md#random-number-generation).

!!! note "Reproducibility"
    Draws through `sample`, `conditional_sample` and the precomputed samplers
    are bit-identical across CPython, interpreted and compiled execution, and
    across platforms, for the same seed. The one-shot family draws are
    draw-for-draw identical across the three execution modes on one machine
    but may differ across platforms; where cross-platform reproducibility
    matters, materialize the distribution and sample from that.

## Exact enumeration

Because a `DiscreteDist` is an explicit PMF, DynaPlex can *enumerate* it
rather than sample it. When an MDP's event step consists of draws from `DiscreteDist`s (or
samplers built from those) stored on the model, the distribution of the event
is known statically, and the model satisfies the
[discrete identifiable events](promises.md#discrete-identifiable-events)
promises that exact enumeration of the event step requires. This is one
more reason to express randomness through `DiscreteDist` instead of ad-hoc
draws: it keeps the door to exact methods open.

## Tails are truncated

A `DiscreteDist` is finite by construction. Families with infinite support
(Poisson, geometric, negative binomial) are cut off where the tail probability
drops below about `1e-16`, and leading or trailing probabilities of that size
are trimmed from any distribution. The truncated mass is *not* redistributed:
the probabilities that remain are the true ones, and they sum to one to
within the tolerance of floating point.

For simulation, optimization and training this is invisible. It does mean
that `DiscreteDist` is not the tool for computing the probability of rare
events. Convolutions and mixtures of truncated distributions compound the
cut-offs, and the validation tolerance on a PMF is of the order of `1e-8`, so
you should not rely on a `DiscreteDist` for probabilities of roughly one in a
hundred million or smaller. If your question is about the far tail, compute
it analytically.

## See also

- [API reference: distributions and randomness](api/randomness.md) — all
  signatures, including the samplers.
- [Language reference: distributions and samplers](language-reference.md#distributions-and-samplers)
  — what is compilable and how to store distributions in fields.
- The [airplane](../tutorials/airplane-mdp.md) and
  [bin packing](../tutorials/binpacking-mdp.md) tutorials — a customer-type
  distribution and an item-weight distribution, each drawn through an
  `AliasSampler`.
