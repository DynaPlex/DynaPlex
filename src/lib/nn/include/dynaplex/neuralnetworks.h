#pragma once
#include <string>
namespace DynaPlex::NeuralNetworks {
	bool TorchAvailable();

	std::string TorchVersion();
}//namespace DynaPlex