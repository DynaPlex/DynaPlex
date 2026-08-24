# Verifying your installation

After installing DynaPlex you can confirm that the wheel works on your
platform — including the LLVM **JIT** — with a single command. Everything
needed ships inside the package, so there is nothing else to download.

## Supported platforms

DynaPlex is distributed as pre-built, self-contained wheels. There is no
build-from-source step and no external LLVM to install — the JIT is bundled.

| | |
|---|---|
| Python | 3.13 or 3.14 |
| macOS (Apple Silicon) | arm64, macOS 14.0 or newer |
| macOS (Intel) | x86_64, macOS 15.0 or newer |
| Linux | x86_64, glibc 2.28 or newer (`manylinux_2_28`) |
| Windows | AMD64 (64-bit) |

The only required runtime dependency is NumPy. PyTorch is needed *only* for
the reinforcement-learning algorithms (`PPO`, `DCL`); install it separately
from <https://pytorch.org/get-started/locally/> if you need them.

## Install

```bash
pip install dynaplex
```

If `pip` cannot find a matching wheel, it is almost always a platform
mismatch — most often a Python version other than 3.13/3.14, an OS older than
the floors above, or a 32-bit interpreter. Check with:

```bash
python -c "import sys, platform; print(sys.version); print(platform.machine())"
```

## Run the self-test

The quickest and most complete check is the bundled self-test. It prints your
environment, exercises the compile/interpret pipeline end to end, and — when
the JIT is present (the default in the shipped wheels) — compiles and runs a
small kernel through LLVM:

```bash
python -m dynaplex.selftest
```

Expected output on a healthy install (details will differ per machine):

```text
DynaPlex installation self-test
================================================
  dynaplex version : 1.10.0
  build config     : Release
  JIT enabled      : True
  python           : 3.13.12 (/path/to/python)
  platform         : macOS-14.5-arm64 / arm64
================================================

  [ OK ]  import dynaplex + numpy
  [ OK ]  interpreter round-trip  -- 2 + 3 == 5
  [ OK ]  JIT compile + run  -- LLVM kernel matches interpreter

RESULT: OK -- your DynaPlex installation works, JIT included.
```

The command exits with status `0` on success and `1` if any check fails,
so it can be dropped straight into a CI pipeline:

```bash
python -m dynaplex.selftest || echo "DynaPlex self-test failed"
```

What each check means:

- **import dynaplex + numpy** — the extension module loaded and NumPy is
  present.
- **interpreter round-trip** — a function was compiled, ingested, and run on
  the DynaPlex interpreter (the core pipeline works even without the JIT).
- **JIT compile + run** — a kernel was compiled through LLVM and its result
  matched the interpreter. If your wheel was built without the JIT this check
  reports `[SKIP]` and is not counted as a failure.

## Manual one-line checks

If you prefer to check things by hand, these are safe to paste into a shell:

```bash
# Version and build configuration
python -c "import dynaplex; print(dynaplex.__version__, dynaplex.__build_config__)"

# Is the JIT compiled into this wheel?
python -c "from dynaplex import _core; print('JIT enabled:', _core.has_jit())"
```

!!! note
    You cannot run the JIT smoke test by pasting a function definition into
    `python -c` or the interactive REPL: the compiler reads a function's
    **source** (via `inspect.getsource`), which only exists for functions
    defined in a real `.py` file. That is exactly why the self-test ships as
    a module — use `python -m dynaplex.selftest` rather than trying to
    reproduce it inline.

## Interpreting the result

| Result | Meaning |
|---|---|
| `RESULT: OK` | Everything works, JIT included. |
| `RESULT: OK` (JIT skipped) | Core works; this wheel has no JIT (unusual for the published wheels — expected only for a JIT-free build). |
| `RESULT: FAILED` | Something is wrong; the failing check and its error are printed above the summary. |

If the self-test fails, please
[open an issue](https://github.com/DynaPlex/DynaPlex/issues) and include the
full output (the environment block and the per-check lines).
