#include "dynaplex/modelling/discretedist.h"
#include "dynaplex/modelling/queue.h"
#include "dynaplex/rng.h"

namespace DynaPlex::Models {
	namespace LostSales /*keep this in line with id below*/
	{

		class MDP
		{
			const DynaPlex::VarGroup varGroup;
			double p, h;
			int64_t leadtime;
			int64_t MaxOrderSize;
			int64_t MaxSystemInv;			
			DynaPlex::DiscreteDist demand_dist;
		
		public:

			using Event = int64_t;
			struct State {
				int64_t state;

				DynaPlex::VarGroup ToVarGroup() const
				{
					DynaPlex::VarGroup vars;
					vars.Add("state", state);
					return vars;
				}
			};

			bool AwaitsAction(const State& state) const;
			bool IsAllowedAction(const State& state, int64_t action) const;

			DynaPlex::VarGroup GetStaticInfo() const;

			double ModifyStateWithAction(State& state, int64_t action) const;
			double ModifyStateWithEvent(State& state, const Event& event) const;
			Event GetEvent(DynaPlex::RNG& rng) const;

			//something to extract features. 
			State GetInitialState() const;

			explicit MDP(const DynaPlex::VarGroup& varGroup);
		};
	}
}

