.. figure:: assets/images/logo.png
   :alt: DynaPlex logo
   :figwidth: 100%

DynaPlex is a software library for solving Markov Decision Problems and similar models (POMDP, HMM) written primarily in C++20 with bindings for python. It supports 
deep reinforcement learning, approximate dynamic programming, classical parameterized policies, and exact methods based on policy and value iteration. Models in DynaPlex are written in C++, and exposed via a generic and vectorized interface. 

DynaPlex focuses on solving problems arising in Operations Management: Supply Chain, Transportation and Logistics, Manufacturing, etc. 

Cloning the Repository with Dependencies
----------------------------------------

When cloning the repository, it's essential to also download the required submodules:

```bash
git clone --recurse-submodules https://github.com/WillemvJ/DynaPlexPrivate.git
```

if you did not recurse submodules, or if you use other tools for cloning repos, please somehow ensure that submodules (especially googletest) are available. 

.. note::

    If you are new to MDPs, you might benefit from first reading the :doc:`introduction to MDPs <getting_started/introduction_to_mdp>` and going thorugh the step-by-step tutorial, starting with the :doc:`MDP formulation <tutorial/airplane_mdp>` pages.
    If you just want to know how to install, setup, and add a model, see the docs under "Getting started"

Contents
--------

.. toctree::
   :maxdepth: 0
   :caption: Getting started

   getting_started/introduction_to_mdp
   getting_started/installation
   getting_started/conda
   getting_started/adding_model
   getting_started/testing
   getting_started/adding_executable
   getting_started/evaluate

.. toctree::
   :maxdepth: 0
   :caption: Tutorial

   tutorial/airplane_mdp
   tutorial/setup
   tutorial/adding_mdp
   tutorial/policy
   tutorial/testing_running

.. toctree::
   :maxdepth: 0
   :caption: Reference

   reference/model_ref
   reference/policy_ref
   reference/exact_ref
   reference/event
   reference/mdp
   reference/state
   reference/tester

.. toctree::
   :maxdepth: 0
   :caption: Algorithms

   algorithms/algorithms

.. toctree::
   :maxdepth: 0
   :caption: Getting help and Contributing

   community/contributing
   community/getting_help

.. toctree::
   :maxdepth: 0
   :caption: Troubleshooting

   faq/faq


.. toctree::
   :maxdepth: 0
   :caption: Legacy

   legacy/legacy
