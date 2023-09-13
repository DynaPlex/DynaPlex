#pragma once
#include <memory>
#include <string>
#include <span>
#include "trajectory.h"
#include "vargroup.h"
namespace DynaPlex
{
	class PolicyInterface
	{
	public:
		virtual std::string Identifier() const = 0;
		/// sets the actions if all trajectories in the span/vector have category IsAwaitAction(), throws otherwise.
		virtual void SetActions(std::span<Trajectory>) const = 0;


	};
	using Policy = std::shared_ptr<PolicyInterface>;
}//namespace DynaPlex