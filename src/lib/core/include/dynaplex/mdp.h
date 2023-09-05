#pragma once
#include <memory>
#include <string>
#include "dynaplex/state.h"
#include "vargroup.h"
#include "policy.h"
namespace DynaPlex
{
	class MDPInterface
	{
	public:
		virtual std::string Identifier() const = 0;

		virtual DynaPlex::dp_State GetInitialState() const = 0;
		virtual DynaPlex::VarGroup GetStaticInfo() const = 0;

		virtual std::vector<int64_t> AllowedActions(const DynaPlex::dp_State&)const = 0;

		virtual DynaPlex::VarGroup ToVarGroup(const DynaPlex::dp_State&) const= 0;
		virtual void IncorporateAction(DynaPlex::dp_State&) const = 0;

		virtual DynaPlex::Policy GetPolicy(const DynaPlex::VarGroup& vars) const =0;
		virtual DynaPlex::Policy GetPolicy(const std::string& id) const = 0;
		virtual ~MDPInterface() = default;
	};
	using MDP = std::shared_ptr<MDPInterface>;
}//namespace DynaPlex