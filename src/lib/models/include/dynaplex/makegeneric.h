#pragma once
#include "dynaplex/mdpadapter.h"
#include "dynaplex/mdp.h"
#include "dynaplex/vargroup.h"

namespace DynaPlex::Erasure {

	template <class t_MDP>
	//Creates a specific MDP of the requested type using the vargroup as adapter, and then interprets it as a generic DynaPlex::MDP;
	::DynaPlex::MDP MakeGenericMDP(const VarGroup& vargroup)
	{
		return std::make_shared<MDPAdapter<t_MDP>>(vargroup);
	}





	
}//namespace DynaPlex