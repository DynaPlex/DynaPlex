//<something.h>
#pragma once
#include "dynaplex/mdp.h"
#include "dynaplex/vargroup.h"
namespace DynaPlex {
	DynaPlex::MDP GetMDP(const DynaPlex::VarGroup& vars);

}//namespace DynaPlex