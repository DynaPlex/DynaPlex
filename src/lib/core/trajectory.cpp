#include "dynaplex/trajectory.h"
namespace DynaPlex {
	VarGroup Trajectory::ToVarGroup() const
	{
		VarGroup vg{};
		vg.Add("CumulativeReturn", CumulativeReturn);
		vg.Add("NextAction", NextAction);
		return vg;
	}
}
