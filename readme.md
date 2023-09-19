# Building Instructions for Windows and Linux

## Prerequisites

- **CMake**: Building is supported with a modern CMake version (>= 3.21).
- **PyTorch**: Tests were conducted with a recent version. 
- **Python Bindings**: Code uses pybind11 - currently we provide a anaconda environment.yml that can be used to create appropriate environment. 

## Configuration

For the easiest setup, provide a `CMakeUserPresets.txt` in the root directory. Note: `CMakeUserPresets.txt` in the root directory is ignored by Git. This allows for specific configurations tailored to your local setup. An example file can be found at `cmake/resources/`. Be sure to adapt this to your specific needs.

## Windows

With the setup in place, compiling the library should be straightforward in most IDEs that support CMake.

## Linux/Snellius

1. **Initialize Environment and Load modules**:
    ```bash
    cd bash
    source loadmodules.sh
    ```

2. **Build**:
    - For a specific preset from cmake user presets (e.g., `LinRelease`):
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
      You may have forgotten to `source loadmodules.sh` to bring the recent CMake version into scope.

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
**Note**: for this to work, conda needs to be available. It can be loaded via the module environments,
something like
```bash
module load 2022
#instruction for bringing appropriate anaconda environment in scope. 
```
**Note**: A sample environment file `python/environment.yml` is provided.

After setting up the environment, compiled Python bindings will allow you to execute Python scripts.