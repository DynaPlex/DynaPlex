#!/bin/bash
module load 2022
module load CMake/3.23.1-GCCcore-11.3.0
module load OpenMPI/4.1.4-GCC-11.3.0
export OMP_NUM_THREADS=1
export OPENBLAS_NUM_THREADS=1
conda activate envDP

#CUDA not currently tested. 
#module load cuDNN/8.4.1.50-CUDA-11.7.0
#module load cuDNN/8.6.0.163-CUDA-11.8.0

module list

chmod +x exeWrapper.sh
