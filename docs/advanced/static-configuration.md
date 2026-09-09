# Compile-time configuration with `static()`

!!! note "Availability"
    `static()` ships with DynaPlex 1.14. This page is a draft for that release
    and is not yet linked from the navigation.

Many MDP *families* share one model body but differ in configuration: a lead
time that is present or absent, a finite or infinite horizon, a variant that
has an extra state field. In plain Python you write `if self.mdp.has_lead_time:`
and move on. Compiled DynaPlex models are type-checked as a whole, so a branch
that refers to a field only *some* variants have would be a compile error — even
though, for the configuration you are running, that branch can never execute.

`static()` solves this. It marks a condition that the compiler must decide
**at compile time**, from the configuration you constructed the engine with; the
branch that is not taken is discarded *before* it is type-checked.

```python
from dynaplex.modelling import static

@const_dataclass
class LotSizingMDP:
    has_lead_time: bool
    lead_time: int
    ...

    def modify_state_with_action(self, state: State, action: int) -> None:
        if static(self.has_lead_time):
            state.pipeline.append(action)          # `pipeline` exists only in this variant
            arriving = state.pipeline.pop(0)
        else:
            arriving = action
        state.inventory += arriving
```

With `has_lead_time=False`, the first branch is never compiled: `state.pipeline`
may not exist and nothing complains. With `has_lead_time=True`, the `else`
branch is the one that disappears.

In plain CPython — for example when you unit-test the model without compiling —
`static(x)` simply returns `x`.

## What the compiler can know

`static()` requires its argument to be **statically known**: derivable from the
objects you passed to the engine when you constructed it, through fields that
can never be rebound. Concretely, a value is known when it is reached by a chain
of hops that starts at a known object and where every hop is a field that is
either declared `Final[...]` or belongs to a `@const_dataclass`:

| Known | Why |
|---|---|
| `self` in a method of the object you built the engine with | that is the very instance being compiled |
| `self.mdp` when `mdp: Final[MyMDP]` | a `Final` hop from a known object |
| `self.mdp.has_lead_time` when `MyMDP` is a `@const_dataclass` | every field of a const class is binding-fixed |
| `self` inside a method of `MyMDP`, when called through a known `self.mdp` | the method is compiled once per known receiver |
| `self.h` inside a `@separate_world_class` whose `h: Final[int]` was passed a known value when the world was created | the world is compiled once per distinct set of known constructor arguments |
| literals, enum members, and `==`, `<`, `and`, `not`, `+`, ... over known values | pure operators |

And what is **never** known, by design:

| Not known | Why |
|---|---|
| parameters of the method (`def step(self, x)` → `x`) | they vary per call |
| local variables, even `x = self.mdp; x.flag` | locals are not tracked (yet) |
| loop variables | they vary per iteration |
| list elements: `self.mdps[0].flag` | element positions are not tracked |
| objects constructed inside compiled code | they are new instances |
| fields that are not `Final` on a class that is not `@const_dataclass` | they may be reassigned |

When a `static()` condition is not known, compilation **fails** and the message
tells you which hop broke the chain, for example:

```
static(): 'self.mdp.scale' is not statically known — field 'scale' of 'LotSizingMDP'
is rebindable — mark it Final[...] (or make the class @const_dataclass) to let
'self.mdp.scale' fold
```

If the failure is about `self` itself — "`self` is not statically known in this
GENERIC body" — the method is being compiled for an *unknown* receiver, and the
message names the call or world-spawn site responsible. Typically some code
constructs the class with values the compiler cannot see (a method parameter, a
list element); either pass a known value there or drop the `static()`.

## Worlds

A `@separate_world_class` created inside compiled code is specialized on the
constructor arguments that are known at the creation site and land in `Final`
fields:

```python
@separate_world_class
class Worker:
    mdp: Final[MyMDP]
    horizon: Final[int]
    acc: float                       # ordinary runtime state

    def run(self, x: float) -> float:
        if static(self.mdp.has_lead_time):
            ...
        for t in range(self.horizon):   # a constant for this worker
            ...

@dataclass
class Root:
    mdp: Final[MyMDP]
    workers: list[Worker]

    def spawn(self) -> None:
        for i in range(4):
            self.workers.append(Worker(self.mdp, 100, 0.0))   # known mdp, known horizon
```

All four workers share one specialized copy of `run`. Two creation sites with
different known arguments (`Worker(self.mdp, 100, …)` and `Worker(self.mdp, 200, …)`)
get two copies, and `static(self.horizon > 150)` decides differently in each.
A creation site whose argument is *not* known (`Worker(self.mdp, h, …)` with `h`
a method parameter) produces a plain, unspecialized world: `static()` on
`self.mdp` still works there (the object part is known), `static()` on
`self.horizon` does not.

## Rules of thumb

- Use `static()` for **variant selection** — branches that reference
  configuration-dependent fields or methods. Everything else can stay an
  ordinary `if`: known values are compiled as constants anyway, and the JIT
  folds the branch when it can.
- Prefer `@const_dataclass` for configuration objects and `Final[...]` for the
  fields on the engine root and worlds that hold them; that is what makes a
  chain of hops known.
- Pass configuration through the **constructor**, not through method
  arguments: constructor arguments of the engine root are known, call arguments
  are not.
- Keep `static()` as the *whole* condition. `if static(a) and b:` is a compile
  error; write nested `if`s instead.
- `DYNAPLEX_DISABLE_FOLDING=1` in the environment turns all compile-time
  knowledge off (useful to A/B a suspected folding issue). Models that use
  `static()` then fail to compile with a message saying so.
