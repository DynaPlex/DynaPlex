#pragma once
#include "mdpinterface.h"
#include <memory>
namespace DynaPlex
{
	using MDP = std::shared_ptr<MDPInterface>;
}//namespace DynaPlex