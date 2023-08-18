//lostsalesMDP.cpp
#include <iostream>
#include <string>
#include <functional>
#include "lostsalesMDP.h"
#include "dynaplex/mdpregistry.h"
#include "dynaplex/convert.h"

namespace DynaPlex::Collections::MDP {
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
				vars.Get_Into("p", p);
				vars.Get_Into("h", h);
			}
		};		

		DynaPlex::MDP GetInstance(const DynaPlex::VarGroup& vars) {
			return DynaPlex::Convert(MDP(vars));
		}

		bool registerLostSales = [] {
			DynaPlex::Collections::MDPRegistry::Register("LostSales", DynaPlex::Collections::MDP::LostSales::GetInstance);
			return true;
		}();
	}
}

