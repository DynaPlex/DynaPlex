command on linux:

cd bash
source loadmodules.sh



build a specific preset - i.e. one in cmake user presets, e.g. LinRelease:

cmake --preset=LinRel
compile:
cmake --build out/LinRel -- -j8
  
    CMake Error: Could not read presets from /home/willemvj/DynaPlexPrivate: Unrecognized "version" field
    Do not forget to source loadmodules to bring into scope recent cmake version
compile specific (test)  target:

cmake --build out/LinRel --target 

//For testing MPI. 
srun -p genoa -c 32 -n 1 -t 00:30:00 --pty /bin/bash

//for testing on linux:
//go to out/LinRel and type ctest --verbose, or simply execute individual test executables, to test on linux. 



for setting up conda environment for pybind:
conda env create -f path/to/environment.yml