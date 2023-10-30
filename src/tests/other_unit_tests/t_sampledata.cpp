#include "dynaplex/vargroup.h"
#include "dynaplex/error.h"
#include <gtest/gtest.h>
#include "dynaplex/dynaplexprovider.h"
#include "dynaplex/trajectory.h"
#include "dynaplex/demonstrator.h"
#include "dynaplex/sampledata.h"
namespace DynaPlex::Tests {
	

	TEST(sampledata, basics) {
		auto& dp = DynaPlexProvider::Get();
		auto& system = dp.System();

		std::string model_name = "lost_sales";
		std::string mdp_config_name = "mdp_config_0.json";
		//configure MDP:
		ASSERT_TRUE(
			system.file_exists("mdp_config_examples", model_name, mdp_config_name)
		);
		DynaPlex::VarGroup mdp_vars_from_json;
		ASSERT_NO_THROW(
			std::string file_path = system.filepath("mdp_config_examples", model_name, mdp_config_name);
		    mdp_vars_from_json = VarGroup::LoadFromFile(file_path);
		);

		auto mdp = dp.GetMDP(mdp_vars_from_json);
		
		DynaPlex::Policy policy = mdp->GetPolicy("random");

		int64_t max_periods = 10;
		auto demonstrator_config = DynaPlex::VarGroup{ {"max_period_count", max_periods},{"seed",123} };
		auto demonstrator = dp.GetDemonstrator(demonstrator_config);
		auto trace = demonstrator.GetObjectTrace(mdp);
		DynaPlex::NN::SampleData data{mdp};		
		for (auto& elem : trace)
		{
			if (elem.cat.IsAwaitAction())
			{
				data.Samples.emplace_back(elem.action, elem.state->Clone());
			}
		}
		std::string path = system.filepath("tests", "sampledata_basics", "data.json");
		data.SaveToFile(mdp, path);

		auto data_from_json = DynaPlex::NN::SampleData::CreateNewFromFile(mdp, path);

		ASSERT_EQ(
			data.Samples.size(), data_from_json.Samples.size()
		);

		data_from_json.AddFromFile(mdp, path);

		ASSERT_EQ(
			2*data.Samples.size(), data_from_json.Samples.size()
		);

		for (size_t i = 0; i < data.Samples.size(); i++)
		{
			
			auto& sample = data.Samples[i];
		//	std::cout << sample.ToVarGroup().Dump() << std::endl;
			auto& other = data_from_json.Samples[i];
			ASSERT_EQ(sample.action_label, other.action_label);
			ASSERT_TRUE(
				mdp->StatesAreEqual(sample.state, other.state)
			);

		}

		//lost_sales starts with action, and alternates between actions and events, never final. Hence, there will be 2*maxevents elements in trace. 
		ASSERT_EQ(trace.size(), max_periods *2);
	}
}