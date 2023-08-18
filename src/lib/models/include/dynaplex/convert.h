#pragma once
#include "dynaplex/mdpadapter.h"
#include "dynaplex/mdpinterface.h"
#include "dynaplex/mdp.h"

namespace DynaPlex {

	template <class t_MDP>
	DynaPlex::MDP Convert(t_MDP mdp_impl)
	{
		auto shared = std::make_shared<DynaPlex::MDPAdapter<t_MDP>>(mdp_impl);
		auto converted = std::static_pointer_cast<DynaPlex::MDPInterface>(shared);
		return converted;
	}
}//namespace DynaPlex