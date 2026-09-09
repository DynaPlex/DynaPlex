# Dynamic System Modelling Language reference

To formulate a Markov decision problem, a formalization of the state and
transition functions is required. In DynaPlex, *states* of a sequential
problem are expressed as objects of a user-defined class, while *transitions*
between states are formalized as modifications of those objects by transition
functions. DynaPlex imposes a certain formalism for expressing these states
and transition functions, that is referred to as the Dynamic System Modelling
Language (DynaML).

DynaML is designed to achieve two core requirements:

1. being an expressive and readable language that is easy to learn, write,
   and read;
2. enabling automatic analysis and **compilation** of the models it
   describes, for efficient execution.

To achieve these design goals, classes (states, MDPs) in DynaML are expressed
as Python dataclasses, whereas transition functions are represented as Python
methods on an MDP class that operate on objects of another dataclass: a State
class. Classes and functions expressed in DynaML are also valid Python code, i.e.
DynaML is a subset of Python, making it easy to learn. At the same time, to enable static code
analysis, DynaML imposes certain structural and semantic properties, which
are described in this document.

## How DynaML code runs

DynaML models lead a double life. On the one hand they are ordinary Python:
you can construct states and call transition methods directly in CPython —
handy for debugging and unit-testing a model. On the other hand, when a
model is handed to an algorithm (`DCL`, `PolicyComparer`, `PPO`, or
an `Engine` directly), DynaPlex **compiles** it: classes and methods are
translated into an efficient internal representation and executed on a
multi-threaded engine with a bundled LLVM JIT. That compiled execution is
where the performance comes from — and both routes produce identical
results, bit for bit.

Throughout this document, *compiled code* refers to methods running on the
engine. The restrictions described here are what makes compilation possible,
and they are enforced when a model is compiled; plain CPython will happily
run code that violates them, which is precisely why validating with pyright
(and reading this document) matters.

## How to use this document

This document describes what can and cannot be accepted in DynaPlex MDP
methods. Summary: write code in Python, but use the following rules:

- Annotate parameters and return types.
- Use dataclasses; avoid dictionaries, tuples.
- Use homogeneous lists; avoid heterogeneous lists.
- Use functions; avoid lambdas.
- Use pure Python. Besides Python built-ins, a limited set of modules is
  supported: parts of `numpy`, `math`, `heapq`, `enum`, `dataclasses`, and
  the built-ins that `dynaplex` itself provides — see
  [Built-in functions and modules](#built-in-functions-and-modules).
- Avoid strings; use enums instead.
- When you need to load data using external files and packages, do so in the
  `__init__` method of the MDP class, not in the transition functions, and
  load the results into supported fields of the state class.

Test your code in CPython. Validate in pyright — if that complains, then it
is likely that the code is not valid DynaML. Calling the runtime no-ops
`assert_mdp(mdp)` and `assert_policy_for_mdp(mdp, policy)` in your script
additionally makes pyright check that your classes satisfy the interfaces
DynaPlex expects. If something fails to compile, look up the error message
on the [troubleshooting page](troubleshooting.md), which groups the common
compile errors by family with causes and fixes. Alternatively,
use an LLM, pointing it to this reference document and to your code, and it
will likely tell you what is wrong. If not, post a question on
[GitHub Discussions](https://github.com/DynaPlex/DynaPlex/discussions).

## DynaML data types

DynaML supports data primitives and objects of user-defined classes. In
DynaML, any object must have a type that can be determined before running the
code.

### Primitives

DynaML programs manipulate a small collection of well-defined data
primitives. Primitive values are scalars (`bool`, `int`, `float`) and lists
of such scalars. These primitives can appear as parameters to functions, as
local variables in expressions, and as fields of state objects.

### Classes and objects

On top of these primitives, DynaML uses Python dataclasses to define
structured objects. State and MDP classes in DynaML are classes whose fields
are either primitive values or other DynaML objects (nested classes), or
lists of primitives or such objects. DynaML also supports NumPy `NDArray`
fields (see [NumPy arrays](#numpy-arrays)). During execution, states are
concrete instances of these dataclasses, and transition functions are
methods that read and update their fields.

### Assignable values and support for None

Scalars (`float`, `int`, `bool`) can be assigned following standard Python
typing rules (as enforced by tools such as pyright). In particular, a `bool`
value is silently accepted wherever an `int` is expected, and an `int` value
is silently accepted wherever a `float` is expected. Floats are not silently
accepted where an `int` is expected, but we can always cast.

If annotated as `Optional[T]` or `T | None`, an **object** field or variable
can be assigned `None`. Only user-defined object types can be nullable —
`None` is *never* allowed for scalars, enums, lists, NumPy arrays, random
generators, `DiscreteDist`, or samplers, and `Optional` annotations on those
types are rejected.

### Syntax for class definition and defaults

Example of a simple DynaML class (throughout, we suppress import
statements):

```python
@dataclass
class Node:
    product_inventories: list[int]
    on_hold: bool = False
    parent: "Node | None" = None
    children: list["Node"] = field(default_factory=list)
```

The listing shows that DynaML supports defaults. For objects, the only
default supported is `None`, whereas the only default supported for lists is
an empty list (via the default-factory pattern, as `list` is mutable).

`field(compare=False)` is supported (it excludes the field from equality and
ordering, exactly as in Python); `field(init=False)` is not supported.

### Const classes and read-only containers

A class can be marked **const** with the `@const_dataclass` decorator
(imported from `dynaplex`). Every instance of a const class — and every
object reachable through its object-typed fields — is treated as
**read-only** inside compiled DynaML methods: such methods may read its
fields but must not assign to them. `@const_dataclass` accepts the same
keyword arguments as `@dataclass` (`init`, `slots`, `frozen`, ...).

This is the required convention for MDP and policy classes, whose `self` is
read-only during a trajectory:

```python
from dynaplex import const_dataclass

@const_dataclass(init=False, slots=True)
class MyMDP:
    num_actions: int
    # __init__ is hand-written CPython (init=False); compiled methods treat self as read-only
```

Const only restricts *compiled* methods; a hand-written
`__init__`/`__post_init__` (plain CPython) may still set fields during
construction. Individual `list` or `NDArray` fields can be marked read-only
with `Annotated[list[int], Const()]` (`Const` is importable from
`dynaplex`), independent of the enclosing class. More conveniently, use the
shorthands `ConstList[int]` and `ConstArray1D[np.float64]` /
`ConstArray2D[...]` / `ConstArray3D[...]` (importable from
`dynaplex.modelling`), which are equivalent to the `Annotated[..., Const()]`
form — e.g. `ConstList[int]` is `Annotated[list[int], Const()]`.

For `list` and `NDArray` (which are passed by reference), const-ness must
**match** at every binding — call arguments, assignments, returns, and
constructor fields. You may not pass a mutable list/array where a const one
is expected, nor a const one where a mutable one is expected; both would let
one side mutate what the other treats as read-only. The exception is a
*freshly created* value used directly in a const slot — a list literal,
`sorted(x)`, `x.copy()`, `x[:]`, `a + b`, `x * n`, or
`np.array(x)`/`np.zeros(...)`/etc. — which is accepted because nothing else
aliases it. So to satisfy a const slot from a mutable variable, pass a fresh
copy (e.g. `x.copy()` or `np.array(x)`). Reading a const container (`len`,
indexing, iteration, `in`) is always allowed; mutating it (`append`,
`x[i] = ...`, `+=`, in-place `sort`) is rejected. (Objects don't need this
rule: their const-ness is fixed by their class.)

### Frozen classes

A `@dataclass(frozen=True)` is automatically recognized: instances are
immutable, and any assignment to a field of a frozen object in compiled code
is a compile-time error (mirroring the `FrozenInstanceError` you would get
in CPython). Frozen classes are useful for value-like objects that are
created once and then only read.

A frozen class whose fields are all primitives, enums, or NumPy arrays plays
a special role as a *bundle*: bundles can be passed across engine
boundaries, with scalar fields copied and array backings shared. This
matters mainly when working with the engine API directly and is covered in
the engine documentation.

### Enums

DynaML supports Python enums for representing finite sets of named integer
values. Only standard `enum.Enum` is supported (not `IntEnum`, `Flag`, or
other enum variants). Enum values must be consecutive integers starting from
`1`; this can be easily achieved using `auto`:

```python
from enum import Enum, auto

class Light(Enum):
    RED = auto()
    ORANGE = auto()
    GREEN = auto()
```

Enums can be used as function parameters, return types, and as fields in
dataclasses. Enums of the same type can be compared with `is` / `is not`
and, equivalently, with `==` / `!=` (e.g. `x is Light.RED`). Enums can be
constructed from integer values (e.g. `Light(1)`), and the integer value is
accessible via the `.value` attribute. Enums are not orderable.

!!! important
    Enums and dataclasses must be defined or imported at module level (not
    inside functions). When DynaPlex analyzes a function's type annotations,
    it resolves types from the function's module globals using
    `get_type_hints()`. Types defined or imported only within a function's
    local scope cannot always be resolved and are unsupported.

## Functions and method signatures

DynaML supports both free functions and methods. Here, we discuss the
allowed syntax and semantics for free functions.

### Free function signatures

- Every parameter must have a type hint.
- Every function must have a return annotation. Use `-> None` for void.
- Multi-return is expressed with tuple annotations: `-> tuple[int, float]`.
- Parameters may have default values (literal constants only). Default
  values for list parameters are not supported.
- Calls to other registered functions are supported with both positional and
  keyword arguments.
- Returns: a single typed value or a typed tuple, e.g. `-> int`,
  `-> tuple[int, float]`.
- Arguments and return types must be valid DynaML types, as described in the
  data-type section above.

Example of a free function:

```python
def calculate_distance(x: int, y: int) -> float:
    return (x**2 + y**2) ** 0.5
```

### Method signatures

Methods can be added to dataclasses. The following rules apply:

- The first parameter (`self`) should **not** have a type annotation (it's
  inferred from the dataclass).
- All other parameters must have type hints.
- Return type annotations work the same as for free functions.
- Methods are called using dot notation: `obj.method(args)`.
- Special methods other than `__init__` and `__post_init__` (`__eq__`,
  `__lt__`, etc.) are not supported; make use of the default equality and
  ordering machinery of dataclasses. `__init__` and `__post_init__` *are*
  supported, under the rules in the next section.
- `@classmethod` and `@staticmethod` are supported and are called on the
  class: `Point.origin()`, `Point.manhattan(a, b)`. Inside a classmethod,
  `cls` names the class, so `cls(...)` builds an instance and
  `cls.other(...)` calls another classmethod; `cls` itself takes no
  annotation and is not a runtime argument. A classmethod or staticmethod
  compiles only when something calls it.

Example of a dataclass with a method:

```python
@dataclass
class Point:
    x: int
    y: int

    def distance_from_origin(self) -> float:
        return (self.x**2 + self.y**2) ** 0.5
```

### Custom `__init__` and `__post_init__`

A dataclass may define its own `__init__` and/or a `__post_init__`. Two
situations:

- **Objects built in CPython** (the MDP passed to the engine, anything you
  construct before handing it over): the `__init__` runs in plain CPython and
  may use any library. It is never compiled.
- **Objects constructed inside DynaML code** (`State(...)` in a transition
  function, say): the `__init__` is compiled like any other method, and is
  only compiled when some DynaML code actually constructs the class.

For the compiled case, the idea is simple: the compiler wants to be sure
that by the time your `__init__` returns, every field holds a value — and
that nothing looks at a half-built object along the way. In practice that
means: assign every field, unconditionally, before you do anything else with
`self`. This is *definite assignment*, checked at compile time; a body that
violates it is rejected with an error pointing at the offending line rather
than producing an object with garbage in it.

Concretely, the body must be valid DynaML and:

- Assign **every field on every path** before it returns. Assigning a field
  only inside a loop, or only in one arm of an `if`, is rejected. (Annotate
  the method `-> None`.)
- Read a field (`self.x`) only after it has been assigned.
- Use `self` only as `self.field` (store or read) until every field has been
  assigned. Calling a method on `self`, passing `self` to a function, storing
  it in a list, or aliasing it (`s = self`) before that point is rejected.
- Assign a `Final[...]` field (`typing.Final`: set at construction, never
  rebound) exactly once — never in a loop, never on more than one path.

A few consequences and limits:

- Field defaults are not allowed on such a class: nothing runs before a
  hand-written `__init__`, so a default would be inert. Move the value into
  the `__init__` body.
- A subclass's `__init__` calls the base's with `super().__init__(...)` or
  `Base.__init__(self, ...)`; that call counts as assigning every base field
  (see [Inheritance](#inheritance)).
- Not supported: `field(init=False)`, `InitVar`, and a custom `__init__` on a
  `@const_dataclass` or a `@separate_world_class` (build those in CPython).

`__post_init__` follows Python exactly: the dataclass-generated `__init__`
calls it after filling the fields (defaults included), so inside it every
field is already assigned and it is an ordinary method — it may read and
rebind fields freely (except `Final` ones) and call other methods. With a
hand-written `__init__` it is *not* called automatically (Python does not
either); call it explicitly from `__init__` if you want it, after every
field has been assigned. It takes no parameters beyond `self` and returns
`None`.

The `__init__` may take defaults and keyword arguments like any method, and
may use a [union parameter](#union-parameters-and-overloads), in which case
each construction site dispatches to the matching variant.

```python
@dataclass
class Order:
    size: Final[int]
    deadline: int
    pieces: list[int]

    def __init__(self, size: int, horizon: int = 10) -> None:
        self.size = size
        self.deadline = horizon + size // 2
        self.pieces = []
        for i in range(size):
            self.pieces.append(i)
```

### Inheritance

A dataclass may inherit from **one** other dataclass, **one level** deep
(`Child(Base)`, where `Base` has no dataclass base of its own; `object` or a
Protocol as an extra base is fine). Fields are the base's followed by the
subclass's, exactly as in CPython; inherited methods and overrides work as
you expect, and the base's version of a method is reachable from the
subclass:

```python
@dataclass
class Point:
    x: int
    y: int
    def norm1(self) -> int:
        return abs(self.x) + abs(self.y)

@dataclass
class Point3(Point):
    z: int = 0
    def norm1(self) -> int:
        return super().norm1() + abs(self.z)     # or Point.norm1(self)
```

Construction style must agree: either both classes use the dataclass-generated
`__init__`, or both write their own. In the hand-written case the subclass
`__init__` calls the base's — `super().__init__(...)`, `super(Child,
self).__init__(...)` or `Base.__init__(self, ...)` — and that call counts as
assigning every base field for the [definite-assignment
rules](#custom-__init__-and-__post_init__): it may come before or after the
subclass's own assignments, base fields are readable only after it, and a
`Final` base field forbids a second call. Inside the base `__init__`, when it
runs for a subclass instance, `self` may not be passed around or have methods
called on it (the subclass's fields are not assigned yet). The mixed case — a
hand-written base `__init__` under a generated subclass one, which CPython
silently bypasses — is rejected (for a trajectory context, already when the
class is defined: `@trajectory_context` checks it).

!!! warning "`super()` and `slots=True`"
    In CPython, zero-argument `super()` does not work inside a class declared
    with `@dataclass(slots=True)` (the dataclass rebuilds the class). Use
    `Base.method(self, ...)` or `super(Child, self).method(...)`, or drop
    `slots=True` on the subclass. Compiled code accepts all three spellings.

Types stay **nominal**: a `Point3` cannot be stored in a `Point`-typed field,
local, parameter or list. Inheritance shares fields and code, not a runtime
type hierarchy.

### Union parameters and overloads

A parameter may be annotated with a union of two or more DynaML types, e.g.
`rng: Generator | np.random.Generator`. Such a function is compiled as an
*overload set*: one variant per union member, and each call site dispatches
to the variant that exactly matches the argument's type. There is no
implicit coercion between union members — the argument must match one member
exactly. (Unions with `None`, like `Node | None`, are not overloads; they
are the nullable object annotations described above.)

Overload sets work on methods, free functions and classmethods alike. A
variant is compiled only when some call site dispatches to it, so a member
whose body cannot compile for that type — a `static_require(False, ...)`
saying the operation is unavailable — costs nothing until it is used, and
then fails with that message at compile time.

## Function bodies

DynaML supports most mathematical constructs that Python supports for
scalars. A few specific requirements are worth mentioning; we summarize
below.

### Parameters

Parameters work as in normal Python. A minor restriction is that you cannot
assign to parameters, but you can of course assign to fields and indexes:

```python
def give_age(node: Node, age: int, x: list[int]) -> None:
    node.on_hold = True       # accepted
    node.children[0] = node   # accepted
    x[0] = 10                 # accepted
    age = 42           # ERROR: assignment to parameter 'age' is not permitted
```

### Types and casting

DynaML supports explicit casts between any two scalar types: `int`, `float`,
and `bool`. These casts follow the usual Python semantics, e.g.
`int(1.7) == 1`, `float(True) == 1.0`, and `bool(0) == False`. In addition,
any object type can be cast to `bool`, with the usual Python semantics
(`None` becomes false). Implicit casts are allowed only where described in
the data-type section (for example, using an `int` where a `float` is
expected).

### Arithmetic

DynaML supports the usual arithmetic operators on scalar types (`bool`,
`int`, `float`), with behavior matching Python.

The supported binary operators are `+`, `-`, `*`, `/`, `//`, `%`, and `**`.
For `+`, `-`, `*`, `//`, `%`, and `**`, the result type is `float` if either
operand is a `float`, and `int` otherwise, while `/` always produces a
`float`, even when both operands are `int`. `//` and `%` follow Python's
floor semantics on integers and floats alike (`-7 // 2 == -4`, `-7 % 2 == 1`,
`-7.0 % 2.0 == 1.0`, `1.0 // 0.1 == 9.0`), not C's truncation or `fmod`.
`**` on two integers is an exact integer power. Boolean values participate in
arithmetic as `0` (`False`) or `1` (`True`), conforming to Python. Augmented
assignment on numeric locals and fields is also supported for all of these
operators: `+=`, `-=`, `*=`, `/=`, `//=`, `%=`, and `**=`.

Integers are 64-bit, which is where the semantics depart from Python's
unbounded ints. A result of `+`, `-`, `*`, `**`, `<<`, unary `-` or `abs`
that leaves the range -2**63 to 2**63-1 is an **error in checked mode**
(`integer overflow: 9223372036854775807 + 1 does not fit in a 64-bit int`),
and **wraps modulo 2**64 in fast mode**, silently; a Python int outside that
range is rejected at ingestion. So a model whose counts could leave the range
fails in rehearsal rather than continuing with a wrapped number. Counts and
money in integer cents fit comfortably; products of large quantities may not,
and belong in a `float`. One more consequence for `**`: a negative exponent gives a `float` in
Python, but `int ** int` is an `int` in DynaML, so it yields that float
truncated (`2 ** -1 == 0`, `1 ** -5 == 1`, `(-1) ** -3 == -1`).

Where Python raises, DynaML raises in checked mode and computes a defined
value in fast mode (see [Runtime checks and fast mode](#runtime-checks-and-fast-mode)):

- **Integer `//` and `%` by zero** are Python's `ZeroDivisionError` in checked
  mode. In fast mode both yield `0`. `-2**63 // -1` is the one division that
  overflows: an overflow error in checked mode, the wrap `-2**63` in fast mode;
  nothing traps.
- **`int(x)`, `math.floor(x)`, `math.ceil(x)` and `round(x)` of a float** that
  is NaN, infinite, or outside the 64-bit range raise in checked mode with
  Python's message (`ValueError` for NaN, `OverflowError` for infinity; a
  finite value beyond 2**63, which Python would turn into a big int, gets a
  DynaPlex message). In fast mode the conversion saturates: NaN gives `0`,
  anything beyond the range gives the nearest int64 limit, on every platform.
- **Floats are IEEE and never raise.** `x / 0.0` is `inf` or `nan`,
  `0.0 ** -1.0` is `inf`, and a negative base with a fractional exponent is
  `nan` where Python returns a complex number. An `int` that meets a `float`
  is converted to a double first, in a comparison and in `/` (which divides
  as floats even for two ints), so both are exact only below 2**53:
  `2**53 + 1 == 2.0**53` is `True`, and `(2**53 + 1) / 3` can differ from
  Python's correctly rounded quotient in the last bit.

**Bitwise and shift operators.** `&`, `|`, `^`, `<<`, `>>` and unary `~`
are supported on `int` and `bool` operands with Python's meaning and types:
`&`, `|`, `^` give a `bool` when both operands are `bool` and an `int`
otherwise; `<<` and `>>` always give an `int`; `~` takes an `int` (Python
deprecates `~` on a `bool`; use `not`). A `float` operand is a compile-time
error, as it is a `TypeError` in Python. The augmented forms `&=`, `|=`,
`^=`, `<<=`, `>>=` work on locals, fields and list elements. The 64-bit rule
above applies: `x << n` wraps modulo 2**64 like every other integer operation,
a shift count of 64 or more yields `0` for `<<` and the sign fill (`0` or `-1`)
for `>>`, exactly Python's value. A negative shift count is a `ValueError` in
Python; in DynaML it is an error in checked mode, and in fast mode the shift
returns the same value as an over-large count.

```python
def arithmetic_examples(inv: int, sold: int, price: float, factor: float, flag: bool) -> None:
    inv -= sold                  # valid: int -= int
    discounted = price * factor  # valid: float * float -> float
    discounted /= 100.0          # valid: float /= float -> float
    # REJECTED by pyright; INVALID in DynaML:
    # flag = inv - sold
    # instead, use:
    flag = bool(inv - sold)
```

### Comparisons, equality and ordering

Boolean expressions in DynaML are built from comparisons. Comparisons
(`==`, `!=`, `<`, `<=`, `>`, `>=`) produce boolean results, which are stored
as `bool`.

**Equality** (`==` / `!=`) is supported for scalars, enums (of the same
type), objects, and lists — including nested objects and lists of objects.
Object and list equality is *deep* (structural, field-by-field and
element-by-element), matching Python's dataclass `__eq__`. Floats compare by
value (so `0.0 == -0.0` and `NaN != NaN`). A class is eligible for
`==`/`!=` unless, through its compared fields, it reaches an `NDArray`
field, a nested list (`list[list[...]]`), or a reference cycle; such a
comparison is rejected at compile time, and you can exclude the offending
field with `dataclasses.field(compare=False)` (which drops it from
equality, exactly as in Python).

**Value equality with arrays** — `dynaplex.equals(a, b)` is `==` with
`NDArray` fields (and arrays passed directly) compared as values: same
shape, same dtype, elementwise equal, `NaN` unequal. On everything else it
is exactly `==`, including a class's own `__eq__` and the
`compare=False` opt-out. Both operands must have the same static type. Use
it wherever two states must be compared as values; plain `==` keeps
Python's meaning, where `array == array` is elementwise. `deep_hash`
hashes arrays the same way, so equal values hash equal.

**Ordering** (`<`, `<=`, `>`, `>=`) is supported for scalars, for
*orderable objects*, and for flat lists whose elements are orderable —
numbers (`list[int]`, `list[bool]`, `list[float]`) or orderable objects
(`list[MyOrderable]`). An object is orderable when it is a
`@dataclass(order=True)` whose leading fields form a homogeneous numeric
prefix — all `int`/`bool`, or all `float` — with any remaining fields marked
`field(compare=False)`. Ordering is then lexicographic over that leading
prefix, matching Python. Lists compare lexicographically (shorter is smaller
on a common prefix). Enums are not orderable.

```python
@dataclass(order=True)
class Job:
    priority: int
    size: int
    label: int = field(compare=False)   # excluded from ordering (and equality)

def pick(a: Job, b: Job) -> bool:
    return a < b                          # lexicographic over (priority, size)
```

### Boolean connectives

Short-circuiting `and` and `or` are supported. They are supported when all
operands are `bool`, and also when all operands have the same type. (Mixing
of `Object` and `Object | None` is allowed, as long as the objects are of
the same type.) The unary operator `not` is supported with the usual Python
semantics and always produces a `bool`.

### Control flow

Control flow in DynaML is restricted to structured constructs: `if` /
`if-else`, `while`, and `for` loops.

The allowed iterables in `for` loops are `range(...)` with one, two, or
three integer arguments (`range(stop)`, `range(start, stop)`,
`range(start, stop, step)`), lists, and `enumerate(...)` over a list. Loops
over lists are supported both for `list` variables and for `list` literals.
For the latter, the type of the list literal must be clear from the code:
`for x in []:` will fail, but `for x in [1, 2, 3]:` is supported. For
`enumerate`, the only variant that compiles is
`for i, x in enumerate(some_list):`, where `some_list` is a list variable or
literal. The index and value variables may reuse the names of already
defined locals, but the types must match.

You can use if-expressions (the "ternary" operator) to assign the result of
a conditional directly. The form is `A if condition else B`, just as in
Python; note that the types of `A` and `B` must be the same. Nested
if-expressions are also supported.

!!! important
    Do not change the length of the list that you are iterating over while
    iterating. This will compile successfully, but it will fail in
    unexpected ways (undefined behavior). It is generally considered bad
    practice even in regular Python.

Loop bodies may use `break` and `continue` to alter control flow. Conditions
in `if` and `while` may be explicit booleans or any expression that Python
would treat as truthy/falsy: for example, `if obj:` tests whether an object
reference is non-`None`. A simple `while` loop example:

```python
def countdown(start: int) -> int:
    steps = 0
    while start != 0: # or while start:
        start -= 1
        steps += 1
    return steps
```

`for` loops iterating over lists are also supported. An example using the
`Point` dataclass defined above:

```python
def sum_points(points: list[Point]) -> tuple[int, int]:
    """Sum the first and second elements of a list of (x, y) points."""
    sum_x = 0
    sum_y = 0
    for point in points:
        sum_x += point.x
        sum_y += point.y
    return sum_x, sum_y
```

### Assertions, `__debug__` and `dynaplex.fail`

`assert condition, "message"` is supported and means what it means in
Python: a **check**, present when the program runs with checks and absent
when it does not — exactly as CPython drops `assert` under `python -O`. The
message is required and may be a string literal or a simple f-string with
bare `{expr}` placeholders:

```python
assert state.remaining_seats >= 0, f"negative seats: {state.remaining_seats}"
```

DynaPlex compiles every model twice over its life: once **checked**, where
each `assert` is live and a violation raises `DynaPlexError` with the
message, and once **fast**, where the asserts (and the bounds checks on list
indexing) are compiled out so the hot loops run branch-free. Which one you
get is described in [Runtime checks and fast mode](#runtime-checks-and-fast-mode);
by default every algorithm rehearses your model checked before running it
fast. Two consequences follow from the Python semantics:

- an `assert` condition must be pure — it is not evaluated in fast mode, so
  it cannot be the thing that advances the state;
- an `assert` is not the way to *fail on purpose*. For the branch that must
  never be reached — no feasible action, an impossible case — use
  **`dynaplex.fail("message")`**. It fires in every mode, in CPython it
  raises `DynaPlexError`, and the compiler treats it as never returning, so
  a method may end on it and code after it is dead:

```python
def get_action(self, state: State) -> int:
    for a in range(self.mdp.num_actions):
        if self.feasible(state, a):
            return a
    dynaplex.fail("no feasible action")     # NoReturn: the method ends here
```

A third form is decided before anything runs: **`dynaplex.static_require(condition,
"message")`** requires something of the *configuration* being compiled. The
condition must be a compile-time constant — a literal, an expression over
`Final[...]` fields of the engine root (an MDP parameter, an enum such as
`horizon_type`), or `implements(obj, Protocol)` — and the compiler settles it
while compiling the method: when it holds, nothing is emitted; when it fails,
compilation stops with the message, the line, and the chain of calls that
pulled the method into the program; when it cannot be decided,
compilation stops saying which field made it unknown. It is never stripped.
Because it fires only where the statement is actually compiled, a method whose
body is `static_require(False, "...")` compiles fine until something calls it,
which makes it the way to say "this method is not available in this
configuration". In plain Python the same call is an always-on check that
raises `DynaPlexError`:

```python
def modify_state_with_action(self, state: State, context: Ctx, action: int) -> None:
    static_require(self.horizon_type == HorizonType.FINITE, "this model is finite-horizon only")
    ...
```

`__debug__` is the compile-time checks flag (`True` checked, `False` fast),
as in CPython. `if __debug__:` compiles only the taken branch, so an
expensive invariant walk costs nothing in fast mode:

```python
if __debug__:
    self.verify_invariants()
```

`print(...)` is *accepted but ignored* in compiled code: no instructions are
generated for it. This lets you keep debug prints in code that you also run
under CPython, but do not rely on `print` for any observable effect.

### Local variables

- Locals are created on first assignment and can be annotated for clarity
  (`x: int = ...`).
- A variable's *first* assignment must occur in a scope that syntactically
  encloses all of its uses. In particular, a variable that is first assigned
  inside an `if` or loop body cannot be read outside that `if` or loop; it
  must be introduced before the control-flow construct.

For example, the first function below is rejected, because the return
statement uses `x`, but `x` is first defined only inside the `if` statement.
To sidestep this, declare `x` before the `if`:

```python
def rejected(flag: bool) -> int:
    if flag:
        x = 1
    else:
        x = 10
    return x # rejected - x is not defined in this scope.

def accepted(flag: bool) -> int:
    x = 0
    if flag:
        x = 1
    else:
        x = 10
    return x
```

The same rule covers `for` loop targets, which are scoped to the loop: the
loop variable is visible in the loop body but not after the loop (nor in its
`else:` clause), and it may not reuse the name of an existing variable. CPython
leaves the loop variable bound to its last value after the loop; DynaML rejects
such a use so that accepted programs behave identically. Copy the value to a
variable declared before the loop if you need it afterwards:

```python
def rejected(n: int) -> int:
    for i in range(n):
        pass
    return i  # rejected - i is not defined after the loop.

def accepted(n: int) -> int:
    last = -1
    for i in range(n):
        last = i
    return last
```

Once a variable has a type in a given (or enclosing) scope, it must not be
reused with a different type. For example, the following is rejected because
`x` is first used as an `int` and later as a `float`:

```python
def bad_retype(flag: bool) -> None:
    x = 0        # x: int
    if flag:
        x = 1.0  # INVALID: cannot reuse x as float
```

Global variables are not supported.

### Accessing and modifying fields

Field access/assignment on dataclass instances is supported, including
augmented assignment, e.g. `location.inventory -= 10`, and nested targets
such as `network.nodes[i].inventory = 5`.

### Calling functions

Tuple unpacking is supported only one level deep: `a, b = f()`; this is the
only way in which multiple return values can be captured by the calling
function. Nested tuple unpacking is not supported.

Functions may be called with positional or keyword arguments. Arguments with
defaults may be omitted from the call. Type checking is performed at compile
time, ensuring passed arguments match the expected types.

### Objects

Objects can be constructed following the usual syntax, and methods can be
called on those objects. For example:

```python
def create_and_use_point() -> float:
    """Create a Point object and call its method."""
    p = Point(x=3, y=4)
    distance = p.distance_from_origin()
    return distance
```

### Lists

DynaML supports lists of primitives (`list[int]`, `list[float]`,
`list[bool]`), lists of objects (`list[MyClass]`), and nested lists of
numbers (`list[list[int]]`, etc.). List literals can be constructed using
standard Python syntax: `[1, 2, 3]` or `[]`. Lists support:

- **Indexing**: positive and negative indices (`my_list[0]`, `my_list[-1]`)
- **Index assignment**: `my_list[i] = value` and augmented forms
  (`my_list[i] += value`, etc.)
- **`len()`**: returns the current length
- **`.append(value)`**: appends a single element in place
- **`.extend(other)`**: appends all elements of `other` in place
- **`.pop()`**: removes and returns the last element
- **`.clear()`**: removes all elements
- **`.copy()`** and **`my_list[:]`**: return a shallow copy
- **`.sort()`**: sorts in place; **`sorted(my_list)`**: returns a new sorted
  list. Both work for any list whose elements support ordering: numbers,
  orderable objects, or `list[list[number]]`. Sorting is stable (matching
  Python).
- **`+`** and **`+=`**: concatenation (produces a new list) and in-place
  extend
- **`*`** and **`*=`**: repetition (produces a new list) and in-place
  repetition
- **`x in my_list`** and **`x not in my_list`**: membership tests

Not supported: `.index()`, `.count()`, `.insert()`, `.remove()`,
`.reverse()`.

### Other unsupported Python syntax

The following Python features are presently unsupported, and not planned for
future support in DynaML: strings (there is no `str` type — use enums);
dictionaries; sets; tuples (with two syntactic exceptions: returning
multiple values, which must be immediately bound to individual variables by
the caller, see above; and in-place shape literals in
[array creators](#numpy-arrays) such as `np.zeros((m, n), ...)` — in
neither case is the tuple a value that can be stored or passed around);
lambdas; `isinstance` and other runtime introspection.

!!! tip "Missing something? DynaML is in rapid development"
    DynaPlex and DynaML are developing rapidly, and the language grows based
    on what modellers actually need. If a limitation genuinely blocks you in
    an application — supply chain, logistics, transportation, maintenance
    optimization, or similar — please
    [open a feature request](https://github.com/DynaPlex/DynaPlex/issues).
    The most useful requests explain three things:

    1. what you are trying to achieve in your MDP;
    2. why that is difficult with the current DynaML limitations;
    3. your proposed addition to DynaML.

## Built-in functions and modules

Beyond the language constructs above, DynaML programs can call a fixed
collection of built-in functions. Everything listed in this section behaves
exactly as its CPython counterpart unless noted otherwise.

### Python built-ins

| Built-in | Notes |
|---|---|
| `int(x)`, `float(x)`, `bool(x)` | scalar casts; `bool` also accepts objects (`None` → `False`) |
| `abs(x)` | `int -> int`, `float -> float` |
| `round(x)` | returns `int` |
| `min(a, b, ...)`, `max(a, b, ...)` | two or more scalar arguments of a common type |
| `sum(xs)` | over `list[int]` or `list[float]` |
| `any(xs)`, `all(xs)` | over `list[bool]` |
| `len(xs)` | list length |
| `sorted(xs)` | new sorted list; same element requirements as `.sort()` |
| `range(...)`, `enumerate(...)` | loop headers only, see [control flow](#control-flow) |
| `assert cond, msg` | a *check*: live in checked mode, compiled out in fast mode (Python `-O` semantics); `msg` may be a simple f-string |
| `__debug__` | compile-time bool: `True` checked, `False` fast; `if __debug__:` keeps only the taken branch |
| `print(...)` | accepted but **ignored** (no code generated) |

### `math` module

`math.sqrt`, `math.exp`, `math.log`, `math.log10`, `math.sin`, `math.cos`,
`math.tan`, `math.floor`, `math.ceil`, `math.isfinite`, `math.isnan`.

All take a `float` (an `int` is implicitly accepted) and return `float`,
except `math.floor` / `math.ceil`, which return `int`, and
`math.isfinite` / `math.isnan`, which return `bool`. Import `math` at module
level.

### `heapq` module

DynaML supports a min-heap maintained on a list via the standard `heapq`
module: `heapq.heapify(x)`, `heapq.heappush(heap, item)`, and
`heapq.heappop(heap)`. These behave exactly like CPython's `heapq` and are
available for any list whose elements support ordering (the same element
types as sorting: numbers, orderable objects, or `list[list[number]]`). The
heap list itself must be mutable (not a const container). Import `heapq` at
module level:

```python
import heapq

def schedule(jobs: list[Job]) -> Job:
    heapq.heapify(jobs)
    heapq.heappush(jobs, Job(priority=0, size=1, label=0))
    return heapq.heappop(jobs)      # returns the smallest Job
```

Note that `Job` in this example works only because it is an *orderable*
object — a `@dataclass(order=True)` with a homogeneous numeric leading
prefix, as defined in
[Comparisons, equality and ordering](#comparisons-equality-and-ordering)
(where this `Job` class is introduced).

`heappop` on an empty heap raises, as in Python. The other `heapq` helpers
(`heappushpop`, `heapreplace`, `nlargest`, `nsmallest`, `merge`) are not
currently supported.

### NumPy arrays

DynaML supports NumPy array fields and locals, with one rule to remember:
an array annotation must pin down **both** the element dtype **and** the
number of dimensions (the rank). The dtype must be one of `np.float64`,
`np.float32`, `np.int64`, `np.bool_`. The most convenient way to declare
both at once is with the `Array1D` / `Array2D` / `Array3D` aliases from
`dynaplex.modelling`:

```python
from dynaplex.modelling import Array1D, Array2D

@dataclass(slots=True)
class State:
    vector: Array1D[np.int64]     # 1-D array of int64
    matrix: Array2D[np.float64]   # 2-D array of float64
    category: StateCategory = StateCategory.AWAIT_EVENT


def make_state(n: int, m: int) -> State:
    return State(
        vector=np.zeros(n, dtype=np.int64),          # shape n: one dimension
        matrix=np.zeros((n, m), dtype=np.float64),   # shape (n, m): two dimensions
    )
```

The rank is simply the number of elements in the shape passed to the
creating call: `np.zeros(n, ...)` creates a 1-D array of length `n`,
`np.zeros((n, m), ...)` a 2-D array, and so on.

`Array1D[T]` / `Array2D[T]` / `Array3D[T]` are shorthands for
`Annotated[NDArray[T], Rank(1)]` (respectively `Rank(2)`, `Rank(3)`), which
DynaML also accepts spelled out. Two other forms are accepted:

- A bare `NDArray[T]` (no rank metadata) declares a **1-D** array — the
  rank defaults to 1, so multi-dimensional arrays always need explicit
  `Rank` metadata (i.e. use `Array2D`/`Array3D`).
- For locals, an annotation may be omitted entirely when both dtype and
  rank are inferable from the creating expression — e.g.
  `x = np.zeros((m, n), dtype=np.float64)` is a 2-D `float64` array.

A `np.ndarray` annotation without a dtype is rejected. Read-only variants
`ConstArray1D[...]` / `ConstArray2D[...]` / `ConstArray3D[...]` are covered
under
[Const classes and read-only containers](#const-classes-and-read-only-containers).

**Creating arrays.** The supported creators are `np.zeros`, `np.ones`,
`np.full`, `np.array`, `np.asarray`, `np.zeros_like`, `np.ones_like`, and
`np.full_like`. Shapes are given as an integer (rank 1) or a tuple of
integers, e.g. `np.zeros((m, n), dtype=np.float64)`; `np.array` /
`np.asarray` convert a list of numbers.

Shape tuples are the one place where a tuple may be *written* in DynaML
(tuples otherwise only appear when returning multiple values, and are never
values you can store — see
[unsupported syntax](#other-unsupported-python-syntax)). Consequently, the
tuple must be spelled out in place at the call site: the individual
dimensions may be arbitrary integer expressions
(`np.zeros((m + 1, 2 * n), ...)`), but the shape as a whole cannot be built
elsewhere and passed in via a variable.

**Element access.** Indexing with one index per dimension is supported —
`a[i]`, `a[i, j]`, `a[i, j, k]` — for both reading and assignment,
including augmented assignment (`a[i] += w`) and negative indices. Slicing
(`a[1:3]`, `a[i, :]`) is **not** supported.

!!! warning "Array indices are not bounds-checked"
    Unlike list indexing, which raises an error on an out-of-range index,
    ndarray indexing is **not** bounds-checked at runtime: an out-of-range
    index (outside `[-n, n - 1]` for a dimension of size `n`) silently
    reads — or, on assignment, overwrites — unrelated memory. Validate
    index arithmetic by running the model under plain CPython, where NumPy
    raises `IndexError`.

**Shape.** The size of dimension `k` is available as `a.shape[k]` (indexed
access only; bare `a.shape` has no tuple type in DynaML).

**Whole-array reductions.** `sum`, `mean`, `min`, `max`, `any`, `all` are
available both as methods (`a.sum()`, `a.mean()`, ...) and as free functions
(`np.sum(a)`, `np.amin(a)`, ...); `np.count_nonzero(a)` is available as a
free function. Reductions always cover the whole array — there is no
`axis=` argument.

**Not supported:** elementwise arithmetic on arrays (`a + b`, `a * 2`,
elementwise comparisons) — loop over elements instead; slicing and views;
`reshape`; boolean masking. Arrays are best used as fixed-shape numeric
storage that you index explicitly.

### Random number generation

!!! tip "Prefer `DiscreteDist` for event distributions"
    Don't roll your own discrete distributions from raw generator draws. The
    preferred route is a [`DiscreteDist`](#distributions-and-samplers) built
    in the MDP's ``__init__`` plus a precomputed sampler
    (``dist.alias_sampler()``) for the draws — validated, kernel-backed, and
    O(1) per draw. Use the raw generator methods below when that genuinely
    doesn't fit (e.g. continuous uniforms or ad-hoc index draws).

DynaML offers two generator families with an identical method surface:

- `dynaplex.default_rng(seed)` returns a `dynaplex.Generator` — DynaPlex's
  native generator (xoshiro256++), the faster choice and the recommended
  default;
- `np.random.default_rng(seed)` returns NumPy's `Generator` (PCG64), useful
  when bit-compatibility with NumPy streams matters.

Seeds must be non-negative integers. A function parameter can accept either
family via a union annotation (see
[union parameters](#union-parameters-and-overloads)).

Methods on both families:

- `rng.random()` — a float in `[0, 1)`;
- `rng.uniform(low, high)` — a float in `[low, high)`; requires
  `high > low`;
- `rng.choice(...)` — in the following forms:
    - `rng.choice(n)` with an integer `n`: a uniform draw from
      `0, ..., n-1`;
    - `rng.choice(xs)` with a list or 1-D array: a uniform draw of an
      element;
    - The NumPy-only keyword arguments `size=`, `replace=`, `axis=`, and
      `shuffle=` are not supported, and neither is the weighted form
      `p=probs` (see the note below).

!!! note "Reproducibility"
    For the NumPy family, `rng.random()` and `rng.uniform()` reproduce
    NumPy's streams bit-for-bit. `rng.choice(n)` (integer form) uses a
    different integer-sampling algorithm than NumPy and will generally
    return a different (equally uniform) stream for the same seed. 

!!! note "Weighted draws"
    `rng.choice(xs, p=probs)` is not available in released DynaPlex: it
    rescanned and renormalized `probs` on every draw. Weighted draws go
    through a [discrete distribution](discrete-distributions.md) built
    once, `DiscreteDist.custom(probs)`, and sampled through its
    `alias_sampler()` (constant time per draw) or `cdf_sampler()`. Passing
    `p=` is a compile-time error that names this replacement.

### DynaPlex built-ins

**`clone(x)`** (importable from `dynaplex.modelling`) — a deep copy of an
object graph, list, or array, callable inside compiled DynaML code.
Constant (`@const_dataclass`) parts are shared rather than copied.

**`dynaplex.fail(msg)`** — unconditional failure: raises `DynaPlexError(msg)`
in every mode (checked, fast and CPython) and never returns. `msg` follows the
assert-message rules. See [Assertions](#assertions-__debug__-and-dynaplexfail).

**`dynaplex.static_require(condition, msg)`** — a requirement settled at
compile time: `condition` must be a compile-time constant (`Final` fields of
the engine root, literals, `implements(...)`); a failed requirement is a
compile error carrying `msg`, a met one emits nothing. In plain Python an
always-on check raising `DynaPlexError(msg)`. See
[Assertions](#assertions-__debug__-and-dynaplexfail).

### Distributions and samplers

The modelling toolkit provides `DiscreteDist` — an explicit distribution
over a finite range of integers — together with two O(1) samplers. These
are ordinary DynaPlex classes: construct them in your MDP's `__init__`,
store them in (const) fields, and call their methods from compiled code.
A high-level tour of what they can do is on the
[Discrete distributions](discrete-distributions.md) page; this section
lists the compilable surface.

- Factories: `DiscreteDist.constant(v)`, `DiscreteDist.custom(probs,
  offset=0)`, `DiscreteDist.poisson(mean)`, `DiscreteDist.geometric(mean)`,
  `DiscreteDist.geometric_from_prob(p)`, `DiscreteDist.binomial(n, p)`,
  `DiscreteDist.negative_binomial(r, p)`,
  and — building a distribution from a mean and standard deviation — the
  two-moment fit `DiscreteDist.adan_eenige_resing(mean, stdev)` (Adan, van
  Eenige & Resing, 1995).
- Methods callable in compiled code: `sample(rng)`,
  `conditional_sample(rng, min_value)`, `min()`, `max()`,
  `probability_at(v)`, `expectation()`, `variance()`, `std()`, `entropy()`,
  `fractile(alpha)`.
- One-shot draws without building a distribution:
  `DiscreteDist.poisson_sample(mean, rng)` and the analogous
  `*_sample` classmethods — a few tens of nanoseconds per draw for the
  small means typical of per-period demand, with no per-parameter setup, so
  the parameters may change every draw.
- **Whenever the distribution is static** (the same for every state — the
  usual case), build a sampler once in the MDP's `__init__` —
  `dist.alias_sampler()` — and draw with `sampler.sample(rng)`: O(1) per
  draw. `dist.sample(rng)` is for the exceptional case where the
  distribution object itself depends on the current state (e.g.
  non-stationary demand); for
  state-dependent *parameters*, the one-shot classmethods above avoid
  building a distribution at all.

See the bin packing tutorial for a worked example
([Python code](../tutorials/binpacking-mdp.md#python-code)).

## Trajectory contexts and statistics

Every trajectory runs with a *context* object: the random streams, the
cumulative cost, the elapsed time and an action-validity scratch. The vanilla
context is `TrajectoryContext`; an MDP that wants per-trajectory **statistics**
declares its own context class and constructs it in a `make_context(self)`
method (worked example: [Airplane MDP with custom
statistics](../advanced/airplane-statistics.md)).

```python
@dataclass
class MyStats(TrajectoryStats):           # what PolicyComparer hands back: one row per trajectory
    stockouts: Final[Array1D[np.int64]]   # the scalar, [n]
    holding: Final[Array2D[np.float64]]   # the 1-D array, [n, k]

@trajectory_context
@dataclass
class MyContext(TrajectoryContext):       # the five base members come from the base
    Stats = MyStats                       # the holder
    stockouts: int                        # statistics
    holding: Final[NDArray[np.float64]]
    lead_time_rng: Final[Generator]       # an extra stream

    def __init__(self, mdp: MyMDP) -> None:
        super().__init__(mdp)             # rng, policy_rng, cumulative_cost, time_elapsed, valid
        ...                               # assign every statistic; shapes from the MDP

class MyMDP:
    def make_context(self) -> MyContext:  # declares (annotation) + constructs
        return MyContext(self)
    def get_initial_state(self, context: MyContext) -> State: ...
```

Rules, all checked when the class is defined (`@trajectory_context`) or when
the MDP is bound (`infer_context_type`):

- **Closed field table.** After the five base members (inherit them from
  `TrajectoryContext`, or spell them out in that order as a standalone
  class): `float` / `int` / `bool`
  scalars, `Final[NDArray[...]]` of dtype float64 / int64 / bool (any rank), and
  `Final[Generator]` streams. Nothing else — in particular no lists: a
  statistic has a fixed shape.
- **Uniform `Final` rule.** Scalars are the only fields you re-assign
  (`context.stockouts += 1`); every other field is `Final`, created once in
  `__init__` and mutated in place (`context.holding[k] += ...`). A generator
  is never replaced — it is reseeded in place.
- **Scratch.** The same scalars and arrays wrapped in `Scratch[...]`
  (`plan_period: Scratch[int]`, `plan: Final[Scratch[NDArray[np.int64]]]`) are
  per-trajectory *scratch*, not statistics: carried by clones, but never
  collected (absent from `Stats`) and never written by the kernels — `reset()`
  leaves them alone. This is where a context-driven policy keeps intermediate
  storage across the decisions of one trajectory (a production plan computed
  once per period, say), and the policy alone is responsible for knowing when
  it is stale: decide that from the state (e.g. "first decision of a period"),
  not from anything the context did. The MDP still declares and sizes the
  field; a scratch array may be non-`Final` (rebindable) if you need that.
- **Statistics holder.** Named as a class attribute (`Stats = MyStats`): a
  dataclass deriving from `TrajectoryStats`
  with exactly the collected fields — `cumulative_cost`, `time_elapsed` (from
  the base) and every statistic — under the same names, each `Final` with
  one extra leading trajectory axis (`bool`/`int`/`float` → `Array1D`, a 1-D
  array → `Array2D`, ...). Names, dtypes, ranks and `Final` are checked when
  the context is defined. `Stats` is what types `assessment.stats` for
  pyright. *Lazy mode:* leave it out and the decorator derives a holder
  (`MyContext.Stats`) — same runtime; `assessment.stats` is `Any` and the
  derived class has no static name to annotate with.
- **Type consistency.** The `context` parameter of `get_initial_state`,
  `modify_state_with_event` and `modify_state_with_action` and the return
  annotation of `make_context` must all name the same class; a context-driven
  policy (`get_action(self, state, context)`) must annotate that same class
  (or the `Ctx` placeholder for a policy generic over the MDP, as
  `RandomPolicy` does). `assert_mdp` lets pyright flag disagreements.
- **Construction.** `new_context(mdp, seed=0)` is the entry point: it calls
  the MDP's `make_context()` when there is one (else builds a
  `TrajectoryContext`) and seeds the result. A context straight out of a
  constructor has placeholder streams; the engine reseeds per trajectory,
  hand-driven code should not construct directly.
- **Synthesized housekeeping.** `reseed(global_seed, eval=0, sample=0,
  trajectory=0)` gives every generator field its own stream, in declaration
  order (never hand-written); `reset()` zeroes every scalar and zero-fills
  every array in place, leaving the generators alone (a hand-written `reset`
  wins); `write_stats(out, row)` copies the collected fields into row `row`
  of the holder (`MyContext.Stats`, i.e. `MyStats`).
- **What comes back.** `PolicyComparer(mdp)` is a `PolicyComparer[MyStats]`;
  `assessment.stats` is a `MyStats`: the collected fields under the same
  names, each with a leading trajectory axis (`stats.holding` is `[n, k]`,
  `stats.cumulative_cost` is `[n]`). For an MDP without a custom context it
  is a `TrajectoryStats`. Raw totals, no summary API; row `i` is trajectory
  `i` under common random numbers for every policy and backend.
- **Warmup boundary.** For infinite-horizon evaluation the comparer calls
  `reset()` — without reseeding — when the warmup ends, so window values and
  `time_elapsed` restart from zero while the draw streams run on. Code that
  reads the context mid-trajectory observes this reset.

## Promises: what a method may do to a parameter

The rules above are about *types*. A second family of rules is about *roles*:
`modify_state_with_action` must not draw random numbers, the context's cost
accumulator is an output channel that a model may only add to, a policy must
not modify the state it is given. Nothing about a method's signature says so,
and a model that breaks one of these compiles and runs — and then fails
subtly, because every harness (the comparer, DCL, PPO) relies on them. DynaPlex
therefore lets a class **carry promises** about its methods, and the compiler
checks them whenever the methods compile.

A promise is always *a method's* promise about *one of its parameters*: what
may happen to the parameter, or to a named field under it, or to every value of
a given type reachable from it. The same context field can be accumulate-only
in the event function and freely readable in a policy — promises are never
attached to fields.

### `@mdp` and `@policy`

Put `@mdp` above `@const_dataclass` on an MDP class and `@policy` above a
policy class (both importable from `dynaplex`). Featurizers get their promise
from `@featurizer`. An undecorated class carries no promises and nothing is
checked.

```python
from dynaplex import mdp, policy, const_dataclass

@mdp
@const_dataclass(init=False, slots=True)
class LostSalesMDP:
    ...

@policy
@const_dataclass(slots=True)
class BaseStockPolicy:
    ...
```

`@mdp` promises, for every compiled variant of the method:

| method | promise |
|---|---|
| `modify_state_with_event(state, context)` | `context.cumulative_cost` appears only as the target of `+=` / `-=`; `context.time_elapsed` only as the target of `+=`; no generator under `context` (`rng`, `policy_rng`, or any stream your own context adds) is re-bound or stored into a field or container |
| `modify_state_with_action(state, context, action)` | the same, and **no generator under `context` is used at all** |
| `write_action_validity(state, valid)` | `state` is only read |

`@policy` promises for `get_action(state)` / `get_action(state, context)`:
`state` is only read; no generator under `context` other than
`context.policy_rng` is used; `context.cumulative_cost` is not read.
`@featurizer` promises that `write_features(state)` only reads the state.

"Only read" is transitive: passing the state to a helper that modifies it
counts, and so does `state.pipeline.pop_front()`. "Used" covers reads, calls,
and handing the value to another function. The checks are static, over the
compiled code; plain CPython (running the same methods without DynaPlex) stays
permissive, as for every other rule in this reference.

A violation is a compile error naming the model's line and teaching the rule:

```
MDP model validation failed:
In function 'MyMDP.modify_state_with_action$State$TrajectoryContext$Int' (my_model.py), line 2:
context.rng is used (load) — breaks the MDP promise 'action-deterministic' (NoUse on every Generator under context).
  modify_state_with_action must be deterministic given (state, action): randomness belongs in
  modify_state_with_event, so every policy sees the same event stream (common random numbers).
  Line 2:     state.on_hand += int(context.rng.random())
```

### An algorithm's promises

Some algorithms need more than the base contract. The exact solver, for
instance, enumerates the draws of `modify_state_with_event` and of
`get_initial_state` and needs every one of them to be discrete (a library
sampler, a one-shot family draw, or `rng.choice`), never `rng.random()` or
`rng.uniform()`. Such a set is a `RoleContract`
(here `dynaplex.validation.DISCRETE_IDENTIFIABLE_EVENTS`). An algorithm checks it when it
is constructed; a model author who wants the check at compile time declares
it up front:

```python
from dynaplex.validation import DISCRETE_IDENTIFIABLE_EVENTS

@mdp(promises=DISCRETE_IDENTIFIABLE_EVENTS)
@const_dataclass(init=False, slots=True)
class LostSalesMDP:
    ...
```

Every promise is named; the [promises reference](promises.md) states each
assumption, what breaks it and how to fix it.
`dynaplex.validation.validate(engine, contract, cls=MyMDP)` evaluates a set
without raising, returning a verdict with the violations. See the API page
[Modelling MDPs → Promises](api/modelling.md#promises).

## Advanced runtime primitives

The functions below are also callable inside compiled DynaML code, but they
live in `dynaplex.runtime` and everyday modelling does not need them — they
are building blocks for writing your own high-performance algorithm
harnesses on top of the engine (see the
[`dynaplex.runtime` API reference](api/runtime.md)):

- **`clone_into(src, dst)`** — deep *in-place* assignment: makes `dst`
  structurally equal to `src` while preserving the identity of `dst` and
  its sub-objects where possible. Both arguments must be of the same
  mutable class. External references to `dst`'s sub-objects observe the
  update. NumPy array fields are copied in place when shapes match exactly,
  and replaced by a fresh array otherwise. Useful for re-initializing
  long-lived state objects without allocation.
- **`reseed(rng, seed)`** — re-seeds an existing generator of either family
  in place, exactly as if it had been freshly constructed with that seed;
  the allocation-free companion to `clone_into` for recycling long-lived
  trajectory state.
- **`combine_seeds(eval, global_seed, sample, trajectory, stream)`** —
  packs coordinates into a single collision-free non-negative seed, for
  constructing independent random streams in simulation experiments.
- **`monotonic()`** — a monotonic wall-clock reading in seconds
  (like `time.perf_counter`), coherent across threads.
- **`time.sleep(seconds)`** — suspends execution.

!!! warning "Determinism"
    `monotonic()` and `time.sleep` are the only deliberately
    non-deterministic built-ins. Clock values must only be written to
    timing/diagnostic buffers — never into state that affects transitions,
    costs, or sampled results.

## Runtime checks and fast mode

Compiled code can run in two modes. **Checked** keeps every runtime check:
`assert` statements, `if __debug__:` blocks, the bounds checks on list and
array indexing (`xs[i]` raises on an index out of range, as in Python), and
the numeric guards that mirror Python's exceptions: a negative shift count,
an integer `//` or `%` by zero, a float-to-int conversion of NaN, infinity
or a value outside the 64-bit range, and a 64-bit integer overflow, which
Python's unbounded ints never have (see [Arithmetic](#arithmetic)). **Fast**
compiles all of those out: an `assert` costs nothing, `xs[i]` is a plain
load with Python's negative-index wrap but no range check, the numeric
operations compute their documented fast-mode values, and the loops the
featurizers and policies spend their time in become branch-free — which is
what lets the JIT hoist and vectorize them. In fast mode a violated check is
*undefined behaviour*, the same trade a C release build makes: an index out
of range reads or writes the wrong memory instead of raising. Results are
bit-identical between the modes whenever no check would have fired.

The mode is a single flag on the compile entry points and on every algorithm:

```python
dynaplex.Engine(Root, ..., checks=False)        # fast
dynaplex.Program(fns, checks=True)              # checked (the default for Engine/Program)
dynaplex.PolicyComparer(mdp, checks="rehearsal")   # the default for the algorithms
dynaplex.DCL(mdp, policy, features=F, checks="rehearsal")
dynaplex.gym.VectorEnv(mdp, features=F, checks="rehearsal")
```

For the algorithms the default is **`"rehearsal"`**: the algorithm first
compiles the model checked and runs a short seeded pass through the very
same kernels it will use — the comparer evaluates each policy on 32
trajectories, the gym env steps a small twin with random valid actions, DCL
runs a tiny collection — and only then compiles fast and does the real work.
A deterministic misuse (a feature-count mismatch, an always-violated assert)
therefore raises before the fast run starts, with the message you wrote.
`checks=True` runs everything checked; `checks=False` skips the rehearsal.
On `DCL` the flag covers the generation-0 collection only: later generations
always collect fast (see *Training → Machines*).

Rare-state failures are probabilistic and can slip through a rehearsal. The
environment variable **`DYNAPLEX_CHECKS=1`** reruns any script fully checked
without a code change (`DYNAPLEX_CHECKS=0` forces fast); an explicit
`checks=` argument always wins over the environment. To check a model on its
own, before any experiment, call the same step directly:

```python
report = dynaplex.rehearse(mdp, policy, features=MyFeaturizer, trajectories=64)
```

`rehearse` validates the policy shape and the featurizer declaration, runs the
checked simulation, and additionally compares the compiled results bytewise
against the same trajectories in CPython — a compiler divergence is the one
class of problem a model author cannot see, and it is reported here as a
`RuntimeError`. See [`dynaplex.rehearse`](api/evaluation.md#rehearsal).

## Common pitfalls

- Missing type hints on parameters or return.
- Using default values for a list parameter.
- Optional primitives or list types (`Optional[int]`, etc.), or assigning
  `None` to any list, array, generator, or scalar.
- Assigning `None` to an object that is not annotated with `| None`.
- Reusing a local variable name with a different type.
- Reading a variable outside the `if`/loop in which it was first assigned.
- Mutating a const container, or passing a mutable container into a const
  slot (pass a fresh copy instead).
- Relying on `print` output from compiled code (it is ignored).
- Defining enums or dataclasses inside a function instead of at module
  level.
