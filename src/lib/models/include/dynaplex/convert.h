#pragma once
#include "dynaplex/mdpadapter.h"
#include "dynaplex/mdp.h"

//Erases the specific type of the argument passed, and returns a generic DynaPlex::MDP
namespace DynaPlex::Erasure {

	template <class t_MDP>
	DynaPlex::MDP Convert(t_MDP mdp_impl)
	{
		return std::make_shared<MDPAdapter<t_MDP>>(mdp_impl);
	}
}//namespace DynaPlex