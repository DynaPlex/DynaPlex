#include <iostream>
#include <dynaplex/params.h>
int main()
{

	DynaPlex::Params pars{

	};

	DynaPlex::Params::DoubleVec vec{ 1.0,2.0 };

	pars.Add("test", vec);

	DynaPlex::Params pars2{};

	pars2.Add("par ",pars);

	pars2.Print();
	pars.Add("test2", 4);
	pars2.Print();


	return 0;
	DynaPlex::Params distprops{
			{"type","geom"},
		{"mean",5} };

	DynaPlex::Params ret1{
		{"name","retailer 1"},
		{"dist", distprops}
	};


	DynaPlex::Params ret2({
		{"name","retailer 2"},
		{"dist", distprops},
		{"leadtime",DynaPlex::Params::DoubleVec{1.4,2,1,1,1,1,1,1,1,1,1,1,1,1,1}}
	});
	
	DynaPlex::Params::ParamsVec rets{
		ret1,ret2,ret1,ret2,ret1,ret2
	};

	auto params= DynaPlex::Params(
		{ { "env", "OWMR"},
		{ "rets",rets },
		{ "warehouse",DynaPlex::Params{{"dist", distprops}}
		} }
	);
	std::cout << "printing" << std::endl;
	params.Print();


	std::cout << "done" << std::endl;
	return 0;
}
