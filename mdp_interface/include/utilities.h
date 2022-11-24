#pragma once
#include "MDPAdapter.h"
#include "MdpInterface.h"
#include "mdp.h"

namespace Dynaplex {

	template <class t_MDP>
	DynaPlex::MDP Convert(t_MDP mdp_impl)
	{
		auto shared = std::make_shared<DynaPlex::MDPAdapter<t_MDP>>(mdp_impl);
		auto converted = std::static_pointer_cast<DynaPlex::MdpInterface>(shared);
		//if (*converted)
		{
			//some error
		}
		return converted;
	}
	

}