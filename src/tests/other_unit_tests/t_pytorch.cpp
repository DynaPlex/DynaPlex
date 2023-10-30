#include <iostream>
#include <gtest/gtest.h>
#include "dynaplex/torchavailability.h"

namespace DynaPlex::Tests {

	TEST(TestPyTorch, CheckAvailCorrect) {
		bool TorchAvailable =  DynaPlex::TorchAvailability::TorchAvailable();

#if DP_TORCH_AVAILABLE
		ASSERT_TRUE(TorchAvailable);
#else
		ASSERT_FALSE(TorchAvailable);
#endif


	}
}