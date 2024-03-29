/*
MIT License

Copyright(c) 2024 Aleksi Anderson

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files(the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions :

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#pragma once
#include <bitset>
#include <stack>
#include <set>
#include <vector>
#include <unordered_map>
#include <typeinfo>
#include <stdexcept>
#include <iostream>
#include <memory>

//Allow max components to be determined outside this file
#ifndef ECS_MAX_COMPONENTS
#define ECS_MAX_COMPONENTS 100
#elif ECS_MAX_COMPONENTS > UINT16_MAX
#error The maximum possible number of components is 65535
#endif


namespace ecs
{
	//Define colors for errors and warnings
	constexpr const char* errorFormat = "\033[31m";
	constexpr const char* warningFormat = "\033[33m";
	constexpr const char* normalFormat = "\033[37m";

	//Entities as IDs, 0 will never be a valid ID
	using Entity = uint32_t;
	//Signatures as bitsets, where each component has its own bit
	using Signature = std::bitset<ECS_MAX_COMPONENTS>;


	//ENTITY MANAGEMENT DATA

	//All currently available and used Entity IDs
	std::stack<Entity> availableEntities;
	std::set<Entity> usedEntities;
	//A map of an Entity's ID to its signature
	std::unordered_map<Entity, Signature> entitySignatures;
	//How many entities are currently reserved
	uint32_t entityCount = 0;

	//COMPONENT MANAGEMENT DATA

	//Base struct all components inherit from
	struct Component {};
	//A map of every components name to it's corresponding component array
	std::unordered_map<const char*, std::vector<Component>> componentArrays;
	//Maps from Entities to their component indexed in component arrays
	std::unordered_map<const char*, std::unordered_map<Entity, uint32_t>> entityToIndex;
	std::unordered_map<const char*, std::unordered_map<uint32_t, Entity>> indexToEntity;
	//Maps from a components type name to its ID
	std::unordered_map<const char*, uint16_t> componentTypeToID;
	std::unordered_map<uint16_t, const char*> componentIDToType;
	//The amount of components registered. Also the next available component ID
	uint16_t componentCount = 0;

	//SYSTEM MANAGEMENT DATA

	//Base class all systems inherit from
	class System
	{
	public:
		//Set of every entity containing the required components for the system
		std::set<Entity> entities;
	};
	//Map of each system accessible by its type name
	std::unordered_map<const char*, std::shared_ptr<System>> systems;
	//Map of each system's signature accessible by their type name
	std::unordered_map<const char*, Signature> systemSignatures;


	//FORWARD DECLARES
	inline uint16_t GetComponentID();

	//INTERNAL FUNCTIONS

	//Implementation internal function. Called whenever an entity's signature changes
	void _OnEntitySignatureChanged(Entity entity)
	{
		const Signature& signature = entitySignatures[entity];

		//Loop through every system
		for (auto const& system : systems)
		{
			//If the entity's signature matches the system's signature
			if ((signature & systemSignatures[system.first]) == systemSignatures[system.first])
			{
				//Add the entity to the system's set
				system.second->entities.insert(entity);
			}
			else
			{
				//Remove the entity from the system's set
				system.second->entities.erase(entity);
			}
		}
	}

	//Implementation internal function. Make a signature from a series of components.
	//This is getting really complicated...
	template<typename Comp, typename... Comps>
	Signature _MakeSignature(bool autoRegister = false)
	{
		//Recursively add each component type to the signature
		Signature signature;
		if constexpr (sizeof...(Comps) > 0)
			signature = _MakeSignature<Comps...>();

		//Register the system if auto registering
		if (autoRegister && componentArrays.count(typeid(Comp).name()) == 0)
			RegisterComponent<Comp>();

		signature.set(GetComponentID<Comp>());
		return signature;
	}

	//Implementation internal function. Removes a component from an entity.
	void _RemoveComponentByName(Entity entity, const char* componentType)
	{
		//Keep track of the deleted component's index, and the entity of the last component in the array
		uint32_t deletedIndex = entityToIndex[componentType][entity];
		Entity lastEntity = indexToEntity[componentType][componentArrays[componentType].size() - 1];

		//Move the last element to the deleted index
		componentArrays[componentType][entityToIndex[componentType][entity]] = componentArrays[componentType].back();

		//Update the maps for the moved component
		entityToIndex[componentType][lastEntity] = deletedIndex;
		indexToEntity[componentType][deletedIndex] = lastEntity;

		//Remove the deleted entity and last entity
		entityToIndex[componentType].erase(entity);
		indexToEntity[componentType].erase(componentArrays[componentType].size() - 1);
		componentArrays[componentType].pop_back();

		//Update the entity's signature
		entitySignatures[entity].reset(componentTypeToID[componentType]);

		_OnEntitySignatureChanged(entity);
	}


	//PUBLIC FUNCTIONS

	//Checks if an entity exists
	inline bool EntityExists(Entity entity)
	{
		return usedEntities.find(entity) != usedEntities.end();
	}

	//Register a new component of type T
	template<typename T>
	void RegisterComponent()
	{
		const char* componentType = typeid(T).name();

#ifdef _DEBUG
		//Make sure the component has not been previously registered
		if (componentArrays.count(componentType) != 0)
		{
			std::cout << warningFormat << "ECS WARNING in RegisterComponent(): The component you are trying to register has alredy been registered!" << normalFormat << std::endl;
			return;
		}
		//Make sure there are not too many components registered
		if (componentCount >= ECS_MAX_COMPONENTS)
		{
			std::cout << errorFormat << "ECS ERROR in RegisterComponent(): Too many registered components! The current limit is "
				<< ECS_MAX_COMPONENTS << ". Consider including \"#define ECS_MAX_COMPONENTS num\" before you include ECS.h!" << normalFormat << std::endl;
			throw std::runtime_error("ECS ERROR: Too many registered components!");
		}
#endif

		//Assigns an ID and makes a new component array for the registered component type
		componentTypeToID[componentType] = componentCount;
		componentIDToType[componentCount] = componentType;
		componentArrays[componentType] = std::vector<Component>();

		componentCount++;
	}

	//Check if the entity has a component
	template<typename T>
	inline bool HasComponent(Entity entity)
	{
		const char* componentType = typeid(T).name();

		//Call HasComponent of the relevant component array
		return entityToIndex[componentType].count(entity);
	}

	//Get a reference to entity's component of type T
	template<typename T>
	inline T& GetComponent(Entity entity)
	{
		const char* componentType = typeid(T).name();

#ifdef _DEBUG
		//Make sure the entity exists
		if (!EntityExists(entity))
		{
			std::cout << errorFormat << "ECS ERROR in GetComponent(): The Entity does not exist!" << normalFormat << std::endl;
			throw std::runtime_error("ECS ERROR: Entity does not exist!");
		}
		//Make sure the entity has the component
		if (!HasComponent<T>(entity))
		{
			std::cout << errorFormat << "ECS ERROR in GetComponent(): Entity does not have the desired component!" << normalFormat << std::endl;
			throw std::runtime_error("ECS ERROR: Entity does not have the desired component!");
		}
#endif

		//Get the entity's component of type T
		Component& c = componentArrays[componentType][entityToIndex[componentType][entity]];
		//Cast it to the desired component type
		return static_cast<T&>(c);
	}

	//Get the ID of a component
	template<typename T>
	inline uint16_t GetComponentID()
	{
		const char* componentType = typeid(T).name();

#ifdef _DEBUG
		//Make sure the component has been registered
		if (componentArrays.count(componentType) == 0)
		{
			std::cout << errorFormat << "ECS ERROR in GetComponentID(): The component has not been registered!" << normalFormat << std::endl;
			throw std::runtime_error("ECS ERROR: Component not registered!");
		}
#endif

		return componentTypeToID[componentType];
	}

	//Add a component to entity. Returns a reference to that component
	template<typename T>
	T& AddComponent(Entity entity, T component)
	{
		const char* componentType = typeid(T).name();

#ifdef _DEBUG
		//Make sure the entity exists
		if (!EntityExists(entity))
		{
			std::cout << errorFormat << "ECS ERROR in AddComponent(): The entity you are trying to add the component to does not exist!" << normalFormat << std::endl;
			throw std::runtime_error("ECS ERROR: Entity does not exist!");
		}
		//Make sure the entity does not already have the component
		if (HasComponent<T>(entity))
		{
			std::cout << warningFormat << "ECS WARNING in AddComponent(): Entity already has the component you are trying to add!" << normalFormat << std::endl;
			return GetComponent<T>(entity);
		}
#endif

		//Update entity and index maps to include new entity at the back
		entityToIndex[componentType][entity] = componentArrays[componentType].size();
		indexToEntity[componentType][componentArrays[componentType].size()] = entity;

		componentArrays[componentType].push_back(component);

		//Update the entity signature
		entitySignatures[entity].set(GetComponentID<T>());

		_OnEntitySignatureChanged(entity);

		return component;
	}

	//Remove a component of type T from entity
	template<typename T>
	inline void RemoveComponent(Entity entity)
	{
		const char* componentType = typeid(T).name();

#ifdef _DEBUG
		//Make sure the entity exists
		if (!EntityExists(entity))
		{
			std::cout << errorFormat << "ECS ERROR in RemoveComponent(): The entity you are trying to remove the component from does not exist!" << normalFormat << std::endl;
			throw std::runtime_error("ECS ERROR: Entity does not exist!");
		}
		//Make sure the entity has the component
		if (!HasComponent<T>(entity))
		{
			std::cout << warningFormat << "ECS WARNING in RemoveComponent(): Entity does not have the component you are trying to remove!" << normalFormat << std::endl;
			return;
		}
#endif

		_RemoveComponentByName(entity, componentType);
	}

	//Returns a new entity with no components
	Entity NewEntity()
	{
#ifdef _DEBUG
		//Make sure there are not too many entities
		if (entityCount > UINT32_MAX)
		{
			std::cout << errorFormat << "ECS ERROR in NewEntity(): Too many Entities!" << normalFormat << std::endl;
			throw std::runtime_error("ECS ERROR: Too many Entities!");
		}
#endif

		entityCount++;

		//Make more entity IDs available in batches of 100
		if (availableEntities.size() == 0)
		{
			for (size_t i = entityCount + 99; i >= entityCount; i--)
			{
				availableEntities.push(i);
			}
		}

		//Get an available ID and mark it as used
		Entity entity = availableEntities.top();
		availableEntities.pop();
		usedEntities.insert(entity);
		entitySignatures[entity] = Signature();

		return entity;
	}

	//Delete an entity and all of its components
	void DestroyEntity(Entity entity)
	{
#ifdef _DEBUG
		//Make sure the entity exists
		if (!EntityExists(entity))
		{
			std::cout << warningFormat << "ECS WARNING in DestroyEntity(): The Entity you are trying to destroy does not exist!" << normalFormat << std::endl;
			return;
		}
#endif

		//Delete all components
		for (uint16_t i = 0; i < componentCount; i++)
		{
			if (entitySignatures[entity][i])
			{
				_RemoveComponentByName(entity, componentIDToType[i]);
			}
		}
		//Set the entitys signature to none temporarily
		entitySignatures[entity].reset();

		_OnEntitySignatureChanged(entity);

		//Set the entity as available and update relevant trackers
		entitySignatures.erase(entity);
		usedEntities.erase(entity);
		availableEntities.push(entity);
		entityCount--;
	}

	//Returns a reference to the desired system
	template<typename T>
	inline std::shared_ptr<T> GetSystem()
	{
		const char* systemType = typeid(T).name();

#ifdef _DEBUG
		//Make sure the system has been registered
		if (systems.count(systemType) == 0)
		{
			std::cout << errorFormat << "ECS ERROR in GetSystem(): The system has not been registered!" << normalFormat << std::endl;
			throw std::runtime_error("ECS ERROR: System not registered!");
		}
#endif

		return std::static_pointer_cast<T>(systems[systemType]);
	}

	//Register a system to require the specified components
	template<typename Sys, typename... Comps>
	std::shared_ptr<Sys> RegisterSystem()
	{
		const char* systemType = typeid(Sys).name();

#ifdef _DEBUG
		//Make sure the system has not been registered
		if (systems.count(systemType) != 0)
		{
			std::cout << warningFormat << "ECS WARNING in RegisterSystem(): The system has already been registered!" << normalFormat << std::endl;
			return GetSystem<Sys>();
		}
#endif

		//Make the signature and system
		systemSignatures[systemType] = _MakeSignature<Comps...>();
		std::shared_ptr<Sys> system = std::make_shared<Sys>();
		systems[systemType] = system;
		return system;
	}
}