#include <iostream>
#include "dynaplex/vargroup.h"
#include "dynaplex/error.h"
#include <gtest/gtest.h>



TEST(DynaPlexTests, VarGroupBasics) {

	DynaPlex::VarGroup vars({ {"p",2} , {"q",3.1} , { "s", "string"} });


	vars.Add("listD", DynaPlex::VarGroup::DoubleVec{ 1.9,2.3 });

	int p;
	vars.Get("p", p);
	EXPECT_EQ(p, 2);

	double q;
	vars.Get("q", q);
	EXPECT_EQ(q, 3.1);

	std::string s;
	vars.Get("s", s);
	EXPECT_EQ(s, "string");

	std::vector<double> listD;
	std::vector<int> listI;
	vars.Get("listD", listD);
	EXPECT_EQ(listD[0], 1.9);

	//Note that doubles are silently truncated to ints when attempting to initiate an int with a double. 
	vars.Get("listD", listI);
	EXPECT_EQ(listI[0], 1);



	vars.Get("q", p);
	EXPECT_EQ(p, 3);

	EXPECT_THROW({
		vars.Get("p",s);
		}, DynaPlex::Error
	);


	EXPECT_THROW({
		vars.Get("not_available",s);
		}, DynaPlex::Error
	);

	auto vars2 = vars;

	std::string string = "test";
	vars.Add("new key", "value");

	EXPECT_NO_THROW(
		{
		vars.Get("new key", string);
		}
	);

	//vars2 is deep copy, so it should not contain the "new key" that was added after the copy was made. 
	EXPECT_THROW({
		vars2.Get("new key", s);
		}, DynaPlex::Error);



	auto vargroup1 = DynaPlex::VarGroup({ {"Id","tset"},{"Size",1.0} });

	auto vargroup2 = DynaPlex::VarGroup({ {"Id","tset"},{"Size",1.0} });

	auto list = ::DynaPlex::VarGroup::VarGroupVec{ vargroup1,vargroup2 };


	EXPECT_EQ(vargroup1, list[0]);
	EXPECT_EQ(vargroup2, list[1]);

	//This will not change the VarGroup in the list, as that was copied when the list was created.
	vargroup2.Add("as", 1);
	EXPECT_NE(vargroup2, list[1]);
}

