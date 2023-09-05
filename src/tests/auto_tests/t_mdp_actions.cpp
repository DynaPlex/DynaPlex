#include <iostream>
#include "dynaplex/vargroup.h"
#include "dynaplex/makegeneric.h"
#include "dynaplex/error.h"
#include <gtest/gtest.h>
#include "dynaplex/rng.h"
#include "dynaplex/statecategory.h"
#include "dynaplex/features.h"
#include "dynaplex/modelling/discretedist.h"
#include "dynaplex/mdp.h"
#include  "dynaplex/state.h"

namespace DynaPlex::Tests {

	namespace AddOn::TestProblem {
		class MDP
		{
		public:
			struct State {
				int64_t i;
			};
			using Event = int64_t;
		private:
			DiscreteDist dist;
			int64_t initial_i;
		public:
			bool IsAllowedAction(const State& state, int64_t action) const
			{
				if (state.i == 0)
				{//this is event-state - may have strange behavior. 
					return false;
				}				
				return (state.i+action)%2==0;
			}
			double ModifyStateWithAction(State& s, int64_t action) const
			{
				return 0.0;
			}
			double ModifyStateWithEvent(State&, const Event&) const
			{
				return 0.0;
			}
			Event GetEvent(DynaPlex::RNG& rng) const
			{
				return dist.GetSample(rng);
			}
			void GetFeatures(State&, DynaPlex::Features&)
			{
			}


			DynaPlex::StateCategory GetStateCategory(const State& state) const
			{
				if (state.i == 0)
				{
					return  DynaPlex::StateCategory::AwaitEvent();
				}
				else
				{
					return  DynaPlex::StateCategory::AwaitAction();
				}
			}

			State GetInitialState() const
			{
				return State{ initial_i };
			}
								
			DynaPlex::VarGroup GetStaticInfo() const
			{
				DynaPlex::VarGroup vars;
				vars.Add("valid_actions", 5);
				return vars;
			}
			explicit MDP(const DynaPlex::VarGroup& vars) 
			{
				vars.Get("dist", dist);
				vars.Get("initial_i", initial_i);
			}
		};
	}
	TEST(RegisterAdditionalMDP, UseUnregisteredMDP) {
		for (int64_t init = 0; init < 3; init++)
		{
			DynaPlex::VarGroup vars;
			vars.Add("id", "Problem");
			vars.Add("dist", DynaPlex::VarGroup({ {"type","poisson"}, {"mean",3.0} }));
			vars.Add("initial_i", init);

			auto mdp = DynaPlex::Erasure::MakeGenericMDP<AddOn::TestProblem::MDP>(vars);
			DynaPlex::dp_State state{ mdp->GetInitialState() };

			if (init == 0)
			{
				EXPECT_THROW({ mdp->AllowedActions(state); }, DynaPlex::Error);
			}
			else
			{
				std::vector<int64_t> actions = mdp->AllowedActions(state);
				std::vector<int64_t> expected = { 0,2,4 };
				if (init == 1)
				{
					expected = { 1,3 };
				}
				EXPECT_EQ(actions, expected);
			}

			const std::string prefix = "Problem";
			EXPECT_EQ(prefix, mdp->Identifier().substr(0, prefix.length()));
		}
	}
}