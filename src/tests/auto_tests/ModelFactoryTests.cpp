#include "dynaplex/vargroup.h"
#include "dynaplex/error.h"
#include <gtest/gtest.h>
#include "dynaplex/factories.h"
#include "dynaplex/registry.h"
TEST(ModelFactory, SimpleGet) {
	DynaPlex::VarGroup vars;



	vars.Add("id", "LostSales");
	vars.Add("p", 9.0);
	vars.Add("h", 1.0);

	DynaPlex::MDP model;

	ASSERT_NO_THROW(
		 model = DynaPlex::GetMDP(vars); 
	);
		
	EXPECT_EQ(model->Identifier(), "lost sales 9.000000");

}