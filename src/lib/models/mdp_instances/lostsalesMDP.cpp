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
			struct State {
				int64_t state;
				DynaPlex::VarGroup ToVarGroup() const
				{
					DynaPlex::VarGroup vars;
					vars.Add("state", state);
					return vars;
				}
			};

			double ModifyStateWithAction(State& state,int64_t action) const
			{
				state.state += action;
				return 0.0;	
			}
	
			
			State GetInitialState() const
			{
				return State{ 123 };
			}

			explicit MDP(const DynaPlex::VarGroup& vars) :
				vars{ vars }
			{
				vars.Get("p", p);
				vars.Get("h", h);

			}
		};
	}
	//int64_the declaration of the static registrar registers the MDP in the central registry, such that DynaPlex::GetMDP can locate it 
	static MDPRegistrar<LostSales::MDP> registrar(/*id*/"LostSales",/*optional brief description*/ "Canonical lost sales problem.");
}

