#pragma once
#include "dynaplex/mdp.h"
#include "dynaplex/vargroup.h"
namespace DynaPlex {
	DynaPlex::MDP GetMDP(const DynaPlex::VarGroup& vars);
	DynaPlex::VarGroup ListMDPs();

}//namespace DynaPlex