#!/bin/bash
ARG1=$1
if [ ! -f /usr/lib64/libcuda.so.1 ]
then
   export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$EBROOTCUDA/lib64/stubs  
fi
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:~/anaconda3/envs/py37_torch/lib/python3.7/site-packages/torch/lib
cd build/linux/bin
./DynaPlexExample
