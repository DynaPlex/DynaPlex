# FIFO queues

`FifoQueue` is a first-in-first-out queue of integers: values enter at the
back and leave at the front. It is what a model uses for a pipeline of
outstanding orders, a line of waiting jobs, or anything else that arrives in
order and is served in order. It runs in plain Python and in compiled code
with the same results.

```python
from dynaplex.modelling import FifoQueue

q = FifoQueue()
q.push_back(5)           # [5]
q.push_back(7)           # [5, 7]
oldest = q.pop_front()   # 5, and q is [7]
```

A list can do the same with `append` and index arithmetic, but removing from
the front of a list is not supported in DynaML and would shift every element.
`FifoQueue` keeps its values in a ring, so `push_back` and `pop_front` cost
the same however long the queue is.

## In a model

The lost-sales model shipped with DynaPlex keeps its outstanding orders in a
queue: the order placed now goes in at the back, and the order placed
`leadtime` periods ago comes out at the front.

```python
@dataclass(slots=True)
class State:
    on_hand: int
    pipeline: FifoQueue
    category: StateCategory

def get_initial_state(self, context: TrajectoryContext) -> State:
    pipeline = FifoQueue()
    pipeline.reserve(self.leadtime)     # the longest it will ever be
    pipeline.fill(self.leadtime - 1)    # that many orders of size 0
    return State(on_hand=0, pipeline=pipeline, category=StateCategory.AWAIT_ACTION)

def modify_state_with_action(self, state: State, context: TrajectoryContext, action: int) -> None:
    state.pipeline.push_back(action)
    state.category = StateCategory.AWAIT_EVENT

def modify_state_with_event(self, state: State, context: TrajectoryContext) -> None:
    ...
    state.on_hand += state.pipeline.pop_front()   # the oldest order arrives
```

## What it offers

| | |
|---|---|
| `q.push_back(v)` | add `v` at the back |
| `q.pop_front()` | remove the front value and return it |
| `q.front()`, `q.back()` | read the front or the back value without removing it |
| `len(q)`, `q.is_empty()` | the number of values; whether there are none |
| `q[i]`, `q[i] = v` | read or replace the value at position `i`, where the front is position 0 |
| `q.sum()` | the sum of all values |
| `q.fill(n, value=0)` | discard the contents and hold `n` copies of `value` |
| `q.clear()` | discard the contents |
| `q.reserve(capacity)` | make room for `capacity` values, so that later pushes do not have to grow the queue |
| `q1 == q2` | the same values in the same order |

Things to know:

- **Integers only.** A queue of objects is not offered; keep the objects in a
  list and their indices in the queue, or keep one queue per quantity.
- **No loop over a queue.** `for x in q:` does not compile. Walk it by
  position: `for i in range(len(q)): ... q[i]`.
- **No negative positions.** `q[-1]` is an error; use `q.back()`.
- **Test for empty with `q.is_empty()`**, not with `if q:`. In compiled code
  `if q:` asks whether the object is there, which it always is.
- **An empty queue has no front.** `pop_front()`, `front()` and `back()` on an
  empty queue, and a position out of range, raise in checked mode
  (`FifoQueue.pop_front: queue is empty`). In fast mode they are not checked;
  see [Runtime checks and fast mode](language-reference.md#runtime-checks-and-fast-mode).
- **A queue is never `None`.** A field of type `FifoQueue` always holds one;
  an empty queue stands for "nothing waiting".
- **Reading and changing.** `pop_front`, `push_back`, `fill`, `clear`,
  `reserve` and `q[i] = v` change the queue, so a policy, a featurizer or
  `write_action_validity` may not call them on a queue in the state; see
  [Promises](promises.md).

A queue copies with `clone`, compares with `==` and can be part of a state
that the [exact solver](../training/exact-solver.md) enumerates.
