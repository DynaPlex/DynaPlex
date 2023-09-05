#pragma once
#include "dynaplex/statecontext.h"
#include <memory>

namespace DynaPlex {
	class StateBase {
	public:
		int64_t mdp_int_hash;
	protected:
		StateBase(int64_t hash_value) : mdp_int_hash(hash_value) {}
	
	public:
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