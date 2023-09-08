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


for setting up conda environment for pybind:

conda env create -f path/to/environment.yml