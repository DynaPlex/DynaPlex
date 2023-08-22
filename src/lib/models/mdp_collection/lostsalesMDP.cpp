#include <iostream>
#include <string>
#include "dynaplex/mdpregistrar.h"

namespace DynaPlex::Models {
	namespace LostSales /*keep this in line with id below*/
	{

		class MDP
		{
			struct State {
				int64_t state;
			};

			const DynaPlex::VarGroup vars;
			double p, h;
		public:
			
			State GetInitialState()
			{
				return State{ 123 };
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

