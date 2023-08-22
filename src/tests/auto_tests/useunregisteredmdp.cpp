#include <iostream>
#include "dynaplex/vargroup.h"
#include "dynaplex/makegeneric.h"
#include "dynaplex/error.h"
#include <gtest/gtest.h>


namespace DynaPlex::Tests {

	namespace AddOn {
		class MDP
		{
			const DynaPlex::VarGroup vars;
			double p, h;
		public:
			//Should return a unique identifier for this MDP. Same MDP (with same parameter), same identifier. Different MDP, different identifier. 
			std::string Identifier()
			{
				//This implementation ensures that identifier will have the desired properties. 
				return vars.Identifier();
			}


			MDP(const DynaPlex::VarGroup& vars) :
				vars{ vars }
			{
				vars.Get("p", p);
				vars.Get("h", h);

			}
		};
	}
	TEST(ModelFactory, ManualRegistration) {

		

		DynaPlex::VarGroup vars;



		vars.Add("id", "LostSales");
		vars.Add("p", 9.0);
		vars.Add("h", 1.0);

		auto DynaPlexMDP =DynaPlex::Erasure::MakeGeneric<AddOn::MDP>(vars);
	}
}