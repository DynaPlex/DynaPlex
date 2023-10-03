#include "dynaplex/vargroup.h"
#include "dynaplex/error.h"
#include <gtest/gtest.h>
#include "dynaplex/dynaplexprovider.h"
#include "dynaplex/trajectory.h"
#include "dynaplex/policycomparer.h"
#include "dynaplex/modelling/discretedist.h"
#include "dynaplex/dynaplex_model_includes.h"

#include "dynaplex/erasure/makegeneric.h"

namespace DynaPlex::Tests {
	namespace AddOn::ProblemWithNonStandardDurations {
		class MDP
		{
		public:
			double discount_factor;
			bool finite_horizon;
			bool reported_finite_horizon;

			struct State {
				int64_t i;
				DynaPlex::StateCategory cat;
				VarGroup ToVarGroup() const
				{
					VarGroup vars{};
					vars.Add("i", i);
					vars.Add("cat", cat);
					return vars;
				}
				bool operator==(const State& other) const = default;

			};
			using Event = int64_t;
		private:
			DiscreteDist dist;
		public:
			bool IsAllowedAction(const State& state, int64_t action) const
			{
				return true;
			}
			double ModifyStateWithAction(State& s, int64_t action) const
			{
				if (action == 0||action==1)
				{
					s.cat = StateCategory::AwaitEvent();
				}else
				if (action == 2 || action == 3)
				{
					s.cat = StateCategory::AwaitEvent(1);
				}else
				if(action==4)
				{
					s.cat = StateCategory::AwaitAction();

					if (finite_horizon)
					{
						if (++s.i == 3)
						{
							s.cat = StateCategory::Final();
						}
					}
					
				}
				else
					throw DynaPlex::Error("error in logic");
				return 1.0;
				
			}
			double ModifyStateWithEvent(State& s, const Event& e) const
			{
				if (e == 0)
				{
				    //stay same
				}
				if (e == 1)
				{
					s.cat = StateCategory::AwaitEvent(0);
				}
				if (e == 2)
				{
					s.cat = StateCategory::AwaitEvent(1);
				}
				if (e == 3)
				{
					s.cat = StateCategory::AwaitAction();
				}
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
				return state.cat;
			}

			State GetInitialState() const
			{
				return State{0,StateCategory::AwaitAction()};
			}

			DynaPlex::VarGroup GetStaticInfo() const
			{
				DynaPlex::VarGroup vars;
				vars.Add("valid_actions", 5);
				vars.Add("discount_factor", discount_factor);
				if (reported_finite_horizon)
				{
					vars.Add("horizon_type", "finite");
				}
				return vars;
			}
			explicit MDP(const DynaPlex::VarGroup& vars)
			{
				dist = DiscreteDist::GetCustomDist({ 0.25,0.25,0.25,0.25 });
				vars.Get("discount_factor", discount_factor);
				vars.Get("finite_horizon", finite_horizon);
				vars.Get("reported_finite_horizon", reported_finite_horizon);

			}
		};
	}

	TEST(PolicyComparer, Infinite_Horizon)
	{

		VarGroup vars{ {"number_of_trajectories",512} };
		auto mdp = DynaPlex::Erasure::MakeGenericMDP<AddOn::ProblemWithNonStandardDurations::MDP>(
			VarGroup{ {"id","customclass"},{"discount_factor",1.0},{"finite_horizon",false},{"reported_finite_horizon",false} }
		);
		auto bad_mdp = DynaPlex::Erasure::MakeGenericMDP<AddOn::ProblemWithNonStandardDurations::MDP>(
			VarGroup{ {"id","customclass"},{"discount_factor",1.0},{"finite_horizon",false},{"reported_finite_horizon",true} }
		);
		auto& dp = DynaPlexProvider::Get();
		auto PolicyComparer = dp.GetPolicyComparer(mdp,vars);
		auto policy = mdp->GetPolicy("random");
		auto Assessment = PolicyComparer.Assess(policy);

		VarGroup vars2{ {"number_of_trajectories",512},{"max_periods_until_error",128} };
		
		ASSERT_THROW(
			{
				auto PolicyComparer = dp.GetPolicyComparer(bad_mdp,vars2);
				auto policy2 = bad_mdp->GetPolicy("random");
				auto Assessment = PolicyComparer.Assess(policy2);
			},
			DynaPlex::Error
			);
		double mean, error;
		Assessment.Get("mean", mean);
		Assessment.Get("error", error);
		ASSERT_NEAR(mean, 2.0 / 3.2, 5 * error);

	}

	TEST(PolicyComparer, finite_horizon)
	{
		VarGroup vars{ {"number_of_trajectories",1048} };

		auto mdp = DynaPlex::Erasure::MakeGenericMDP<AddOn::ProblemWithNonStandardDurations::MDP>(
			VarGroup{ {"id","customclass"},{"discount_factor",1.0},{"finite_horizon",true},{"reported_finite_horizon",true} }
		);
		auto bad_mdp = DynaPlex::Erasure::MakeGenericMDP<AddOn::ProblemWithNonStandardDurations::MDP>(
			VarGroup{ {"id","customclass"},{"discount_factor",1.0},{"finite_horizon",true},{"reported_finite_horizon",false} }
		);
		auto& dp = DynaPlexProvider::Get();
		auto PolicyComparer = dp.GetPolicyComparer(mdp,vars);
		auto policy = mdp->GetPolicy("random");
		auto Assessment = PolicyComparer.Assess(policy);
		double mean, error;

		ASSERT_THROW(
			{
				auto PolicyComparer = dp.GetPolicyComparer(bad_mdp,vars);
				auto policy2 = bad_mdp->GetPolicy("random");
				auto Assessment = PolicyComparer.Assess(policy2);
			},
			DynaPlex::Error
		);
		Assessment.Get("mean", mean);
		Assessment.Get("error", error);
		ASSERT_NEAR(mean, 3*5.0, 5 * error);

	}

	TEST(PolicyComparer, WithLostSales) {
		auto& dp = DynaPlexProvider::Get();
		auto& system = dp.System();

		std::string model_name = "lost_sales";
		std::string mdp_config_name = "mdp_config_0.json";
		//configure MDP:
		ASSERT_TRUE(
			system.file_exists("mdp_config_examples", model_name, mdp_config_name)
		);
		DynaPlex::VarGroup mdp_vars_from_json;
		ASSERT_NO_THROW(
			std::string file_path = system.filename("mdp_config_examples", model_name, mdp_config_name);
		    mdp_vars_from_json = VarGroup::LoadFromFile(file_path);
		);

		auto mdp = dp.GetMDP(mdp_vars_from_json);
		
		DynaPlex::Policy policy = mdp->GetPolicy("random");

		VarGroup vars{ {"number_of_trajectories",1048} };

		auto evaluator = dp.GetPolicyComparer(mdp,vars);
		auto assessment = evaluator.Assess(policy);
		std::cout << assessment.Dump() << std::endl;
	}
}