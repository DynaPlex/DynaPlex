#pragma once
#include "dynaplex/statecontext.h"
#include <memory>

namespace DynaPlex {
	class StateBase {
		const int64_t mdp_int_hash;
	protected:
		StateBase(int64_t hash_value) : mdp_int_hash(hash_value) {}
		StateBase(const StateBase&) = delete;
		StateBase& operator=(const StateBase&) = delete;


	protected: 
		virtual ~StateBase() = default;
		virtual std::unique_ptr<StateBase> Clone() const = 0;
	};

	//A DynaPlex::State can be cloned, and moved, but not copied
	//Difference is that with State other = std::move(state);
	//state will come unusable.
	//with State other = state->Clone();
	//a copy is made and both other and state will be separate copies that can be separately
	// modified 
	using State = std::unique_ptr<StateBase>;
}