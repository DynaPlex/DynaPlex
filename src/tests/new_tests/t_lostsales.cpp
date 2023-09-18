#include "dynaplex/vargroup.h"
#include "dynaplex/error.h"
#include <gtest/gtest.h>
#include "dynaplex/dynaplexprovider.h"
#include "dynaplex/trajectory.h"

namespace DynaPlex::Tests {
	

	TEST(LostSales, Basics) {
		auto& dp = DynaPlexProvider::Get();
		DynaPlex::VarGroup vars;
		vars.Add("id", "lost_sales");
		vars.Add("p", 4.0);
		vars.Add("h", 1.0);
		vars.Add("leadtime", 3);


		vars.Add("demand_dist", DynaPlex::VarGroup({
			{"type", "poisson"},
			{"mean", 4.0}
		}));



		DynaPlex::MDP mdp;
		DynaPlex::Policy policy;

	
		ASSERT_NO_THROW(
			mdp = dp.GetMDP(vars);
		    policy = mdp->GetPolicy("basestock");
		);
			

		const std::string prefix = "lost_sales";
		EXPECT_EQ(prefix, mdp->Identifier().substr(0, prefix.length())) ;



		std::vector<DynaPlex::Trajectory> Trajectories;
		int number = 5;
		Trajectories.reserve(number);
		for (size_t i = 0; i < number; i++)
		{
			Trajectories.push_back(DynaPlex::Trajectory(mdp->NumEventRNGs(), i));
			Trajectories.back().Reset(i, 0, false);
		}
		
		ASSERT_THROW(Trajectories[0].GetState()->ToString(),DynaPlex::Error);

		
		mdp->InitiateState(Trajectories);
		//std::cout << "initiate" << std::endl;
		for (auto& traj: Trajectories)
		{
		//	std::cout << traj.GetState()->ToString() << std::endl;
		}

		for (int i = 0; i < 10; i++)
		{			
			bool anotherEvent = false;
			do
			{
				anotherEvent = mdp->IncorporateEvent(Trajectories);
				//if (anotherEvent)
				{
				//	std::cout << "event" << std::endl;
					for (auto& traj : Trajectories)
					{
				//		std::cout << traj.GetState()->ToString() << std::endl;
					}
				}
			} while (anotherEvent);			
			mdp->IncorporateAction(Trajectories, policy);
			std::cout << "action" << std::endl;
			for (auto& traj : Trajectories)
			{
				std::cout << traj.GetState()->ToString() << std::endl;
			}
		}

		mdp->IncorporateEvent(Trajectories);
		
		std::cout << "copy" << std::endl;

		mdp->InitiateState({ &Trajectories[2],1 }, Trajectories.back().GetState());
		for (auto& traj : Trajectories)
		{
			std::cout << traj.GetState()->ToString() << std::endl;
		}		
	}
}