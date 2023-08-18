#include <iostream>
#include "dynaplex/vargroup.h"
#include "dynaplex/error.h"
#include <gtest/gtest.h>



TEST(DynaPlexTests, VarGroup) {

	DynaPlex::VarGroup vars({ {"p",2} , {"q",3.1} , { "s", "string"} });


	vars.Add("listD", DynaPlex::VarGroup::DoubleVec{ 1.9,2.3 });

	int p;
	vars.Get_Into("p", p);
	EXPECT_EQ(p, 2);

	double q;
	vars.Get_Into("q", q);
	EXPECT_EQ(q, 3.1);

	std::string s;
	vars.Get_Into("s", s);
	EXPECT_EQ(s, "string");

	std::vector<double> listD;
	std::vector<int> listI;
	vars.Get_Into("listD", listD);
	EXPECT_EQ(listD[0], 1.9);

	//Note that doubles will silently get truncated to ints when attempting to convert.
	vars.Get_Into("listD", listI);
	EXPECT_EQ(listI[0], 1);



	vars.Get_Into("q", p);
	EXPECT_EQ(p, 3);

	EXPECT_THROW({
		vars.Get_Into("p",s);
		}, DynaPlex::Error
	);


	EXPECT_THROW({
		vars.Get_Into("not_available",s);
		}, DynaPlex::Error
	);

	auto vars2 = vars;

	std::string string = "test";
	vars.Add("new key", "value");

	EXPECT_NO_THROW(
		{
		vars.Get_Into("new key", string);
		}
	);

	//vars2 is deep copy, so it should not contain the "new key" that was added after the copy was made. 
	EXPECT_THROW({
		vars2.Get_Into("new key", s);
		}, DynaPlex::Error);

}
