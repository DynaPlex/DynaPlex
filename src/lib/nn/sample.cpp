#include "dynaplex/sample.h"

namespace DynaPlex::NN {

	Sample::Sample(int64_t action_label, DynaPlex::dp_State state)
		: action_label(action_label), state(std::move(state))
	{
	}
	DynaPlex::VarGroup Sample::ToVarGroup() const
	{
		//note: loading logic is in sampledata.cpp
		DynaPlex::VarGroup vars;
		vars.Add("action_label", action_label);
		vars.Add("sample_number", sample_number);
		vars.Add("state", state->ToVarGroup());
		return vars;
		
	}

}

