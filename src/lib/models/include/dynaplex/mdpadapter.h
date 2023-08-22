#pragma once
#include "dynaplex/mdp.h"
#include "dynaplex/vargroup.h"

//This adapts any MDP to the MDPInterface, which is related to DynaPlex::MDP =std::shared_pointer<MDPInterface>
namespace DynaPlex::Erasure
{
	template<class t_MDP>
	class MDPAdapter : public MDPInterface
	{

		std::string id;
		t_MDP model;
	public:
		MDPAdapter(DynaPlex::VarGroup vars) :
			model{ vars }, id{vars.Identifier()}
		{
		}
		std::string Identifier() override
		{
			return model.Identifier();//id
		}
		
	};
}//namespace DynaPlex