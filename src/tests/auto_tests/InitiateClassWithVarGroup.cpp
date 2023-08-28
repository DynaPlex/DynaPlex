#include <iostream>
#include "dynaplex/vargroup.h"
#include "dynaplex/error.h"
#include <gtest/gtest.h>
#include "someclass.h"

namespace DynaPlex::Tests {

	TEST(InitiateClassWithVarGroup, VarGroup) {

		auto nested = DynaPlex::VarGroup({ {"Id","1"},{"Size",1.0} });

		auto nested2 = DynaPlex::VarGroup({ {"Id","2"},{"Size",4.0} });

		DynaPlex::VarGroup varGroup({
			{"testEnumClass",static_cast<int>(SomeClass::Test::option)},
			{"myString","string"},
			{"myInt",42},
			{"myVector", DynaPlex::VarGroup::Int64Vec{123,123}},
			{ "nestedClass",nested},
			{"myNestedVector",DynaPlex::VarGroup::VarGroupVec{nested,nested2} },
			{ "myQueue",DynaPlex::VarGroup::Int64Vec{1,2,3,4,5,6,7,8,9,10,11,12,13,14} } ,{ "as", "as"} });




		//Initiate someclass with the varGroup
		SomeClass someclass(varGroup);

		ASSERT_EQ(someclass.testEnumClass, SomeClass::Test::option);
		//now someclass should have taken the values from the vargroup.
		ASSERT_EQ(someclass.myInt, 42);
		ASSERT_EQ(someclass.myString, "string");
		ASSERT_EQ(someclass.myVector.size(), 2);
		ASSERT_EQ(someclass.nestedClass.Size, 1.0);
		ASSERT_EQ(someclass.myQueue.back(), 14);
		ASSERT_EQ(someclass.myNestedVector.back().Id, "2");


		auto vars = someclass.ToVarGroup();

		SomeClass someclass2(vars);
		auto vars2 = someclass2.ToVarGroup();

		EXPECT_EQ(vars, vars2);

		if (vars != vars2)
		{
			{
				std::cout << vars.ToAbbrvString() << std::endl;
				std::cout << vars2.ToAbbrvString() << std::endl;
			}
		}
	}
}