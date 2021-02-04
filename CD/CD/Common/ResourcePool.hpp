#pragma once

#include <CD/Common/Debug.hpp>
#include <memory>
#include <vector>

namespace CD {

template<typename ResourceType>
class ResourcePool {
public:
	ResourcePool(std::size_t size);

	std::size_t add(const ResourceType&);
	void remove(std::size_t index);
	template<typename T> ResourceType& get(T index);
private:
	std::unique_ptr<ResourceType[]> resources;
	std::vector<std::size_t> release_list;
	std::size_t offset;
	std::size_t size;
};

template<typename ResourceType>
inline ResourcePool<ResourceType>::ResourcePool(std::size_t size) :
	resources(std::make_unique<ResourceType[]>(size)),
	offset(),
	size(size) {
}

template<typename ResourceType>
inline std::size_t ResourcePool<ResourceType>::add(const ResourceType& resource) {
	std::size_t index = 0;
	if(release_list.size()) {
		index = release_list.front();
		release_list.pop_back();
	}
	else {
		CD_ASSERT(offset < size);
		index = offset;
		++offset;
	}
	resources[index] = resource;
	return index;
}

template<typename ResourceType>
inline void ResourcePool<ResourceType>::remove(std::size_t index) {
	release_list.push_back(index);
}

template<typename ResourceType>
template<typename T>
inline ResourceType& ResourcePool<ResourceType>::get(T index) {
	return resources[static_cast<std::size_t>(index)];
}

}