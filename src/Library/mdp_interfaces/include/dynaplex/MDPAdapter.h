#pragma once
#include "MdpInterface.h"

namespace DynaPlex
{
	template<class t_MDP>
	class MDPAdapter : public MDPInterface
	{

	public:
		MDPAdapter(t_MDP model) :
			model{ model }
		{

		}
		std::string Identifier() override
		{
			return model.Identifier();
		}
		
		t_MDP model;
	};
}//namespace DynaPlex