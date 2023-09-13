#pragma once
#include <cstdint>
#include <vector>
#include "vargroup.h"
#include "statecategory.h"
#include "rngprovider.h"
#include "state.h"
namespace DynaPlex {

	struct Trajectory {
		/// used by MDPAdapter when IncorporateActions is called without argument.
	    int64_t NextAction;
		/**
		  * The Category associated with the current state; kept up to date when making calls to MDP->func(Trajectories, ...)
		  * Do not manually change this. 
	   	  */
		StateCategory Category;
		/// count of the number of events since the last initiation/reset. 
		int64_t EventCount;
		/// count of the number of actions since the last initiation/reset.
		int64_t ActionCount;
		/// cumulative return obtained since the last initiation/reset. 
		double CumulativeReturn;
	private:
		/// (type-erased) state of the system.
		DynaPlex::dp_State state;
	public:
		DynaPlex::dp_State& GetState()
		{
			if (state)
			{
				return state;
			}
			else
				throw DynaPlex::Error("Trajectory: Attempting to get state that has not been initialized. ");
		}
		/// moves the state into the trajectory. 
		void ReplaceState(DynaPlex::dp_State&& State)
		{
			state = std::move(State);
		}
		/// provider of random sequences for use in MDP. May be reset by calling Trajectory::Reset. 
		RNGProvider RNGProvider;
		/**
		 * Convenience member that may be used to store the (index of) an object in a container that contains more information
		 * on this trajectory, like initial information or functions to call when the trajectory completes.
		 */
		int64_t ExternalIndex;	
	


		/// resets the RNGProvider, to potentially ensure consistent random numbers when comparing things.
		void Reset(const uint32_t& experiment_number, const uint32_t& world_rank, const bool& eval)
		{
			RNGProvider.Reset(constructseeds(experiment_number, world_rank, eval));
		}

		/**
		 * Creates a trajectory that supports NumEventRNGs, without initial state.
		 * external_index is some context_defined item that is not muted by MDPAdapter. 
		 */
		Trajectory(int64_t NumEventRNGs, int64_t externalIndex =0) :
			NextAction{},
			Category{},
			EventCount{ 0 },
			ActionCount{ 0 },
			CumulativeReturn{ 0.0 },
			state{},
			RNGProvider(NumEventRNGs),
			ExternalIndex{ externalIndex }
		{}
		/// converts Trajectory to VarGroup
		VarGroup ToVarGroup() const;

	private:
		std::vector<uint32_t> constructseeds(const uint32_t& experiment_number, const uint32_t& world_rank, const bool& eval);
		
	};
}