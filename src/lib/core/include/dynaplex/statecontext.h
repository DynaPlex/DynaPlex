
namespace DynaPlex {
	struct StateContext {
		int64_t NextAction;
		double IncurredCost;
		StateContext() = default;
	};
}