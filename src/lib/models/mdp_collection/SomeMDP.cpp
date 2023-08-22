#include <iostream>
#include <string>
#include "dynaplex/mdpregistrar.h"

namespace DynaPlex::Models {
	namespace SomeMDP /*keep this in line with id below*/
	{

		class MDP
		{
			const DynaPlex::VarGroup vars;
		public:
			std::string Identifier()
			{
				return "SomeMDP";
			}
			MDP(const DynaPlex::VarGroup& vars)
				:vars{ vars }
			{
			}
		};
	}
	//The declaration of the static registrar registers the MDP in the central registry, such that DynaPlex::GetMDP can locate it 
	static MDPRegistrar<SomeMDP::MDP> registrar(/*id*/"SomeMDP",/*optional brief description*/ "Some crazy MDP");
}

