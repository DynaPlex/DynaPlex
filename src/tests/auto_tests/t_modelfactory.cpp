#include "dynaplex/vargroup.h"
#include "dynaplex/error.h"
#include <gtest/gtest.h>
#include "dynaplex/dynaplexprovider.h"

namespace DynaPlex::Tests {
	

	TEST(ModelFactory, SimpleGet) {
		auto& dp  = DynaPlexProvider::get();

		
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
			model = dp.GetMDP(vars);
		);

		const std::string prefix = "lost_sales";
		EXPECT_EQ(prefix, model->Identifier().substr(0, prefix.length())) ;

		auto State = model->GetInitialState();

		DynaPlex::VarGroup policyvars{ {"id","basestock"} };

		DynaPlex::Policy policy{};
		ASSERT_NO_THROW(
			policy = model->GetPolicy(policyvars);
		);

		EXPECT_EQ(policy->Identifier(), "basestock");

	}



	TEST(ModelFactory, FailGet) {
		auto& dp = DynaPlexProvider::get();

		DynaPlex::VarGroup vars;



		vars.Add("id", "LostTypoSales");
		vars.Add("p", 9.0);
		vars.Add("h", 1.0);

		DynaPlex::MDP model;

		ASSERT_THROW(
			model = dp.GetMDP(vars), DynaPlex::Error
		);

	}
}