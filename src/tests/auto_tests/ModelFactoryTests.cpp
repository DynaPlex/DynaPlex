#include "dynaplex/vargroup.h"
#include "dynaplex/error.h"
#include <gtest/gtest.h>
#include "dynaplex/factories.h"

TEST(DynaPlexTests, ModelFactoryTests) {
	DynaPlex::VarGroup vars;


	vars.Add("id", "LostSales");
	vars.Add("p", 9.0);
	vars.Add("h", 1.0);

	
	ASSERT_NO_THROW(
		{ auto model = DynaPlex::GetMDP(vars); }
	);


}