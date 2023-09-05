#pragma once
#include "dynaplex/statecontext.h"
#include <memory>

namespace DynaPlex {

	class TrajectoryBase {
	protected:
		int64_t mdp_int_hash;
		TrajectoryBase(int64_t hash_value) : mdp_int_hash(hash_value) {}

	public:
		StateContext context;

	protected: 
		virtual ~TrajectoryBase() = default;
		virtual std::unique_ptr<TrajectoryBase> Clone() const = 0;
	};

	using Trajectory = std::unique_ptr<TrajectoryBase>;
}