# Airplane MDP with custom statistics

!!! note "Version"
    Trajectory-context statistics ship in the release after 1.12.3; the code
    on this page does not run on older wheels.

The [airplane tutorial](../tutorials/airplane-mdp.md) evaluates policies on a single
number: the total cost (negative revenue) per trajectory. Real studies want
more. For the airline, natural questions are:

- How many customers of each type does a policy accept, and how many does it
  turn away?
- How is revenue spread over the 25 selling days?

DynaPlex answers these with **trajectory-context statistics**: you declare a
context class carrying the statistics you care about, update them inside the
MDP where the information is already in scope, and `PolicyComparer` hands
back one array per statistic with a row per trajectory. This page extends the
airplane example step by step; the full file is at the bottom.

## 1. Declare a context

Every trajectory runs with a *context* — the object holding the random
streams, the cumulative cost and the elapsed time. The basic tutorial uses
the library's `TrajectoryContext`. To add statistics, declare your own:

```python
@trajectory_context
@dataclass
class AirplaneContext(TrajectoryContext):
    # --- statistics ---
    accepted_per_type: Final[NDArray[np.int64]]     # [num_customer_types]
    rejected_per_type: Final[NDArray[np.int64]]     # [num_customer_types]
    revenue_per_day: Final[NDArray[np.float64]]     # [initial_days]
    stocked_out: bool                               # did the flight sell out?

    def __init__(self, mdp: AirplaneMDP) -> None:
        super().__init__(mdp)              # the base members: random streams, cost, time, validity scratch
        self.accepted_per_type = np.zeros(mdp.num_customer_types, dtype=np.int64)
        self.rejected_per_type = np.zeros(mdp.num_customer_types, dtype=np.int64)
        self.revenue_per_day = np.zeros(mdp.initial_days)
        self.stocked_out = False
```

The base class contributes the members every context has — the event and
policy random streams, `cumulative_cost`, `time_elapsed` and the action-validity
scratch — and `super().__init__(mdp)` fills them in (with placeholder seeds;
every generator is reseeded before use). Below them, any number of statistics:

| Field type | Meaning |
|---|---|
| `float`, `int`, `bool` | a scalar statistic; re-assign freely (`context.stocked_out = True`) |
| `Final[NDArray[T]]` (1-D; `Array2D[T]` etc. for more dimensions; float/int/bool) | a fixed-shape array statistic; mutate in place (`context.x[i] += 1`) |
| `Final[Generator]` | an extra random stream, reseeded with the others |
| `Scratch[int]`, `Final[Scratch[NDArray[T]]]`, ... | per-trajectory scratch owned by a context-driven policy: never collected (not in `Stats`), never reset by the kernels |

Scalars are the only fields you re-assign; everything else is `Final` and
created once in `__init__`. Lists are not allowed — a statistic has a fixed
shape, so use an array. Taking the MDP as the constructor argument keeps the
array shapes in one place: the context reads `mdp.num_customer_types` and
`mdp.initial_days` rather than repeating the numbers.

### What the decorator generates

`@trajectory_context` checks the field table above and then writes the
housekeeping for you — all of it compiled, and called by the kernels once per
trajectory:

| Generated | Does |
|---|---|
| `reseed(seed, ...)` | gives every `Final[Generator]` field its own independent stream |
| `reset()` | zeroes every scalar and zero-fills every array in place (the streams run on) |
| `write_stats(out, row)` | copies the statistics into row `row` of the holder |
| `stats_spec()` | dtype and shape of every statistic, read off the instance |
| `AirplaneContext.Stats` | the *holder* class the comparer fills: every statistic under the same name, with one leading trajectory axis |

So the class body is just the declaration and the constructor.

### Optional: declare the holder yourself

The derived `AirplaneContext.Stats` is a real class at runtime but invisible
to pyright: `assessment.stats` shows as `Any`, and there is no name to
annotate your own analysis functions with. If you want the type, declare the
holder and name it in the class body with `Stats = ...`:

```python
@dataclass
class AirplaneStats(TrajectoryStats):               # base: cumulative_cost, time_elapsed
    accepted_per_type: Final[Array2D[np.int64]]     # [n, num_customer_types]
    rejected_per_type: Final[Array2D[np.int64]]     # [n, num_customer_types]
    revenue_per_day: Final[Array2D[np.float64]]     # [n, initial_days]
    stocked_out: Final[Array1D[np.bool_]]           # [n]


@trajectory_context
@dataclass
class AirplaneContext(TrajectoryContext):
    Stats = AirplaneStats                           # opt in to the typed holder
    ...
```

The holder mirrors the context: the same field names, and every field gets
one extra leading axis — the scalar `stocked_out` becomes a 1-D array, the
1-D arrays become `Array2D`. Every holder field is `Final` (it is filled in
place, row by row). The decorator checks that the two match (names, dtypes,
ranks) when the class is defined, and pyright then knows that
`PolicyComparer(mdp).assess(...).stats` is an `AirplaneStats`. Runtime is
identical either way; the full code at the bottom ships with this block
commented out.

## 2. Tell the MDP about it

One method, whose return annotation declares the context type and whose body
constructs it — exactly like `get_initial_state` does for the state:

```python
@const_dataclass(init=False, slots=True)
class AirplaneMDP:
    ...
    num_customer_types: int                # added: sizes the per-type statistics

    def make_context(self) -> AirplaneContext:
        return AirplaneContext(self)

    def get_initial_state(self, context: AirplaneContext) -> State: ...
    def modify_state_with_event(self, state: State, context: AirplaneContext) -> None: ...
    def modify_state_with_action(self, state: State, context: AirplaneContext, action: int) -> None: ...
```

The `context` parameters of the transition functions are annotated with the
same class. DynaPlex checks that all of them agree (and `assert_mdp` lets
pyright check it in the editor).

## 3. Update the statistics where the information is

The MDP already knows, inside `modify_state_with_action`, which customer type
arrived, whether it was accepted and what day it is. A few lines:

```python
    def modify_state_with_action(self, state: State, context: AirplaneContext, action: int) -> None:
        assert state.remaining_days > 0, "No selling days left"
        state.remaining_days -= 1
        day = context.time_elapsed - 1        # the event incremented time_elapsed

        if action == 0:
            context.rejected_per_type[state.customer_type] += 1
            state.price_offered_per_seat = 0
        elif action == 1:
            assert state.remaining_seats > 0, "Cannot accept customer: no seats available"
            state.remaining_seats -= 1
            context.cumulative_cost -= state.price_offered_per_seat
            context.accepted_per_type[state.customer_type] += 1
            context.revenue_per_day[day] += state.price_offered_per_seat
            if state.remaining_seats == 0:
                context.stocked_out = True
            state.price_offered_per_seat = 0
        ...
```

Nothing else in the MDP changes. In particular the *state* is untouched: the
statistics are not part of the Markov state, so they do not affect the
policy (or, later, a featurizer and training).

## 4. Get the statistics back

Nothing changes in how you evaluate. Build the MDP and a `PolicyComparer` as
in [Comparing policies](../training/policy-comparison.md) and assess the rule:

```python
mdp = AirplaneMDP(initial_days=25, initial_seats=10,
                  prices_per_customer_type=[3000, 2000, 1000],
                  customer_type_probs=[0.4, 0.3, 0.3])
policy = SimplePolicy(mdp=mdp)

comparer = PolicyComparer(mdp, number_of_trajectories=10000, seed=0)
assessment = comparer.assess(policy)
print(f"Average profit: €{-assessment.mean:.2f} (SE {assessment.error:.2f})")
```

```text
Average profit: €25082.60 (SE 19.95)
```

That is the cost table's number (negated: the MDP books revenue as negative
cost). New is `assessment.stats`, the holder with one array per statistic and
a row per trajectory:

```python
stats = assessment.stats                 # AirplaneContext.Stats (an AirplaneStats, if you declared it)

stats.accepted_per_type                  # int64   [10000, 3]
stats.rejected_per_type                  # int64   [10000, 3]
stats.revenue_per_day                    # float64 [10000, 25]
stats.stocked_out                        # bool    [10000]
stats.cumulative_cost                    # float64 [10000]
stats.time_elapsed                       # int64   [10000]
```

There is no summary API on purpose: the arrays are raw, and you compute what
the study needs in numpy. The questions from the top of the page, answered:

```python
n = len(stats.cumulative_cost)
arrivals = stats.accepted_per_type + stats.rejected_per_type          # [n, 3]
acceptance_rate = stats.accepted_per_type.sum(0) / arrivals.sum(0)   # per customer type
seats_sold = stats.accepted_per_type.sum(1)                           # per trajectory
revenue_curve = stats.revenue_per_day.mean(0)                         # per selling day

print(f"acceptance rate per type: {np.round(acceptance_rate, 3)}")
print(f"seats sold: {seats_sold.mean():.2f} ± {seats_sold.std(ddof=1) / np.sqrt(n):.2f}")
print(f"flights sold out: {100 * stats.stocked_out.mean():.1f}%")
print(f"mean revenue per day (first 5 / last 5): {np.round(revenue_curve[:5])} ... {np.round(revenue_curve[-5:])}")
```

```text
acceptance rate per type: [0.658 0.254 0.201]
seats sold: 10.00 ± 0.00
flights sold out: 99.9%
mean revenue per day (first 5 / last 5): [2092. 2100. 2104. 2102. 2110.] ... [133.  69.  30.  15.   8.]
```

Already a picture: the rule takes two thirds of the €3000 customers and a
fifth of the €1000 ones, essentially every flight sells out, and revenue is
front-loaded — the last days sell almost nothing because the seats are gone.

### Comparing policies on their statistics

`compare` works as before and prints the cost table; every result carries its
own `stats`. Here a second parameterisation of the same rule, which holds out
for fewer seats and asks less:

```python
policy2 = SimplePolicy(mdp=mdp, seat_threshold=3, days_threshold=5,
                       min_price_low_days=1500, min_price_high_days=2500)
results = comparer.compare({"simple rule": policy, "eager rule": policy2})
print(results)
```

```text
policy                 mean       error    delta_mean  delta_error
simple rule *      -25082.6       19.95             0            0
eager rule         -23670.8       22.22        1411.8        11.83
(* = benchmark; delta = policy - benchmark, paired on common random numbers)
```

The eager rule earns €1412 less per flight. The statistics say why. The
guarantee that makes this possible: **row `i` is trajectory `i` under common
random numbers, for every policy** — the same customers arrived, in the same
order, at the same prices. Differences between policies can therefore be
taken row by row, with paired standard errors:

```python
rule, eager = results["simple rule"].stats, results["eager rule"].stats

print(f"acceptance rate per type, eager: {np.round(eager.accepted_per_type.sum(0) / arrivals.sum(0), 3)}")
print(f"seats sold, eager: {eager.accepted_per_type.sum(1).mean():.2f}, flights sold out: {100 * eager.stocked_out.mean():.1f}%")
d = eager.accepted_per_type[:, 0] - rule.accepted_per_type[:, 0]   # €3000 customers, per flight
print(f"€3000 customers accepted per flight, eager minus simple (paired): "
      f"{d.mean():+.2f} ± {d.std(ddof=1) / np.sqrt(len(d)):.2f}")
```

```text
acceptance rate per type, eager: [0.576 0.283 0.281]
seats sold, eager: 10.00, flights sold out: 99.9%
€3000 customers accepted per flight, eager minus simple (paired): -0.81 ± 0.01
```

Both rules sell all ten seats (10.00 above, and 10.00 for the simple rule in
the previous section); the eager one gives 0.81 of them per flight
to a €2000 or €1000 customer instead of a €3000 one — and thanks to the
pairing, that number has a standard error of 0.01, far tighter than a
comparison of the two independent acceptance rates would allow.

## Manual simulation

Stepping through an episode by hand works as in the basic tutorial;
`new_context(mdp, seed)` builds and seeds the MDP's own context:

```python
context = new_context(mdp, seed)
state = mdp.get_initial_state(context)
...
```

At the end, the statistics are simply the context's fields
(`context.accepted_per_type`, `context.stocked_out`, ...).

## Full code

Download: [airplane_statistics_example.py](../downloads/airplane_statistics_example.py).

```python title="airplane_statistics_example.py" linenums="1"
--8<-- "downloads/airplane_statistics_example.py"
```
