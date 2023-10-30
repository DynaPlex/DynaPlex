#pragma once
#include <vector>
#include "dynaplex/sample.h" 
#include "dynaplex/rng.h"
#include "dynaplex/mdp.h"

namespace DynaPlex::NN
{
	class SampleData
	{
		std::string unique_identifier;
	public:
		std::vector<DynaPlex::NN::Sample> Samples;
		SampleData(DynaPlex::MDP);
		void SaveToFile(DynaPlex::MDP, std::string path, int64_t json_indent=-1);
		static SampleData CreateNewFromFile(DynaPlex::MDP, std::string path);
		void AddFromFile(DynaPlex::MDP, std::string path);
	};
}