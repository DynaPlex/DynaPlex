#pragma once
#include "MdpInterface.h"

namespace DynaPlex
{
	template<class t_MDP>
	class MDPAdapter : public MdpInterface
	{

	public:
		MDPAdapter(t_MDP model) :
			model{ model }
		{

		}
		void write() override
		{
			model.write();
		}

		t_MDP model;
	};
}