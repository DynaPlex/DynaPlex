Python Environment Setup for PyBindings
=======================================

PyBindings allows you to seamlessly integrate C++ models with Python, combining the speed of C++ with the flexibility of Python. This setup enables you to run DynaPlex algorithms alongside state-of-the-art reinforcement learning algorithms, use various neural network approximators like graph neural networks, and more.

Prerequisites
~~~~~~~~~~~~~

Before you begin, make sure you have the following prerequisites in place:

1. **Anaconda or Miniconda**: Install Anaconda or Miniconda, and ensure it is accessible from your command line.

2. **Python IDE**: Have a Python Integrated Development Environment (IDE) installed, such as PyCharm.

Environment Setup
~~~~~~~~~~~~~~~~~

Follow these steps to set up your Python environment and install the necessary libraries for PyBindings:

1. **Create a Python Virtual Environment**

   Open your Anaconda prompt or the terminal in your Python IDE. Navigate to the DynaPlex folder and run the following command::

.. code-block:: bash

   conda env create -f python/environment.yml

This command sets up the Python virtual environment and installs the required libraries. Note that the initial installation may take some time.

2. **Update CMake User Presets**

   Open your ``CMakeUserPresets.json`` file and ensure that the ``WinPB`` block contains the correct paths to your newly created Python environment. Additionally, set ``dynaplex_enable_pythonbindings`` to ``true``.

.. hint::

   Run ``conda info --envs`` to find the path to your Python virtual environments.

3. **Install DynaPlex as Python package**

   Navigate to the ``DynaPlex/python`` folder in your Anaconda prompt or Python IDE terminal and run the following command:

   .. code-block:: bash

      pip install -e .

4. **Build PyBindings in Your C++ IDE**

   In your C++ IDE, build the PyBindings to complete the setup.

5. **Start Using Python with DynaPlex**

   You are now ready to use Python with DynaPlex. Explore various example usages provided in the ``python/scripts`` folder. For instance, we show how you can load your MDP, run the DCL algorithm using different neural networks, or train a different RL-algorithm, for instance PPO. Note that we provide RL-algorithms via the Python library ``tianshou``, see: https://tianshou.readthedocs.io/en/stable/ for the documentation and all available algorithms. Note that if you make changes to the C++ code, you will need to rebuild the PyBindings.

Additional Notes (Optional)
~~~~~~~~~~~~~~~~~~~~~~~~~~~

If you're working on Snellius, you can load Conda via module environments using commands like:

   .. code-block:: bash

      module load 2022
      module load ...

For those that make changes to the C++ core library, the Python stub files need to be regenerated. Stub files are used for type hinting, navigate to the ``DynaPlex/python`` folder in your Anaconda prompt or Python IDE terminal and run the following commands:

   .. code-block:: bash

      pip install pybind11-stubgen
      pybind11-stubgen -o ./ dp

These commands generate Python stub files with a ".pyi" extension. These stub files provide type hint information for Python code, including third-party libraries.
