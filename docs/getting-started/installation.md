# Installation

## Requirements

- **Python 3.13 only**
- **Platform support:**
    - Windows: AMD64
    - Linux: x86_64 (glibc 2.28 or newer, `manylinux_2_28`)
    - macOS: Apple Silicon (arm64), macOS 14.0 or newer; Intel (x86_64), macOS 15.0 or newer
- **PyTorch:** Algorithms require a compatible PyTorch installation. Use the
  [PyTorch selector](https://pytorch.org/get-started/locally/) to install the
  appropriate version for your system.

!!! note
    Building from source is presently not supported.

## Install

Install DynaPlex using pip:

```bash
pip install dynaplex
```

Or install with all optional dependencies:

```bash
pip install "dynaplex[complete]"
```

!!! note
    The quotes are required in some shells (notably zsh, the default on
    macOS), where square brackets would otherwise be interpreted as a
    pattern-matching expression.

!!! note
    Even with a complete install, separate installation of PyTorch is still
    required for the RL algorithms.

## Next steps

Once you have DynaPlex installed,
[verify your installation](verifying-installation.md), then start with the
[introduction to MDPs](introduction-to-mdps.md) or dive right into the
[tutorials](../tutorials/airplane-mdp.md).
