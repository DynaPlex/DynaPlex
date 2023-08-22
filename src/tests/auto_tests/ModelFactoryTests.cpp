#include "dynaplex/vargroup.h"
#include "dynaplex/error.h"
#include <gtest/gtest.h>
#include "dynaplex/factories.h"
#include "dynaplex/registry.h"


namespace DynaPlex::Tests {
	

	TEST(ModelFactory, SimpleGet) {
		DynaPlex::VarGroup vars;



		vars.Add("id", "LostSales");
		vars.Add("p", 9.0);
		vars.Add("h", 1.0);

		DynaPlex::MDP model;


		ASSERT_NO_THROW(
			model = DynaPlex::GetMDP(vars);
		);
		const std::string prefix = "LostSales";

		EXPECT_EQ(prefix, model->Identifier().substr(0, prefix.length())) ;

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