Testing and running the MDP
===========================

We can test and run the new MDP. Unit testing is an important component of implementing your code.

.. hint::
	First check if your current code can build.

Testing
-------

Using tests to check whether code is following intended logic is a good way to verify that an MDP implements all required logic and to quickly identify oversights.

	Go to ``src/tests/mdp_unit_tests/`` and make a new file called ``t_airplane.cpp``.


We will add a simple test that loads JSON from ``IORootDir/IO_DynaPlex/defaults/airplane/mdp_config_0`` - Attempts to configure MDP from that JSON file - Sets the ``rule_base`` policy - Performs a range of tests using the MDP and the Policy, i.e., checks whether a number of simulation steps can be run and whether all things work appropriately.

.. code-block:: cpp

    ﻿#include <gtest/gtest.h>
    #include "testutils.h" // for ExecuteTest
    namespace DynaPlex::Tests {

        TEST(airplane, mdp_config_0) {
            std::string model_name = "airplane"; // this should match the id and namespace name discussed earlier
            std::string config_name = "mdp_config_0.json";
            std::string policy_config_name = "policy_config_0.json";
            Tester tester{};
            tester.ExecuteTest(model_name, config_name, policy_config_name);
        }
    }

.. note::
    During compilation, the JSON files in models/models get copied to a subdirectory in ``DYNAPLEX_IO_ROOT_DIR/IO_DynaPlex/...``.

    Now, run ``DP_mdp_unit_tests.exe``. All unit test will be executed and the output will show potential bugs.

.. hint:: 
    For more extensive unit testing, see the other files in ``src/tests/mdp_unit_tests/`` for examples.

Adding executable
-----------------

If all unit testing has passed, we can add the executable. We will add an executable that trains the DCL algorithm.

Setup
~~~~~

Go to ``src/executables/executable_example`` and copy and paste the directory with its contents.
Change the directory name to ``airplane_mdp``.
In ``src/executables/CMakeLists.txt`` change the following:

.. code-block:: cmake

	add_subdirectory(executable_example) # Existing
	add_subdirectory(airplane_mdp) # Added


Next go to ``src/executables/airplane_mdp/CMakeLists.txt`` and change:


.. code-block:: cmake

	#set(targetname executable_example) # Remove
	set(targetname airplane_mdp) # Add

Rerunning cmake should enable you to select the target ``dp_airplane_mdp``. Note that this target is excluded from all, meaning that you have to specifically select it to compile and run.

Define a run script
~~~~~~~~~~~~~~~~~~~

In ``src/executables/airplane_mdp/airplane_mdp.cpp`` add the following code that runs DCL:

.. code-block:: cpp

	﻿#include <iostream>
	#include "dynaplex/dynaplexprovider.h"
	using namespace DynaPlex;

	int main() {

		//initialize
		auto& dp = DynaPlexProvider::Get();
		std::string model_name = "airplane";
		std::string mdp_config_name = "mdp_config_0.json";
		//Get path to IO_DynaPlex/mdp_config_examples/airplane_mdp/mdp_config_name:
		std::string file_path = dp.System().filepath("mdp_config_examples", model_name, mdp_config_name);
		auto mdp_vars_from_json = DynaPlex::VarGroup::LoadFromFile(file_path);
		auto mdp = dp.GetMDP(mdp_vars_from_json);


		//for illustration purposes, create a different mdp 
		//that is compatible with the first - same number of features, same number of valid actions:
		DynaPlex::MDP different_mdp = dp.GetMDP(mdp_vars_from_json);

		//we can also input the rule_based policy here, if you defined it before.
		auto policy = mdp->GetPolicy("random");

		//set several DCL parameters
		DynaPlex::VarGroup nn_training{
			{"early_stopping_patience",10}
		};

		DynaPlex::VarGroup nn_architecture{
			{"type","mlp"},
			{"hidden_layers",DynaPlex::VarGroup::Int64Vec{64,64}}
		};
		int64_t num_gens = 2;
		DynaPlex::VarGroup dcl_config{
			//just for illustration, so we collect only little data, so DCL will run fast but will not perform well. 
			{"N",100},
			{"num_gens",num_gens},
			{"M",1000},
			{"nn_architecture",nn_architecture},
			{"nn_training",nn_training},
			{"retrain_lastgen_only",false}
		};

		try
		{
			//Create a trainer for the mdp, with appropriate configuratoin. 
			auto dcl = dp.GetDCL(mdp, policy, dcl_config);
			//this trains the policy, and saves it to disk.
			dcl.TrainPolicy();
			//using a dcl instance that has same parameterization (i.e. same dcl_config, same mdp), we may recover the trained polciies.
			//This gets the policy that was trained last:
			//auto policy = dcl.GetPolicy();
			//This gets policy with specific index:
			//auto first = dcl.GetPolicy(1);

			return 0;
			//This gets all trained policy, as well as the initial policy, in a vector:
			auto policies = dcl.GetPolicies();

			//Compare the various trained policies:
			auto comparer = dp.GetPolicyComparer(mdp);
			auto comparison = comparer.Compare(policies);
			for (auto& VarGroup : comparison)
			{
				std::cout << VarGroup.Dump() << std::endl;
			}

			//policies are automatically saved when training, but it may be usefull to save at custom location:
			auto last_policy = dcl.GetPolicy();
			//gets a file_path without file extension (file extensions are automatically added when saving): 
			auto path = dp.System().filepath("dcl", "airplane_mdp", "airplane_mdp_policy_gen" + num_gens);
			//this is IOLocation/dcl/dcl_example/lost_sales_policy
			//IOLocation is typically specified in CMakeUserPresets.txt

			//saves two files, one .json file with the architecture (e.g. trained_lost_sales_policy.json), and another file with neural network weights (.pth):		
			dp.SavePolicy(last_policy, path);

			//This loads the policy again from the same path, automatically adding the right extensions:
			auto policy = dp.LoadPolicy(mdp, path);

			//Even possible to load the policy trained for one MDP, and make it applicable to another mdp:
			//this however only works if the two policies have consistent input and output dimensions, i.e.
			//same number of valid actions and same number of features. 
			auto different_policy = dp.LoadPolicy(different_mdp, path);
		}
		catch (const std::exception& e)
		{
			std::cout << "exception: " << e.what() << std::endl;
		}
		return 0;
	}
