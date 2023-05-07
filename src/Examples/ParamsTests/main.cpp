#include <iostream>
#include <dynaplex/params.h>
#include <dynaplex/utilities.h>
int main()
{
	
	DynaPlex::Params pars;
	DynaPlex::Params pars2;

	DynaPlex::Params pars3 = pars;

	pars.Add("asd", 1);

	pars2.Add("tes", 112);

	pars.Add("as", pars2);

	pars2.Add("asdf", nullptr);
	//pars.Print();

	pars3.Print();

	return 0;

	DynaPlex::Params distprops{
			{"type","geom"},
		{"mean",5} };

	DynaPlex::Params ret1{
		{"name","retailer 1"},
		{"aa",nullptr},
		{"asfd", false},
		{"dist", distprops}
	};


	DynaPlex::Params ret2({
		{"name","retailer 2"},
		{"dist", distprops},
		{"leadtime",DynaPlex::Params::DoubleVec{1.4,2,1,1,1,1,1,1,1,1,1,1,1,1,1}}
	});
	DynaPlex::Params retlast({
		{"name","retailer 10"},
		{"dist", distprops},
		{"leadtime",DynaPlex::Params::DoubleVec{1.4,2,1,1,1,1,1,1,1,1,1,1,1,1,1}}
		});

	
	DynaPlex::Params::ParamsVec rets{
		ret1,ret2,retlast
	};


	auto params= DynaPlex::Params(
		{ { "env", "OWMR"},
		{ "rets",rets },
		{ "warehouse",DynaPlex::Params{{"dist", distprops}}
		} }
	);

	retlast.Add("test", params);

	std::cout << "printing" << std::endl;
	params.Print();
//	return 0;
	params.SaveToFile("test.txt");

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
