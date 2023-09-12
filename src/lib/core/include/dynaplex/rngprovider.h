#pragma once
#include <initializer_list>
#include <random>

namespace DynaPlex {
	class RNGProvider {
	public:
		RNG& Get(int64_t number)
		{

		}
		

	private:
		RNGProvider(std::initializer_list<int64_t> list)
		{
			
		}

		std::initializer_list<int64_t> list;

	};
}

