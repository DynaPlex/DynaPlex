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
		virtual std::string Identifier() const = 0;
		virtual DynaPlex::States GetInitialStateVec(size_t) const = 0;
		virtual DynaPlex::VarGroup::VarGroupVec ToVarGroup(const DynaPlex::States&) const= 0;
	};
	using MDP = std::shared_ptr<MDPInterface>;
}//namespace DynaPlex