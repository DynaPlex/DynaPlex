#pragma once
#include <memory>
#include <string>
#include <span>
#include "trajectory.h"
#include "vargroup.h"
namespace DynaPlex
{
	/**
	 * DynaPlex algorithms access policies through this interface. For custom policies for specific MDPs, there 
	 * is no need to manually implement this interface. Instead the DynaPlex accepts a duck-typed specific policy 
	 * (see examples under models/models.)
	 * that adheres to an informal contract, and will adapt this policy to implement PolicyInterface.
	 */
	class PolicyInterface
	{
	public:
		/**
		 * Return an identifier of the underlying type of this MDP.
		 */
		virtual std::string TypeIdentifier() const = 0;
		/**
		 * Returns the configuration of this policy. 
		 */
		virtual const DynaPlex::VarGroup& GetConfig() const = 0;
		/// sets the actions if all trajectories in the span/vector have category IsAwaitAction(), throws otherwise.
		virtual void SetAction(std::span<Trajectory>) const = 0;


	};
	using Policy = std::shared_ptr<PolicyInterface>;
}//namespace DynaPlex