#include <iostream>
#include "dynaplex/vargroup.h"
#include "dynaplex/makegeneric.h"
#include "dynaplex/error.h"
#include <gtest/gtest.h>


namespace DynaPlex::Tests {

	namespace AddOn ::Problem {
		class MDP
		{
		public:
			struct State {
				int i;
			};
		private:
			double p, h;
		public:
			

			MDP(const DynaPlex::VarGroup& vars) 
			{
				vars.Get("p", p);
				vars.Get("h", h);
			}
		};
	}
	TEST(RegisterAdditionalMDP, UseUnregisteredMDP) {

		

		DynaPlex::VarGroup vars;



		vars.Add("id", "Problem");
		vars.Add("p", 9.0);
		vars.Add("h", 1.0);

		auto DynaPlexMDP =DynaPlex::Erasure::MakeGenericMDP<AddOn::Problem::MDP>(vars);

		//Note that Problem::MDP does not yet support GetInitialStateVec
		EXPECT_THROW(DynaPlexMDP->GetInitialStateVec(10), DynaPlex::Error);

	  //  EXPECT_THROW(DynaPlexMDP->)

		const std::string prefix = "Problem";
		EXPECT_EQ(prefix, DynaPlexMDP->Identifier().substr(0, prefix.length()));

	}
}