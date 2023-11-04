Conda Environment Setup
=======================

To set up a Conda environment for compiling the Python bindings and running Python scripts that load those bindings:

.. code-block:: bash

   conda env create -f path/to/environment.yml

.. note::
	For this to work, Conda needs to be available. On Snellius, it can be loaded via the module environments, something like:

.. code-block:: bash

   module load 2022
   module load  ...

.. hint::
	A sample environment file ``python/environment.yml`` is provided.

After setting up the environment, and compiling the bindings, Python bindings will allow you to execute the Python scripts just, see also windows descriptions.