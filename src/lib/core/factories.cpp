#include "DynaPlex/Factories.h"
#include "dynaplex/registry.h"
namespace DynaPlex {
	DynaPlex::MDP GetMDP(const DynaPlex::VarGroup& vars) {
		return DynaPlex::Models::Registry::GetMDP(vars);
	}
	DynaPlex::VarGroup GetListOfMDPs() {
		return DynaPlex::Models::Registry::MDPList();
	}
}