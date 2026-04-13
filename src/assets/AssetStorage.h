#pragma once

#include <filesystem>
#include <stdexcept>
#include <type_traits>
#include <unordered_map>

#include <SFML/Audio/Music.hpp>
#include <SFML/Graphics/Font.hpp>

// Loads resource from file and stores it by id. Throws on failure or duplicate id.
template <typename Resource, typename Identifier>
class AssetStorage
{
public:
	void LoadFromFile(const Identifier& id, const std::filesystem::path& path);
	[[nodiscard]] const Resource& Get(const Identifier& id) const;
	[[nodiscard]] Resource& Get(const Identifier& id);

private:
	std::unordered_map<Identifier, Resource> resources;
};

template <typename Resource, typename Identifier>
void AssetStorage<Resource, Identifier>::LoadFromFile(const Identifier& id, const std::filesystem::path& path)
{
	Resource resource;

	if constexpr (std::is_same_v<Resource, sf::Music> || std::is_same_v<Resource, sf::Font>)
	{
		if (!resource.openFromFile(path))
			throw std::runtime_error("Failed to load resource: " + path.string());
	}
	else
	{
		if (!resource.loadFromFile(path))
			throw std::runtime_error("Failed to load resource: " + path.string());
	}

	auto [_, isInserted] { resources.emplace(id, std::move(resource))};
	if (!isInserted)
		throw std::runtime_error("Resource already exists: " + path.string());
}

template <typename Resource, typename Identifier>
const Resource& AssetStorage<Resource, Identifier>::Get(const Identifier& id) const
{
	auto iterator{ resources.find(id) };
	if (iterator == resources.end())
		throw std::runtime_error("Resource is not found");

	return iterator->second;
}

template <typename Resource, typename Identifier>
Resource& AssetStorage<Resource, Identifier>::Get(const Identifier& id)
{
	return const_cast<Resource&>(static_cast<const AssetStorage&>(*this).Get(id));
}