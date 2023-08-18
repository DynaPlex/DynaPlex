#pragma once
#include "nlohmann/json.h"

namespace DynaPlex::VarGroupHelpers {

	using ordered_json = nlohmann::ordered_json;

	std::string hash_json(const ordered_json& j);


	bool check_validity(const ordered_json& j);

	void SortOrderedJson(ordered_json& j);

}