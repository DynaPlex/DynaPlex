#include <iostream>
#include <dynaplex/vargroup.h>
#include <dynaplex/utilities.h>
#include "SomeClass.h"
#include <iostream>
#include <deque>
#include <vector>



int main_() {
	const int numDeques = 10000;
	
	std::vector<std::deque<int64_t>> deques(numDeques);
	for (auto& myDeque : deques) {
		for (int i = 0; i < 10; ++i) {
			myDeque.push_back(i);
		}
	}

	return 0;
}

int main()
{

	try {


		DynaPlex::VarGroup distprops{
			{"type","geom"},
		{"mean",5} };

		DynaPlex::VarGroup ret1{
		{"myString","retailer 1"},
			{"myInt",42},
			{"myVector",DynaPlex::VarGroup::Int64Vec{14,2,1,1,1,1,1,1,1,1,1,1,1,1,1}},
			{"pars",nullptr}
		};
		DynaPlex::VarGroup ret1extra{
		{"myString","retailer 1"},
			{"myInt",42},
			{"myVector",DynaPlex::VarGroup::Int64Vec{14,2,1,1,1,1,1,1,1,1,1,1,1,1,1}},
			{"pars",nullptr}
		};


		DynaPlex::VarGroup ret2({
			{"myString","retailer 2"},
			{"myInt",42},
			{"myVector",DynaPlex::VarGroup::Int64Vec{14,2,1,1,1,1,1,1,1,1,1,1,1,1,1}},
			{"pars",nullptr}
			});
		DynaPlex::VarGroup retlast({
			{"myString","retailer 10"},
			{"myInt",42},
			{"myVector",DynaPlex::VarGroup::Int64Vec{1,2,-1,1,1,1,1,1,1,1,1,1,1,1,1}},
			{"pars",nullptr}
			});




		DynaPlex::VarGroup::VarGroupVec rets{
			ret1,ret2,retlast
		};

		auto nested = DynaPlex::VarGroup({ {"Id","tset"},{"Size",1.0} });

		auto nested2 = DynaPlex::VarGroup({ {"Id","tset"},{"Size",2.0} });



		DynaPlex::VarGroup parts({
			{"myString","string"},
			{"myInt",42},
			{"pars",rets},
			{"ret", ret1},
			{"myVector", DynaPlex::VarGroup::Int64Vec{123,123}},
			{ "nestedClass",nested},
			{"myNestedVector",DynaPlex::VarGroup::VarGroupVec{nested,nested2}}

			});

		DynaPlex::VarGroup parts2 = parts;

		std::cout << ret1.Hash() << std::endl;
		std::cout << ret1extra.Hash() << std::endl;



		std::cout << parts.Hash() << std::endl;
		std::cout << parts2.Hash() << std::endl;

		parts.Add("sas", std::vector<int>({ 1,2,3 }));


		std::cout << parts.Hash() << std::endl;
		std::cout << parts2.Hash() << std::endl;


		SomeClass someClass(parts);

		//someClass.Print();
		return 0;




		auto vars = DynaPlex::VarGroup(
			{ { "env", "OWMR"},
			{ "rets",rets },
			{ "warehouse",DynaPlex::VarGroup{{"dist", distprops}}
			} }
		);
		vars.Add("test", std::vector<double>{1.0, 2.1, 123.1});
		vars.Print();
		return 0;
		//vars.SaveToFile("test.txt");

		try
		{
			auto pars = DynaPlex::VarGroup::LoadFromFile("test.txt");

			pars.Print();
		}
		catch (const std::exception& e)
		{
			std::cerr << "caught an error:" << e.what() << std::endl;
		}

		std::cout << "done" << std::endl;
		return 0;
	}
	catch (const std::exception& e)
	{
		std::cout << e.what() << std::endl;
	}
	return 0; 
}
