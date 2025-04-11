Running on a Linux cluster
==========================

After implementing your model, you may want to scale up your training procedure by running on a cluster. Below we provide basic instructions for installation, building, and running on a cluster. This manual assumes you work on a Linux cluster with command line interface. We provide specific suggestions for running on the Snellius cluster, which is the Dutch national supercomputer hosted by Surf.


**0. Download and unzip LibTorch (Only on Snellius):**

    Easiest way to link LibTorch: tou can download the LibTorch version for Linux and upload it (still zipped) to your home folder. Next, you can unzip the library:

    .. code-block:: bash

      unzip libtorch-version-name.zip
    
**1. Build:**

   We provide a special `build.job` file in the `/bash` folder that will automate all build processes for you, you will only need to change the specific target for your build and next run it using:

    . code-block:: bash

        cd bash
        sbatch build.job

    For those that prefer a manual build, below we provide the steps for building. Note that it is not allowed to build on the login node, so you will first need to request a compute node for your job:

    . code-block:: bash
    
        srun -p genoa -c 192 -n 1 -t 00:15:00 --pty /bin/bash

    As soon as you have been allocated a node, you need to load the modules using `source loadmodules.sh`
   
    .. code-block:: bash

      cd bash
      source loadmodules.sh
      cd .. #back to root

     Next follow the below steps.

- For a specific preset from CMake user presets (e.g., `LinRel`):

     .. code-block:: bash

        cmake --preset=LinRel  # Other options: LinDeb/ LinDB

   - Compile all code:

     .. code-block:: bash

        cmake --build out/LinRel -- -j120

     Note: The option ``-- -j120`` instructs to parallelize the build.

   - If you encounter the error:

     .. code-block:: text

        CMake Error: Could not read presets from /home/willemvj/DynaPlexPrivate: Unrecognized "version" field

     Your CMake version is not recent enough. On Snellius, you may have forgotten to ``source loadmodules.sh`` to bring the recent CMake version into scope.

   - Compile a specific target (e.g., ``sometarget``). This will only build you specific target, e.g., a target in the ``src``

     .. code-block:: bash

        cmake --build out/LinRel --target sometarget -j12

**3. Run:**
    - Executables may be run from using a job script, using the `sbatch` command, see `CPU.job` in the 'bash/' folder for an example file.
