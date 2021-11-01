#pragma once

#include <memory>
#include <string_view>
#include <unordered_map>

class CStringPool final
{
public:
	CStringPool() = default;
	~CStringPool() = default;

	CStringPool(const CStringPool&) = delete;
	CStringPool& operator=(const CStringPool&) = delete;

	CStringPool(CStringPool&&) = default;
	CStringPool& operator=(CStringPool&&) = default;

	const char* Allocate(const char* string);

private:
	std::unordered_map<std::string_view, std::unique_ptr<char[]>> m_Pool;
};