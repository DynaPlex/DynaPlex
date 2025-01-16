Evaluating models
=================

As soons as you implemented your MDP, you might want to evaluate the working of policies, e.g., the DCL policy or your own implemented benchmark policy. The PolicyComparer can be used to compare policies in terms of costs. 

Possibly, you want to evaluate the policies in more detail. Below we probide two examples. First, we show how you can simulate a horizon and store statistics during this horizon. Next, we show how you can evaluate the actions taken by the policy given a customized input state. Both can be used to report statistics or construct graphs.

Demonstrator
------------

We can use the ``Demonstrator`` to go over a trajectory of states with a given policy and print out the complete trace of states, below is an example which you can add to your executable.

.. code-block:: cpp

	try {
		auto& dp = DynaPlexProvider::Get();
		//update to name of your MDP.
		auto name = "lost_sales";
		//get the path to the configuration file for the MDP.
		auto path_to_json = dp.FilePath({ "mdp_config_examples", name },
				"mdp_config_0.json");
		//load the configuration file for the MDP.
		auto config = VarGroup::LoadFromFile(path_to_json);

		//create an MDP from the configuration file.
		DynaPlex::MDP mdp = dp.GetMDP(config);
		//get some policy, either the random built-in policy or a policy defined by you
		//for this MDP
		auto policy = mdp->GetPolicy("random");

		//get a demonstrator:
		auto demonstrator = dp.GetDemonstrator({{ "max_period_count", 10 }});

		auto trace = demonstrator.GetTrace(mdp, policy);
		//iterator over the trace, printing each element:
		for (auto& t : trace) {
			std::cout << t.Dump(3) << std::endl;
			}
                }
        catch (const std::exception& e)
        {
		std::cout << "exception: " << e.what() << std::endl;
        }
        return 0;


Simulate Trajectory
-------------------

To simulate a trajectory and store (performance) statistics calculated during/after, you can create a new executable. Below we provide an example implementation for the ``bin_packing`` mdp.
Please note that the definition of the state and mdp must be accesible for ``StateRetrieval``. Therefore, you will need to move the ``mdp.h`` (header) file from ``models/models/<mdp_folder_name>`` to ``models/include/dynaplex/models/<mdp_folder_name>``, 
After that change, you need to change the ``#include`` in the ``mdp.cpp`` file (and other files, e.g., ``policies.h``) to point to the new folder. We refer to the ``binpacking_evaluate`` folder in the ``executables`` folder for an example.

.. code-block:: cpp

	try {
        auto& dp = DynaPlexProvider::Get();
        auto& system = dp.System();
        std::string model_name = "bin_packing";
        std::string mdp_config_name = "mdp_config_0.json";

        std::string file_path = system.filepath("mdp_config_examples", model_name, mdp_config_name);
        auto mdp_vars_from_json = VarGroup::LoadFromFile(file_path);
        auto mdp = dp.GetMDP(mdp_vars_from_json);

        //Note that definition of mdp and state must be accesible for StateRetrieval to work 
        //therefore, you will need to move the mdp.h (header) file from models/models/<mdp_folder_name> to 
        //models/include/dynaplex/models/<mdp_folder_name>, 
        //furthermore, you need to change the #include in the mdp.cpp file (and other files, e.g., policies.h) to point to the new folder
        using underlying_mdp = DynaPlex::Models::bin_packing::MDP;

        std::int64_t rng_seed = 11112014;
        Trajectory trajectory{ mdp->NumEventRNGs() };
        trajectory.SeedRNGProvider(true, rng_seed);
        mdp->InitiateState({ &trajectory,1 });

        bool final_reached = false;
        std::int64_t max_period_count = 100;
        
        //get a random policy, you can alternatively load a DCL policy or a custom policy here
        auto policy = mdp->GetPolicy("random");


        int64_t weight_after_adding = 0;//just an example of a KPI
        while (trajectory.PeriodCount < max_period_count && !final_reached) {

            auto& cat = trajectory.Category;
            auto& state = DynaPlex::RetrieveState<underlying_mdp::State>(trajectory.GetState());

            if (cat.IsAwaitEvent()) {
                mdp->IncorporateEvent({ &trajectory,1 });
                //track KPIs here
            }
            else if (cat.IsAwaitAction()) {
                policy->SetAction({ &trajectory,1 });
                //track KPIs here
                weight_after_adding += state.weight_vector[trajectory.NextAction] + state.upcoming_weight;
                mdp->IncorporateAction({ &trajectory,1 });

            }
            else if (cat.IsFinal()) {
                final_reached = true;
                //or here
            }
        }

        //just an example of storing the variable after evaluation
        VarGroup kpis{};
        kpis.Add("weight_after_adding", weight_after_adding);

        auto filename = dp.System().filepath("bin_packing", "Evaluation", "somefile.json");
        kpis.SaveToFile(filename);

	}
	catch (const DynaPlex::Error& e)
	{
		std::cout << e.what() << std::endl;
	}

Evaluate Actions
----------------

You might want to evaluate the actions taken by a policy given that the state changes. This way, you can evaluate the behavior of a policy given (slight) change sin the state. Below, we provide an example how this can be done, using the ``bin_packing`` mdp.

.. code-block:: cpp

	    try {
        auto& dp = DynaPlexProvider::Get();
        auto& system = dp.System();
        std::string model_name = "bin_packing";
        std::string mdp_config_name = "mdp_config_0.json";

        std::string file_path = system.filepath("mdp_config_examples", model_name, mdp_config_name);
        auto mdp_vars_from_json = VarGroup::LoadFromFile(file_path);
        auto mdp = dp.GetMDP(mdp_vars_from_json);

        //get a random policy, you can alternatively load a DCL policy or a custom policy here
        //with the random policy we will not be asbtract something meaningful, but this is just as an example
        auto policy = mdp->GetPolicy("random");

        //get some initializing variables from json
        int64_t number_of_bins;
        int64_t max_bin_size;
        mdp_vars_from_json.Get("number_of_bins", number_of_bins);
        mdp_vars_from_json.Get("max_bin_size", max_bin_size);
        auto weight_vector = std::vector<int64_t>(number_of_bins, max_bin_size-1);

        std::vector<int64_t> actions;
        std::vector<int64_t> upcoming_weights;
        //we iteratively change a state variable to observe the action from the given policy, given the state
        for (int upcoming_weight = 0; upcoming_weight < 9; upcoming_weight++)
        {
            DynaPlex::VarGroup stateVars{
                {"cat",StateCategory::AwaitAction().ToVarGroup()},
                {"weight_vector", weight_vector},
                {"upcoming_weight", upcoming_weight}
            };
            auto state = mdp->GetState(stateVars);

            std::vector<Trajectory> trajVec{};
            trajVec.push_back(std::move(Trajectory(mdp->NumEventRNGs(), 0)));
            trajVec[0].SeedRNGProvider(false, 12, 0);
            mdp->InitiateState({ &trajVec[0] ,1 }, state);
            policy->SetAction(trajVec);

            actions.push_back(trajVec[0].NextAction);
            upcoming_weights.push_back(upcoming_weight);
        };
        
        //just an example of storing the variables after evaluation
        VarGroup kpis{};
        kpis.Add("actions", actions);
        kpis.Add("upcoming_weights", upcoming_weights);

        auto filename = dp.System().filepath("bin_packing", "Evaluation", "somefile2.json");
        kpis.SaveToFile(filename);

    }
    catch (const DynaPlex::Error& e)
    {
        std::cout << e.what() << std::endl;
    }
