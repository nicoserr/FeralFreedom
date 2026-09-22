#pragma once

#include <algorithm>
#include <vector>
#include <unordered_map>
#include <assert.h>
#include <iostream>

// Unique identifyer for all entities
class Entity
{
	unsigned int id;
	static unsigned int id_count; // starts from 1, entity 0 is the default initialization
public:
	unsigned int getId() const { return id; }
	void setId(int new_id) { id = new_id; };
	void setIdCount(unsigned int count) { id_count = count; }
	bool operator==(const Entity& other) const {
		return id == other.id;
	}
	Entity()
	{
		id = id_count++;
		// Note, indices of already deleted entities aren't re-used in this simple implementation.
	}
	operator unsigned int() { return id; } // this enables automatic casting to int
};

// Custom hash function for Entity
struct EntityHash {
	std::size_t operator()(const Entity& entity) const {
		return std::hash<int>()(entity.getId()); // Use the id to generate the hash
	}
};

// Common interface to refer to all containers in the ECS registry
struct ContainerInterface
{
	virtual void clear() = 0;
	virtual size_t size() = 0;
	virtual bool remove(Entity e) = 0;
	virtual bool has(Entity entity) = 0;
};

template <typename Component>
class ComponentContainer : public ContainerInterface
{
private:

    // Map<Entity, Index>
	std::unordered_map<unsigned int, unsigned int> map_entity_componentID;

    // keep track of removed components to rewrite them while adding
    std::vector<int> emptyIndex;

public:

	std::vector<Component> components;

	std::vector<Entity> entities;

	ComponentContainer()
	{
	}

	// Inserting a component c associated to entity e
	inline Component& insert(Entity e, Component c, bool check_for_duplicates = true)
	{
		assert(!(check_for_duplicates && has(e)) && "Entity already contained in ECS registry");

        if (emptyIndex.empty()) {
            map_entity_componentID[e] = (unsigned int)components.size();
            components.push_back(std::move(c)); // the move enforces move instead of copy constructor
        }
        else {
            int new_index = emptyIndex.back();
            map_entity_componentID[e] = new_index;
            emptyIndex.pop_back();
            components[new_index] = c; // Fill the available space
        }
        entities.push_back(e);

		return components.back();
	};

    // Not sure about the next two functions

	// The emplace function takes the the provided arguments Args, creates a new object of type Component, and inserts it into the ECS system
	template<typename... Args>
	Component& emplace(Entity e, Args &&... args) {
		return insert(e, Component(std::forward<Args>(args)...));
	};
	template<typename... Args>
	Component& emplace_with_duplicates(Entity e, Args &&... args) {
		return insert(e, Component(std::forward<Args>(args)...), false);
	};

	// A wrapper to return the component of an entity
	Component& get(Entity e) {
		assert(has(e) && "Entity not contained in ECS registry");
		return components[map_entity_componentID[e]];
	}

	// Check if entity has a component of type 'Component'
	bool has(Entity entity) {
		return map_entity_componentID.count(entity) > 0;
	}

	// Remove an component and pack the container to re-use the empty space
	bool remove(Entity e)
	{
		if (has(e)){

            int tbremoved = map_entity_componentID[e];
			int last = components.size() - 1;

			if (tbremoved != last) {
				components[tbremoved] = std::move(components[last]);
				entities[tbremoved] = entities[last];
				
				map_entity_componentID[entities[tbremoved]] = tbremoved;
			}

			components.pop_back();
			entities.pop_back();
			map_entity_componentID.erase(e);
            return true;
		} else {
            return false;
        }
	};

	// Remove all components of type 'Component'
	void clear()
	{
		map_entity_componentID.clear();
		components.clear();
		entities.clear();
	}

	// Report the number of components of type 'Component'
	size_t size()
	{
		return components.size();
	}

	// Sort the components and associated entity assignment structures by the comparisonFunction, see std::sort
	template <class Compare>
	void sort(Compare comparisonFunction)
	{
		// First sort the entity list as desired
		std::sort(entities.begin(), entities.end(), comparisonFunction);
		// Now re-arrange the components (Note, creates a new vector, which may be slow! Not sure if in-place could be faster: https://stackoverflow.com/questions/63703637/how-to-efficiently-permute-an-array-in-place-using-stdswap)
		std::vector<Component> components_new; components_new.reserve(components.size());
		std::transform(entities.begin(), entities.end(), std::back_inserter(components_new), [&](Entity e) { return std::move(get(e)); }); // note, the get still uses the old hash map (on purpose!)
		components = std::move(components_new); // note, we use move operations to not create unneccesary copies of objects, but memory is still allocated for the new vector
		// Fill the new hashmap
		for (unsigned int i = 0; i < entities.size(); i++)
			map_entity_componentID[entities[i]] = i;
	}
};