#include "dynaplex/factories.h"
#include "dynaplex/registry.h"
namespace DynaPlex {
	DynaPlex::MDP GetMDP(const DynaPlex::VarGroup& vars) {
		return DynaPlex::Registry::GetMDP(vars);
	}
	DynaPlex::VarGroup ListMDPs() {
		return DynaPlex::Registry::ListMDPs();
	}
}