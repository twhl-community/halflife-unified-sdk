#pragma once

#include <memory>
#include <string_view>
#include <unordered_map>

class StringPool final
{
public:
	StringPool() = default;
	~StringPool() = default;

	StringPool(const StringPool&) = delete;
	StringPool& operator=(const StringPool&) = delete;

	StringPool(StringPool&&) = default;
	StringPool& operator=(StringPool&&) = default;

	const char* Allocate(const char* string);

	const char* Allocate(std::string_view string);

private:
	std::unordered_map<std::string_view, std::unique_ptr<char[]>> m_Pool;
};