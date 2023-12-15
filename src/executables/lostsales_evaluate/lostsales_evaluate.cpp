#include <iostream>
#include "dynaplex/dynaplexprovider.h"
#include "dynaplex/vargroup.h"
#include "dynaplex/modelling/discretedist.h"
#include "dynaplex/retrievestate.h"
#include "dynaplex/models/lost_sales/mdp.h"
using namespace DynaPlex;

int main() {
	try {
		auto& dp = DynaPlexProvider::Get();
		auto& system = dp.System();
        auto path_to_json = dp.FilePath({ "mdp_config_examples", "lost_sales" }, "mdp_config_0.json");
        auto config = VarGroup::LoadFromFile(path_to_json);
        auto mdp = dp.GetMDP(config);

        //Note that definition of mdp and state must be accesible for StateRetrieval to work 
        //therefore, you will need to move the mdp.h (header) file from models/models/<mdp_folder_name> to 
        //models/include/dynaplex/models/<mdp_folder_name>, 
        //furthermore, you need to change the #include in the mdp.cpp file (and other files, e.g., policies.h) to point to the new folder
        using underlying_mdp = DynaPlex::Models::lost_sales::MDP;

        std::int64_t rng_seed = 11112014;
        Trajectory trajectory{ mdp->NumEventRNGs() };
        trajectory.SeedRNGProvider(true, rng_seed);
        mdp->InitiateState({ &trajectory,1 });
        
        //get a random policy, you can alternatively load a DCL policy or a custom policy here
        auto policy = mdp->GetPolicy("random");

        bool final_reached = false;
        std::int64_t max_period_count = 100;

        int64_t InventoryTracker = 0;//just an example of a KPI
        while (trajectory.PeriodCount < max_period_count && !final_reached) {

            auto& cat = trajectory.Category;
            auto& state = DynaPlex::RetrieveState<underlying_mdp::State>(trajectory.GetState());

            if (cat.IsAwaitEvent()) {
                mdp->IncorporateEvent({ &trajectory,1 });
                //track KPIs here
                InventoryTracker += state.total_inv;
            }
            else if (cat.IsAwaitAction()) {
                policy->SetAction({ &trajectory,1 });
                //track KPIs here:
                mdp->IncorporateAction({ &trajectory,1 });

            }
            else if (cat.IsFinal()) {
                final_reached = true;
                //or here
            }
        }

        //just an example of storing the variable after evaluation
        VarGroup kpis{};
        kpis.Add("InventoryTracker", InventoryTracker);

        auto filename = dp.System().filepath("lost_sales", "Evaluation", "somefile.json");
        kpis.SaveToFile(filename);

	}
	catch (const DynaPlex::Error& e)
	{
		std::cout << e.what() << std::endl;
	}
	return 0;
}
