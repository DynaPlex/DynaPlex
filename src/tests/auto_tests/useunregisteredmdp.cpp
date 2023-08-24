#include <iostream>
#include "dynaplex/vargroup.h"
#include "dynaplex/makegeneric.h"
#include "dynaplex/error.h"
#include <gtest/gtest.h>


namespace DynaPlex::Tests {

	namespace AddOn ::Problem {
		class MDP
		{
			const DynaPlex::VarGroup vars;
			double p, h;
		public:
			

			MDP(const DynaPlex::VarGroup& vars) :
				vars{ vars }
			{
				vars.Get("p", p);
				vars.Get("h", h);

			}
		};
	}
	TEST(UseUnregisteredMDP, UseUnregisteredMDP) {

		

		DynaPlex::VarGroup vars;



		vars.Add("id", "Problem");
		vars.Add("p", 9.0);
		vars.Add("h", 1.0);

		//auto DynaPlexMDP =DynaPlex::Erasure::MakeGeneric<AddOn::Problem::MDP>(vars);

		//const std::string prefix = "Problem";
		//EXPECT_EQ(prefix, DynaPlexMDP->Identifier().substr(0, prefix.length()));

	}
}