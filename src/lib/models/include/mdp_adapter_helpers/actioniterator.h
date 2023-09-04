#pragma once
#include "mdpadapter_concepts.h"
#include "dynaplex/vargroup.h"
namespace DynaPlex::Erasure
{
	template<typename t_MDP>
	class ActionIterator 
	{
		int64_t min_action;
		int64_t max_action;
		ActionIterator(const DynaPlex::VarGroup& vars)
		{

			//some implementation;
		}
		//t_MDP is expected 
	};
}
