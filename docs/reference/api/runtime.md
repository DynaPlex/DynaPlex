# API reference — `dynaplex.runtime`

Advanced runtime primitives, for authors of custom algorithm harnesses.
Everyday modelling and solving does not need anything on this page.

## Engine

::: dynaplex.runtime.Engine

## State recycling and streams

::: dynaplex.runtime.clone_into

::: dynaplex.runtime.reseed

::: dynaplex.runtime.combine_seeds

::: dynaplex.runtime.monotonic

## Concurrent worlds

::: dynaplex.runtime.separate_world_class

::: dynaplex.runtime.sync_worlds

## Compilation pipeline

The classes below are deep plumbing; they are listed for completeness and
documented by their docstrings only.

::: dynaplex.runtime.Program
    options:
      members: false

::: dynaplex.runtime.compile_function_from_string

`build_datamodel_from_dataclasses(dataclasses)` — build a `DataModel` from a
list of dataclass types (see its docstring via `help()`).

::: dynaplex.runtime.DataModel
    options:
      members: false

::: dynaplex.runtime.TypedWordHandle
    options:
      members: false

::: dynaplex.runtime.TypedWordView
    options:
      members: false
