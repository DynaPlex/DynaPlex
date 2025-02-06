Running on a Linux cluster
==========================

After implementing your model, you may want to scale up your training procedure by running on a cluster. Below we provide basic instructions for installation, building, and running on a cluster. This manual assumes you work on a Linux cluster with command line interface. We provide specific suggestions for running on the Snellius cluster, which is the Dutch national supercomputer hosted by Surf.


**0. Download and unzip LibTorch (Only on Snellius):**

    Easiest way to link LibTorch: tou can download the LibTorch version for Linux and upload it (still zipped) to your home folder. Next, you can unzip the library:

    .. code-block:: bash

      unzip libtorch-version-name.zip
    

**1. Initialize Environment and Load Modules (Only on Snellius):**

   .. code-block:: bash

      cd bash
      source loadmodules.sh
      cd .. #back to root

**2. Build:**

   - For a specific preset from CMake user presets (e.g., `LinRel`):

     .. code-block:: bash

        cmake --preset=LinRel  # Other options: LinDeb/ LinDB

   - Compile all code:

     .. code-block:: bash

        cmake --build out/LinRel -- -j12

     Note: The option ``-- -j12`` instructs to parallelize the build.

   - If you encounter the error:

     .. code-block:: text

        CMake Error: Could not read presets from /home/willemvj/DynaPlexPrivate: Unrecognized "version" field

     Your CMake version is not recent enough. On Snellius, you may have forgotten to ``source loadmodules.sh`` to bring the recent CMake version into scope.

   - Compile a specific target (e.g., ``sometarget``). This will only build you specific target, e.g., a target in the ``src``

     .. code-block:: bash

        cmake --build out/LinRel --target sometarget -j12

   - Building DynaPlex for the first time may take a long time. Therefore, we advice to request a node and build on this node, which allows to paralellize the build over more threads and will speedup the building process signficantly. First, request a node:

    .. code-block:: bash
    
        srun -p genoa -c 192 -n 1 -t 00:59:00 --pty /bin/bash

    Next, follow the steps above again (load the modules and indicate the CMake preset). Next, we can build, and parallelize the build over more nodes (j120 instead of j12), e.g.:

    .. code-block:: bash
    
        cmake --build out/LinRel --target sometarget -j120

**3. Run:**
    - Executables may be run from a compute node, that can be obtained as follows:

     .. code-block:: bash
    
        srun -p genoa -c 192 -n 1 -t 00:59:00 --pty /bin/bash
    
    also use srun to then do something on that node. You can alternatively run using the sbatch command, see `CPU.job` in the 'bash/' folder for an example file.
