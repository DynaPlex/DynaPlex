#pragma once
#include <string>

namespace DynaPlex
{	
	class MDPInterface
	{
	public:
		virtual std::string Identifier() = 0;
	};
}//namespace DynaPlex