#pragma once
#include <memory>
#include <string>
#include "dynaplex/state.h"
#include "vargroup.h"
#include "policy.h"
namespace DynaPlex
{
	class MDPInterface
	{
	public:
		/**
		 * Returns a unique identifier for this MDP.
		 * The identifier consists of the MDP id plus a hash of the parameters.
		 * MDPs with the exact same parameters in the same order will have the same identifier.
		 */
		virtual std::string Identifier() const = 0;

		/**
		 * Retrieves the initial state for this MDP.
		 * Note: States are unique for the DynaPlex::MDP instance from which they were created.
		 * Never attempt to call functions on one MDP with states from another MDP,
		 * even if the two MDPs have the same "id".
		 */
		virtual DynaPlex::dp_State GetInitialState() const = 0;

		/// Provides information about the MDP.
		virtual DynaPlex::VarGroup GetStaticInfo() const = 0;

		/// Returns a vector containing actions allowed in the provided state.
		virtual std::vector<int64_t> AllowedActions(const DynaPlex::dp_State&)const = 0;

		/**
		 * Converts the given state to VarGroup, typically for display purposes.
		 * Avoid calling this function in tight loops.
		 */
		virtual DynaPlex::VarGroup ToVarGroup(const DynaPlex::dp_State&) const = 0;

		virtual void IncorporateAction(DynaPlex::dp_State&) const = 0;

		/**
		 * Generates a policy specific to this MDP based on the provided VarGroup.
		 * Policies are NOT cross-compatible across MDPs, even generic policies like "random".
		 */
		virtual DynaPlex::Policy GetPolicy(const DynaPlex::VarGroup& vars) const = 0;

		/**
		 * Convenience function that calls GetPolicy(VarGroup) with the parameter { "id": id },
		 * i.e., providing only the id. Suitable for "random" and other generic policies,
		 * as well as client-provided policies that need no information beyond the id.
		 */
		virtual DynaPlex::Policy GetPolicy(const std::string& id) const = 0;

		/// Lists policies that can be obtained for this MDP
		virtual DynaPlex::VarGroup ListPolicies() const = 0;


		virtual ~MDPInterface() = default;
	};
	using MDP = std::shared_ptr<MDPInterface>;
}
