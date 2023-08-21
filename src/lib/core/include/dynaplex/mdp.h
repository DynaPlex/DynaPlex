#pragma once
#include <memory>
#include <string>
namespace DynaPlex
{
	class MDPInterface
	{
	public:
		virtual std::string Identifier() = 0;
	};
	using MDP = std::shared_ptr<MDPInterface>;
}//namespace DynaPlex