#include <iostream>
#include "lostsalesMDP.h"
#include <dynaplex/errors.h>

std::string lostsalesMDP::Identifier()
{

	return "lost sales "+ std::to_string(j);
}

lostsalesMDP::lostsalesMDP(const DynaPlex::Params& params)
{
	params.Get_Into("j", j);
}