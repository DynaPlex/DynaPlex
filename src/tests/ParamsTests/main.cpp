#include <iostream>
#include <dynaplex/params.h>
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


		DynaPlex::Params distprops{
			{"type","geom"},
		{"mean",5} };

		DynaPlex::Params ret1{
		{"myString","retailer 1"},
			{"myInt",42},
			{"myVector",DynaPlex::Params::Int64Vec{14,2,1,1,1,1,1,1,1,1,1,1,1,1,1}},
			{"pars",nullptr}
		};
		DynaPlex::Params ret1extra{
		{"myString","retailer 1"},
			{"myInt",42},
			{"myVector",DynaPlex::Params::Int64Vec{14,2,1,1,1,1,1,1,1,1,1,1,1,1,1}},
			{"pars",nullptr}
		};


		DynaPlex::Params ret2({
			{"myString","retailer 2"},
			{"myInt",42},
			{"myVector",DynaPlex::Params::Int64Vec{14,2,1,1,1,1,1,1,1,1,1,1,1,1,1}},
			{"pars",nullptr}
			});
		DynaPlex::Params retlast({
			{"myString","retailer 10"},
			{"myInt",42},
			{"myVector",DynaPlex::Params::Int64Vec{1,2,-1,1,1,1,1,1,1,1,1,1,1,1,1}},
			{"pars",nullptr}
			});




		DynaPlex::Params::ParamsVec rets{
			ret1,ret2,retlast
		};

		auto nested = DynaPlex::Params({ {"Id","tset"},{"Size",1.0} });

		auto nested2 = DynaPlex::Params({ {"Id","tset"},{"Size",2.0} });



		DynaPlex::Params parts({
			{"myString","string"},
			{"myInt",42},
			{"pars",rets},
			{"ret", ret1},
			{"myVector", DynaPlex::Params::Int64Vec{123,123}},
			{ "nestedClass",nested},
			{"myNestedVector",DynaPlex::Params::ParamsVec{nested,nested2}}

			});

		DynaPlex::Params parts2 = parts;

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




		auto params = DynaPlex::Params(
			{ { "env", "OWMR"},
			{ "rets",rets },
			{ "warehouse",DynaPlex::Params{{"dist", distprops}}
			} }
		);
		params.Add("test", std::vector<double>{1.0, 2.1, 123.1});
		params.Print();
		return 0;
		//params.SaveToFile("test.txt");

		try
		{
			auto pars = DynaPlex::Params::LoadFromFile("test.txt");

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
