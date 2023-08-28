#pragma once
#include "nlohmann/json.h"

namespace DynaPlex::VarGroupHelpers {

	using ordered_json = nlohmann::ordered_json;

	std::string hash_json_string(const ordered_json& j);
	std::int64_t hash_json_int64(const ordered_json& j);

	int64_t levenshteinDist(const std::string& word1, const std::string& word2);

	bool check_validity(const ordered_json& j);

	void SortOrderedJson(ordered_json& j);

}