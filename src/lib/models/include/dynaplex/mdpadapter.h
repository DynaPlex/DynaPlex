#pragma once
#include "dynaplex/mdp.h"


//This adapts any MDP to the MDPInterface, which is related to DynaPlex::MDP =std::shared_pointer<MDPInterface>
namespace DynaPlex::Erasure
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