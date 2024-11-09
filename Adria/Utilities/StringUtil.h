#pragma once
#include <string>
#include <vector>

namespace adria
{
	std::wstring ToWideString(std::string const& in);
	std::string ToString(std::wstring const& in);
	
	std::string ToLower(std::string const& in);
	std::string ToUpper(std::string const& in);

	Bool FromCString(Char const* in, int& out);
	Bool FromCString(Char const* in, Float& out);
	Bool FromCString(Char const* in, const Char*& out);
	Bool FromCString(Char const* in, Bool& out);

	std::string IntToString(int val);
	std::string FloatToString(Float val);
	std::string CStrToString(Char const* val);
	std::string BoolToString(Bool val);

	std::vector<std::string> SplitString(std::string const& text, Char delimeter);
}