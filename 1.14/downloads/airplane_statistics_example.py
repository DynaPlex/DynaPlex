"""
Airplane ticket selling MDP, extended with per-trajectory statistics.

This builds on airplane_mdp_example.py. The MDP and the policy are the same;
the additions are:

1. A custom trajectory context (AirplaneContext) carrying four statistics:
   accepted and rejected customers per type, revenue per selling day, and
   whether the flight sold out — plus, commented out, the optional typed
   holder (AirplaneStats) for pyright.
2. One line on the MDP, make_context(), that constructs the context.
3. `+=` updates in modify_state_with_action, where the MDP already knows the
   customer type, the price and the day.
4. Analysis of the raw per-trajectory arrays PolicyComparer hands back,
   including a paired comparison of two parameterisations of the rule.

NOTE: this example targets the trajectory-context statistics feature
(roadmap/comparer_statistics.md) and does not run on released DynaPlex yet.
"""

from __future__ import annotations

from dataclasses import dataclass
from typing import Final

import numpy as np
from numpy.typing import NDArray

from dynaplex import PolicyComparer
from dynaplex.modelling import (
    Featurizer,
    AliasSampler,
    Array1D,
    Array2D,
    DiscreteDist,
    GlobalStateWriter,
    HorizonType,
    StateCategory,
    TrajectoryContext,
    TrajectoryStats,
    Validity,
    assert_mdp,
    assert_policy_for_mdp,
    const_dataclass,
    featurizer,
    new_context,
    trajectory_context,
)
from dynaplex.validation import mdp, policy


# ============================================================================
# State (unchanged from the basic tutorial)
# ============================================================================

@dataclass(slots=True)
class State:
    remaining_days: int
    remaining_seats: int
    customer_type: int
    price_offered_per_seat: int
    category: StateCategory = StateCategory.AWAIT_EVENT


# ============================================================================
# Trajectory context with statistics — NEW
# ============================================================================
#
# PolicyComparer can collect additional per-trajectory statistics, returned as
# `assessment.stats` by assess() and compare(). To do that, define a context
# class that inherits from TrajectoryContext, decorate it with
# @trajectory_context, and add a field per statistic. The MDP keeps them up to
# date during the simulation (see modify_state_with_action below); the comparer
# harvests them after every trajectory.
#
# What @trajectory_context generates for you (all compiled, called by the kernels):
#   reseed(seed, ...)      one independent random stream per Final[Generator] field
#   reset()                zero every scalar, zero-fill every array (streams run on)
#   write_stats(out, row)  copy the statistics into row `row` of the holder
#   stats_spec()           dtype/shape of every statistic, read off the instance
#   Stats                  the holder class the comparer fills: every statistic under
#                          the same name with one leading trajectory axis, derived
#                          automatically as AirplaneContext.Stats
#
# Optionally, declare the holder yourself and name it with `Stats = ...` in the
# class body (uncomment below). Runtime is identical; the gain is typing: pyright
# then knows PolicyComparer(mdp).assess(...).stats is an AirplaneStats, and you
# can annotate your own analysis functions with it. The fields must match the
# context's statistics, with one extra leading axis each (a scalar becomes an
# Array1D, a 1-D array an Array2D) — the decorator checks this at import.

# @dataclass
# class AirplaneStats(TrajectoryStats):               # base: cumulative_cost, time_elapsed
#     accepted_per_type: Final[Array2D[np.int64]]     # [n, num_customer_types]
#     rejected_per_type: Final[Array2D[np.int64]]     # [n, num_customer_types]
#     revenue_per_day: Final[Array2D[np.float64]]     # [n, initial_days]
#     stocked_out: Final[Array1D[np.bool_]]           # [n]


@trajectory_context
@dataclass
class AirplaneContext(TrajectoryContext):
    """Per-trajectory bookkeeping: the members every context has (inherited from
    TrajectoryContext: the two random streams, cumulative_cost, time_elapsed and
    the validity scratch) plus the statistics we want back from the comparer.

    Scalars (float/int/bool) may be re-assigned; every other field is Final
    and mutated in place.
    """
    accepted_per_type: Final[NDArray[np.int64]]     # [num_customer_types]
    rejected_per_type: Final[NDArray[np.int64]]     # [num_customer_types]
    revenue_per_day: Final[NDArray[np.float64]]     # [initial_days]
    stocked_out: bool                               # did the flight sell out?

    # Stats = AirplaneStats                         # opt in to the typed holder declared above

    def __init__(self, mdp: AirplaneMDP) -> None:
        super().__init__(mdp)             # the base members (placeholder seeds; reseeded before use)
        # Shapes come straight from the MDP — the one place they are stated.
        self.accepted_per_type = np.zeros(mdp.num_customer_types, dtype=np.int64)
        self.rejected_per_type = np.zeros(mdp.num_customer_types, dtype=np.int64)
        self.revenue_per_day = np.zeros(mdp.initial_days)
        self.stocked_out = False


# ============================================================================
# MDP — one added field, one added method, three added lines
# ============================================================================

@mdp
@const_dataclass(init=False, slots=True)
class AirplaneMDP:
    initial_days: int
    initial_seats: int
    prices_per_customer_type: NDArray[np.int64]   # 1-D by default; use Array2D etc. for more dimensions
    num_customer_types: int                # NEW: sizes the per-type statistics
    average_price: float
    customer_type_dist: DiscreteDist
    customer_type_sampler: AliasSampler
    num_actions: int
    horizon_type: HorizonType

    def __init__(
        self,
        initial_days: int,
        initial_seats: int,
        prices_per_customer_type: list[int],
        customer_type_probs: list[float],
    ):
        assert initial_days > 0 and initial_seats > 0
        assert prices_per_customer_type and all(price > 0 for price in prices_per_customer_type)
        assert customer_type_probs and len(customer_type_probs) == len(prices_per_customer_type)
        assert all(prob >= 0 for prob in customer_type_probs) and np.isclose(sum(customer_type_probs), 1.0, atol=1e-6)

        self.initial_days = initial_days
        self.initial_seats = initial_seats
        self.prices_per_customer_type = np.array(prices_per_customer_type)
        self.num_customer_types = len(prices_per_customer_type)
        self.average_price = sum(prices_per_customer_type) / len(prices_per_customer_type)
        self.customer_type_dist = DiscreteDist.custom(customer_type_probs)
        self.customer_type_sampler = self.customer_type_dist.alias_sampler()
        self.num_actions = 2
        self.horizon_type = HorizonType.FINITE

    def make_context(self) -> AirplaneContext:
        """Declares (by its return annotation) and constructs the context this
        MDP runs with — the sibling of get_initial_state for the state."""
        return AirplaneContext(self)

    def get_initial_state(self, context: AirplaneContext) -> State:
        return State(
            remaining_days=self.initial_days,
            remaining_seats=self.initial_seats,
            customer_type=0,
            price_offered_per_seat=0,
            category=StateCategory.AWAIT_EVENT,
        )

    def modify_state_with_event(self, state: State, context: AirplaneContext) -> None:
        state.customer_type = self.customer_type_sampler.sample(context.rng)
        state.price_offered_per_seat = int(self.prices_per_customer_type[state.customer_type])
        state.category = StateCategory.AWAIT_ACTION
        context.time_elapsed += 1

    def modify_state_with_action(self, state: State, context: AirplaneContext, action: int) -> None:
        assert state.remaining_days > 0, "No selling days left"
        state.remaining_days -= 1
        # The current day, 0-based. We could read context.time_elapsed here, but the
        # MDP promises would not let us: that creates a dependency on the harness's
        # clock (time-accumulate-only), so the day is derived from the state instead.
        day = self.initial_days - state.remaining_days - 1

        if action == 0:
            context.rejected_per_type[state.customer_type] += 1          # NEW
            state.price_offered_per_seat = 0
        elif action == 1:
            assert state.remaining_seats > 0, "Cannot accept customer: no seats available"
            state.remaining_seats -= 1
            context.cumulative_cost -= state.price_offered_per_seat
            context.accepted_per_type[state.customer_type] += 1          # NEW
            context.revenue_per_day[day] += state.price_offered_per_seat  # NEW
            if state.remaining_seats == 0:
                context.stocked_out = True                                # NEW
            state.price_offered_per_seat = 0
        else:
            assert False, f"Invalid action: Must be 0 (reject) or 1 (accept)"

        if state.remaining_days == 0:
            state.category = StateCategory.FINAL
        else:
            state.category = StateCategory.AWAIT_EVENT

    def write_action_validity(self, state: State, valid: Validity) -> None:
        valid.set(0, True)
        valid.set(1, state.remaining_seats > 0)


# ============================================================================
# Policy (unchanged)
# ============================================================================

@policy
@const_dataclass(slots=True)
class SimplePolicy:
    mdp: AirplaneMDP
    seat_threshold: int = 5
    days_threshold: int = 9
    min_price_low_days: int = 2000
    min_price_high_days: int = 3000

    def get_action(self, state: State) -> int:
        if state.remaining_seats == 0:
            return 0
        elif state.remaining_seats > self.seat_threshold:
            return 1
        elif state.remaining_days <= self.days_threshold and state.price_offered_per_seat >= self.min_price_low_days:
            return 1
        elif state.remaining_days > self.days_threshold and state.price_offered_per_seat >= self.min_price_high_days:
            return 1
        else:
            return 0


# ============================================================================
# Featurizer (unchanged from the basic tutorial; used by the gym/DCL tests)
# ============================================================================

@featurizer
@dataclass(slots=True)
class AirplaneFeaturizer(Featurizer):
    mdp: Final[AirplaneMDP]
    v: Final[GlobalStateWriter]

    def write_features(self, state: State) -> None:
        self.v.append(state.remaining_days / self.mdp.initial_days)
        self.v.append(state.remaining_seats / self.mdp.initial_seats)
        self.v.append(state.price_offered_per_seat / self.mdp.average_price)


# ============================================================================
# Manual simulation: the context is now the MDP's own
# ============================================================================

def simulate_episode(mdp: AirplaneMDP, policy: SimplePolicy, *, seed: int = 42) -> None:
    context = new_context(mdp, seed)   # -> AirplaneContext, via mdp.make_context()
    state = mdp.get_initial_state(context)
    while state.category != StateCategory.FINAL:
        if state.category == StateCategory.AWAIT_EVENT:
            mdp.modify_state_with_event(state, context)
        else:
            mdp.modify_state_with_action(state, context, policy.get_action(state))
    print(f"revenue €{-context.cumulative_cost:.0f}")
    print(f"accepted per type {context.accepted_per_type}, rejected per type {context.rejected_per_type}")
    print(f"revenue per day {context.revenue_per_day}")
    print(f"sold out: {context.stocked_out}")


# ============================================================================
# Evaluation: raw per-trajectory statistics, summarised in numpy
# ============================================================================

# note - if we define the stats explicitly, then we can annotate stats : AirplaneStats here
# and everything will be strongly typed. 

def report(name: str, stats) -> None:
    """stats is AirplaneContext.Stats, the derived holder (declare AirplaneStats
    and set `Stats = AirplaneStats` above to annotate this parameter with it).
    stats.<field> is the context field with a leading trajectory axis:
    accepted_per_type [n, 3], rejected_per_type [n, 3], revenue_per_day [n, 25],
    stocked_out [n], cumulative_cost [n], time_elapsed [n]."""
    n = len(stats.cumulative_cost)
    arrivals = stats.accepted_per_type + stats.rejected_per_type          # [n, 3]
    acceptance_rate = stats.accepted_per_type.sum(0) / arrivals.sum(0)   # [3]
    seats_sold = stats.accepted_per_type.sum(1)                           # [n]
    print(f"{name}: n={n}, revenue €{-stats.cumulative_cost.mean():.0f} "
          f"± {stats.cumulative_cost.std(ddof=1) / np.sqrt(n):.0f}")
    print(f"  acceptance rate per type: {np.round(acceptance_rate, 3)}")
    print(f"  seats sold: {seats_sold.mean():.2f} ± {seats_sold.std(ddof=1) / np.sqrt(n):.2f}")
    print(f"  flights sold out: {100 * stats.stocked_out.mean():.1f}%")
    print(f"  mean revenue per day (first 5 / last 5): "
          f"{np.round(stats.revenue_per_day.mean(0)[:5])} ... {np.round(stats.revenue_per_day.mean(0)[-5:])}")


def main() -> None:
    mdp = AirplaneMDP(
        initial_days=25,
        initial_seats=10,
        prices_per_customer_type=[3000, 2000, 1000],
        customer_type_probs=[0.4, 0.3, 0.3],
    )
    policy = SimplePolicy(mdp=mdp)
    # A second parameterisation of the same rule: holds out for fewer seats,
    # switches to the low thresholds later, and asks less at both stages.
    policy2 = SimplePolicy(mdp=mdp, seat_threshold=3, days_threshold=5,
                           min_price_low_days=1500, min_price_high_days=2500)
    assert_mdp(mdp)
    assert_policy_for_mdp(mdp, policy)

    simulate_episode(mdp, policy, seed=42)

    comparer = PolicyComparer(mdp, number_of_trajectories=10000, seed=0)
    assessment = comparer.assess(policy)
    print(f"Average profit: €{-assessment.mean:.2f} (SE {assessment.error:.2f})")
    report("simple rule", assessment.stats)

    results = comparer.compare({"simple rule": policy, "eager rule": policy2})
    print(results)                                  # the cost table, as before

    rule, eager = results["simple rule"].stats, results["eager rule"].stats
    report("eager rule", eager)

    # Paired comparison: row i of every holder is trajectory i under common
    # random numbers, so per-trajectory differences are meaningful.
    d = eager.accepted_per_type[:, 0] - rule.accepted_per_type[:, 0]     # type-0 (€3000) customers
    print(f"€3000 customers accepted per flight, eager minus simple (paired): "
          f"{d.mean():+.2f} ± {d.std(ddof=1) / np.sqrt(len(d)):.2f}")


if __name__ == "__main__":
    main()
