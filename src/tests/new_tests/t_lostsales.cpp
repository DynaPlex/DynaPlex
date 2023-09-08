#include "dynaplex/vargroup.h"
#include "dynaplex/error.h"
#include <gtest/gtest.h>
#include "dynaplex/provider.h"

namespace DynaPlex::Tests {
	

	TEST(LostSales, Basics) {
		DynaPlex::Provider provider;
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
		DynaPlex::Policy policy;

		std::cout << provider.ListMDPs().Dump(2) << std::endl;

		ASSERT_NO_THROW(
			model = provider.GetMDP(vars);
		    policy = model->GetPolicy("basestock");
		);

		std::cout<<model->ListPolicies().Dump(2) << std::endl;


		//auto policy2= model->GetPolicy("random");


		const std::string prefix = "lost_sales";
		EXPECT_EQ(prefix, model->Identifier().substr(0, prefix.length())) ;

		auto State = model->GetInitialState();


		std::cout << model->ToVarGroup(State).Dump() << std::endl;



		model->IncorporateAction(State);
		std::cout << model->ToVarGroup(State).Dump() << std::endl;
		model->IncorporateAction(State);
		std::cout << model->ToVarGroup(State).Dump() << std::endl;
		std::cout << model->GetStaticInfo().Dump() << std::endl;

	}
}