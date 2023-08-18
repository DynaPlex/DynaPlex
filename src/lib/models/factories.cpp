#include "DynaPlex/Factories.h"
#include "dynaplex/mdpregistry.h"
namespace DynaPlex {
	DynaPlex::MDP GetMDP(const DynaPlex::VarGroup& vars) {
		return DynaPlex::Collections::MDPRegistry::GetMDP(vars);
	}
}
