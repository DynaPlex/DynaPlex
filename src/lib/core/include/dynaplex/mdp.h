#pragma once
#include <memory>
#include <string>
#include "dynaplex/states.h"
#include "vargroup.h"
namespace DynaPlex
{
	class MDPInterface
	{
	public:
		virtual std::string Identifier() = 0;
		virtual DynaPlex::States GetInitialStateVec(size_t) = 0;
		virtual DynaPlex::VarGroup::VarGroupVec ToVarGroup(DynaPlex::States&) = 0;
	};
	using MDP = std::shared_ptr<MDPInterface>;
}//namespace DynaPlex