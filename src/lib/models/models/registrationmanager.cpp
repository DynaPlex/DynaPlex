#include "dynaplex/models/registrationmanager.h"
#include "dynaplex/registry.h"

namespace DynaPlex::Models {
	//forward declarations of the registration functions of various MDPs:
	namespace LostSales {
		void Register(DynaPlex::Registry&);
	}


	void RegistrationManager::RegisterAll(DynaPlex::Registry& registry) {
		LostSales::Register(registry);		
	}
}
