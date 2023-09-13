#include "dynaplex/trajectory.h"
namespace DynaPlex {
	VarGroup Trajectory::ToVarGroup() const
	{
		throw DynaPlex::Error("not implemented");
		//VarGroup vg{};
	//	vg.Add("CumulativeReturn", CumulativeReturn);
		//vg.Add("NextAction", NextAction);
		//return vg;
	}

	std::vector<uint32_t> Trajectory::constructseeds(const uint32_t& experiment_number, const uint32_t& world_rank, const bool& eval)
	{
		std::vector<uint32_t> seeds{};
		seeds.reserve(3);
		seeds.push_back(experiment_number);
		seeds.push_back(world_rank);
		//To avoid any overlap between trajectories used for training and
		//those used for evaluation. 
		if (eval)
		{
			seeds.push_back(26071983);
		}
		else
		{
			seeds.push_back(24031984);
		}
		return seeds;
	}
}
