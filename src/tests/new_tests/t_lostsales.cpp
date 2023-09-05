#include "dynaplex/vargroup.h"
#include "dynaplex/error.h"
#include <gtest/gtest.h>
#include "dynaplex/factories.h"
#include "dynaplex/registry.h"

namespace DynaPlex::Tests {
	

	TEST(LostSales, Basics) {
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

		ASSERT_NO_THROW(
			model = DynaPlex::GetMDP(vars);
		    policy = model->GetPolicy("basestock");
		);



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