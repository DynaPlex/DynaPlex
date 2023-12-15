#include "dynaplex/vargroup.h"
#include "dynaplex/error.h"
#include <gtest/gtest.h>
#include "dynaplex/dynaplexprovider.h"
#include "dynaplex/retrievestate.h"
#include "dynaplex/models/bin_packing/mdp.h"

namespace DynaPlex::Tests {
	

	TEST(StateRetrieval, bin_packing) {
		auto& dp = DynaPlexProvider::Get();
		auto& system = dp.System();
		std::string model_name = "bin_packing";
		std::string mdp_config_name = "mdp_config_0.json";
	

		std::string file_path = system.filepath("mdp_config_examples", model_name, mdp_config_name);
		auto mdp_vars_from_json = VarGroup::LoadFromFile(file_path);
		auto mdp = dp.GetMDP(mdp_vars_from_json);

        //Note that definition of mdp and state must be accesible for StateRetrieval to work 
        //Which is why the header of bin_packing mdp was moved from models/models/bin_packing
        //to models/include/dynaplex/models/bin_packing, such that it can be included elsewhere. 
        using underlying_mdp = DynaPlex::Models::bin_packing::MDP;

        std::int64_t rng_seed = 11112014;
        Trajectory trajectory{ mdp->NumEventRNGs() };
        trajectory.SeedRNGProvider(true, rng_seed);
        mdp->InitiateState({ &trajectory,1 });
        auto policy = mdp->GetPolicy("random");

        bool final_reached = false;
        std::int64_t max_period_count = 100;



        while (trajectory.PeriodCount < max_period_count && !final_reached) {

            auto& cat = trajectory.Category;
            auto& state = DynaPlex::RetrieveState<underlying_mdp::State>(trajectory.GetState());
         
            if (cat.IsAwaitEvent()) {
                mdp->IncorporateEvent({ &trajectory,1 });
                //or here
            }
            else if (cat.IsAwaitAction()) {
                policy->SetAction({ &trajectory,1 });
                //use the state to do some actual computations:
                auto weight_after_adding = state.weight_vector[trajectory.NextAction] + state.upcoming_weight;
                mdp->IncorporateAction({ &trajectory,1 });
                
            }
            else if (cat.IsFinal()) {
                final_reached = true;
                //you could also add code here
            }
        }
	}


    TEST(StateRetrieval, wrong_mdp) {
        auto& dp = DynaPlexProvider::Get();
        auto& system = dp.System();
        std::string model_name = "lost_sales";
        std::string mdp_config_name = "mdp_config_0.json";


        std::string file_path = system.filepath("mdp_config_examples", model_name, mdp_config_name);
        auto mdp_vars_from_json = VarGroup::LoadFromFile(file_path);
        auto mdp = dp.GetMDP(mdp_vars_from_json);

        using underlying_mdp = DynaPlex::Models::bin_packing::MDP;

        std::int64_t rng_seed = 11112014;
        Trajectory trajectory{ mdp->NumEventRNGs() };
        trajectory.SeedRNGProvider(true, rng_seed);
        mdp->InitiateState({ &trajectory,1 });
        auto policy = mdp->GetPolicy("random");

        bool final_reached = false;
        std::int64_t max_period_count = 100;


        //Here, we try to get a state of type order_picking::State, even though the state in trajectory is of type lost_sales::State. 
        //This could never fly, obviously, and an error is thrown, as asserted:
        ASSERT_THROW({
            auto & state = DynaPlex::RetrieveState<underlying_mdp::State>(trajectory.GetState());
            }, DynaPlex::Error
        );
    }


}