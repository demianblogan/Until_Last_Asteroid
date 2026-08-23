#pragma once

#include <filesystem>
#include <stdexcept>
#include <type_traits>
#include <unordered_map>
#include <utility>

#include <SFML/Audio/Music.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/Texture.hpp>

// Loads resource from file and stores it by id. Throws on failure or duplicate id.
template <typename Resource, typename Identifier>
class AssetCache
{
public:
	// Marked const on purpose: like Get() below, it only ever writes to
	// `resources`, which is mutable specifically so loading can happen as a
	// cache-filling side effect without weakening the class's const contract.
	void LoadFromFile(const Identifier& id, const std::filesystem::path& path) const;
	void RegisterLazy(const Identifier& id, std::filesystem::path path, bool isSmooth = false);

	[[nodiscard]] const Resource& Get(const Identifier& id) const;
	[[nodiscard]] Resource& Get(const Identifier& id);

private:
	// What RegisterLazy() remembers instead of touching disk: enough to load
	// the resource later, on the first Get() that actually needs it.
	struct PendingResource
	{
		std::filesystem::path path;

		// Only meaningful for Resource == sf::Texture (applied in Get() below
		// via setSmooth()). Every resource type carries it anyway because
		// RegisterLazy() is called the same way regardless of Resource --
		// specializing the whole class just for this one texture-only flag
		// isn't worth it.
		bool isTextureSmooth = false;
	};

	mutable std::unordered_map<Identifier, Resource> resources;
	mutable std::unordered_map<Identifier, PendingResource> pendingResources;
};

template <typename Resource, typename Identifier>
void AssetCache<Resource, Identifier>::LoadFromFile(const Identifier& id, const std::filesystem::path& path) const
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

	auto [_, isInserted] = resources.emplace(id, std::move(resource));
	if (!isInserted)
		throw std::runtime_error("Resource already exists: " + path.string());
}

template <typename Resource, typename Identifier>
void AssetCache<Resource, Identifier>::RegisterLazy(
	const Identifier& id, 
	std::filesystem::path path, 
	bool isSmooth)
{
	if (resources.contains(id) || pendingResources.contains(id))
		throw std::runtime_error("Resource already exists: " + path.string());

	pendingResources.emplace(id, PendingResource{ std::move(path), isSmooth });
}

template <typename Resource, typename Identifier>
const Resource& AssetCache<Resource, Identifier>::Get(const Identifier& id) const
{
	auto iterator = resources.find(id);
	if (iterator == resources.end())
	{
		const auto pendingResourcesIterator = pendingResources.find(id);
		if (pendingResourcesIterator != pendingResources.end())
		{
			const PendingResource descriptor = pendingResourcesIterator->second;

			pendingResources.erase(pendingResourcesIterator);

			LoadFromFile(id, descriptor.path);

			if constexpr (std::is_same_v<Resource, sf::Texture>)
				resources.at(id).setSmooth(descriptor.isTextureSmooth);

			iterator = resources.find(id);
		}
	}

	if (iterator == resources.end())
		throw std::runtime_error("Resource is not found");

	return iterator->second;
}

template <typename Resource, typename Identifier>
Resource& AssetCache<Resource, Identifier>::Get(const Identifier& id)
{
	return const_cast<Resource&>(static_cast<const AssetCache&>(*this).Get(id));
}