#include "cbase.h"

#include "StringPool.h"

const char* StringPool::Allocate(const char* string)
{
	//Treat null pointers as empty strings
	if (!string)
	{
		ALERT(at_warning, "nullptr string passed to StringPool::Allocate\n");
		return "";
	}

	const std::string_view source{string};

	if (auto it = m_Pool.find(source); it != m_Pool.end())
	{
		return it->second.get();
	}

	auto destination{std::make_unique<char[]>(source.size() + 1)};

	std::strncpy(destination.get(), source.data(), source.size());
	destination[source.size()] = '\0';

	const std::string_view key{destination.get(), source.size()};

	m_Pool.emplace(key, std::move(destination));

	return key.data();
}
