#include "dynaplex/sampledata.h"
#include "dynaplex/error.h"
#include "dynaplex/rng.h"
namespace DynaPlex::NN
{
	void SampleData::SaveToFile(DynaPlex::MDP mdp, std::string path,int64_t json_indent)
	{
		if (!mdp->SupportsGetStateFromVarGroup())
		{
			throw DynaPlex::Error("This MDP does not support getting state from VarGroup. Currently, samples cannot be saved.");
		}
		for (auto & sample :Samples)
		{
			if (!mdp->CheckConformant(sample.state))
			{
				throw DynaPlex::Error("SampleData::save : Error - trying to save samples that contain states not created with this mdp.");
			}
		}
		VarGroup vars{};
		vars.Add("unique_identifier", mdp->Identifier());
		vars.Add("Samples", Samples);

		vars.SaveToFile(path,json_indent);
	}

	SampleData SampleData::CreateNewFromFile(DynaPlex::MDP mdp, std::string path)
	{
		if (!mdp->SupportsGetStateFromVarGroup())
		{
			throw DynaPlex::Error("This MDP does not support getting state from VarGroup. Currently, samples cannot be saved or loaded.");
		}
		auto vars = VarGroup::LoadFromFile(path);
		std::string unique_identifier;
		vars.Get("unique_identifier", unique_identifier);

		std::string mdp_identifier = mdp->Identifier();
		if (mdp_identifier != unique_identifier)
		{
			throw DynaPlex::Error("SampleData::CreateNewFromFile : Error - trying to load samples using a different (or differently parameterized) mdp compared to the mdp with which the states were created.");
		}
		std::vector<DynaPlex::VarGroup> vg_vec;
		vars.Get("Samples", vg_vec);

		SampleData result{mdp};
		result.Samples.reserve(vg_vec.size());
		for (auto& vg : vg_vec)
		{
			//emplace a default-constructed sample. 
			result.Samples.emplace_back();
			auto& sample = result.Samples.back();

			VarGroup state_as_vg{};
			vg.Get("state", state_as_vg);
		    sample.state = mdp->GetState(state_as_vg);

			vg.Get("action_label", sample.action_label);
			vg.Get("sample_number", sample.sample_number);
		}

		return result;
	}


	void SampleData::AddFromFile(DynaPlex::MDP mdp, std::string path)
	{
		if (mdp->Identifier() != unique_identifier)
		{
			throw DynaPlex::Error("SampleData::AddFromFile - attempting to add data that results from a different (or differently parameterized) mdp:"+mdp->Identifier()+" vs "+ unique_identifier);
		}
		SampleData dataToAdd = SampleData::CreateNewFromFile(mdp, path);
		Samples.insert(Samples.end(),std::make_move_iterator( dataToAdd.Samples.begin()),std::make_move_iterator( dataToAdd.Samples.end()));
	}
		


	SampleData::SampleData(DynaPlex::MDP mdp) :Samples{}, unique_identifier{mdp->Identifier()}
	{
	}
}
