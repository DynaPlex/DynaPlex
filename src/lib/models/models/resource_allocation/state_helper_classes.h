#pragma once
#include "dynaplex/dynaplex_model_includes.h"
#include "dynaplex/modelling/discretedist.h"
#include "dynaplex/modelling/queue.h"
#include "dynaplex/modelling/scheduledevent.h"
#include "mdp_helper_classes.h"
#include <vector>

namespace DynaPlex::Models {
	namespace resource_allocation {
		struct Assignment;

		struct Job {
			//last task is current task. 
			DynaPlex::Queue<std::int64_t> task_sequence_indices;
			int64_t arrival_time;
			int64_t current_assignment_id{ -1 };

			//for purposes of visualization. 
			int64_t job_number{ 0 };
			Job() {}
			Job(const VarGroup& vg)
			{
				vg.Get("job_number", job_number);
				vg.Get("arrival_time", arrival_time);
				vg.Get("task_sequence_indices", task_sequence_indices);
				vg.GetOrDefault("current_assignment_id", current_assignment_id, -1);
			}
			VarGroup ToVarGroup() const
			{
				VarGroup vg{};	
				vg.Add("job_number", job_number);
				vg.Add("task_sequence_indices", task_sequence_indices);
				if(current_assignment_id>=0)
					vg.Add("current_assignment_id", current_assignment_id);
				vg.Add("arrival_time", arrival_time);
				return vg;
			}
		};
		struct Resource {
			int64_t resource_type_index;
			int64_t current_assignment_id{ -1 };

			Resource() {}
			Resource(const VarGroup& vg)
			{
				vg.Get("resource_type_index", resource_type_index);
				vg.GetOrDefault("current_assignment_id", current_assignment_id,-1);
			}
			VarGroup ToVarGroup() const
			{
				VarGroup vg{};
				vg.Add("resource_type_index", resource_type_index);
				if(current_assignment_id>=0)
					vg.Add("current_assignment_id", current_assignment_id);
				return vg;
			}
		};

		struct Assignment {
			int64_t job_id{ -1 }, resource_id{ -1 };
			int64_t assignment_time;

			Assignment() {}
			Assignment(const VarGroup& vg)
			{
				vg.Get("assignment_time", assignment_time);
				vg.Get("job_id", job_id);
				vg.Get("resource_id", resource_id);
			}
			VarGroup ToVarGroup() const
			{
				VarGroup vg{};
				vg.Add("assignment_time", assignment_time);
				vg.Add("job_id", job_id);
				vg.Add("resource_id", resource_id);
				return vg;
			}
		};

		

	
		//Event Payload Types:
		struct NewTaskAssignment
		{
			//store the action that was taken. 
			int64_t action_index;
			explicit NewTaskAssignment(const DynaPlex::VarGroup& vg)
			{
				vg.Get("action_index", action_index);
			}
			NewTaskAssignment(int64_t action_index=0):
				action_index{action_index}
			{
			}
			auto operator<=>(const NewTaskAssignment& other) const
			{
				return action_index <=> other.action_index;
			}
			DynaPlex::VarGroup ToVarGroup() const  {
				VarGroup vg;
				vg.Add("action_index", action_index);
				return vg;
			}
		};

		struct JobArrival
		{
			int64_t tasktype_index{-1};

			explicit JobArrival(const DynaPlex::VarGroup& vg)
			{
				vg.Get("tasktype_index", tasktype_index);
			}

			JobArrival(const TaskType& tasktype)
			{
				tasktype_index = tasktype.index;
			}
			auto operator<=>(const JobArrival& other) const
			{
				return tasktype_index <=> other.tasktype_index;
			}

			DynaPlex::VarGroup ToVarGroup() const {
				VarGroup vg;
				vg.Add("tasktype_index", tasktype_index);
				return vg;
			}
		};

		struct TaskCompletion
		{
			int64_t assignment_id{ -1 };
			int64_t action_index{ -1 };
			explicit TaskCompletion(const DynaPlex::VarGroup& vg)
			{
				vg.Get("assignment_id", assignment_id);
				vg.Get("action_index", action_index);
			}
			auto operator<=>(const TaskCompletion& other) const
			{//sort on assignment_id
				return assignment_id <=> other.assignment_id;
			}
			TaskCompletion(int64_t assignment_id, int64_t action_index)
			{
				this->assignment_id = assignment_id;
				this->action_index = action_index;
			}
			DynaPlex::VarGroup ToVarGroup() const {
				VarGroup vg;
				vg.Add("assignment_id", assignment_id);
				vg.Add("action_index", action_index);
				return vg;
			}
		};
		
		//the ScheduledEvent can hold any payload that is vargroupconvertible and that supports sorting via <=> operator.
		// in this case the possibly payloads are NewTaskAssignment, JobArrival, TaskCompletion. Events will be sorted first on
		//trigger_time, then on type of event (in the order of below list), and finally, to compare two events of the same type with the same trigger_time
		//using the comparison operator <=> of that class.  
		// 
		//Note that for technical reasons, the first element in the list must have a default constructor, i.e.
		//a constructor without arguments.
		using ScheduledEvent = ScheduledEventType<NewTaskAssignment, JobArrival, TaskCompletion>;
	}
}