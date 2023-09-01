#pragma once
#include "dynaplex/policy.h"
#include "dynaplex/states.h"
#include "dynaplex/vargroup.h"
#include "dynaplex/error.h"
#include "statesadapter.h"
#include <type_traits>

namespace DynaPlex::Erasure
{
	template<typename t_MDP, typename t_Agent>
	class PolicyAdapter : public PolicyInterface
	{

	};
}
