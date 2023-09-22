# DynaPlex

DynaPlex is a software library for solving Markov Decision Problems and similar models (POMDP, HMM) written primarily in C++20 with bindings for python. It supports 
deep reinforcement learning, approximate dynamic programming, classical parameterized policies, and exact methods based on policy and value iteration. Models in DynaPlex are written in C++, and exposed via a generic and vectorized interface. 

DynaPlex focuses on solving problems arising in Operations Management: Supply Chain, Transportation and Logistics, Manufacturing, etc. 

# Building Instructions for Windows and Linux

## Prerequisites

- **CMake**: Building is supported with a modern CMake version (>= 3.21).
- **PyTorch**: Tests were conducted with a recent version. 
- **Python Bindings**: With pybind11 - we provide an anaconda python/environment.yml that can be used to create appropriate environment. 

## Configuration

For the easiest setup, provide a `CMakeUserPresets.txt` in the root directory. Note: `CMakeUserPresets.txt` in the root directory is ignored by Git. This allows for passing in specific configurations and paths tailored to your local environment. An example file can be found at `cmake/resources/`. You could copy this to the root directory, and adapt it to your specific needs.

The CMakeUserPresets.txt should provide paths to certain dependences, including DYNAPLEX_IO_ROOT_DIR where input and output for DynaPlex will be stored and retrieved from. 

## Windows

With the setup in place, compiling the library should be straightforward in most IDEs that support CMake (possibly via a proper extension). 

For using the python bindings, we recommend setting up a specific conda environment using python/environment.yml. 

After this, update the CMakeUserPresets.txt for WinPB to point to the relevant dependencies inside the newly created environment. Compiling WinPB afterwards will result in the bindings being compiled. CMake automatically copies them to python/dp/libs, where python/dp/load.py should be able to locate them. As a consequence, the scripts in python/test should run them. 

## Linux/Snellius

1. **Initialize Environment and Load modules**: only on Snellius. 
    ```bash
    cd bash
    source loadmodules.sh
    ```

2. **Build**:
    - For a specific preset from cmake user presets (e.g., `LinRel`):
        ```bash
        cmake --preset=LinRel  # Other options: LinDeb/ LinDB
        ```
    - Compile:
        ```bash
        cmake --build out/LinRel -- -j12
        ```
    note - the option -- -j12 instructs to paralellize the build. 
    - If you encounter this error:
        ```
        CMake Error: Could not read presets from /home/willemvj/DynaPlexPrivate: Unrecognized "version" field
        ```
      Your CMake version is not recent enough. On Snellius, you may have forgotten to `source loadmodules.sh` to bring the recent CMake version into scope.

    - Compile a specific target [e.g. sometarget] that is not included in all:
        ```bash
        cmake --build out/LinRel --target sometarget
        ```

3. **Testing MPI**:
    ```bash
    srun -p genoa -c 32 -n 1 -t 00:30:00 --pty /bin/bash
    ```

4. **Running Tests**:
    - Go to the build directory (`out/LinRel`):
        ```bash
        ctest --verbose
        ```
    - Alternatively, execute individual test executables directly.

## Conda Environment Setup

For setting up a Conda environment, for compiling the python bindings and running python scripts that load those bindings:

```bash
conda env create -f path/to/environment.yml
```
**Note**: for this to work, conda needs to be available. On snellius, it can be loaded via the module environments,
something like
```bash
module load 2022
module load  ... 
```

**Note**: A sample environment file `python/environment.yml` is provided.

After setting up the environment, and compiling the bindings, Python bindings will allow you to execute the Python scripts just, see also windows descriptions.

## Adding Models

Detailed instructions on how to add new models or elements to the project can be found [here](docs/adding_models.md).
