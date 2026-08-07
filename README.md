# DynaPlex

DynaPlex is a Python library for solving Markov Decision Processes and
similar models (POMDP, HMM). Its design prioritizes clean modelling and solving of problems arising in operations management (OM): supply chain management, transportation, manufacturing and warehousing, and maintenance optimization. Models are written in
**DynaML** — a subset of Python suitable for modelling OM problems — and executed by a fast multi-threaded
engine with a bundled LLVM JIT: you read and write canonical Python code,
and get auto-vectorized C++ speed. Just as importantly, the library bundles
algorithms such as deep controlled learning (DCL), specifically designed for the highly stochastic problems that arise in typical OM applications. 

**[Documentation & tutorials](https://dynaplex.github.io/DynaPlex/)** ·
**[PyPI](https://pypi.org/project/dynaplex/)** ·
**[Discussions](https://github.com/DynaPlex/DynaPlex/discussions)**

## A new DynaPlex

DynaPlex has been rewritten from the ground up around a purpose-built
engine that compiles and runs DynaML at native (C++) speed,
multi-threaded, with the LLVM JIT bundled in the wheel. No C++, no compiler, no build step needed: `pip install dynaplex` is the whole setup.

DynaPlex is free to use for everyone — research, teaching, and
commercial applications alike, with no feature or usage limits. It ships
as pre-built wheels on PyPI under a free-use license; the engine's
source is not published. This repository is the library's public home:
the documentation source, the issue tracker and the discussions live
here. Documentation contributions are welcome — every docs page has an
edit button that leads back to this repo. See [License](#license) below.

The original C++20 library is preserved in full — all branches, including
the research branches — in the archived
[DynaPlex/DynaPlex-legacy](https://github.com/DynaPlex/DynaPlex-legacy).

## Installation

```bash
pip install dynaplex
```

DynaPlex is distributed as pre-built, self-contained wheels — there is no
build-from-source step and no external LLVM to install.

| | |
|---|---|
| Python | 3.13 only |
| macOS (Apple Silicon) | arm64, macOS 14.0 or newer |
| macOS (Intel) | x86_64, macOS 15.0 or newer |
| Linux | x86_64, glibc 2.28 or newer (`manylinux_2_28`) |
| Windows | AMD64 (64-bit) |

The only required runtime dependency is NumPy. PyTorch is needed only for
the reinforcement-learning algorithms; install it separately via the
[PyTorch selector](https://pytorch.org/get-started/locally/).

Verify your install (including the JIT) with:

```bash
python -m dynaplex.selftest
```

## Citing

When using DynaPlex in your research, please cite:

```bibtex
@software{DynaPlex,
  author = {Akkerman, Fabian and Begnardi, Luca and {Lo Bianco}, Riccardo and Temizoz, Tarkan and Mes, Martijn and {van Jaarsveld}, Willem},
  title = {{DynaPlex} software library and documentation},
  url = {https://github.com/DynaPlex/DynaPlex},
  year = {2026}
}
```

## License

- **The DynaPlex engine** (the wheels on PyPI) is free to use for any
  purpose — academic and commercial alike — under the DynaPlex License
  Agreement included in every wheel as `LICENSE.txt`. The grant for each
  version you obtain is royalty-free, perpetual and irrevocable, so
  published results stay reproducible. Models and code you write with
  DynaPlex are entirely yours. The engine's source code is not
  published.
- **This repository** (documentation, examples, supporting files) is
  licensed under the [MIT License](LICENSE). By contributing, you agree
  that your contributions are licensed under the same terms.

## Getting help

- **Questions and discussion** —
  [GitHub Discussions](https://github.com/DynaPlex/DynaPlex/discussions).
- **Bug reports and DynaML feature requests** —
  [GitHub issues](https://github.com/DynaPlex/DynaPlex/issues); the issue
  forms tell you what to include.
- **Documentation fixes** — edit the page directly via its edit button, or
  open a PR against `docs/` in this repository.

## Working on the documentation

The docs are built with
[Material for MkDocs](https://squidfunk.github.io/mkdocs-material/):

```bash
python3 -m venv .venv
.venv/bin/pip install -r requirements.txt
.venv/bin/mkdocs serve        # live preview at http://127.0.0.1:8000
```

`mkdocs build --strict` must pass (no warnings) before a change can be
merged. Content lives in `docs/` (Markdown). The runnable example scripts
in `docs/downloads/` are copies of tested originals maintained alongside
the engine — please point out problems in them via an issue rather than
editing them in place.
