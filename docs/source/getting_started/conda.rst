Running with Python
===================

After implementing your MDP in C++, it s possible to interact with DynaPlex through Python. This way, you can run algorithms, collect statistics, and benchmark the built-in DynaPlex algorithm with existing algorithms via Python libraries, e.g., benchmark DCL with PPO. To do this, we will need to build PyBindings. Follow the below steps. We assume you already installed a suitable Python IDE, e.g., PyCharm or VSCode. Note that we provide several example Python scripts in the `python` folder which show you how to run DCL and PPO (using the `tianshou` library) from Python.


**1. Virtual environment creation**

First, in your Python IDE or in your terminal set up a Conda environment for compiling the Python bindings and running Python scripts that load those bindings:

.. code-block:: bash

   conda envDP create -f path/to/environment.yml

.. note::
	For this to work, Conda needs to be available. On Snellius, it can be loaded via the module environments, something like:

.. code-block:: bash

   module load 2022
   module load  ...

.. hint::
	An environment file ``python/environment.yml`` is provided.

**2. Adapt your CMakeUserPresets.json**

Adapt the paths linking to your Conda virtual environment. Do this for WinPB and/or LinPB, depending if your work on Windows or Linux.

**3. Compile PyBindings**

In your C++ IDE, compile the PyBindings. For instance, in Visual Studio, you can select WinPB from the drop-down build configurations.

**4. Activate your virtual environment**

In your Python IDE or terminal, activate your virtual environment

.. code-block:: bash

	conda activate EnvDP

**5. Activate Python executable**

In your Python IDE or terminal, navigate to the python folder. Next, run:

.. code-block:: bash

	pip install -e .

Note: do not forget the dot (.) at the end.

After this step, you should be able to import DynaPlex as a library in Python:

.. code-block:: bash

	from dp import dynaplex

On Linux (Snellius), you can simply call, which should be enough:

.. code-block:: bash

	conda activate EnvDP
