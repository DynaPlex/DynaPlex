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

	//template <class t_MDP>
	//DynaPlex::MDP AsDynaPlexMDP( arguments )
	//User perfect forwarding to forward to constructor of dynaplex MDP
	

	//DynaPlex::MDP AsDynaPlexMDP( arguments )
	//perfect forwarding to GetInstance of that MDP. 

}//namespace DynaPlex