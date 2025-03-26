#pragma once
#include "dynaplex/dynaplex_model_includes.h"
#include "dynaplex/modelling/discretedist.h"
#include "dynaplex/modelling/queue.h"
#include <list>
#include <optional>
#include <algorithm>

namespace DynaPlex::Models {
	namespace resource_allocation {
		//MDP nested classes:
		struct ResourceType {
			std::string key;
			int64_t index;
			int64_t number_available{0};
			ResourceType() {};
			explicit ResourceType(const DynaPlex::VarGroup& vars) {
				vars.Get("number_available", number_available);
				vars.Get("key", key);
			}
		};

		struct ResourceOption {
			//indices will be initiated based on class context and keys, when MDP is initiated from VarGroup:
			int64_t resource_type_index{ -1 };
			int64_t task_type_index{ -1 };

			DynaPlex::DiscreteDist duration_dist{};
			double expected_duration{};
			ResourceOption() {}
			explicit ResourceOption(const DynaPlex::VarGroup& vars) {
				vars.Get("duration_dist", duration_dist);
				duration_dist.OptimizeForSampling();
				expected_duration = duration_dist.Expectation();
			}
		};
		
			
		//A TaskType has NextTasks, that indicate a certain probability to go to a certain task after current task
		struct NextTask {
			//this will store the index of the task_type corresponding to the next task.
			int64_t task_type_index{ -1 };
			double probability{ 0.0 };
			NextTask() {}
			explicit NextTask(const DynaPlex::VarGroup& vars) {
				vars.Get("probability", probability);
			}
		};


		struct TaskType {
			//will be convenient to have this here - will be automatically initiated 
			//when the idkeycontainer is initiated. index will range from 0 to |TaskTypes|-1
			std::string key{};
			int64_t index{-1};

			bool has_arrivals{ false };
			double arrival_rate{ 0.0 };

			DynaPlex::DiscreteDist interarrival_dist;
			
			std::unordered_map<std::string, ResourceOption> resource_options;

			//probabilities of nexttask do not have to sum up to 1.0, remainder is finish probability.  
			std::unordered_map<std::string, NextTask> next_tasks;
						
			
			const ResourceOption& FindResourceOption(const std::string& resource_type_key) const {
				const auto& it = resource_options.find(resource_type_key);
				if (it == resource_options.end()) {
					throw DynaPlex::Error("TaskType::FindNextTask: NextTask with key '" + resource_type_key + "' not found.");
				}
				return it->second;
			}

			//probability that the job is finished after this task:
			//will be updated after deserialization from json. 
			double finishing_probability{ 1.0 };
			TaskType() {}
			explicit TaskType(const DynaPlex::VarGroup& vars) {
				vars.Get("key", key);
				vars.Get("resource_options", resource_options);
				if (vars.HasKey("next_tasks"))
					vars.Get("next_tasks", next_tasks);
				for (auto& [key,next_task] : next_tasks)
					finishing_probability -= next_task.probability;
				if (finishing_probability < 0.0)
				{
					throw DynaPlex::Error("DynaPlex::Models::resource_allocation::TaskType: probabilities assigned to next_task should not exceed 1.0, and there can only be one next_task. ");
				}
				
				if (vars.HasKey("arrival_rate"))
				{//NOTE: current code assumes this is a geometric dist, and uses its memoryless property!
			     //-this is assumed in reinitiate_hidden_variables.\
			     //refactoring to other interarrivaltimes would be straightforward.
					vars.Get("arrival_rate", arrival_rate);
					interarrival_dist = DiscreteDist::GetGeometricDist(1.0 / arrival_rate);
					interarrival_dist.OptimizeForSampling();
					has_arrivals = true;
				}
				else
					has_arrivals = false;
			}
		};
	}
}