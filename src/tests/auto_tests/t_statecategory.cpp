#include "dynaplex/vargroup.h"
#include "dynaplex/error.h"
#include <gtest/gtest.h>
#include "dynaplex/statecategory.h"


namespace DynaPlex::Tests {
	

	TEST(StateCategory, Basics) {
		{
			auto await_ac = DynaPlex::StateCategory::AwaitAction();
			auto await_ac_of_type = DynaPlex::StateCategory::AwaitAction(4);
			auto final = DynaPlex::StateCategory::Final();
			auto await_event_of_type = DynaPlex::StateCategory::AwaitEvent(2);


			EXPECT_FALSE(await_ac.IsFinal());
			EXPECT_FALSE(final.IsAwaitAction());
			EXPECT_FALSE(final.IsAwaitEvent());
			EXPECT_TRUE(await_ac_of_type.IsAwaitAction());
			EXPECT_TRUE(final.IsFinal());
			EXPECT_TRUE(await_event_of_type.IsAwaitEvent());

			EXPECT_NO_THROW(
				{
			switch (await_event_of_type.Index()) {
			case 1:
				throw DynaPlex::Error("error");
				break;
			case 2:

				break;
			default:
				throw DynaPlex::Error("error");
			}
				});

			EXPECT_EQ(await_ac_of_type.Index(), 4);

			EXPECT_EQ(await_ac.Index(), 0);
			EXPECT_EQ(final.Index(), 0);
		}
		{
			// Test ToVarGroup() and the VarGroup constructor
			auto await_ac = StateCategory::AwaitAction();
			auto await_ac_of_type = StateCategory::AwaitAction(4);
			auto final = StateCategory::Final();
			auto await_event_of_type = StateCategory::AwaitEvent(2);

			// Convert to VarGroup and back
			auto await_ac_vg = await_ac.ToVarGroup();
			StateCategory await_ac_from_vg(await_ac_vg);
			EXPECT_TRUE(await_ac_from_vg.IsAwaitAction());
			EXPECT_EQ(await_ac_from_vg.Index(), 0);

			auto await_ac_of_type_vg = await_ac_of_type.ToVarGroup();
			StateCategory await_ac_of_type_from_vg(await_ac_of_type_vg);
			EXPECT_TRUE(await_ac_of_type_from_vg.IsAwaitAction());
			EXPECT_EQ(await_ac_of_type_from_vg.Index(), 4);

			auto final_vg = final.ToVarGroup();
			StateCategory final_from_vg(final_vg);
			EXPECT_TRUE(final.IsFinal());
			EXPECT_EQ(final.Index(), 0);
			EXPECT_TRUE(final_from_vg.IsFinal());

			auto await_event_of_type_vg = await_event_of_type.ToVarGroup();
			StateCategory await_event_of_type_from_vg(await_event_of_type_vg);
			EXPECT_TRUE(await_event_of_type_from_vg.IsAwaitEvent());
			EXPECT_EQ(await_event_of_type_from_vg.Index(), 2);
		}
		{
			auto eoh = StateCategory::EndOfHorizon();

			EXPECT_FALSE(eoh.IsAwaitAction());
			EXPECT_FALSE(eoh.IsAwaitEvent());
			EXPECT_FALSE(eoh.IsFinal());
			EXPECT_TRUE(eoh.IsEndOfHorizon());
			EXPECT_EQ(eoh.Index(), 0);

			// Convert EOH to VarGroup and back
			auto eoh_vg = eoh.ToVarGroup();
			StateCategory eoh_from_vg(eoh_vg);
			EXPECT_TRUE(eoh_from_vg.IsEndOfHorizon());
			EXPECT_EQ(eoh_from_vg.Index(), 0);
		}
	}
}