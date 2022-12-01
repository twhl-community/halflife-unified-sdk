#include "cbase.h"

#include "StringPool.h"

const char* StringPool::Allocate(const char* string)
{
	// Treat null pointers as empty strings
	if (!string)
	{
		CBaseEntity::Logger->warn("NULL string passed to StringPool::Allocate");
		return "";
	}

	const std::string_view source{string};

	return Allocate(source);
}

const char* StringPool::Allocate(std::string_view string)
{
	if (auto it = m_Pool.find(string); it != m_Pool.end())
	{
		return it->second.get();
	}

	auto destination{std::make_unique<char[]>(string.size() + 1)};

	std::strncpy(destination.get(), string.data(), string.size());
	destination[string.size()] = '\0';

	const std::string_view key{destination.get(), string.size()};

	m_Pool.emplace(key, std::move(destination));

	return key.data();
}
