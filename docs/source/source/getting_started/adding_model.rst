Adding your MDP model
=====================

DynaPlex facilitates the extension with new native models (e.g., Markov Decision Processes (MDPs) and policies). This document explains how to add an additional model, which will then be generally usable and retrievable (both in Python and for the generic C++ functionality).

.. seealso::
    If you instead want to have a step-by-step guide for adding a model, we refer to the Tutorial pages, starting with :ref:`label_airplane`.

Understanding what needs to be provided
----------------------------------------

To begin, review the implementations under ``src/lib/models/models``, especially the one in ``src/lib/models/models/lost_sales``.

The ``lost_sales`` model offers both a native MDP (in ``../lost_sales/mdp.h`` and ``../lost_sales/mdp.cpp``) and a native policy (in ``../lost_sales/policies.h`` and ``../lost_sales/policies.cpp``). It's possible to define an MDP without native policies by excluding the ``policies.*`` files. Standard policies (like "random") and learned policies will be available for every MDP, so it is not strictly needed to define policies, and when defining your first MDP, you could do well without it. Additionally, some JSON files are provided, acting as default configuration files for unit testing. They also help in understanding the configuration parameters that an MDP accepts, as a form of documentation.

Adding a new model
------------------

Adding a model is easiest in debug mode, such as in WinDeb or LinDeb.

Start by copying the code of an existing MDP and registering it under a new name.

.. hint::
    We advise you to start with the ``empty_example`` as your starting MDP

Copy an MDP and register it under a new name:
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

1. **Duplicate Directory**: Copy the content of a directory associated with an existing MDP to a new directory. Name this new directory to match your new MDP's desired name (e.g., "mdp_id" in this example).
   - Choose an MDP with similar functionality if possible. Another option is ``../models/models/empty_example/``, which is an MDP that throws NotImplementedExceptions in most functions, meaning that those can be reimplemented later with the desired functionality.
2. **Cleanup**: From the new directory, delete ``policy.h``, ``policy.cpp``, and all ``.json`` config files except for ``mdp_config_0.json`` (skip step if using ``/empty_example/``).
3. **Remove Policy References**: Delete any references to policies in these files. (skip step if using ``/empty_example/``).
4. **Namespace Update**: Modify namespaces in ``mdp.h`` and ``mdp.cpp`` in the newly created directory. For example, change from ``lost_sales`` or ``empty_example`` to the new name (``mdp_id``).
   - Refer to the below ``Namespaces`` section for more details.
5. **Register Function**: Adjust the ``MDP::Register`` function in ``../models/models/mdp_id/mdp.cpp`` to register the new MDP under an appropriate "mdp_id" (matching the directory name) and include an appropriate description for your MDP.
6. **Registration Manager**: In ``/models/models/registrationmanager.cpp``, forward-declare the Register function for the new model in the appropriate namespace. Call this function within the ``RegisterAll`` function.
   - Refer to the below ``Namespaces`` section for more details.

Following these steps allows for creating a duplicate of an MDP registered under a new name. Ensure that the code compiles and runs without issues. If problems arise, refer to the :ref:`label_troubleshooting` section.

Change code in template
~~~~~~~~~~~~~~~~~~~~~~~

Iteratively combine the following steps to adjust the MDP to your desired logic:

1. **Modify Member Variables**: Adjust member variables in both the `mdp` and nested `state` struct located in `mdp.h`: Make sure to update the functions providing serialization and initiation within the class body.

2. **Initial State Logic**: Provide logic for creating an initial state.

3. **State Transition Logic**: Update the logic in functions that determine state transitions, i.e., ModifyStateWith...

4. **Custom Event Logic**: Default events are `int64_t`, but often this is not appropriate.

5. **Modify the Test Configuration**: While adapting the logic, make sure to keep the `mdp_config_0` up to date to ensure that the MDP can be tested.

Running ``dp_mdp_unit_tests`` is a must to avoid oversights.

To verify that your code logic is in order, you might want to visualize a sequence of states when following some policy. For an example, see the test ``TEST(bin_packing, trace)`` in ``src/tests/mdp_unit_tests/t_bin_packing.cpp``.

Details on Namespaces, Directory Names, IDs, Registration
---------------------------------------------------------

When adding a new MDP, consistency in naming directories, files, classes, and namespaces is essential. In the examples below, the placeholder ``mdp_id`` represents the name identifier for the MDP. Ensure you replace each mention of ``mdp_id`` with the appropriate name for the new MDP you're adding.

Naming the MDP Class
~~~~~~~~~~~~~~~~~~~~

It's recommended to name the MDP class simply as ``MDP``. To differentiate it from other MDPs, it should be enclosed within a specific namespace:

.. code-block:: cpp

    // File: ../models/models/mdp_id/mdp.h
    #pragma once

    namespace DynaPlex::Models {
        // Convention: MDP with id "mdp_id" is defined in namespace DynaPlex::Models::mdp_id 
        namespace mdp_id {
            class MDP {
                // Class definition goes here.    
            };
        }
    }



Function Definitions
~~~~~~~~~~~~~~~~~~~~

Ensure that corresponding function definitions align with the above declaration by defining them within the same namespace:

.. code-block:: cpp

    // File: ../models/models/mdp_id/mdp.cpp
    #include "mdp.h"
    #include "dynaplex/erasure/mdpregistrar.h"
    #include "policies.h"

    namespace DynaPlex::Models::mdp_id {
        // function definitions 
    }

Incorporating Custom Policies
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

If you decide to introduce custom policies in the future, make sure they are declared and defined within the consistent namespace, i.e., ``DynaPlex::Models::mdp_id``.

Preferred filenames for policy files are ``../models/models/mdp_id/policies.h`` and ``../models/models/mdp_id/policies.cpp``. For a working example, refer to the ``lost_sales`` model.