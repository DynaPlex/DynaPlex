#pragma once
#include <memory>
#include <string>
#include "vargroup.h"
namespace DynaPlex {
	class StateBase {
	public:
		int64_t mdp_int_hash;
	protected:
		StateBase(int64_t hash_value) : mdp_int_hash(hash_value) {}
	
	public:


		std::string ToString(int indent = -1) {
			return this->ToVarGroup().Dump(indent);
		}
		/**
		 * Converts the state to a VarGroup. Intended use is for display, illustration and/or debug purposes.
		 * Avoid calling this function in tight loops. 
		 */
		virtual VarGroup ToVarGroup() const = 0;
		virtual ~StateBase() = default;
		virtual std::unique_ptr<StateBase> Clone() const = 0;
	};

	//A DynaPlex::State can be cloned, and moved, but not copied
	//Difference is that with State other = std::move(state);
	//state will come unusable.
	//with State other = state->Clone();
	//a copy is made and both other and state will be separate copies that can be separately
	// modified
	//and context which can be separately modified. 
	using dp_State = std::unique_ptr<StateBase>;
}