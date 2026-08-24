# Featurizers

!!! warning "Under development"
    The featurizer system is being actively developed and its API may still
    change. This page documents only the **flat featurizer** — a single flat
    vector of scalar features per state. Richer, structured observations
    (multi-field bundles, and object-token representations for
    attention / pointer-network policies) already exist in the package but are
    still evolving and are intentionally not documented here yet.

A **featurizer** defines the numeric state representation fed to a neural
network — the encoding of a state that an [`NNAgent`](../reference/api/training.md),
[PPO](ppo.md), or [DCL](dcl.md) policy consumes. It is declared **separately
from the MDP**, so the same MDP can be paired with different representations
without changing the model.

## The flat featurizer

The simplest featurizer writes a **flat vector of scalar features** for each
state through a single `GlobalStateWriter`:

```python
from dataclasses import dataclass
from dynaplex import featurizer, GlobalStateWriter


@featurizer
@dataclass(slots=True)
class AirplaneFeaturizer:
    mdp: AirplaneMDP
    v: GlobalStateWriter

    def write_features(self, state: State) -> None:
        # Must be valid DynaML. Each append adds one scalar to the flat vector,
        # in a fixed order.
        self.v.append(state.remaining_days / self.mdp.initial_days)
        self.v.append(state.remaining_seats / self.mdp.initial_seats)
        self.v.append(state.price_offered_per_seat / self.mdp.average_price)
```

The essentials:

- Decorate a `@dataclass(slots=True)` with `@featurizer`.
- Declare an `mdp` field (the MDP this representation is for) and one **writer**
  field. For the flat featurizer that writer is a `GlobalStateWriter`.
- `write_features(self, state)` fills one row by `append`-ing scalars in a fixed
  order. Its body must be valid DynaML — the same restricted subset used for the
  MDP's own methods (see the
  [language reference](../reference/language-reference.md)).
- `@featurizer` derives a `Holder` class and synthesizes the field-walks
  (install / reset / finish) and a `spec()` describing the observation. `spec()`
  is auto-sized by counting the `append`s on a probe state, so a flat featurizer
  needs no hand-written spec.

## Using a featurizer

A featurizer is supplied to the training and agent APIs as the observation
representation. See [PPO](ppo.md) and [Deep Controlled Learning](dcl.md) for how
a featurizer is wired into training, and [Comparing policies](policy-comparison.md)
for evaluating the resulting neural-network policy.

## Checking your featurizer statically

`assert_featurizer_for_mdp(mdp, AirplaneFeaturizer)` does nothing at runtime, but
it lets [pyright](../reference/language-reference.md) verify that the featurizer
matches the MDP's state type and the expected featurizer interface — catching
mistakes before anything is compiled.
