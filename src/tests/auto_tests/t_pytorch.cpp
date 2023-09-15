#include <iostream>
#include <gtest/gtest.h>
#include "dynaplex/neuralnetworks.h"

namespace DynaPlex::Tests {

	TEST(TestPyTorch, CheckAvailCorrect) {
		bool TorchAvailable =  DynaPlex::NeuralNetworks::TorchAvailable();

#if DP_TORCH_AVAILABLE
		ASSERT_TRUE(TorchAvailable);
#else
		ASSERT_FALSE(TorchAvailable);
#endif


	}
}