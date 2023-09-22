#include "dynaplex/vargroup.h"
#include "dynaplex/error.h"
#include <gtest/gtest.h>
#include "dynaplex/dynaplexprovider.h"

namespace DynaPlex::Tests {
	

	TEST(ModelFactory, SimpleGet) {
		auto& dp  = DynaPlexProvider::Get();

		ASSERT_NO_THROW(
			dp.System()
		);

		DynaPlex::VarGroup config;
		//retrieve MDP registered under the id string "lost_sales":
		config.Add("id", "lost_sales");
		//add other parameters required by that MDP:
		config.Add("p", 9.0);
		config.Add("h", 1.0);
		config.Add("leadtime", 3);
		config.Add("demand_dist", DynaPlex::VarGroup({
			{"type", "poisson"},
			{"mean", 4.0}
			}));

		DynaPlex::MDP model;


		//id corresponds to mdp provided in class src/lib/models/lost_sales/mdp.cpp, 
		//see the function register_mdp defined over there:
		ASSERT_NO_THROW(
			model = dp.GetMDP(config);
		);
		//Note that model is of type DynaPlex::MDP, even though the underlying type is lost_sales. 
		//This means that it can be passed to any function that requires DynaPlex::MDP. 

		//We check this as follows:
		EXPECT_EQ("lost_sales", model->TypeIdentifier()) ;
		//Note the -> DynaPlex::MDP is of pointer type, so its member functions are all retrieved using ->.
		auto State = model->GetInitialState();

		DynaPlex::VarGroup policyvars{ {"id","base_stock"} };
		//from the MDP, we may retrieve a policy _for that mdp_
		DynaPlex::Policy policy{};
		ASSERT_NO_THROW(
			policy = model->GetPolicy(policyvars);
		);

		EXPECT_EQ(policy->TypeIdentifier(), "base_stock");

	}



	TEST(ModelFactory, FailGet) {
		auto& dp = DynaPlexProvider::Get();

		DynaPlex::VarGroup vars;


		//note the typo:
		vars.Add("id", "LostTypoSales");
		vars.Add("p", 9.0);
		vars.Add("h", 1.0);

		DynaPlex::MDP model;
		//this fails as there is no matching id:
		ASSERT_THROW(
			model = dp.GetMDP(vars), DynaPlex::Error
		);

	}
}