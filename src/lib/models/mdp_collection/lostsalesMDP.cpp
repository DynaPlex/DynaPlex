//lostsalesMDP.cpp
#include <iostream>
#include <string>
#include <functional>
#include "lostsalesMDP.h"
#include "dynaplex/mdpregistrar.h"

namespace DynaPlex::Models::MDP {
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
			return DynaPlex::Erasure::Convert(MDP(vars));
		}

		bool registerLostSales = [] {
			DynaPlex::Models::Registry::Register("LostSales", DynaPlex::Models::MDP::LostSales::GetInstance);
			return true;
		}();
	}
}

