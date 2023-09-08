#pragma once
#include "dynaplex/registry.h"

namespace DynaPlex::Models {
	class RegistrationManager {
	public:
		static void RegisterAll(DynaPlex::Registry& registry);
	};
}