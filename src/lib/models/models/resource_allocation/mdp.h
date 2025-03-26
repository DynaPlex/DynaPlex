#pragma once
#include "dynaplex/dynaplex_model_includes.h"
#include "dynaplex/modelling/discretedist.h"
#include "state_helper_classes.h"
#include "mdp_helper_classes.h"
#include "dynaplex/modelling/idcontainer.h"
#include "dynaplex/modelling/idkeycontainer.h"

#include "dynaplex/modelling/eventheap.h"

namespace DynaPlex::Models {
	namespace resource_allocation
	{		
		class MDP
		{			

		public:				 
			//from config:
			DynaPlex::IdKeyContainer<ResourceType> resource_types;
			DynaPlex::IdKeyContainer<TaskType> task_types;

			//basic implementation:
			//each action corresponds to a resource option, in the sense that 
			//an action will be to couple a TaskType to a ResourceType.
			//for ease of reference, here we will store a copy of all the resource options:
			std::vector<ResourceOption> actions;	

			//this will also be handy ; costs are scaled by this such that
			//objective corrresponds to mimimizing the long run average number of jobs
			//in the system. 
			double total_arrival_rate{ 0.0 };

			struct State {
				DynaPlex::StateCategory cat;
				//e.g. in minutes/seconds
				int64_t current_time;
				int64_t total_jobs_arrived;
				DynaPlex::IdContainer<Job> jobs;
				DynaPlex::IdContainer<Resource> resources;
				DynaPlex::IdContainer<Assignment> assignments;
				DynaPlex::EventHeap<ScheduledEvent> scheduled_event_queue;

				//to simplify the implementation of our current action representation
				//which is simply to match resource types and task types. 
				std::vector<int64_t> unassigned_tasks;
				std::vector<int64_t> unassigned_resources;
				DynaPlex::VarGroup ToVarGroup() const;

				inline void Schedule(int64_t& trigger_time, ScheduledEvent::EventPayLoad&& payload)
				{
					scheduled_event_queue.push_back(ScheduledEvent(trigger_time, payload));
				}
	
			};

			//helper functions:
			void FinishTaskSequence(const TaskType& task_type, DynaPlex::Queue<std::int64_t>& TaskSequence, DynaPlex::RNG& rng) const;
			//API:
			double ModifyStateWithAction(State&, int64_t action) const;
			double ModifyStateWithEvent(State&, DynaPlex::RNG& ) const;
			DynaPlex::VarGroup GetStaticInfo() const;
			DynaPlex::StateCategory GetStateCategory(const State&) const;
			bool IsAllowedAction(const State& state, int64_t action) const;			
			State GetInitialState(DynaPlex::RNG& rng) const;
			State GetState(const VarGroup&) const;
			void RegisterPolicies(DynaPlex::Erasure::PolicyRegistry<MDP>&) const;
			void GetFeatures(const State&, DynaPlex::Features&) const;
			explicit MDP(const DynaPlex::VarGroup&);
			void ResetHiddenStateVariables(State&, DynaPlex::RNG&) const;

		};
	}
}

