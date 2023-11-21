![Dynaplex logo](docs/source/assets/images/logo.png)

DynaPlex is a software library for solving Markov Decision Problems and similar models (POMDP, HMM) written primarily in C++20 with bindings for python. It supports 
deep reinforcement learning, approximate dynamic programming, classical parameterized policies, and exact methods based on policy and value iteration. Models in DynaPlex are written in C++, and exposed via a generic and vectorized interface. 

DynaPlex focuses on solving problems arising in Operations Management: Supply Chain, Transportation and Logistics, Manufacturing, etc. 

---

## High-level overview of folder structure

- **`LICENSES/`**: Contains the licenses to used libraries and packages.
- **`bash/`**: Contains the files used for running on a Linux HPC.
- **`cmake/`**: Contains main fucntionality for building with CMake.
- **`docs/`**: Contains the documentation.
- **`python/`**: Contains all functionality for pybindings.
- **`src/`**: Contains the main code, users will only work in this folder.
  - **`executables/`**: Contains all executables you can run (you can add additional executables yourself here, that use the library).
  - **`extern/`**: Contains all external libraries used (e.g., googletest).
  - **`lib/`**: Contains all algorithms and all MDP models, you can implement your MDP in src/lib/models/models.
  - **`tests/`**: Contains all code for unit testing (supported by googletest).

---

## Cloning the Repository with Dependencies

When cloning the repository, it's essential to also download the required submodules:

```bash
git clone --recurse-submodules https://github.com/WillemvJ/DynaPlexPrivate.git
```

if you did not recurse submodules, or if you use other tools for cloning repos, please somehow ensure that submodules (especially googletest) are available. 

### Prerequisites

- **CMake**: Building is supported with a modern CMake version (>= 3.21), often supplied with modern C++ IDEs.
- **PyTorch**: Tests were conducted with a recent version. 
- **Python Bindings**: With pybind11 - we provide an anaconda python/environment.yml that can be used to create appropriate environment. 

---

## Documentation

The documentation can be found in the [docs](docs/) folder in this repo.

---

## Examples

We provide several example implementations of MDPs, these can be found in [here](src/lib/models/models/). 

---

## Contributing and Getting help

We are very happy if you want to share your contributions to DynaPlex with the rest of the community.
All contributions are submitted through GitHub Pull Requests. For all instructions see the docs/

We are happy to discuss development and issues in the GitHub repository. Please open a new issue if you want to discuss something with us.
When you find a bug: open a new issue in the repository, please include a short, self-contained code snippet that reproduces the problem.

---

## Citing

To do.

## Algorithms

The DCL algorithm is available, for an example usage see [here](src/examples/dcl_example/dcl_example.cpp). 