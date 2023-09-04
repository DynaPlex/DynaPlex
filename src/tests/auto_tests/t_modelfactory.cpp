#include "dynaplex/vargroup.h"
#include "dynaplex/error.h"
#include <gtest/gtest.h>
#include "dynaplex/factories.h"
#include "dynaplex/registry.h"


namespace DynaPlex::Tests {
	

	TEST(ModelFactory, SimpleGet) {
		DynaPlex::VarGroup vars;



		vars.Add("id", "lost_sales");
		vars.Add("p", 9.0);
		vars.Add("h", 1.0);
		vars.Add("leadtime", 3);
		vars.Add("demand_dist", DynaPlex::VarGroup({
			{"type", "poisson"},
			{"mean", 4.0}
			}));

		DynaPlex::MDP model;


		ASSERT_NO_THROW(
			model = DynaPlex::GetMDP(vars);
		);

		const std::string prefix = "lost_sales";
		EXPECT_EQ(prefix, model->Identifier().substr(0, prefix.length())) ;

		auto States = model->GetInitialStateVec(10);

		DynaPlex::VarGroup policyvars{ {"id","basestock"} };

		DynaPlex::Policy policy{};
		ASSERT_NO_THROW(
			policy = model->GetPolicy(policyvars);
		);
		EXPECT_EQ(policy->Identifier(), "basestock");

	}



	TEST(ModelFactory, FailGet) {
		DynaPlex::VarGroup vars;



		vars.Add("id", "LostTypoSales");
		vars.Add("p", 9.0);
		vars.Add("h", 1.0);

		DynaPlex::MDP model;

		ASSERT_THROW(
			model = DynaPlex::GetMDP(vars), DynaPlex::Error
		);

	}
}