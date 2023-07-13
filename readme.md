command on linux:

cd bash
source loadmodules.sh



build a specific preset - i.e. one in cmake user presets, e.g. LinRelease:

cmake --preset=LinRelease
compile:
cmake --build out/LinRelease -- -j8

compile specific (test)  target:

cmake --build out/LinRelease --target 


for setting up conda environment for pybind:

conda env create -f environment.yml