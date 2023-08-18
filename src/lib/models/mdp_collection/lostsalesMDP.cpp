#include <iostream>
#include <string>
#include "dynaplex/mdpregistrar.h"

namespace DynaPlex::Models {
	namespace LostSales
	{

		class MDP
		{
			double p, h;
		public:
			std::string Identifier()
			{
				return "lost sales " + std::to_string(p);
			}
			MDP(const DynaPlex::VarGroup& vars)
			{
				vars.Get("p", p);
				vars.Get("h", h);
			}
		};


		//The declaration of the static registrar ensures that the MDP that we just declared
		//is registered in the central registry under an appropriate identifier.
		//Adding a brief description is optional. 
		static DynaPlex::Models::MDPRegistrar<DynaPlex::Models::LostSales::MDP> registrar("LostSales", "Canonical lost sales problem.");

	}
}

