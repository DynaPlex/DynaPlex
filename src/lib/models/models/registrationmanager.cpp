#include "dynaplex/models/registrationmanager.h"
#include "dynaplex/registry.h"

namespace DynaPlex::Models {
	//forward declarations of the registration functions of MDPs:
	namespace lost_sales {
		void Register(DynaPlex::Registry&);
	}
	namespace bin_packing {
		void Register(DynaPlex::Registry&);
	}
	namespace order_picking {
		void Register(DynaPlex::Registry&);
	}
	namespace perishable_systems {
		void Register(DynaPlex::Registry&);
	}
	namespace dual_sourcing {
		void Register(DynaPlex::Registry&);
	}
	void RegistrationManager::RegisterAll(DynaPlex::Registry& registry) {
		lost_sales::Register(registry);
		bin_packing::Register(registry);
		order_picking::Register(registry);
		perishable_systems::Register(registry);
		dual_sourcing::Register(registry);
	}
}
