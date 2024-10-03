#include "dynaplex/models/dual_sourcing/mdp.h"
#include "dynaplex/erasure/mdpregistrar.h"
#include "policies.h"


//Code by: Fabian Akkerman

namespace DynaPlex::Models {
	namespace dual_sourcing /*keep this in line with id below and with namespace name in header*/
	{
		VarGroup MDP::GetStaticInfo() const
		{
			VarGroup vars;
			//Needs to update later:
			vars.Add("valid_actions", decisionSpace);
			//This is the absolute maximum number of actions, limited by the maximum inventory position (S)
			// 
			// In case that batchSizeCM = 1, we can use:
			//C(n + k−1, k−1) = (n+k−1)!(k−1)!n!
			//Fact(sMax + 3 - 1) / (Fact(3 - 1) * Fact(sMax));

			//If batchSizeCM > 1, we can use a recursive formula
			return vars;
		}


		double MDP::ModifyStateWithEvent(State& state, const Event& event) const
		{
			int64_t Backorders = 0;
			if constexpr (!sampleParams)
			{
				Backorders = InstalledBase_[state.Inst] - state.cmPart.operatingItems - state.amPart.operatingItems;
			}
			else
			{
				Backorders = state.InstalledBase - state.cmPart.operatingItems - state.amPart.operatingItems;
			}
			

			//helper variables
			int64_t y_cm = std::min(Backorders + event.demand_cm + event.demand_am, state.cmPart.onHandStock);
			int64_t y_am = std::min(Backorders + event.demand_cm + event.demand_am - y_cm, state.amPart.onHandStock);
			
			int64_t A_cm = 0;
			if constexpr (!sampleParams)
			{
				if (state.cmPart.orderQuantities.size() >= leadtimeCM_[state.Inst])
				{
					A_cm = state.cmPart.orderQuantities[state.cmPart.orderQuantities.size() - leadtimeCM_[state.Inst] - 1];//the number of parts that may arrive
				}
			}
			else
			{
				if (state.cmPart.orderQuantities.size() >= state.leadtimeCM)
				{
					A_cm = state.cmPart.orderQuantities[state.cmPart.orderQuantities.size() - state.leadtimeCM - 1];//the number of parts that may arrive
				}
			}

			int64_t z_cm = std::min(Backorders + event.demand_cm + event.demand_am - y_cm - y_am, A_cm);

			int64_t A_am = 0;
			if constexpr (!sampleParams)
			{
				if (state.amPart.orderQuantities.size() >= leadtimeAM_[state.Inst])
				{
					A_am = state.amPart.orderQuantities[state.amPart.orderQuantities.size() - leadtimeAM_[state.Inst] - 1];//change this if review period == lead time
				}
			}
			else
			{
				if (state.amPart.orderQuantities.size() >= state.leadtimeAM)
				{
					A_am = state.amPart.orderQuantities[state.amPart.orderQuantities.size() - state.leadtimeAM- 1];//change this if review period == lead time
				}
			}
			
			int64_t z_am = std::min(Backorders + event.demand_cm + event.demand_am - y_cm - y_am - z_cm, A_am);

			//demand event, here we apply the state-dependendness
			state.cmPart.operatingItems += -event.demand_cm + y_cm + z_cm;
			state.amPart.operatingItems += -event.demand_am + y_am + z_am;

			state.cmPart.onHandStock += A_cm - y_cm - z_cm;
			state.amPart.onHandStock += A_am - y_am - z_am;

			double hCosts = 0.0;
			double bCosts = 0.0;
			double mCosts = 0.0;
			if constexpr (!sampleParams)
			{
				hCosts = holdingCosts_[state.Inst] * (state.cmPart.onHandStock + state.amPart.onHandStock);
				//bCosts = backorderCosts_[state.Inst] * std::max((meanFailureCM_[state.Inst] * (double)state.cmPart.operatingItems + meanFailureAM_[state.Inst] * (double)state.amPart.operatingItems) + (double)Backorders - (double)state.cmPart.onHandStock - (double)state.amPart.onHandStock, (double)0.0);
				bCosts = backorderCosts_[state.Inst] * (double)Backorders;
				//double backorderCosts = backorderCosts * std::max(meanFailureCM + event.demand_am - state.cmPart.onHandStock - state.amPart.onHandStock, (size_t)0);

				/*if (event.demand_am>0 || event.demand_cm>0)
				{
					mCosts += maintenanceCosts_[state.Inst];
				}*/

				mCosts = maintenanceCosts_[state.Inst] * (meanFailureCM_[state.Inst] * state.cmPart.operatingItems + meanFailureAM_[state.Inst] * state.amPart.operatingItems);

			}
			else
			{
				hCosts = state.holdingCosts * (state.cmPart.onHandStock + state.amPart.onHandStock);
				bCosts = state.backorderCosts * std::max((state.meanFailureCM * (double)state.cmPart.operatingItems + state.meanFailureAM * (double)state.amPart.operatingItems) + (double)Backorders - (double)state.cmPart.onHandStock - (double)state.amPart.onHandStock, (double)0.0);

				mCosts = state.maintenanceCosts* (state.meanFailureCM * state.cmPart.operatingItems + state.meanFailureAM * state.amPart.operatingItems);

			}

			//update inventory positions for dual index policy
			state.cmPart.InventoryPosition += -event.demand_cm + state.cmPart.orderQuantities[state.cmPart.orderQuantities.size() - 1] + state.amPart.orderQuantities[state.amPart.orderQuantities.size() - 1];
			if constexpr (!sampleParams)
			{
				state.amPart.InventoryPosition += -event.demand_am + state.amPart.orderQuantities[state.amPart.orderQuantities.size() - 1] + state.cmPart.orderQuantities[state.cmPart.orderQuantities.size() - (leadtimeCM_[state.Inst] - leadtimeAM_[state.Inst]) - 1];
			}
			else
			{
				state.amPart.InventoryPosition += -event.demand_am + state.amPart.orderQuantities[state.amPart.orderQuantities.size() - 1] + state.cmPart.orderQuantities[state.cmPart.orderQuantities.size() - (state.leadtimeCM - state.leadtimeAM) - 1];
			}

			//for single index policy
			state.lastdemand = event.demand_am + event.demand_cm;
			state.inventoryposition -= state.lastdemand;
			

			//after processing this event, we await an action.
			state.cat = StateCategory::AwaitAction();

			return hCosts + bCosts + mCosts;
		}

		double MDP::ModifyStateWithAction(MDP::State& state, int64_t action) const
		{
			//every review period (tau) we need to decide on ordering
			//note that we do not track the evolution of the installed base during the review period, i.e., compisiton might change during a time step
			
			//order
			int64_t orderCM = actionMatrix[action][0];//number of batches cm
			int64_t orderAM = actionMatrix[action][1];//number of items am

			if constexpr (!sampleParams)
			{
				state.cmPart.orderQuantities.push_back(orderCM * batchSizeCM[state.Inst]);
			}
			else
			{
				state.cmPart.orderQuantities.push_back(orderCM);
			}
			state.amPart.orderQuantities.push_back(orderAM);

			double pCosts = 0.0;
			if constexpr (!sampleParams)
			{
				pCosts = (piecePriceCM_[state.Inst] * orderCM * batchSizeCM[state.Inst]) + orderAM * piecePriceAM_[state.Inst];
			}
			else
			{
				pCosts = (state.piecePriceCM * orderCM) + orderAM * state.piecePriceAM;
			}
			if (orderCM > 0)
			{//fixed costs for ordering CM
				if constexpr (!sampleParams)
				{
					pCosts += orderCostsCM_[state.Inst];
				}
				else
				{
					pCosts += state.orderCostsCM;
				}
			}
			if (orderAM > 0)
			{//fixed costs for ordering AM
				if constexpr (!sampleParams)
				{
					pCosts += orderCostsCM_[state.Inst];
				}
				else
				{
					pCosts += state.orderCostsCM;
				}
			}

			//for single index policy
			state.inventoryposition += orderCM + orderAM;

			state.cat = StateCategory::AwaitEvent();

			return pCosts;//costs
		}

		DynaPlex::VarGroup MDP::PartInfoState::ToVarGroup() const
		{//for the nested struct in the state
			DynaPlex::VarGroup vars;
			vars.Add("operatingItems", operatingItems);
			vars.Add("onHandStock", onHandStock);
			vars.Add("InventoryPosition", InventoryPosition);
			vars.Add("sizeOfReplenishment", orderQuantities);
			//add any other state variables. 
			return vars;
		}

		DynaPlex::VarGroup MDP::State::ToVarGroup() const
		{
			DynaPlex::VarGroup vars;
			vars.Add("cat", cat);
			vars.Add("cmPart.operatingItems", cmPart.operatingItems);
			vars.Add("cmPart.onHandStock", cmPart.onHandStock);
			vars.Add("cmPart.InventoryPosition", cmPart.InventoryPosition);
			vars.Add("cmPart.orderQuantities", cmPart.orderQuantities);
			vars.Add("amPart.operatingItems", amPart.operatingItems);
			vars.Add("amPart.onHandStock", amPart.onHandStock);
			vars.Add("amPart.InventoryPosition", amPart.InventoryPosition);
			vars.Add("amPart.orderQuantities", amPart.orderQuantities);

			vars.Add("lastdemand", lastdemand);
			vars.Add("inventoryposition", inventoryposition);

			vars.Add("meanFailureCM", meanFailureCM);
			vars.Add("varFailureCM", varFailureCM);
			vars.Add("leadtimeCM", leadtimeCM);
			vars.Add("piecePriceCM", piecePriceCM);
			vars.Add("orderCostsCM", orderCostsCM);
			vars.Add("meanFailureAM", meanFailureAM);
			vars.Add("varFailureAM", varFailureAM);
			vars.Add("leadtimeAM", leadtimeAM);
			vars.Add("piecePriceAM", piecePriceAM);
			vars.Add("maintenanceCosts", maintenanceCosts);
			vars.Add("holdingCosts", holdingCosts);
			vars.Add("backorderCosts", backorderCosts);
			vars.Add("installedBase", InstalledBase);

			vars.Add("ref_meanFailureCM", ref_meanFailureCM);
			vars.Add("ref_varToMeanRatio", ref_varToMeanRatio);
			vars.Add("ref_meanFailureAM", ref_meanFailureAM);

			vars.Add("Inst", Inst);
			//add any other state variables. 
			return vars;
		}


		MDP::State MDP::GetState(const VarGroup& vars) const
		{
			State state{};			
			vars.Get("cat", state.cat);
			vars.Get("cmPart.operatingItems", state.cmPart.operatingItems);
			vars.Get("cmPart.onHandStock", state.cmPart.onHandStock);
			vars.Get("cmPart.InventoryPosition", state.cmPart.InventoryPosition);
			vars.Get("cmPart.orderQuantities", state.cmPart.orderQuantities);
			vars.Get("amPart.operatingItems", state.amPart.operatingItems);
			vars.Get("amPart.onHandStock", state.amPart.onHandStock);
			vars.Get("amPart.InventoryPosition", state.amPart.InventoryPosition);
			vars.Get("amPart.orderQuantities", state.amPart.orderQuantities);

			vars.Get("lastdemand", state.lastdemand);
			vars.Get("inventoryposition", state.inventoryposition);

			vars.Get("meanFailureCM", state.meanFailureCM);
			vars.Get("varFailureCM", state.varFailureCM);
			vars.Get("leadtimeCM", state.leadtimeCM);
			vars.Get("piecePriceCM", state.piecePriceCM);
			vars.Get("orderCostsCM", state.orderCostsCM);
			vars.Get("meanFailureAM", state.meanFailureAM);
			vars.Get("varFailureAM", state.varFailureAM);
			vars.Get("leadtimeAM", state.leadtimeAM);
			vars.Get("piecePriceAM", state.piecePriceAM);
			vars.Get("maintenanceCosts", state.maintenanceCosts);
			vars.Get("holdingCosts", state.holdingCosts);
			vars.Get("backorderCosts", state.backorderCosts);
			vars.Get("installedBase", state.InstalledBase);
			
			vars.Get("ref_meanFailureCM", state.ref_meanFailureCM);
			vars.Get("ref_varToMeanRatio", state.ref_varToMeanRatio);
			vars.Get("ref_meanFailureAM", state.ref_meanFailureAM);

			vars.Get("Inst", state.Inst);

			return state;
			//Note we have to Get all individual nested struct components
		}

		MDP::State MDP::GetInitialState(RNG& rng) const
		{			
			//we initialize everything to zero and rely on a warm-start
			State state{};
			state.cat = StateCategory::AwaitEvent();

			if constexpr (!sampleParams)
			{
				//first, we sample the instance number from an uniform dist.
				state.Inst = inst_no;
				// instance_probs.GetSample(rng);

				//new state vars for robust policy
				state.meanFailureCM = meanFailureCM_[state.Inst];
				state.varFailureCM = varFailureCM_[state.Inst];
				state.leadtimeCM = leadtimeCM_[state.Inst];
				state.piecePriceCM = piecePriceCM_[state.Inst];
				state.orderCostsCM = orderCostsCM_[state.Inst];
				state.meanFailureAM = meanFailureAM_[state.Inst];
				state.varFailureAM = varFailureAM_[state.Inst];
				state.leadtimeAM = leadtimeAM_[state.Inst];
				state.piecePriceAM = piecePriceAM_[state.Inst];
				state.maintenanceCosts = maintenanceCosts_[state.Inst];
				state.holdingCosts = holdingCosts_[state.Inst];
				state.backorderCosts = backorderCosts_[state.Inst];

				state.cmPart.operatingItems = InstalledBase_[state.Inst];
				for (int64_t i = 0; i <= leadtimeCM_[state.Inst] + 5; ++i) { state.cmPart.orderQuantities.push_back(0); }
				for (int64_t i = 0; i <= leadtimeAM_[state.Inst] + 5; ++i) { state.amPart.orderQuantities.push_back(0); }
			}
			else//when sampling params (EPL)
			{//drawn from uniform distribtuon with custom values set by ourselves from the EPL procedure

				state.Inst = 1000000;//to ensure an error is thrown in case of logic error
				state.ref_meanFailureCM = three_probs.GetSample(rng);
				state.meanFailureCM = meanFailureCM_[state.ref_meanFailureCM];
				state.leadtimeCM = leadtimeCM_[three_probs.GetSample(rng)];
				state.piecePriceCM = piecePriceCM_[two_probs.GetSample(rng)];
				state.leadtimeAM = leadtimeAM_[three_probs.GetSample(rng)];
				state.maintenanceCosts = maintenanceCosts_[two_probs.GetSample(rng)];

				state.ref_varToMeanRatio = two_probs.GetSample(rng);
				state.varFailureCM = (state.ref_varToMeanRatio + 1.01) * state.meanFailureCM;//var must be > mean
				state.varFailureAM = state.varFailureCM;

				std::vector<double> demand_{ 0.0003, 0.003, 0.01, 0.03 };
				double demand = demand_[four_probs.GetSample(rng)];
				state.InstalledBase = std::ceil(demand / state.meanFailureCM);

				state.holdingCosts = 0.2 * state.piecePriceCM / 365;
				state.orderCostsCM = std::pow(5, 2) * state.holdingCosts*365 / 2.0 * demand*365;//5=virtual batch size, such that order costs such that batch size =EOQ

				std::vector<double> failure_ratio{ 0.7, 1.0, 2.0 };
				state.ref_meanFailureAM = three_probs.GetSample(rng);
				state.meanFailureAM = failure_ratio[state.ref_meanFailureAM] * state.meanFailureCM;
				if (state.meanFailureAM >= state.varFailureAM)
				{
					state.varFailureAM = state.meanFailureAM + 0.01;//var must be > mean
				}

				std::vector<double> price_ratio{ 0.5, 1.0, 5.0 };
				state.piecePriceAM = price_ratio[three_probs.GetSample(rng)] * state.piecePriceCM;

				std::vector<double> fill_rates_{ 0.9,0.95,0.99 };
				double fr = fill_rates_[three_probs.GetSample(rng)];
				state.backorderCosts = fr * state.holdingCosts / (1.0 - fr);

				state.cmPart.operatingItems = state.InstalledBase;
				for (int64_t i = 0; i <= state.leadtimeCM + 5; ++i) { state.cmPart.orderQuantities.push_back(0); }
				for (int64_t i = 0; i <= state.leadtimeAM + 5; ++i) { state.amPart.orderQuantities.push_back(0); }	
			}

			//cm
			state.cmPart.onHandStock = 10;
			state.cmPart.InventoryPosition = 10;

			//am
			state.amPart.onHandStock = 0;
			state.amPart.operatingItems = 0;
			state.amPart.InventoryPosition = 0;

			state.lastdemand = 0;
			state.inventoryposition = 10;

			return state;
		}

		MDP::Event MDP::GetEvent(const State& state,RNG& rng) const {
		//Note that we consider state-dependent events
			int64_t demand_cm;
			int64_t demand_am;
			if (state.cmPart.operatingItems == 0)
			{
				demand_cm = 0 ;
			}
			else
			{
				if constexpr (!sampleParams)
				{
					demand_cm = failureProbs_cm_[state.Inst][state.cmPart.operatingItems].GetSample(rng);
				}
				else
				{
					demand_cm = GetSample(rng, failureProbs_cm[state.ref_meanFailureCM][state.ref_varToMeanRatio][state.ref_meanFailureAM][state.cmPart.operatingItems]);
				}
			}
			if (state.amPart.operatingItems == 0)
			{
				demand_am = 0;
			}
			else
			{
				if constexpr (!sampleParams)
				{
					demand_am = failureProbs_am_[state.Inst][state.amPart.operatingItems].GetSample(rng);
				}
				else
				{
					demand_am = GetSample(rng, failureProbs_am[state.ref_meanFailureCM][state.ref_varToMeanRatio][state.ref_meanFailureAM][state.amPart.operatingItems]);
				}
			}

			if (demand_cm>0 || demand_am>0)
			{
				int x = 1;
			}

			return Event(demand_cm, demand_am);
		}

		//std::vector<std::tuple<MDP::Event, double>> MDP::EventProbabilities() const
		//{
		//	//This is optional to implement. You only need to implement it if you intend to solve versions of your problem
		//	//using exact methods that need access to the exact event probabilities.
		//	//Note that this is typically only feasible if the state space if finite and not too big, i.e. at most a few million states.
		//	throw DynaPlex::NotImplementedError();
		//}


		void MDP::GetFeatures(const State& state, DynaPlex::Features& features)const {
			//add IP and comming pipeline orders (limted)

			features.Add(state.cmPart.operatingItems);
			features.Add(state.amPart.operatingItems);
			features.Add(state.cmPart.onHandStock);
			features.Add(state.amPart.onHandStock);
			features.Add(state.cmPart.InventoryPosition);
			features.Add(state.amPart.InventoryPosition);

			//new
			features.Add(state.meanFailureCM);
			features.Add(state.varFailureCM);
			features.Add(state.leadtimeCM);
			features.Add(state.piecePriceCM);
			features.Add(state.orderCostsCM);
			features.Add(state.meanFailureAM);
			features.Add(state.varFailureAM);
			features.Add(state.leadtimeAM);
			features.Add(state.piecePriceAM);
			features.Add(state.maintenanceCosts);
			features.Add(state.holdingCosts);
			features.Add(state.backorderCosts);
			features.Add(state.InstalledBase);

		}
		
		void MDP::RegisterPolicies(DynaPlex::Erasure::PolicyRegistry<MDP>& registry) const
		{//Here, we register any custom heuristics we want to provide for this 	
		 //On the generic DynaPlex::MDP constructed from this, these heuristics can be obtained
		 //in generic form using mdp->GetPolicy(VarGroup vars), with the id in var set
		 //to the corresponding id given below.
			registry.Register<DualIndexPolicy>("dual_index",
				"This is a subpolicy, obtained using an iterative procedure. This policy is partially based on Veeraraghavan and Scheller-Wolf (2008) ");

			registry.Register<SingleIndexPolicy>("single_index",
				"This is a subpolicy, obtained using an iterative procedure. This policy is partially based on Scheller-Wolf, Veeraraghavan, and van Houtum (2007) ");

			registry.Register<BaseStockPolicy>("base_stock",
				"This is a single sourcing (CM parts only) baseline ");
		}

		DynaPlex::StateCategory MDP::GetStateCategory(const State& state) const
		{
			//this typically works, but state.cat must be kept up-to-date when modifying states. 
			return state.cat;
		}

		bool MDP::IsAllowedAction(const State& state, int64_t action) const {
			if (action == 0) return true;//not ordering is always allowed

			////first, we need to know what an action means
			int64_t orderCM = actionMatrix[action][0];//number of items
			int64_t orderAM = actionMatrix[action][1];//number of items

			//int64_t Backorders = InstalledBase[state.Inst] - state.cmPart.operatingItems - state.amPart.operatingItems;
			//int64_t InventoryPosition = state.cmPart.InventoryPosition + state.amPart.InventoryPosition;

			if constexpr (!sampleParams)
			{
				if ((orderCM * batchSizeCM[state.Inst]) + orderAM > (sMax[state.Inst] - state.inventoryposition))
				{//cannot increase IP above sMax
					return false;
				}
			}
			else
			{
				if (orderCM  + orderAM > (5 - state.inventoryposition))
				{//cannot increase IP above sMax
					return false;
				}
			}

			//action allowed
			return true;
		}


		void Register(DynaPlex::Registry& registry)
		{
			DynaPlex::Erasure::MDPRegistrar<MDP>::RegisterModel("dual_sourcing", "A dual sourcing problem for a spare parts supply chain with additive manufacturing (3d printing)", registry);
		}


		MDP::MDP(const VarGroup& config)
		{
			if constexpr (!sampleParams)
			{
				config.Get("Instance", Instance);
				config.Get("reviewPeriod", reviewPeriod);
				config.Get("InstalledBase", InstalledBase_);
				config.Get("meanFailureCM", meanFailureCM_);
				config.Get("varFailureCM", varFailureCM_);
				config.Get("leadtimeCM", leadtimeCM_);
				config.Get("piecePriceCM", piecePriceCM_);
				config.Get("orderCostsCM", orderCostsCM_);
				config.Get("batchSizeCM", batchSizeCM);
				config.Get("meanFailureAM", meanFailureAM_);
				config.Get("varFailureAM", varFailureAM_);
				config.Get("leadtimeAM", leadtimeAM_);
				config.Get("piecePriceAM", piecePriceAM_);
				config.Get("maintenanceCosts", maintenanceCosts_);
				config.Get("holdingCosts", holdingCosts_);
				config.Get("backorderCosts", backorderCosts_);
				config.Get("sMax", sMax);
				config.Get("inst_no", inst_no);

				std::vector<double> probs(Instance.size(), 1.0 / Instance.size());
				instance_probs = DiscreteDist::GetCustomDist(probs);


				auto size_pmfs = std::max_element(InstalledBase_.begin(), InstalledBase_.end());//check that this takes no weird values!
				failureProbs_cm_ = std::vector<std::vector<DynaPlex::DiscreteDist>>(Instance.size(), std::vector<DynaPlex::DiscreteDist>(*size_pmfs + 1));
				failureProbs_am_ = std::vector<std::vector<DynaPlex::DiscreteDist>>(Instance.size(), std::vector<DynaPlex::DiscreteDist>(*size_pmfs + 1));


				for (int64_t i = 0; i < Instance.size(); i++)
				{
					if (i==12)
					{
						int x = 1;
					}
					//check input data
					if (varFailureCM_[i] < meanFailureCM_[i])
					{
						throw DynaPlex::Error("var CM must be bigger or equal to mean!");

					}
					if (varFailureAM_[i] < meanFailureAM_[i])
					{
						throw DynaPlex::Error("var AM must be bigger or equal to mean!");
					}
					if (leadtimeCM_[i] % reviewPeriod[i] != 0)
					{
						throw DynaPlex::Error("leadtime CM must be multiple of review period!");
					}
					if (leadtimeAM_[i] % reviewPeriod[i] != 0)
					{
						throw DynaPlex::Error("leadtime AM must be multiple of review period!");
					}

					//determine event probabailities upfront for every possible nC and nA
					//CM parts
					for (int64_t nc = 0; nc < InstalledBase_[i] + 1; nc++)
					{

						double avgFailuresCM = (double)nc * meanFailureCM_[i];
						double varFailuresCM = (double)nc * varFailureCM_[i];
						if (avgFailuresCM == varFailuresCM)
						{
							failureProbs_cm_[i][nc] = DiscreteDist::GetCustomDist(GetPoissonPMF((long)nc, avgFailuresCM));
						}
						else if (varFailuresCM > avgFailuresCM)
						{
							double p = 1.0 - (avgFailuresCM / varFailuresCM);
							double r = avgFailuresCM * ((1.0 - p) / p);
							failureProbs_cm_[i][nc] = DiscreteDist::GetCustomDist(GetNegBinPMF((long)nc, r, 1.0 - p));
						}
						else
						{
							std::cout << "var<mean";
						}

					}

					//AM parts
					for (int64_t na = 0; na < InstalledBase_[i] + 1; na++)
					{
						double avgFailuresAM = (double)na * meanFailureAM_[i];
						double varFailuresAM = (double)na * varFailureAM_[i];
						if (avgFailuresAM == varFailuresAM)
						{
							failureProbs_am_[i][na] = DiscreteDist::GetCustomDist(GetPoissonPMF((long)na, avgFailuresAM));
						}
						else if (varFailuresAM > avgFailuresAM)
						{
							double p = 1.0 - (avgFailuresAM / varFailuresAM);
							double r = avgFailuresAM * ((1.0 - p) / p);
							failureProbs_am_[i][na] = DiscreteDist::GetCustomDist(GetNegBinPMF((long)na, r, 1.0 - p));
						}
						else
						{
							std::cout << "var<mean";
						}

					}
				}//end Instances loop
			}
			else//sample parameters, rest is determined in initial state function (EPL)
			{//the value sbelow are examples, we use the EPL procedure detailed in the paper (external code) to set these
				InstalledBase_ = { 1 };
				meanFailureCM_ = { 0.001, 0.025, 0.01 };
				leadtimeCM_ = { 60, 120, 240 };
				piecePriceCM_ = { 1000, 10000 };
				leadtimeAM_ = { 2, 5, 10 };
				maintenanceCosts_ = { 250, 5000 };

				std::vector<double> twoprobs(2, 1.0 / 2.0);
				two_probs = DiscreteDist::GetCustomDist(twoprobs);
				std::vector<double> threeprobs(3, 1.0 / 3.0);
				three_probs = DiscreteDist::GetCustomDist(threeprobs);
				std::vector<double> fourprobs(3, 1.0 / 3.0);
				four_probs = DiscreteDist::GetCustomDist(fourprobs);

				

				//2 temporary variables, not used hereafter!
				std::vector<double> vartomeanratio_{ 1.0, 2.0};
				std::vector<double> failure_ratio{ 0.7, 1.0, 2.0 };

				//30 is the largest possible installedbase for our exp.
				failureProbs_cm = std::vector<std::vector<std::vector<std::vector<std::vector<double>>>>>(
					meanFailureCM_.size(), std::vector<std::vector<std::vector<std::vector<double>>>>(
						vartomeanratio_.size(), std::vector<std::vector<std::vector<double>>>(
							failure_ratio.size(), std::vector<std::vector<double>>(
								31, std::vector<double>(
									0.0
								)
							)
						)
					)
				);
				failureProbs_am = std::vector<std::vector<std::vector<std::vector<std::vector<double>>>>>(
					meanFailureCM_.size(), std::vector<std::vector<std::vector<std::vector<double>>>>(
						vartomeanratio_.size(), std::vector<std::vector<std::vector<double>>>(
							failure_ratio.size(), std::vector<std::vector<double>>(
								31, std::vector<double>(
									0.0
								)
							)
						)
					)
				);


				for (size_t mu_cm = 0; mu_cm < meanFailureCM_.size(); mu_cm++)
				{
					for (size_t var = 0; var < vartomeanratio_.size(); var++)
					{
						for (size_t mu_am = 0; mu_am < failure_ratio.size(); mu_am++)
						{
							//CM parts
							for (int64_t nc = 0; nc < 31; nc++)
							{

								double avgFailuresCM = (double)nc * meanFailureCM_[mu_cm];
								double varFailuresCM = (double)nc * (vartomeanratio_[var] + 0.01) * meanFailureCM_[mu_cm];
								if (avgFailuresCM == varFailuresCM)
								{
									failureProbs_cm[mu_cm][var][mu_am][nc] = GetPoissonPMF((long)nc, avgFailuresCM);
								}
								else if (varFailuresCM > avgFailuresCM)
								{
									double p = 1.0 - (avgFailuresCM / varFailuresCM);
									double r = avgFailuresCM * ((1.0 - p) / p);
									failureProbs_cm[mu_cm][var][mu_am][nc] = GetNegBinPMF((long)nc, r, 1.0 - p);
								}
								else
								{
									std::cout << "var<mean";
								}

							}

							//AM parts
							for (int64_t na = 0; na < 31; na++)
							{	
								double avgFailuresAM = (double)na * failure_ratio[mu_am] * meanFailureCM_[mu_cm];
								double var_am = (vartomeanratio_[var] + 0.01) * meanFailureCM_[mu_cm];
								if (failure_ratio[mu_am] * meanFailureCM_[mu_cm] >= var_am)
								{
									var_am = failure_ratio[mu_am] * meanFailureCM_[mu_cm] + 0.01;//var must be > mean
								}
								double varFailuresAM = (double)na * var_am;
								if (avgFailuresAM == varFailuresAM)
								{
									failureProbs_am[mu_cm][var][mu_am][na] = GetPoissonPMF((long)na, avgFailuresAM);
								}
								else if (varFailuresAM > avgFailuresAM)
								{
									double p = 1.0 - (avgFailuresAM / varFailuresAM);
									double r = avgFailuresAM * ((1.0 - p) / p);
									failureProbs_am[mu_cm][var][mu_am][na] = GetNegBinPMF((long)na, r, 1.0 - p);
								}
								else
								{
									std::cout << "var<mean";
								}

							}


						}
						
					}
				}
			}

			//decision space size
			//note that we only model a single decision space, as it is the same across all instances with sMax equal
			int64_t orderOptions[] = { 1,1}; //batchSizeCM
			int64_t n = sizeof(orderOptions) / sizeof(orderOptions[0]);
			int64_t sumDecision = 0;
			int64_t sMax_ = 5;
			for (int64_t sum = 0; sum < sMax_ + 1; sum++)
			{
				sumDecision += findPossibleWays(orderOptions, n, sum);
			}
			decisionSpace = sumDecision;

			actionMatrix = std::vector<std::vector<int64_t>>(decisionSpace, std::vector<int64_t>(2));
			int64_t iter = 0;
			for (int64_t i = 0; i <= sMax_; i++)
			{
				for (int64_t j = 0; j <= sMax_; j++)
				{
					if (i * 1 + j <= sMax_)//i*batchSizeCM
					{
						actionMatrix[iter][0] = i;
						actionMatrix[iter][1] = j;
						iter++;
					}
				}
			}



			//we may also have config arguments that are not mandatory, and the internal value takes on 
			// a default value if not provided. Use sparingly. 
			if (config.HasKey("discount_factor"))
				config.Get("discount_factor", discount_factor);
			else
				discount_factor = 1.0;

		}


		//Below are custom functions for this specific MDP
		//----------------------------------------------------------------------------------------------------------
		//----------------------------------------------------------------------------------------------------------
		int64_t MDP::GetSample(RNG& rng, std::vector<double> pmf) const 
		{
			// Generate a uniform random number between 0 and 1
			double randomValue = rng.genUniform();
			double cumulativeProbability = 0.0;
			for (size_t i = 0; i < pmf.size(); i++) {
				cumulativeProbability += pmf[i];
				if (randomValue < cumulativeProbability) {
					return static_cast<int64_t>(i);
				}
			}

			DynaPlex::Error("Mistake in sampling from demand distribution");
			return 0;
		}


		int64_t MDP::Fact(int64_t num) const
		{
			if (num > 1)
			{
				return num * MDP::Fact(num - 1);
			}
			else
			{
				return 1;
			}
		}

		std::vector<double> MDP::GetNegBinPMF(long uBound, double r, double p) const
		{
			if (p < 0 || p> 1.0 || r < 0.0 || uBound < 0)
			{
				std::cout << "wrong Neg Bin param";
			}
			std::vector<double> returnVec;
			returnVec.reserve(uBound);
			long i = 0;
			double sum = 0.0;
			while (i < uBound)
			{
				double factor = (std::tgamma(r + (double)i) / (Fact(i) * std::tgamma(r))) * std::pow(p, r) * std::pow(1.0 - p, (double)i);
				if (factor < 0.0 )
				{
					return returnVec;
				}
				sum += factor;
				returnVec.push_back(factor);
				i++;
			}
			if (uBound > 0)
			{
				returnVec.push_back(std::max(1.0 - sum, 0.0));
			}
			else
			{
				returnVec.push_back(1.0);
			}

			return returnVec;

		}

		int64_t MDP::BinomialCoefficient(const double n, const int64_t k) const
		{
			if (k < 1)
			{
				return 0;
			}
			std::vector<double> aSolutions(k);
			aSolutions[0] = n - k + 1;

			for (int64_t i = 1; i < k; ++i) {
				aSolutions[i] = aSolutions[i - 1] * (n - (double)k + 1.0 + (double)i) / (double)(i + 1);
			}

			return aSolutions[k - 1];
		}

		std::vector<double> MDP::GetPoissonPMF(long uBound, double rate) const
		{//rate<100
			std::vector<double> returnVec;

			returnVec.reserve(uBound);
			long i = 0;
			double factor = std::exp(-rate);
			double sum = 0.0;
			while (i < uBound)
			{
				if (factor < 0.0)
				{
					return returnVec;
				}
				sum += factor;
				returnVec.push_back(factor);
				factor *= rate / (++i);
			}
			if (uBound > 0)
			{
				if ((1.0 - sum) < 0.0)
				{
					returnVec.push_back(0.0);
				}
				else
				{
					returnVec.push_back(1.0 - sum);
				}
				
			}
			else
			{
				returnVec.push_back(1.0);
			}


			return returnVec;

		}

		int64_t MDP::findPossibleWays(int64_t coins[], int64_t n, int64_t sum) const
		{
			// If sum is 0 then there is 1 solution
			// (do not include any coin)
			if (sum == 0)
				return 1;

			// If sum is less than 0 then no
			// solution exists
			if (sum < 0)
				return 0;

			// If there are no coins and sum
			// is greater than 0, then no
			// solution exist
			if (n <= 0)
				return 0;

			// count is sum of solutions (i)
			// including coins[n-1] (ii) excluding coins[n-1]
			return findPossibleWays(coins, n - 1, sum)
				+ findPossibleWays(coins, n, sum - coins[n - 1]);
		}


	}
}

