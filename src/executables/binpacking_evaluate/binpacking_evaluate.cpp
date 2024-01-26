#include <iostream>
#include "dynaplex/dynaplexprovider.h"
#include "dynaplex/vargroup.h"
#include "dynaplex/modelling/discretedist.h"
#include "dynaplex/retrievestate.h"
#include "dynaplex/models/bin_packing/mdp.h"
using namespace DynaPlex;

int evaluateTrajectory() {
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
        Trajectory trajectory{};
        trajectory.RNGProvider.SeedEventStreams(true, rng_seed);
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
	return 0;
}

int evaluateActions() {
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
            trajVec.push_back(std::move(Trajectory( 0)));
            trajVec[0].RNGProvider.SeedEventStreams(false, 12, 0);
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
    return 0;
}

int main() {

    evaluateTrajectory();

    evaluateActions();

    return 0;
}



