#pragma once
#include "dynaplex/dynaplex_model_includes.h"
#include "dynaplex/modelling/discretedist.h"
#include "dynaplex/modelling/queue.h"
#include "dynaplex/modelling/scheduledevent.h"
#include "mdp_helper_classes.h"
#include <variant>

namespace DynaPlex::Models {
	namespace collaborative_picking {

		struct Picker {
			int64_t node_id{ -1 };
			//assigned when making the assignment, reset to -1 when pick completes or when unassigning.
			int64_t assigned_vehicle_id{ -1 };
			int64_t num_picks_started{ 0 };


			Picker() {}

			explicit Picker(const VarGroup& vg) {
				vg.GetOrDefault("node_id", node_id, -1);
				vg.GetOrDefault("assigned_vehicle_id", assigned_vehicle_id, -1);
				vg.GetOrDefault("num_picks_started", num_picks_started,0);
			}

			VarGroup ToVarGroup() const {
				VarGroup vg{};
				if (node_id >= 0)
					vg.Add("node_id", node_id);
				if (assigned_vehicle_id >= 0)
					vg.Add("assigned_vehicle_id", assigned_vehicle_id);
				if(num_picks_started!=0)
					vg.Add("num_picks_started", num_picks_started);
				return vg;
			}
		};
		struct Vehicle {
			DynaPlex::Queue<int64_t> remaining_pick_list{};
			
			//assigned when making the assignment, reset to -1 when pick completes (or when unassigning)
			int64_t assigned_picker{ -1 };
			int64_t node_id{ -1 };
			int64_t drop_off_truck_doc_index{ -1 };
			int64_t drop_off_node_id{ -1 };
	
			

			//Set when we enter queue for next node (without immediately firing the vehicle entry - i.e. without starting entry). Unset when vehicle entry is fired.
			bool is_blocked{ false };
			//Set when the last pick completes. Unset when a new pickrun starts.
			bool is_dropping_off{ false };
			//Set when the vehicle enters the pick location. Unset when pick completes. 
			bool at_pick_location{ false };
			//Set when pick starts. Unset when pick completes. 
			bool is_being_picked_for{ false };
		
			///returns whether this vehicle can be assigned (reassign=false) or reassigned (reassign=true) to a picker.
			bool IsAssignable(bool reassign) const
			{
				if (assigned_picker != -1 || remaining_pick_list.IsEmpty())
					return false;
				if (reassign)
					return at_pick_location;
				else
					return !is_dropping_off;
			}

			Vehicle() {}

			explicit Vehicle(const VarGroup& vg) {
				vg.Get("remaining_pick_list", remaining_pick_list);
				vg.GetOrDefault("assigned_picker", assigned_picker, -1);
				vg.GetOrDefault("node_id", node_id, -1);
				vg.GetOrDefault("drop_off_node_id", drop_off_node_id, -1);
				vg.GetOrDefault("drop_off_truck_doc_index", drop_off_truck_doc_index, -1);
				vg.GetOrDefault("is_blocked", is_blocked, false);
				vg.GetOrDefault("at_pick_location", at_pick_location, false);
				vg.GetOrDefault("is_being_picked_for", is_being_picked_for, false);
				vg.GetOrDefault("is_dropping_off", is_dropping_off, false);
			}

			VarGroup ToVarGroup() const {
				VarGroup vg{};
				vg.Add("remaining_pick_list", remaining_pick_list);
				if (assigned_picker >= 0)
					vg.Add("assigned_picker", assigned_picker);
				if (node_id >= 0)
					vg.Add("node_id", node_id);
				if (drop_off_truck_doc_index >= 0)
					vg.Add("drop_off_truck_doc_index", drop_off_truck_doc_index);
				if (drop_off_node_id >= 0)
					vg.Add("drop_off_node_id", drop_off_node_id);
				if (is_blocked)
					vg.Add("is_blocked", is_blocked);
				if (at_pick_location)
					vg.Add("at_pick_location", at_pick_location);
		    	if (is_being_picked_for)
					vg.Add("is_being_picked_for", is_being_picked_for);
				if (is_dropping_off)
					vg.Add("is_dropping_off", is_dropping_off);
				return vg;
			}
		};
		struct Node {

			DynaPlex::Queue<int64_t> node_entry_queue{};
			//mostly for features and graphical representation
			int64_t occupying_vehicle_id{ -1 };
			bool is_accessible{ true };

			Node() {}

			explicit Node(const VarGroup& vg) {
				if (vg.HasKey("node_entry_queue"))
					vg.Get("node_entry_queue", node_entry_queue);
				vg.GetOrDefault("occupying_vehicle_id", occupying_vehicle_id, -1);
				vg.GetOrDefault("is_accessible", is_accessible, true);
			}

			VarGroup ToVarGroup() const {
				VarGroup vg{};
				if(!node_entry_queue.IsEmpty())
					vg.Add("node_entry_queue", node_entry_queue);
				if (occupying_vehicle_id >= 0)
					vg.Add("occupying_vehicle_id", occupying_vehicle_id);
				if (!is_accessible)
					vg.Add("is_accessible", is_accessible);
				return vg;
			}
		};


		struct PickCompletes {
			int64_t picker_id;
			PickCompletes(const DynaPlex::VarGroup& vg)
			{
				vg.GetOrDefault("picker_id", picker_id,-1);
			}

			PickCompletes(int64_t picker_id) :
				picker_id{ picker_id }
			{

			}
			auto operator<=>(const PickCompletes& other) const
			{
				return picker_id <=> other.picker_id;
			}
			DynaPlex::VarGroup ToVarGroup() const {
				DynaPlex::VarGroup vg;
				if(picker_id>=0)
					vg.Add("picker_id", picker_id);
				return vg;
			}
		};		
		struct PickerArrival
		{
			int64_t picker_id;
			/// arrival node
			int64_t node_id;
			PickerArrival(const DynaPlex::VarGroup& vg)
			{
				vg.Get("picker_id", picker_id);
				vg.Get("node_id", node_id);
			}

			PickerArrival(int64_t picker_id, int64_t node_id) :
				picker_id{ picker_id }, node_id{ node_id }
			{

			}
            auto operator<=>(const PickerArrival& other) const
            {
	            auto cmp = picker_id <=> other.picker_id;
                if (cmp != 0) {
                    return cmp;
                }
                return node_id <=> other.node_id;
            }
			DynaPlex::VarGroup ToVarGroup() const {
				DynaPlex::VarGroup vg;
				vg.Add("picker_id", picker_id);
				vg.Add("node_id", node_id);
				return vg;
			}
		};	
		struct PickerAwaitsAction
		{
			int64_t picker_id;
			bool reassign;
			PickerAwaitsAction(const DynaPlex::VarGroup& vg)
			{
				vg.Get("picker_id", picker_id);
				vg.GetOrDefault("reassign", reassign, false);
			}
			PickerAwaitsAction(int64_t picker_id=-1,bool reassign=false) :
				picker_id{ picker_id }, reassign{reassign}
			{
			}
			auto operator<=>(const PickerAwaitsAction& other) const
			{
				auto comp = reassign <=> other.reassign;
				if (comp != 0)
					return comp;
				return picker_id <=> other.picker_id;
			}
			DynaPlex::VarGroup ToVarGroup() const {
				DynaPlex::VarGroup vg;
				vg.Add("picker_id", picker_id);
				if (reassign)
					vg.Add("reassign", reassign);
				return vg;
			}
		};
		struct MakeNodeAccessible
		{
			int64_t node_id;
			MakeNodeAccessible(const DynaPlex::VarGroup& vg)
			{
				vg.Get("node_id", node_id);
			}
			MakeNodeAccessible(int64_t node_id) :
				node_id{ node_id }
			{

			}
			auto operator<=>(const MakeNodeAccessible& other) const
			{
				return node_id <=> other.node_id;
			}
			DynaPlex::VarGroup ToVarGroup() const {
				DynaPlex::VarGroup vg;
				vg.Add("node_id", node_id);
				return vg;
			}
		};
		struct VehicleNodeEntry
		{
			int64_t node_id;

			VehicleNodeEntry(const DynaPlex::VarGroup& vg)
			{
				vg.Get("node_id", node_id);
			}
			VehicleNodeEntry(int64_t node_id) : node_id{ node_id }
			{

			}
			auto operator<=>(const VehicleNodeEntry& other) const
			{				
				return node_id <=> other.node_id;
			}
			DynaPlex::VarGroup ToVarGroup() const {
				DynaPlex::VarGroup vg;
				vg.Add("node_id", node_id);
				return vg;
			}
		};
		struct VehicleReadyForPick
		{
			int64_t vehicle_id;
			VehicleReadyForPick(const DynaPlex::VarGroup& vg)
			{
				vg.Get("vehicle_id", vehicle_id);
			}
			VehicleReadyForPick(int64_t vehicle_id) :
				vehicle_id{ vehicle_id }
			{

			}
			auto operator<=>(const VehicleReadyForPick& other) const
			{
				return vehicle_id <=> other.vehicle_id;
			}
			DynaPlex::VarGroup ToVarGroup() const {
				DynaPlex::VarGroup vg;
				vg.Add("vehicle_id", vehicle_id);
				return vg;
			}
		};
		
		struct ConfigurePickRun
		{
			int64_t vehicle_id;
			ConfigurePickRun(const DynaPlex::VarGroup& vg)
			{
				vg.Get("vehicle_id", vehicle_id);
			}
			ConfigurePickRun(int64_t vehicle_id) :
				vehicle_id{ vehicle_id }
			{

			}
			auto operator<=>(const ConfigurePickRun& other) const
			{
				return vehicle_id <=> other.vehicle_id;
			}
			DynaPlex::VarGroup ToVarGroup() const {
				DynaPlex::VarGroup vg;
				vg.Add("vehicle_id", vehicle_id);
				return vg;
			}
		};

		struct ChangeDistribution
		{
			int64_t distribution_id;
			ChangeDistribution(const DynaPlex::VarGroup& vg)
			{
				vg.Get("distribution_id", distribution_id);
			}
			ChangeDistribution(int64_t distribution_id) :
				distribution_id{ distribution_id }
			{

			}
			auto operator<=>(const ChangeDistribution& other) const
			{
				return distribution_id <=> other.distribution_id;
			}
			DynaPlex::VarGroup ToVarGroup() const {
				DynaPlex::VarGroup vg;
				vg.Add("distribution_id", distribution_id);
				return vg;
			}
		};

		struct TimeStep
		{
			TimeStep(const DynaPlex::VarGroup& vg)
			{
			}
			TimeStep()
			{
			}
			auto operator<=>(const TimeStep& other) const
			{
				return std::strong_ordering::equivalent;
			}

			DynaPlex::VarGroup ToVarGroup() const {
				return DynaPlex::VarGroup{};
			}
		};
		
		struct PickerCheckDeadlock
		{
			int64_t picker_id;
			int64_t old_vehicle_node_id;
			int64_t last_num_picks_started;
			PickerCheckDeadlock(const DynaPlex::VarGroup& vg)
			{
				vg.Get("picker_id", picker_id);
				vg.Get("old_vehicle_node_id", old_vehicle_node_id);
				vg.Get("last_num_picks_started", last_num_picks_started);
			}
			PickerCheckDeadlock(int64_t picker_id, int64_t old_vehicle_node_id, int64_t last_num_picks_started) :
				picker_id{ picker_id }, old_vehicle_node_id{ old_vehicle_node_id }, last_num_picks_started{ last_num_picks_started }
			{

			}
			auto operator<=>(const PickerCheckDeadlock& other) const
			{
				auto cmp = picker_id <=> other.picker_id;
				if (cmp != 0) {
					return cmp;
				}
				cmp = last_num_picks_started <=> other.last_num_picks_started;
				if (cmp != 0) {
					return cmp;
				}
				return old_vehicle_node_id <=> other.old_vehicle_node_id;
			}
			DynaPlex::VarGroup ToVarGroup() const {
				DynaPlex::VarGroup vg;
				vg.Add("picker_id", picker_id);
				vg.Add("old_vehicle_node_id", old_vehicle_node_id);
				vg.Add("last_num_picks_started", last_num_picks_started);
				return vg;
			}
		};

		using ScheduledEvent = ScheduledEventType<TimeStep, PickerAwaitsAction, PickerArrival, PickerCheckDeadlock, PickCompletes, MakeNodeAccessible, VehicleNodeEntry, VehicleReadyForPick, ConfigurePickRun, ChangeDistribution>;
	}
}