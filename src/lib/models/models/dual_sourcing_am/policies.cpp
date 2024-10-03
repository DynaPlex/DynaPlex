#include "policies.h"
#include "dynaplex/models/dual_sourcing/mdp.h"
#include "dynaplex/error.h"
namespace DynaPlex::Models {
	namespace dual_sourcing /*keep this namespace name in line with the name space in which the mdp corresponding to this policy is defined*/
	{

		//MDP and State refer to the specific ones defined in current namespace
		DualIndexPolicy::DualIndexPolicy(std::shared_ptr<const MDP> mdp, const VarGroup& config)
			:mdp{ mdp }
		{
			config.Get("delta_", delta_);
			config.Get("orderupto_am_", orderupto_am_);
		}

		int64_t DualIndexPolicy::GetAction(const MDP::State& state) const
		{
			int64_t delta, orderupto_am;
			try
			{
				delta = delta_[mdp->inst_no];
				orderupto_am = orderupto_am_[mdp->inst_no];
			}
			catch (const std::exception&)
			{
				DynaPlex::Error("the policy config JSON does not contain a vector or the vector is not large enough!");
			}
			
			int64_t l = 0;
			if (!mdp->sampleParams)
			{
				l = mdp->leadtimeCM_[state.Inst] - mdp->leadtimeAM_[state.Inst];
			}
			else
			{
				l = state.leadtimeCM - state.leadtimeAM;
			}
			
			int64_t X_nl = 0;
			if (state.amPart.orderQuantities.size() > l)
			{
				X_nl = state.cmPart.orderQuantities[state.cmPart.orderQuantities.size() - l - 1];
			}

			int64_t	order_am = std::max((int64_t)0, orderupto_am - state.amPart.InventoryPosition - X_nl);
			int64_t	order_cm = std::max((int64_t)0, orderupto_am + delta - (state.cmPart.InventoryPosition + order_am));

			int64_t order_am_cap = std::min((int64_t)5, order_am);
			int64_t order_cm_cap = std::min((int64_t)5, order_cm);

			for (int64_t i = 0; i < mdp->actionMatrix.size(); i++)
			{
				if (mdp->actionMatrix[i][0] == order_cm_cap && mdp->actionMatrix[i][1] == order_am_cap)
				{
					return i;
				}
			}



			std::cout << order_cm << order_am << std::endl;

			throw DynaPlex::Error("Action in dual index policy not found");
		}

		SingleIndexPolicy::SingleIndexPolicy(std::shared_ptr<const MDP> mdp, const VarGroup& config)
			:mdp{ mdp }
		{
			//note that in the paper we do not report single index results, as double index is state-of-the-art
			config.GetOrDefault("delta", delta, 0);
			config.GetOrDefault("orderupto_cm", orderupto_cm, 0);
		}

		int64_t SingleIndexPolicy::GetAction(const MDP::State& state) const
		{
			int64_t order_am = 0;
			if (state.inventoryposition < (orderupto_cm-delta))
			{
				order_am = std::max((int64_t)0, state.lastdemand - delta);
			}
			int64_t order_cm = std::min(delta, state.lastdemand);

			int64_t Backorders = 0;
			if (!mdp->sampleParams)
			{
				Backorders = mdp->InstalledBase_[state.Inst] - state.cmPart.operatingItems - state.amPart.operatingItems;
			}
			else
			{
				Backorders = state.InstalledBase - state.cmPart.operatingItems - state.amPart.operatingItems;
			}
			int64_t InventoryPosition = state.cmPart.InventoryPosition + state.amPart.InventoryPosition;

			if (!mdp->sampleParams)
			{
				if (InventoryPosition + (order_cm * mdp->batchSizeCM[state.Inst]) + order_am > (mdp->sMax[state.Inst] - InventoryPosition))
				{//cannot increase IP above sMax
					order_am = mdp->sMax[state.Inst] - InventoryPosition;
					order_cm = 0;
				}
			}
			else
			{
				if (InventoryPosition + order_cm + order_am > (15 - InventoryPosition))
				{//cannot increase IP above sMax
					order_am = 15 - InventoryPosition;
					order_cm = 0;
				}
			}
			

			for (int64_t i = 0; i < mdp->actionMatrix.size(); i++)
			{
				if (mdp->actionMatrix[i][0] == order_cm && mdp->actionMatrix[i][1] == order_am)
				{
					return i;
				}
			}
			std::cout << delta << std::endl;
			std::cout << order_cm << order_am << std::endl;

			throw DynaPlex::Error("Action in single index policy not found");
		};

		BaseStockPolicy::BaseStockPolicy(std::shared_ptr<const MDP> mdp, const VarGroup& config)
			:mdp{ mdp }
		{
			config.GetOrDefault("orderupto_cm", orderupto_cm, 0);
		}

		int64_t BaseStockPolicy::GetAction(const MDP::State& state) const
		{

			int64_t order_am = 0;
			int64_t order_cm = 0;
			int64_t InventoryPosition = state.cmPart.InventoryPosition + state.amPart.InventoryPosition;

			order_cm = std::max((int64_t)0,orderupto_cm - InventoryPosition);
			if (order_cm == 0) { return 0; }
			
			if (!mdp->sampleParams)
			{
				if (InventoryPosition + (order_cm * mdp->batchSizeCM[state.Inst]) > (mdp->sMax[state.Inst] - InventoryPosition))
				{//cannot increase IP above sMax
					order_cm = std::max((int64_t)0, mdp->sMax[state.Inst] - InventoryPosition);
				}
			}
			else
			{
				if (InventoryPosition + order_cm  > (5 - InventoryPosition))
				{//cannot increase IP above sMax
					order_cm = std::max((int64_t)0, 5 - InventoryPosition);
				}
			}
			

			for (int64_t i = 0; i < mdp->actionMatrix.size(); i++)
			{
				if (mdp->actionMatrix[i][0] == order_cm && mdp->actionMatrix[i][1] == order_am)
				{
					return i;
				}
			}
			std::cout << order_cm << "," << order_am << std::endl;

			throw DynaPlex::Error("Action in base stock policy not found");
		};

	}
}