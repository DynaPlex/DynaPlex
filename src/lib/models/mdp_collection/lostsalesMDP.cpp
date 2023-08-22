#include <iostream>
#include <string>
#include "dynaplex/mdpregistrar.h"

namespace DynaPlex::Models {
	namespace LostSales /*keep this in line with id below*/
	{

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


			MDP(const DynaPlex::VarGroup& vars):
				vars{ vars }
			{
				vars.Get("p", p);
				vars.Get("h", h);

			}
		};
	}
	//The declaration of the static registrar registers the MDP in the central registry, such that DynaPlex::GetMDP can locate it 
	static MDPRegistrar<LostSales::MDP> registrar(/*id*/"LostSales",/*optional brief description*/ "Canonical lost sales problem.");
}

