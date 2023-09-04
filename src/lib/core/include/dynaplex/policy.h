#pragma once
#include <memory>
#include <string>
#include "dynaplex/states.h"
#include "dynaplex/statecontext.h"
#include "vargroup.h"
namespace DynaPlex
{
	class PolicyInterface
	{
	public:
		virtual std::string Identifier() const = 0;

		virtual void SetActions(const DynaPlex::States& states, std::vector<StateContext>& context) const = 0;


	};
	using Policy = std::shared_ptr<PolicyInterface>;
}//namespace DynaPlex