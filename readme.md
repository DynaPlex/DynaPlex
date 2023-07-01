command on linux:

module load 2022
module load CMake/3.23.1-GCCcore-11.3.0


build a specific preset - i.e. one in cmake presets:
cmake --preset=linCPU
compile:
cmake --build out/linCPU

compile specific (test)  target:

cmake --build out/linCPU --target 