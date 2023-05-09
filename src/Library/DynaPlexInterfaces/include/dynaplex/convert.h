#pragma once
#include "mdpadapter.h"
#include "mdpinterface.h"
#include "mdp.h"

namespace DynaPlex {

	template <class t_MDP>
	DynaPlex::MDP Convert(t_MDP mdp_impl)
	{
		auto shared = std::make_shared<DynaPlex::MDPAdapter<t_MDP>>(mdp_impl);
		auto converted = std::static_pointer_cast<DynaPlex::MDPInterface>(shared);
		return converted;
	}
}//namespace DynaPlex