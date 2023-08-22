#pragma once
#include "dynaplex/mdpadapter.h"
#include "dynaplex/mdp.h"
#include "dynaplex/vargroup.h"

//Erases the specific type of the argument passed, and returns a generic DynaPlex::MDP
namespace DynaPlex::Erasure {

	template <class t_MDP>
	DynaPlex::MDP MakeGeneric(const VarGroup& vargroup)
	{
		return std::make_shared<MDPAdapter<t_MDP>>(vargroup);
	}
}//namespace DynaPlex