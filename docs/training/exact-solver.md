# Exact solver

`ExactSolver` is DynaPlex's exact method for **finite-state MDPs**. It
enumerates every decision state reachable under the model's own dynamics,
computes the **exact** cost of any compiled policy — the long-run average cost
per period of an infinite-horizon model, the expected total cost of an episode
of a finite-horizon one — and finds the **optimal** policy by policy
iteration. Where the
[`PolicyComparer`](policy-comparison.md) estimates a cost from simulated
trajectories, the solver computes it to a chosen tolerance and, for the
optimum, tells you how much room any heuristic or trained agent leaves on the
table. It applies to models whose reachable state space fits in memory — up to
a few million states; everything larger stays with simulation.

The running example is the [bin packing MDP](../tutorials/binpacking-mdp.md)
on the instance the tutorial trains a DCL agent on: three bins of capacity 100
and weights 10, 20, 30, 40 or 50.

```python
import numpy as np
import dynaplex as dp
from dynaplex.modelling import DiscreteDist
from dynaplex.models.binpacking import BinPackingMDP, FirstFitPolicy, LowestWeightPolicy

weight_probs = np.zeros(41)
weight_probs[[0, 10, 20, 30, 40]] = [0.2, 0.3, 0.25, 0.15, 0.1]
mdp = BinPackingMDP(max_bin_size=100, number_of_bins=3,
                    weight_dist=DiscreteDist.custom(weight_probs, offset=10))
lowest_weight = LowestWeightPolicy(mdp=mdp)
first_fit = FirstFitPolicy(mdp=mdp)

solver = dp.ExactSolver(mdp)                    # discovers every reachable decision state
print(solver)
print("lowest-weight:", solver.evaluate(lowest_weight))
print("first-fit:    ", solver.evaluate(first_fit))
opt = solver.solve(warm_start=first_fit)        # the optimal policy
print("optimal:      ", opt)
```

```text
ExactSolver: 5000 states (0.67 s)
lowest-weight: cost/period 2.6546  (5000 states, 143 sweeps, 0 improvements, span 5.9e-07, 0.70 s)
first-fit:     cost/period 2.6546  (5000 states, 1144 sweeps, 0 improvements, span 9.0e-07, 0.76 s)
optimal:       cost/period 0.0688667  (5000 states, 88 sweeps, 8 improvements, span 2.0e-07, 0.74 s)
```

Three bins holding a multiple of 10 each, times five possible upcoming
weights, gives 5000 decision states. The two heuristics cost 2.65 per period,
and they cost *exactly the same* — the simulation in the tutorial could only
say they are within noise of each other. The optimum is 0.069 per period,
almost forty times lower: a bin that reaches exactly 100 empties at no cost,
and with weights in multiples of 10 a policy that aims for exact fills almost
never overflows. The tutorial's generation-3 DCL agent reaches 2.02, so the
solver also shows how far that agent still is from optimal.

Construction runs the discovery; `evaluate` and `solve` reuse it, so
evaluating several policies on one solver costs one discovery. Call
`solver.close()` when done to release the compiled engines.

## What the numbers mean

The metric is the MDP's **native cost** — the same number the comparer
estimates — computed exactly. Which number depends on the model's
`horizon_type`:

- **Infinite horizon** (`HorizonType.INFINITE`): `res.cost` is the long-run
  **average cost per period**, converged until the span of the value
  increments is below `epsilon` (default `1e-6`). `res.values` is the **bias**
  (relative value) per state; `res.get_bias_function()` wraps it as a callable
  `state -> float` for plain Python states (the best state has bias 0; a
  larger bias means a worse state to be in).
- **Finite horizon** (`HorizonType.FINITE`): `res.cost` is the **expected
  total cost** of an episode, from `get_initial_state` until the state becomes
  `FINAL`, converged until the largest value increment is below `epsilon`.
  `res.values` is the **expected cost-to-go** per decision state;
  `res.get_value_function()` wraps it as `state -> float`. The initial
  distribution is enumerated like the events: every discrete outcome of
  `get_initial_state`, advanced through events to the first decision, with
  the cost accrued on the way.
- `res.actions` is the action per state, indexed like `res.values` by the
  solver's global state index. `res.stats` carries `states`, `transitions`,
  `leaves` (the enumerated successors), `horizon`, `sweeps`, `improvements`,
  the final `span`, `seconds` and `discovery_seconds`. `print(res)` is the
  one-line summary shown above.

`evaluate(policy)` runs policy evaluation only: `improvements` is 0 and the
cost is that policy's. `solve()` alternates evaluation sweeps and policy
improvement (modified policy iteration, `m` evaluation sweeps per improvement)
until no state changes its action. `solve(warm_start=policy)` starts from a
policy's actions instead of the lowest valid action in every state; a good
warm start saves improvement rounds but does not change the optimum.

## A finite-horizon model: the airplane

The [airplane MDP](../tutorials/airplane-mdp.md) sells the seats of one flight
over a fixed number of days and ends when the days run out — a finite horizon.
The solver then reports the **expected total cost** of an episode (here the
negative revenue), from the initial state to the `FINAL` state, on the
tutorial's instance: 25 days, 10 seats, customers paying 3000, 2000 or 1000
with probabilities 0.4, 0.3 and 0.3.

```python
import dynaplex as dp
from dynaplex.models.airplane import AirplaneMDP, SimplePolicy

mdp = AirplaneMDP(initial_days=25, initial_seats=10,
                  prices_per_customer_type=[3000, 2000, 1000],
                  customer_type_probs=[0.4, 0.3, 0.3])
rule = SimplePolicy(mdp=mdp)

solver = dp.ExactSolver(mdp)
print("rule-based:", solver.evaluate(rule))
opt = solver.solve(warm_start=rule)
print("optimal:   ", opt)
```

```text
rule-based: expected total cost -25074.7  (660 states, 2 sweeps, 0 improvements, span 0.0e+00, 0.69 s)
optimal:    expected total cost -28769.2  (660 states, 4 sweeps, 2 improvements, span 0.0e+00, 0.68 s)
```

The rule-based policy earns 25 075 in expectation, the optimal policy 28 769 —
about 15% more, exactly, where the tutorial's `PolicyComparer` run puts the
same two policies at 25 124 ± 64 and 28 714 ± 54. Finite-horizon models need
no clock: a *stopping problem*, where reaching `FINAL` depends on the action
and states can repeat, is solved the same way, iterating to `epsilon`.

??? note "Sweep order"
    Finite-horizon sweeps update each state's value in place and, when the
    model defines an optional `sweep_rank(self, state) -> int`, visit states in
    ascending rank. The library's airplane model returns `remaining_days`, so
    the last day is swept first and the solve above is one backward-induction
    pass plus a confirmation — hence the low sweep counts. Without the method
    the solver sweeps in index order and converges all the same, in more
    passes. The hook is provisional and may change shape.

## What the solver needs from the model

The solver runs the model's own `modify_state_with_event` once per outcome of
its random draws, so a model that runs in the comparer runs in the solver, with
three conditions on top:

- **Episodes that end (finite horizon).** A finite-horizon model must reach a
  `FINAL` state with probability one under *every* policy — the assumption is
  the model's, not checked by the solver, exactly as in legacy DynaPlex. A
  model with an internal clock (the airplane's `remaining_days`) satisfies it
  by construction. In a stopping problem, where ending depends on the action,
  a policy that never stops has no finite total cost; the solver then reports
  non-convergence (or, when the cycle costs nothing, converges to a
  meaningless answer). How `time_elapsed` advances is irrelevant under a
  finite horizon; under an infinite horizon every decision-to-decision
  transition must advance it by exactly one period.
- **Discrete, enumerable events.** Every draw in the event step must be a
  discrete one the solver can enumerate — `.sample` on a `DiscreteDist`,
  `AliasSampler` or `CdfSampler`, a one-shot `DiscreteDist.<family>_sample`,
  or `rng.choice(n)` — and so must every draw in `get_initial_state` (a random
  start is enumerated as the initial distribution). This is the promise set
  `DISCRETE_IDENTIFIABLE_EVENTS`, described under
  [Promises → Discrete identifiable events](../reference/promises.md#discrete-identifiable-events);
  the solver checks it at construction and a continuous draw such as
  `rng.random()` is a compile error naming the line. Any number of draws, in
  any order and under any control flow, is fine: the bin packing event draws
  one weight, the lost-sales event draws one demand, and a model that draws a
  regime and then a regime-dependent demand enumerates the product.
- **A countable state.** State fields must be integers, booleans, enums,
  `list[int]`, `FifoQueue`, integer arrays, or nested objects of those. A
  float field is refused at construction, naming the field: an exactly
  solvable state takes finitely many values, so a float that does is an index
  and should be modelled as one.

Whether the reachable state space is *finite and small enough* is not
something the compiler can establish; the solver discovers it. Discovery
stops with an error when it passes `max_states` (default `1 << 22`, about
four million) or when a single `(state, action)` expands to more than
`max_event_branches` successors. The action validity mask is what usually
keeps a state space finite: in the lost-sales model, for example, the bound on
the inventory position in `write_action_validity` is what makes the model
solvable, and every model with unbounded accumulation needs such a bound.

**Policies** are compiled DynaML policies with `get_action(state)` or
`get_action(state, context)`, as in the comparer; a policy that draws from
the context cannot be evaluated exactly. `evaluate` expects the policy to
respect the validity mask: a policy that takes a forbidden action in a
discovered state fails loudly rather than silently. `solve` takes the mask as
given and optimizes over the valid actions.

## The optimal policy is a normal policy

`opt.get_policy()` returns the action table as a compiled policy — an
`ExactPolicy` holding the state encoding and one action per state. It goes
wherever a policy goes: into the comparer, as the generation-0 policy of
[DCL](dcl.md), or called on a plain Python state.

```python
exact = opt.get_policy()

comparer = dp.PolicyComparer(mdp, number_of_trajectories=100,
                             warmup_time=100, horizon=1000, seed=0)
print(comparer.compare({"LowestWeight": lowest_weight,
                        "FirstFit": first_fit,
                        "Exact": exact}))
```

```text
policy                  mean       error    delta_mean  delta_error
LowestWeight *        2.6381     0.01583             0            0
FirstFit              2.6298     0.01543       -0.0083     0.004839
Exact                 0.0711    0.003372        -2.567      0.01618
(* = benchmark; delta = policy - benchmark, paired on common random numbers)
```

The comparer's estimate of the exact policy, 0.071 ± 0.003, agrees with the
solver's 0.0689; the simulated heuristic costs agree with the exact 2.6546
within their standard errors. Running the exact policy through the comparer
is the natural way to put it in one table with policies the solver cannot
evaluate, such as a trained `NNAgent`.

The exact policy has an action only for states the solver discovered. Calling
it on an unreachable state (one the dynamics can never produce) raises an
assertion; build states through the model, or take them from
`solver.states()` as below.

## Inspecting the solution

`solver.states()` yields every discovered decision state as a plain Python
object together with its global index, in index order. With the bias function
and the policies as Python callables, the optimal decisions can be read state
by state:

```python
bias = opt.get_bias_function()
print("bins          arrival  optimal  first-fit  lowest  bias")
for state, _ in solver.states():
    if state.upcoming_weight == 40 and state.weight_vector[1] == state.weight_vector[2] == 0:
        print(f"{str(state.weight_vector):13} {state.upcoming_weight:7}  "
              f"{exact.get_action(state):7}  {first_fit.get_action(state):9}  "
              f"{lowest_weight.get_action(state):6}  {bias(state):5.3f}")
```

```text
bins          arrival  optimal  first-fit  lowest  bias
[0, 0, 0]          40        1          0       0  0.118
[10, 0, 0]         40        0          0       1  0.115
[20, 0, 0]         40        0          0       1  0.174
[30, 0, 0]         40        0          0       1  0.127
[40, 0, 0]         40        0          0       1  0.317
[50, 0, 0]         40        1          0       1  0.322
[60, 0, 0]         40        0          0       1  0.000
[70, 0, 0]         40        1          1       1  0.344
[80, 0, 0]         40        1          1       1  0.778
[90, 0, 0]         40        1          1       1  1.799
```

A weight of 40 arriving at a bin holding 60 completes it exactly, and that
state has bias 0: it is the best state to be in. At 50 the optimum keeps the
bin open for a 50 rather than fill it to 90; first fit adds the 40 anyway. A
bin holding 90 can only be completed by a 10, and the bias of 1.8 says what
that liability costs relative to the best state. Empty bins are
interchangeable, so the first row's choice of bin 1 over bin 0 is a tie.

## Discovery, state identity and scale

Discovery starts from the initial state and expands every valid action and
every event outcome, running the model's transition methods compiled in the
engine. `solver.report()` describes what it found:

```text
5000 states in 1 shape(s), 15000 transitions, 75000 leaves — perfect (mixed-radix) path
shape 0 [3]: 5000 states, index space 5000, density 1.000
  [0] weight_vector[0]: radix 10 (affine, 0..90)
  [1] weight_vector[1]: radix 10 (affine, 0..90)
  [2] weight_vector[2]: radix 10 (affine, 0..90)
  [3] upcoming_weight: radix 5 (affine, 10..50)
  [4] category: radix 1 (constant, 2..2)
```

A **shape** is the state class with its runtime dimensions filled in (list
lengths, queue lengths, array dims); a model whose lists never change length
has one shape. Within a shape the solver identifies a state by a **mixed-radix
code**: each field becomes a digit over the values it was seen to take, and the
code indexes the value and action tables directly. That is the *perfect* path.
When the states of a shape do not fit such a code densely enough, or the model
has several shapes, the solver falls back to the *table* path (a sorted row
table with binary-search lookup) and `get_policy()` returns an
`ExactTablePolicy` instead — the same policy interface, slightly slower to
call. `ExactSolver(path=...)` accepts `"auto"` (the default), `"perfect"` or
`"table"`.

The cost of a solve is roughly linear in the number of leaves (enumerated
successors) times the number of sweeps. On a laptop, the bin packing instance
with more bins:

| bins | states | discovery | evaluate first fit | solve |
| --- | --- | --- | --- | --- |
| 3 | 5 000 | 0.7 s | 0.8 s | 0.7 s |
| 4 | 50 000 | 1.0 s | 1.6 s | 1.2 s |
| 5 | 500 000 | 2.2 s | 7.8 s | 6.4 s |

The optimal cost falls from 0.069 to 0.0017 to 0.000007 per period as bins
are added, while both heuristics stay at 2.6546. This version runs
single-threaded and keeps the tables in memory; four million states is the
default guardrail.

## Options

```python
dp.ExactSolver(mdp,
               epsilon=1e-6,                # convergence: span of the value increments
               max_states=1 << 22,          # discovery guardrail
               max_event_branches=1 << 20,  # successors of one (state, action)
               m=10,                        # evaluation sweeps per improvement
               path="auto",                 # "auto" | "perfect" | "table"
               checks=False,                # True: run the model in checked mode
               verbose=False)               # print the discovery and each result
```

`checks=True` runs the model's transition methods with the runtime checks on
(bounds, overflow, asserts) — use it when a model misbehaves under
enumeration; the default is fast mode, since the model is validated elsewhere.
`verbose=True` prints the discovery report and every result as it is
computed.

!!! note "Not in this version"
    Parallel sweeps (`workers` must be 0) and persisting the tables to a work
    directory are planned; see
    [Planned features](../community/roadmap.md#exact-solvers).

See the [API reference](../reference/api/exact-solver.md) for all options.
