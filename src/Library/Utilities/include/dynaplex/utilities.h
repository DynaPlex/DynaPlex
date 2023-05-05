#pragma once
#include <string>
using namespace std::string_literals;

namespace DynaPlex {
	namespace Utilities {
		void Fail(std::string message);

		std::string GetOutputLocation(const std::string filename);

	}
}