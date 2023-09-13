#pragma once
#include <memory>
#include <string>
#include "trajectory.h"
#include "vargroup.h"
namespace DynaPlex
{
	class PolicyInterface
	{
	public:
		virtual std::string Identifier() const = 0;

		virtual void SetActions(std::vector<Trajectory>&) const = 0;


	};
	using Policy = std::shared_ptr<PolicyInterface>;
}//namespace DynaPlex