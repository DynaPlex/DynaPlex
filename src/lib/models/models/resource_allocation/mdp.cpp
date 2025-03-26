#include "mdp.h"
#include "dynaplex/erasure/mdpregistrar.h"
#include "policies.h"
#include <algorithm>
#include <cmath>
#include <numeric>

namespace DynaPlex::Models {
	namespace resource_allocation
	{
		VarGroup MDP::GetStaticInfo() const
		{
			int64_t num_actions = actions.size() + 1;
			return DynaPlex::VarGroup{
				{"valid_actions",num_actions}
			};
		}

		double MDP::ModifyStateWithEvent(State& state, DynaPlex::RNG& rng) const
		{
			ScheduledEvent& first_event = state.scheduled_event_queue.first();
			if (first_event.trigger_time < state.current_time)
				throw DynaPlex::Error("resource_allocation::ModifyStateWithAction - time is decreasing");

			double costs = state.jobs.size() * (first_event.trigger_time - state.current_time) * total_arrival_rate;
			state.current_time = first_event.trigger_time;


			
			switch (first_event.PayLoadIndex())
			{
			case ScheduledEvent::IndexOf<JobArrival>():
			{
				auto& jobarrival = first_event.get<JobArrival>();
				//add corresponding job to system: 
				auto& [id, job] = state.jobs.AddNew();
				job.job_number = state.total_jobs_arrived++;
				job.arrival_time = first_event.trigger_time;
				//update amount of unassigned tasks:
				state.unassigned_tasks.at(jobarrival.tasktype_index)++;
				//determine the sequence of tasks that will be worked on for this job:
				auto& initial_task_type = task_types[jobarrival.tasktype_index];
				FinishTaskSequence(initial_task_type, job.task_sequence_indices, rng);
				//schedule next arrival:				
				int64_t trigger_time = state.current_time + initial_task_type.interarrival_dist.GetSample(rng);

				state.Schedule(trigger_time, JobArrival(initial_task_type));
				break;
			}
			case ScheduledEvent::IndexOf<TaskCompletion>():
			{
				auto& taskCompletion = first_event.get<TaskCompletion>();
				auto& completed_assignment = state.assignments[taskCompletion.assignment_id];
				auto& job = state.jobs[completed_assignment.job_id];
				//first task is not completed, remove it:
				job.task_sequence_indices.pop_front();

				if (job.task_sequence_indices.IsEmpty())
					//job is completely finished: remove from system.
				{
					state.jobs.Delete(completed_assignment.job_id);
				}
				else
				{
					//task completion implies that the assignment is finished
					//remove connected assignment by setting id to -1:
					job.current_assignment_id = -1;
					//make next task on job available to process
					state.unassigned_tasks.at(job.task_sequence_indices.front())++;
				}
				auto& resource = state.resources[completed_assignment.resource_id];
				//free the resource;
				resource.current_assignment_id = -1;
				state.unassigned_resources.at(resource.resource_type_index)++;
				//delete the assignment, now that it is completed:
				state.assignments.Delete(taskCompletion.assignment_id);
				break;
			}
			case ScheduledEvent::IndexOf<NewTaskAssignment>():
			{//note that this is scheduled at the current time, and takes precedence. So no costs incurred here.
				auto& new_task_assignment = first_event.get<NewTaskAssignment>();
				auto& action = actions[new_task_assignment.action_index];
				auto& [assignment_id, assignment] = state.assignments.AddNew();
				assignment.assignment_time = state.current_time;
				int64_t completion_time = state.current_time + action.duration_dist.GetSample(rng);

				{
					int64_t earliest_number = std::numeric_limits<int64_t>::max();
					int64_t best_id{ -1 };
					for (auto& [id, job] : state.jobs)
					{	//not currently assigned
						if (job.current_assignment_id < 0)
						{
							//matches the task_type that was selected in the action
							if (job.task_sequence_indices.front() == action.task_type_index)
							{ //of those that have these properties, select the first:
								if (job.job_number < earliest_number) {
									earliest_number = job.job_number;
									best_id = id;
								}
							}
						}
					}
					if (best_id == -1)
						throw DynaPlex::Error("ModifyStateWithEvent - newassigment: best_id appears un-updated.");

					//assign the job:
					assignment.job_id = best_id;
					auto& job = state.jobs[best_id];
					job.current_assignment_id = assignment_id;
					state.unassigned_tasks.at(action.task_type_index)--;
				}
				{
					//find a resource to assign that matches the action
					int64_t lowest_id = std::numeric_limits<int64_t>::max();
					for (auto& [id, resource] : state.resources)
					{
						if (resource.current_assignment_id < 0)
						{//resource not currently assigned
							if (resource.resource_type_index == action.resource_type_index)
							{
								if (id < lowest_id)
									lowest_id = id;
							}
						}
					}
					//assign the resource:
					assignment.resource_id = lowest_id;
					auto& resource = state.resources[lowest_id];
					resource.current_assignment_id = assignment_id;
					state.unassigned_resources.at(action.resource_type_index)--;
				}
				state.Schedule(completion_time, TaskCompletion(assignment_id, new_task_assignment.action_index));
				break;
			}
			default:
				throw DynaPlex::Error("MDP::ModifyStateWithEvent : case not covered.");
			}
			//the event has been processed, now it must be removed from the queue:
			state.scheduled_event_queue.pop();
			bool any_action_allowed{ false };
			for (int64_t action = 0; action < actions.size(); action++)
			{
				if (IsAllowedAction(state, action))
				{
					any_action_allowed = true;
					break;
				}
			}


			///int64_t sum_unassigned_resources = std::accumulate(state.unassigned_resources.begin(), state.unassigned_resources.end(), 0ll);
			///int64_t sum_unassigned_tasks = std::accumulate(state.unassigned_tasks.begin(), state.unassigned_tasks.end(), 0ll);
			//if (sum_unassigned_resources > 0 && sum_unassigned_tasks > 0) {
			if (any_action_allowed) {
				state.cat = StateCategory::AwaitAction();
			}
			else
			{
				switch (state.scheduled_event_queue.first().PayLoadIndex())
				{
				case ScheduledEvent::IndexOf<JobArrival>():
					//a job arrival corresponds to a period in dynaplex period counting.
					//since these are exogenous, it is a good way to keep time
					state.cat = StateCategory::AwaitEvent(0);
					break;
				case ScheduledEvent::IndexOf<TaskCompletion>():
					//this ensures that taskcompletions are on a different random stream
					//which promotes variance reduction.
					//also ensures that period counting is not affected by task completions.
					state.cat = StateCategory::AwaitEvent(1);
					break;
				case ScheduledEvent::IndexOf<NewTaskAssignment>():
					throw DynaPlex::Error("ModifyStatewithEvent: this is unexpected");
				default:
					throw DynaPlex::Error("ModifyStateWithEvent: logical error while determining next status");
				}
			}
			return costs;
		}

		double MDP::ModifyStateWithAction(MDP::State& state, int64_t action_index) const
		{
			if (action_index != actions.size())
			{
				state.Schedule(state.current_time, NewTaskAssignment(action_index));
			}
			//For processing the action, we use an event stream that is disjunct from 
			//the event streams for the other models.
			//also - time is not affected by these events.
			state.cat = StateCategory::AwaitEvent(2);
			return 0.0;
		}

		DynaPlex::VarGroup MDP::State::ToVarGroup() const
		{
			DynaPlex::VarGroup vars;
			vars.Add("cat", cat);
			vars.Add("current_time", current_time);
			vars.Add("unassigned_resources", unassigned_resources);
			vars.Add("unassigned_tasks", unassigned_tasks);
			vars.Add("jobs", jobs);
			vars.Add("resources", resources);
			vars.Add("assignments", assignments);
			vars.Add("scheduled_event_queue", scheduled_event_queue);
			vars.Add("total_jobs_arrived", total_jobs_arrived);
			return vars;
		}

		MDP::State MDP::GetState(const VarGroup& vars) const
		{
			State state{};
			vars.Get("cat", state.cat);
			vars.Get("current_time", state.current_time);
			vars.Get("unassigned_resources", state.unassigned_resources);
			vars.Get("unassigned_tasks", state.unassigned_tasks);
			vars.Get("jobs", state.jobs);
			vars.Get("resources", state.resources);
			vars.Get("assignments", state.assignments);
			vars.Get("scheduled_event_queue", state.scheduled_event_queue);
			vars.Get("total_jobs_arrived", state.total_jobs_arrived);
			return state;
		}

		MDP::State MDP::GetInitialState(DynaPlex::RNG& rng) const
		{
			State state{};
			state.cat = StateCategory::AwaitEvent(0);
			state.current_time = 0;
			state.unassigned_resources = std::vector<int64_t>(resource_types.size(), 0);
			state.unassigned_tasks = std::vector<int64_t>(task_types.size(), 0);
			//Make available resources of the various types. 
			for (auto& resource_type : resource_types)
			{
				//set initial availability
				state.unassigned_resources.at(resource_type.index) = resource_type.number_available;
				for (int64_t i = 0; i < resource_type.number_available; i++)
				{
					auto& [index, resource] = state.resources.AddNew();
					resource.resource_type_index = resource_type.index;
				}
			}

			for (auto& task_type : task_types)
			{
				if (task_type.has_arrivals)
				{
					int64_t trigger_time = state.current_time + task_type.interarrival_dist.GetSample(rng);
					state.Schedule(trigger_time, JobArrival(task_type));
				}
			}
			return state;
		}

		MDP::MDP(const VarGroup& config)
		{
			config.Get("resource_types", resource_types);
			config.Get("task_types", task_types);

			if (task_types.size() == 0)
				throw DynaPlex::Error("resource_allocation:: not a single task type.");
			if (resource_types.size() == 0)
				throw DynaPlex::Error("resource_allocation:: not a single resource type.");

			for (auto& task_type : task_types)
			{
				total_arrival_rate += task_type.arrival_rate;
				for (auto& [next_task_key, next_task] : task_type.next_tasks)
				{
					next_task.task_type_index = task_types.index_for(next_task_key);
				}
				for (auto& [resource_key, resource_option] : task_type.resource_options)
				{
					resource_option.task_type_index = task_type.index;
					resource_option.resource_type_index = resource_types.index_for(resource_key);
					//store a copy of all resource_options in one long list, for ease of iterating and indexing.
					actions.push_back(resource_option);
				}
			}
		}

		//this completes the TaskSequence, assuming that that the next_task_type is given/
		void MDP::FinishTaskSequence(const TaskType& upcoming_task_type, DynaPlex::Queue<std::int64_t>& TaskSequence, DynaPlex::RNG& rng) const
		{
			//if this is part of task sequence, add it to back.
			TaskSequence.push_back(upcoming_task_type.index);
			double rand = rng.genUniform();
			//now determine which task_type is next, given the probabilities:
			for (auto& [key, task] : upcoming_task_type.next_tasks)
			{
				if (rand < task.probability)
				{	//this means that this task is the NextTask
					auto& task_type = task_types[task.task_type_index];
					FinishTaskSequence(task_type, TaskSequence, rng);
					return;
				}
				else
				{
					//subtract probability and continue to search for next task:
					rand -= task.probability;
				}
			}
			if (rand < upcoming_task_type.finishing_probability)
				//this means that there is no next task anymore, return;
			{
				return;
			}
			else
				throw DynaPlex::Error("TaskType:: FinishTaskType - error in logic.");
		}


		void MDP::GetFeatures(const State& state, DynaPlex::Features& features)const {
			//note that otherwise it throws as it cannot find the features during MDP construction. 
			features.Add(state.unassigned_tasks);
		}

		void MDP::RegisterPolicies(DynaPlex::Erasure::PolicyRegistry<MDP>& registry) const
		{
			registry.Register<ShortestProcessingTime>("shortest_processing_time", "Always give priority to job with shortest expected processing time in next step.");
		}

		DynaPlex::StateCategory MDP::GetStateCategory(const State& state) const
		{
			return state.cat;
		}

		bool MDP::IsAllowedAction(const State& state, int64_t action_index) const {
			if (action_index == actions.size())
				//means do nothing, which is always allowed. 
				return true;
			auto& action = actions.at(action_index);
			return state.unassigned_resources.at(action.resource_type_index) > 0 &&
				state.unassigned_tasks.at(action.task_type_index) > 0;
		}


		void Register(DynaPlex::Registry& registry)
		{
			DynaPlex::Erasure::MDPRegistrar<MDP>::RegisterModel("resource_allocation",
				"a resource allocation problem, mostly in line with ``Learning policies for resource allocation in business processes'', Middelhuis et al. Cost rate/objective: minimize average number of jobs in the system in steady state. Time is measured in number of job arrivals. ", registry);
		}
		void MDP::ResetHiddenStateVariables(State& state, DynaPlex::RNG& rng) const
		{
			for (auto& [id, job] : state.jobs)
			{
				auto& ongoing_task_type = task_types[job.task_sequence_indices.front()];
				job.task_sequence_indices.clear();
				FinishTaskSequence(ongoing_task_type, job.task_sequence_indices, rng);
			}
			for (auto& event : state.scheduled_event_queue)
			{
				switch (event.PayLoadIndex())
				{
				case ScheduledEvent::IndexOf<JobArrival>():
				{
					auto& jobarrival = event.get<JobArrival>();
					//geometric, so memoryless. Knowing that the job hasn't arrived by now
					//we know remaining time is again geometric. So because of memoryless property we can simply do:
					int64_t trigger_time = state.current_time + task_types[jobarrival.tasktype_index].interarrival_dist.GetSample(rng);
					event.trigger_time = trigger_time;
					break;
				}
				case ScheduledEvent::IndexOf<TaskCompletion>():
				{
					auto& task_completion = event.get<TaskCompletion>();
					auto& assignment = state.assignments[task_completion.assignment_id];
					int64_t time_passed = state.current_time - assignment.assignment_time;
					auto& resource_option = actions.at(task_completion.action_index);

					int64_t new_trigger_time = assignment.assignment_time +
						resource_option.duration_dist.GetConditionalSample(rng, time_passed);
					event.trigger_time = new_trigger_time;
					break;
				}
				case ScheduledEvent::IndexOf<NewTaskAssignment>():
					//nothing to do
					break;
				default:
					throw DynaPlex::Error("MDP::ResetHiddenStateVariables : case not covered.");
				}
			}
			state.scheduled_event_queue.resort();
		}

	}
}

