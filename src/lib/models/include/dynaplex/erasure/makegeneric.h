#pragma once
#include "dynaplex/mdp.h"
#include "dynaplex/vargroup.h"
#include "mdpadapter.h"

namespace DynaPlex::Erasure {
	template <class t_MDP>
	//Creates a specific MDP of the requested type using the vargroup as adapter, and then interprets it as a generic DynaPlex::MDP;
	::DynaPlex::MDP MakeGenericMDP(const VarGroup& vargroup)
	{
		return std::make_shared<MDPAdapter<t_MDP>>(vargroup);
	}
}//namespace DynaPlex