# Troubleshooting compile errors

When DynaPlex compiles a model and something in it is not valid DynaML, the
compiler raises an error. Most tracebacks end like this:

```
ValueError: In function 'LostSalesMDP.modify_state_with_action$State$TrajectoryContext$Int'
(model.py), line 3: Type mismatch: expected Int, got Float
  Line 1: def modify_state_with_action(self, state: State, context: TrajectoryContext, action: int) -> None:
  Line 3:     state.total_inv += self.h
DynaML compilation failed. For supported constructs and language rules, see ...
```

Read from the bottom up: the last lines carry the function, source line, and
the message. This page groups the messages you are most likely to see into
families, with the typical cause and fix for each. Search the page for a
distinctive fragment of your message. The rules the compiler enforces are
described in full in the [language reference](language-reference.md); if you
get stuck, paste the error and your model into an LLM together with that
document, or ask on
[GitHub Discussions](https://github.com/DynaPlex/DynaPlex/discussions).

Errors are raised during *compilation*, i.e. when a model is handed to an
algorithm (`DCL`, `PolicyComparer`, `PPO`, or an `Engine` directly) — never
while running your model in plain CPython. Testing your methods in CPython
first, and validating with pyright, catches many of these before the
compiler does.

## Type mismatch

```
Type mismatch: expected Int, got Float
Type mismatch: cannot assign Float to variable 'x' (expected Int)
Type mismatch for argument 'demand': expected Int, got Float
Cannot assign Float to field 'total_inv' (expected Int)
```

Every variable, field, and parameter in DynaML has one fixed type. The only
implicit conversions are numeric widenings `bool` → `int` → `float`; nothing
narrows implicitly, so assigning a `float` into an `int` slot is an error
(where CPython would happily truncate later or not at all).

Fix: convert explicitly (`int(x)`, `float(x)`) at the point where you mean
it, or reconsider the field's declared type. When a formatted type carries a
`const` prefix (e.g. `expected const list[int], got list[int]`), the mismatch
is about mutability, not the element type — see
[Const containers](#const-containers) below.

## Void expression used as a value

```
Cannot use statement-only expression in ... . Expressions like list.append() and
list.sort() don't return a value and can only be used as statements in DynaML compiled code.
Expression must return exactly 1 value(s), but got 0 value(s). This expression
returns nothing (void). Cannot assign void result to a variable.
```

Mutating calls (`list.append`, `list.sort`, `heapq.heappush`,
`FifoQueue.push_back`, …) return nothing in DynaML, and unlike Python there
is no `None` value to bind. Use them as bare statements:

```python
x = q.push_back(3)   # error: void result
q.push_back(3)       # correct
```

## Unknown field, method, or name

```
Field 'on_hand' not found in class 'State'
Unknown method 'FifoQueue.nosuchmethod'. Method not found on class 'FifoQueue'
Method 'helper' not found on dataclass 'MyMDP'
Variable 'x' is used before being defined. Variable must be defined before use.
Unknown class 'Foo'
```

Usually a typo — DynaML cannot create fields or variables on the fly, so a
misspelled assignment target is an error rather than a silent new attribute.
Two non-typo causes:

- You are calling a CPython-only API. Compiled code supports a fixed set of
  methods per built-in type and a limited set of modules — see
  [Built-in functions and modules](language-reference.md#built-in-functions-and-modules).
- A variable is read before every path assigns it. Assign an initial value
  before the `if`/loop that conditionally overwrites it.

## Unsupported construct

```
Dictionaries are not supported in DynaML. Use a dataclass with named fields instead ...
Tuples are not supported in DynaML. Use a dataclass with named fields, or separate variables. ...
Sets are not supported in DynaML. Use a list instead.
Unsupported expression type: ListComp
Unsupported statement type: With
Built-in function 'zip' not yet supported
Only simple function calls and method calls supported (e.g., int(x) or obj.method())
Slice assignment is not supported. Use list[i] = value with an integer index.
Nested tuple unpacking is not supported. Use separate assignment statements instead.
```

DynaML is a subset of Python: dictionaries, tuples, sets, strings (beyond
assert messages), comprehensions, lambdas, generator expressions, slicing,
`with`, `try`, and most of the standard library are outside it. The
[function bodies](language-reference.md#function-bodies) section lists what
is in. The usual fixes:

- Where you would reach for a dict or tuple, use a dataclass with named
  fields — the direct DynaML equivalent of a small fixed key set — or a list
  indexed by an `int` or enum for uniform collections. (Returning multiple
  values from a function *is* supported: `return a, b` with `x, y = f(...)`
  at the call site; only tuples as *values* are out.)
- Replace comprehensions with explicit loops, and slices with indexed loops.
- Hoist unsupported preprocessing into the MDP `__init__` (which runs in
  full CPython) and store the result in supported fields.

## Cannot determine / infer a type

```
Cannot determine type of ...
Cannot infer type for variable 'x' from None literal. ...
Cannot infer list element type for variable 'xs'. Please provide a type annotation (e.g., xs: list[...] = ...).
sorted() does not accept an empty list literal []. Assign to a typed variable first.
```

The compiler types every expression before running anything, and a bare `[]`
or `None` carries no type of its own. Give the value a home with a declared
type first:

```python
xs: list[int] = []
xs.append(action)
best = sorted(xs)
```

`None` can only ever be assigned where the annotation says `SomeClass | None`
— see [Optional objects](#none-and-optional-objects).

## Variable declaration rules

```
Annotated variable 'x' must be initialized with a value. Use 'x: int = value' syntax.
Variable 'x' is already declared. Type annotations can only be used when first declaring a variable, ...
Cannot redeclare function parameter 'x' with a type annotation. ...
```

A type annotation introduces a variable, exactly once, together with its
initial value. Reassignment uses plain `x = value`, and the new value must
have the variable's established type — a local cannot change type midway
through a function.

## Function signatures and annotations

```
the return type must be annotated (and not None)
Invalid type annotation: ...
*args/**kwargs are not supported
List parameters cannot have defaults (mutable default arguments are shared across calls). ...
Function 'f' ... uses type 'X' which is not a dataclass or supported primitive type. ...
```

Every compiled function annotates all parameters and its return type, using
supported types only: primitives, homogeneous `list[...]`, ndarrays,
dataclasses, enums, and the `dynaplex` built-in classes. See
[Functions and method signatures](language-reference.md#functions-and-method-signatures)
for the exact rules, including which defaults are allowed.

## Const containers

```
expected const list[int], got list[int]
Class 'X' is const (declared with @const_dataclass, or reachable from a const class) but field ... 
Mutating a const container / passing a mutable container into a const slot
```

Fields of `@const_dataclass` classes (like your MDP) are deeply immutable in
compiled code, and const-ness must match at every binding: a mutable list or
array is never accepted in a const slot, because both sides would alias the
same storage. The sanctioned way in is a *fresh* value — a list literal,
`sorted(...)`, `.copy()`, `[:]`, `numpy.zeros(...)`, etc. — produced directly
into the const slot. Conversely, state (reachable from your State class) is
mutable and cannot be stored into a const class.

## None and optional objects

```
Reserved built-in type 'FifoQueue' is never noneable ...; only user Object classes support None.
Non-optional Object type cannot have a default. Either make the field optional (T | None) or remove the default.
```

Only user-defined dataclass types can be optional, via `SomeClass | None`.
Primitives, lists, ndarrays, and the built-in runtime classes can never hold
`None`. If you need an "absent" primitive, use a sentinel value or an extra
`bool` field.

## ndarray restrictions

```
Too many indices for ndarray rank 1: got 2
Bare `.shape` is not supported (DynaML has no tuple type). Use indexed access `a.shape[k]` ...
Unknown or unsupported ndarray method 'cumsum'. Supported reduction methods: ...
len() requires a list or ndarray argument, got ...
```

Compiled code supports a deliberate subset of numpy: fixed dtype and rank per
array (declared via `NDArray` annotations), whole-array reductions
(`sum`/`mean`/…, no `axis=`), element access with exactly `rank` integer
indices, and a fixed set of constructors (`zeros`, `ones`, `full`, `*_like`,
`array`). Anything else — fancy indexing, broadcasting arithmetic across
arrays, reshaping — belongs in `__init__` (CPython) or in a redesign of the
state layout.

## Class and enum definition rules

```
field(init=False) is not supported. All dataclass fields must participate in __init__.
Scalar fields cannot use default_factory
Enum default must be a member of ...
X must derive directly from enum.Enum, not ...
Dataclass 'A.B' has '.' in its name. Dots are reserved for qualified names ...
Dataclass 'X' defines __ne__, which is not supported: '!=' always compiles to the negation of __eq__ ...
```

State and MDP classes are plain dataclasses defined at module level, with
every field annotated with a supported type and defaults restricted to
compile-time constants. Enums derive directly from `enum.Enum` with integer
values. The [data types](language-reference.md#dynaml-data-types) section
has the full field-type table.

## Assert restrictions

```
Assert requires a message string (literal or simple f-string)
Assert f-strings must use bare {expr} without !r/!s/!a or format specifiers
```

`assert` is supported in compiled code, but its message must be a literal
string or a simple f-string — the only place strings appear in DynaML.

## Internal error

```
Internal error: ...
```

Anything prefixed `Internal error` (or naming instructions, temps, or
opcodes) indicates a compiler bug, not a modelling mistake. Please report it
on the [issue tracker](https://github.com/DynaPlex/DynaPlex/issues) with the
model that triggers it.
