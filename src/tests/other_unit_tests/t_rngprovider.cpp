#include <gtest/gtest.h>
#include "dynaplex/rngprovider.h"
#include "dynaplex/error.h"

namespace DynaPlex::Tests {
	

	TEST(rngprovider, basics) {
		DynaPlex::RNGProvider provider(2);
		DynaPlex::RNGProvider provider2(2);

		provider.SeedEventStreams({ 2,3 });
		provider2.SeedEventStreams({ 2,3 });

		auto& rng = provider.GetEventRNG(1);
		auto& rng2 = provider2.GetEventRNG(1);
	
		std::vector<double> vec{};
		for (size_t i = 0; i < 3; i++)
		{//same seed results in same event stream. 
			double d = rng.genUniform();
			vec.push_back(d);
			ASSERT_EQ(d, rng2.genUniform());
		}
		//reset to initial value. 
		provider.SeedEventStreams({ 2,3 });

		auto rng3 = provider.GetEventRNG(1);
		for (size_t i = 0; i < 3; i++)
		{
			double d = vec[i];
			ASSERT_EQ(d, rng3.genUniform());
		}

		DynaPlex::RNGProvider provider3{};

		//Not seeded. 
		ASSERT_THROW(provider3.GetInitiationRNG(), DynaPlex::Error);

		
		provider3.SeedEventStreams({ 2,3 });

		{
			auto& rng3 = provider.GetInitiationRNG();
			auto& rng4 = provider2.GetInitiationRNG();
			for (size_t i = 0; i < 100; i++)
			{				
				ASSERT_EQ(rng3.genUniform(), rng4.genUniform());
			}
		}

	}
}