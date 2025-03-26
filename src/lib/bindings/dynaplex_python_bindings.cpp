#include <pybind11/pybind11.h>
#include <torch/torch.h>

//forward declarations - functions defined in respective .cpp files:
void define_state_bindings(pybind11::module_& m);
void define_trajectory_bindings(pybind11::module_& m);
void define_policy_bindings(pybind11::module_& m);
void define_mdp_bindings(pybind11::module_& m);
void define_mdp_numpy_bindings(pybind11::module_& m);
void define_provider_bindings(pybind11::module_& m);
void define_comparer_bindings(pybind11::module_& m);
void define_dcl_bindings(pybind11::module_& m);
void define_gym_emulator_bindings(pybind11::module_& m);
void define_sample_generator_bindings(pybind11::module_& m);
void define_demonstrator_bindings(pybind11::module_& m);
	
PYBIND11_MODULE(DP_Bindings, m) {
	m.doc() = "DynaPlex extension for Python";	
	// Expose the PolicyInterface declared in policy_bindings.h
	define_state_bindings(m);
	define_trajectory_bindings(m);
	define_policy_bindings(m);
	define_mdp_bindings(m);
	define_mdp_numpy_bindings(m);
	define_comparer_bindings(m);
	define_dcl_bindings(m);
	define_gym_emulator_bindings(m);
	define_sample_generator_bindings(m);
	define_demonstrator_bindings(m);
	define_provider_bindings(m);

}