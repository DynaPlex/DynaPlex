#include "dynaplex/sampledata.h"
#include "dynaplex/error.h"
#include "dynaplex/rng.h"
namespace DynaPlex::NN
{
	void SampleData::SaveToFile(DynaPlex::MDP mdp, std::string path,int64_t json_indent, bool silent)
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

		if (!silent) {
			PrintStatistics();
		}

		VarGroup vars{};
		vars.Add("unique_identifier", mdp->Identifier());
		vars.Add("Samples", Samples);

		vars.SaveToFile(path,json_indent);
	}

	void SampleData::PrintStatistics()
	{
		std::vector<double> levels = { 0.5, 1.0, 1.5, 2.0, 2.5, 3.0 };
		std::vector<size_t> counts(levels.size(), 0);
		double avgMU = 0.0;
		double avgMUsamp = 0.0;

		for (auto& sample : Samples)
		{
			for (size_t i = 0; i < levels.size(); i++)
			{
				if (sample.z_stat > levels[i])
				{
					counts[i]++;
				}
			}
			avgMU += sample.q_hat;
		}
		std::cout << "Simulator statistics " << std::endl;
		std::cout << Samples.size() << " samples; ";
		for (size_t i = 0; i < levels.size(); i++)
		{
			std::cout << (counts[i] * 100) / Samples.size() << "% at Z>" << levels[i] << "; ";
		}
		std::cout << std::endl;
		std::cout << "Avg Mean of Q values: " << avgMU / Samples.size() <<std::endl;
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
			vg.Get("q_hat", sample.q_hat);
			vg.Get("q_hat_vec", sample.q_hat_vec);
			vg.Get("z_stat", sample.z_stat);
			vg.Get("cost_improvement", sample.cost_improvement);
			vg.Get("probabilities", sample.probabilities);
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
