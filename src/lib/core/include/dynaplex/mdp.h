#pragma once
#include <memory>
#include <string>
#include "dynaplex/states.h"
#include "vargroup.h"
#include "policy.h"
namespace DynaPlex
{
	class MDPInterface
	{
	public:
		virtual std::string Identifier() const = 0;
		virtual DynaPlex::States GetInitialStateVec(size_t) const = 0;
		virtual DynaPlex::VarGroup GetStaticInfo() const = 0;

		virtual std::vector<int64_t> AllowedActions(const DynaPlex::States&, size_t index=0)const = 0;

		virtual DynaPlex::VarGroup ToVarGroup(const DynaPlex::States&,size_t index=0) const= 0;
		virtual void IncorporateActions(DynaPlex::States&) const = 0;

		virtual DynaPlex::Policy GetPolicy(const DynaPlex::VarGroup& vars) const =0;
		virtual DynaPlex::Policy GetPolicy(const std::string& id) const = 0;
	};
	using MDP = std::shared_ptr<MDPInterface>;
}//namespace DynaPlex