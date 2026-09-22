#pragma once
#include <vector>

#include "ecs.hpp"
#include "components.hpp"

class ECSRegistry
{
	// Callbacks to remove a particular or all entities in the system
	std::vector<ContainerInterface*> registryList;
    std::vector<Entity> entities;

public:
	ComponentContainer<DeathTimer> deathTimers;
	ComponentContainer<Player> players;
	ComponentContainer<ConsumableItem> consumableItems;
	ComponentContainer<EquippableItem> equippableItems;
	ComponentContainer<NPC> npcs; 
	ComponentContainer<Motion> motions;
    ComponentContainer<Stats> stats;
    ComponentContainer<BoundingBox> boundingBoxes;
    ComponentContainer<Collider> colliders;
    ComponentContainer<Patrol> patrols;
    ComponentContainer<Hidden> hiddens;
	ComponentContainer<RenderRequest> renderRequests;
	ComponentContainer<Mesh*> meshPtrs;
	ComponentContainer<RandomWalker> randomWalkers;
	ComponentContainer<Converger> convergers;
	ComponentContainer<Chaser> chasers;
	ComponentContainer<Inventory> inventory;
	ComponentContainer<UIElement> uiElements;
	ComponentContainer<VisualEffect> visualEffects;
	ComponentContainer<SetMotion> setMotions;
	ComponentContainer<CameraComponent> cameraComponent;
	ComponentContainer<Door> doors;
	ComponentContainer<Rotatable> rotatables;
	ComponentContainer<Time> times;
	ComponentContainer<Key> key;
	ComponentContainer<KeyInventory> keyInventory;

	ECSRegistry()
	{
		registryList.push_back(&deathTimers);
		registryList.push_back(&players);
		registryList.push_back(&consumableItems);
		registryList.push_back(&equippableItems);
		registryList.push_back(&npcs);
		registryList.push_back(&motions);
        registryList.push_back(&stats);
		registryList.push_back(&boundingBoxes);
		registryList.push_back(&colliders);
		registryList.push_back(&patrols);
		registryList.push_back(&hiddens);
		registryList.push_back(&meshPtrs);
		registryList.push_back(&renderRequests);
		registryList.push_back(&randomWalkers);
		registryList.push_back(&convergers);
		registryList.push_back(&chasers);
		registryList.push_back(&uiElements);
		registryList.push_back(&visualEffects);
		registryList.push_back(&inventory);
		registryList.push_back(&setMotions);
		registryList.push_back(&cameraComponent);
		registryList.push_back(&doors);
		registryList.push_back(&rotatables);
		registryList.push_back(&times);
		registryList.push_back(&key);
		registryList.push_back(&keyInventory);
	}

    Entity create_entity() {
        Entity new_entity;
        entities.push_back(new_entity);
        return new_entity;
    }

	std::vector<Entity>& get_entities() {
		return entities;
	}

	void clear_all_components() {
		for (ContainerInterface* reg : registryList)
			reg->clear();
	}

	void list_all_components() {
		printf("Debug info on all registry entries:\n");
		for (ContainerInterface* reg : registryList)
			if (reg->size() > 0)
				printf("%4d components of type %s\n", (int)reg->size(), typeid(*reg).name());
	}

	void list_all_components_of(Entity e) {
		printf("Debug info on components of entity %u:\n", (unsigned int)e);
		for (ContainerInterface* reg : registryList)
			if (reg->has(e))
				printf("type %s\n", typeid(*reg).name());
	}

	void remove_all_components_of(Entity e) {
		for (ContainerInterface* reg : registryList) {
			reg->remove(e);
		}
	}

	int get_entity_index(Entity entity) {
		// Iterate through the entities vector to find the index of the specified entity
		for (size_t i = 0; i < this->entities.size(); ++i) {
			if (this->entities[i] == entity) {
				return static_cast<int>(i); // Return the index if found
			}
		}
		// If the entity is not found, return -1 or another indicator for "not found"
		return -1;
	}

	Entity get_entity_by_id(int id)
	{
		for (size_t i = 0; i < this->entities.size(); ++i) {
			if (this->entities[i].getId() == id) {
				return entities[i]; // Return Entity if found
			}
		}
		Entity null_entity = Entity();
		null_entity.setId(UINT_MAX);
		return null_entity;
	}
};

extern ECSRegistry registry;