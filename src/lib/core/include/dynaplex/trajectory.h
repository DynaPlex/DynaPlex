#pragma once
#include <cstdint>
#include <vector>
#include "vargroup.h"
#include "statecategory.h"
#include "rngprovider.h"
#include "state.h"
#include "system.h"



namespace DynaPlex {
	struct Trajectory {
		/// used by MDPAdapter when IncorporateActions is called without argument.
	    int64_t NextAction;
		/**
		  * The Category associated with the current state; kept up to date when making calls to MDP->func(Trajectories, ...)
		  * Do not manually change this. 
	   	  */
		StateCategory Category;
		/**
		  * counts the number of periods since the last initiation/reset. Be default, the period is increased whenever an
		  * event is incorporated, and the index of the StateCategory is 0. 
		  * Automatically kept up-to-date with calls to MDP->func(Trajectories, ...). 
		  * Do not manually change this, unless you know what you are doing. 
		  */
		int64_t PeriodCount;
		/**
	      * Effective discount factor based on the number of events since initiation / last reset. 
		  * Multiplied by the DiscountFactor before each event: Automatically kept up-to-date with calls to MDP->func(Trajectories, ...).
          * Do not manually change this, unless you know what you are doing. 
          */
		double EffectiveDiscountFactor;

		/**
	      * Cumulative return since initiation/last reset. 
	      * Automatically kept up-to-date with calls to MDP->func(Trajectories, ...).
	      * Do not manually change this.
	      */
		double CumulativeReturn;

		// Move constructor
		Trajectory(Trajectory&& other) noexcept = default;

		// Move assignment operator
		Trajectory& operator=(Trajectory&& other) noexcept = default;


		// Copy constructor
		Trajectory(const Trajectory& other)
			: NextAction(other.NextAction),
			Category(other.Category),
			PeriodCount(other.PeriodCount),
			EffectiveDiscountFactor(other.EffectiveDiscountFactor),
			CumulativeReturn(other.CumulativeReturn),
			RNGProvider(other.RNGProvider),
			ExternalIndex(other.ExternalIndex) {
			if (other.HasState()) {
				state = other.state->Clone();  // Use the clone method for dp_State
			}
		}

		// Copy assignment operator
		Trajectory& operator=(const Trajectory& other) {
			if (this != &other) {  // self-assignment check
				NextAction = other.NextAction;
				Category = other.Category;
				PeriodCount = other.PeriodCount;
				EffectiveDiscountFactor = other.EffectiveDiscountFactor;
				CumulativeReturn = other.CumulativeReturn;
				RNGProvider = other.RNGProvider;
				ExternalIndex = other.ExternalIndex;
				if (other.HasState()) {
					state = other.state->Clone();  // Use the clone method for dp_State
				}
				else {
					state.reset();
				}
			}
			return *this;
		}

	private:
		/// (type-erased) state of the system.
		DynaPlex::dp_State state;
	public:
		DynaPlex::dp_State& GetState()
		{
			if (state)
				return state;
			else
				throw DynaPlex::Error("Trajectory: Attempting to get state that has not been initialized. ");
		}

		bool HasState() const
		{
			return static_cast<bool>(state);
		}

		const DynaPlex::dp_State& GetState() const
		{
			if (state)
				return state;
			else
				throw DynaPlex::Error("Trajectory: Attempting to get state that has not been initialized. ");
		}

		void DeleteState()
		{
			state.reset(); 			
		}

		/// moves the state into the trajectory, and re-initiates CumulativeReturn, EffectiveDiscountFactor, and PeriodCount. 
		void Reset(DynaPlex::dp_State&&);
		
		/// re-initiates PeriodCount, EffectiveDiscountFactor, CumulativeReturn.  
		void Reset();
		/// provider of random sequences for use in MDP. 
		DynaPlex::RNGProvider RNGProvider;
		/**
		 * Convenience member that may be used to store the (index of) an object in a container that contains more information
		 * on this trajectory, like initial information or functions to call when the trajectory completes.
		 */
		int64_t ExternalIndex;	
	
		/**
		 * Creates a trajectory without initial state, and with uninitialized RNGProvider.
		 * must set initial state and seed RNGProvider before using. 
		 * external_index is some context_defined item that is not muted by MDPAdapter. 
		 */
		explicit Trajectory(int64_t externalIndex = 0);



	
	};
}