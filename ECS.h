/*
MIT Licence
Copyright (c) 2023 Aleksi Anderson

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#pragma once
#include <bitset>
#include <stack>
#include <set>
#include <unordered_map>
#include <typeinfo>
#include <stdexcept>
#include <iostream>

//Allow max components to be determined outside this file
#ifndef ECS_MAX_COMPONENTS
#define ECS_MAX_COMPONENTS 100
#elif ECS_MAX_COMPONENTS > UINT16_MAX
#error The maximum possible number of components is 65535
#endif

namespace ecs
{
	//Define colors for errors and warnings
	constexpr const char* ERROR_FORMAT = "\033[31m";
	constexpr const char* WARNING_FORMAT = "\033[33m";
	constexpr const char* NORMAL_FORMAT = "\033[37m";

	//Entities as IDs, 0 will never be a valid ID
	using Entity = uint32_t;
	//Signatures as bitsets, where each component has its own bit
	using Signature = std::bitset<ECS_MAX_COMPONENTS>;

	//Entity Management Data

	//A list of all currently available Entity IDs, this is dynamically increased 100 at a time
	std::stack<Entity> availableEntities;
	//A list of all the currently in use entities
	std::set<Entity> usedEntities;
	//A map of an Entity's ID to its signature
	std::unordered_map<Entity, Signature> entitySignatures;
	//How many entities are currently reserved
	uint32_t entityCount = 0;

	//Component Management Data

	//Generic component array interface for component arrays
	class IComponentArray 
	{
	public:
		virtual void RemoveComponent(Entity entity) = 0;
	};
	//A map of every components name to it's corresponding component array
	std::unordered_map<const char*, std::shared_ptr<IComponentArray>> componentArrays;
	//A map from a components name to its ID
	std::unordered_map<const char*, uint16_t> componentTypeToID;
	//A map from a components ID to its name
	std::unordered_map<uint16_t, const char*> componentIDToType;
	//The amount of components registered. Also the next available component ID
	uint16_t componentCount = 0;

	//System Management Data

	//Base class all systems inherit from
	class System
	{
	public:
		//Set of every entity containing the required components for the system
		std::set<Entity> entities;
	};
	//Map of each system accessible by its name
	std::unordered_map<const char*, std::shared_ptr<System>> systems;
	//Map of each system's signature accessible by their type name
	std::unordered_map<const char*, Signature> systemSignatures;


	//System Functions

	//Register a system of type T
	template<typename T>
	std::shared_ptr<T> RegisterSystem()
	{
		const char* systemType = typeid(T).name();

		//Make sure the system has not been registered
		if (systems.count(systemType) != 0)
		{
			std::cout << WARNING_FORMAT << "ECS WARNING in RegisterSystem(): The system has already been registered!" << NORMAL_FORMAT << std::endl;
			return nullptr;
		}

		//Create new system and return a pointer to it
		std::shared_ptr<T> system = std::make_shared<T>();
		systems[systemType] = system;
		return system;
	}

	//Sets the signature (required components) for the system
	template<typename T>
	void SetSystemSignature(Signature signature)
	{
		const char* systemType = typeid(T).name();

		//Make sure the system has been registered
		if (systems.count(systemType) == 0)
		{
			std::cout << WARNING_FORMAT << "ECS WARNING in SetSignature(): The system has not been registered!" << NORMAL_FORMAT << std::endl;
			return;
		}
		//Make sure the system's signature has not already been set
		if (systemSignatures.count(systemType) != 0)
		{
			std::cout << WARNING_FORMAT << "ECS WARNING in SetSignature(): Don't try to set a system's signature twice!" << NORMAL_FORMAT << std::endl;
			return;
		}

		systemSignatures[systemType] = signature;
	}

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


	//Entity Functions

	//Create a new entity as a unique ID
	//Returns 0 on failure
	Entity NewEntity()
	{
		//Make sure there are not too many entities
		if (entityCount > UINT32_MAX)
		{
			std::cout << ERROR_FORMAT << "ECS ERROR in NewEntity(): Too many Entities!" << NORMAL_FORMAT << std::endl;
			throw std::runtime_error("ECS ERROR: Too many Entities!");
		}

		entityCount++;

		//Dynamically make entity IDs available in batches of 100
		if (availableEntities.size() == 0)
		{
			for (size_t i = entityCount + 99; i >= entityCount; i--)
			{
				availableEntities.push(i);
			}
		}

		//Get an available ID and mark the it as used
		Entity entity = availableEntities.top();
		availableEntities.pop();
		usedEntities.insert(entity);
		entitySignatures[entity] = Signature();

		return entity;
	}

	//Checks if the entity exists
	bool EntityExists(Entity entity)
	{
		return usedEntities.find(entity) != usedEntities.end();
	}

	//Makes the Entity available and destroys all its components
	void DestroyEntity(Entity entity)
	{
		//Make sure the entity exists
		if (!EntityExists(entity))
		{
			std::cout << WARNING_FORMAT << "ECS WARNING in DestroyEntity(): The Entity you are trying to destroy does not exist!" << NORMAL_FORMAT << std::endl;
			return;
		}

		//Delete all components
		for (uint16_t i = 0; i < componentCount; i++)
		{
			if (entitySignatures[entity][i])
			{
				componentArrays[componentIDToType[i]]->RemoveComponent(entity);
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


	//Component Functions

	//A component array class is created for each type of component
	template<typename T>
	class ComponentArray : public IComponentArray
	{
	private:
		//A map of entity to its component
		std::unordered_map<Entity, T> components;

	public:
		//Add a component to entity
		void AddComponent(Entity entity, T component)
		{
			//Make sure the entity does not already have the component
			if (HasComponent(entity))
			{
				std::cout << WARNING_FORMAT << "ECS WARNING in AddComponent(): Entity already has the component you are trying to add!" << NORMAL_FORMAT << std::endl;
				return;
			}

			components[entity] = component;
		}

		//Removes a component from entity and deletes all its data
		void RemoveComponent(Entity entity) override
		{
			//Make sure the entity has the component
			if (!HasComponent(entity))
			{
				std::cout << WARNING_FORMAT << "ECS WARNING in RemoveComponent(): Entity does not have the component you are trying to remove!" << NORMAL_FORMAT << std::endl;
				return;
			}

			components.erase(entity);
		}

		//Returns a reference to the component of entity
		T& GetComponent(Entity entity)
		{
			//Make sure the entity has the component
			if (!HasComponent(entity))
			{
				std::cout << ERROR_FORMAT << "ECS ERROR in GetComponent(): Entity does not have the desired component!" << NORMAL_FORMAT << std::endl;
				throw std::runtime_error("ECS ERROR: Entity does not have the desired component!");
			}

			return components[entity];
		}

		//Returns true if the entity has this type of component
		bool HasComponent(Entity entity)
		{
			return components.count(entity) > 0;
		}
	};

	//Register a new component of type T
	template<typename T>
	void RegisterComponent()
	{
		const char* componentType = typeid(T).name();

		//Make sure the component has not been previously registered
		if (componentArrays.count(componentType) != 0)
		{
			std::cout << WARNING_FORMAT << "ECS WARNING in RegisterComponent(): The component you are trying to register has alredy been registered!" << NORMAL_FORMAT << std::endl;
			return;
		}
		//Make sure there are not too many components registered
		if (componentCount >= ECS_MAX_COMPONENTS)
		{
			std::cout << ERROR_FORMAT << "ECS ERROR in RegisterComponent(): Too many registered components! The current limit is "
				<< ECS_MAX_COMPONENTS << ". Consider including \"#define ECS_MAX_COMPONENTS num\" before you include ECS.h!" << NORMAL_FORMAT << std::endl;
			throw std::runtime_error("ECS ERROR: Too many registered components!");
		}

		//Assigns an ID and makes a new component array for the registered component type
		componentTypeToID[componentType] = componentCount;
		componentIDToType[componentCount] = componentType;
		componentArrays[componentType] = std::make_shared<ComponentArray<T>>();

		componentCount++;
	}

	//Implementation internal function. Returns the component array of type T
	template<typename T>
	std::shared_ptr<ComponentArray<T>> _GetComponentArray()
	{
		const char* componentType = typeid(T).name();

		//Make sure the component has been registered
		if (componentArrays.count(componentType) == 0)
		{
			std::cout << ERROR_FORMAT << "ECS ERROR in AddComponent(): The component you are trying add has not been registered!" << NORMAL_FORMAT << std::endl;
			throw std::runtime_error("ECS ERROR: Component not registered!");
		}

		//Get the component array of type T from the componentArrays map
		return std::static_pointer_cast<ComponentArray<T>>(componentArrays[componentType]);
	}

	//Get a reference to entity's component of type T
	template<typename T>
	T& GetComponent(Entity entity)
	{
		//Make sure the entity exists
		if (!EntityExists(entity))
		{
			std::cout << ERROR_FORMAT << "ECS ERROR in GetComponent(): The Entity does not exist!" << NORMAL_FORMAT << std::endl;
			throw std::runtime_error("ECS ERROR: Entity does not exist!");
		}

		return _GetComponentArray<T>()->GetComponent(entity);
	}

	//Get the ID of a component
	template<typename T>
	uint16_t GetComponentID()
	{
		const char* componentType = typeid(T).name();

		//Make sure the component has been registered
		if (componentArrays.count(componentType) == 0)
		{
			std::cout << ERROR_FORMAT << "ECS ERROR in GetComponentID(): The component has not been registered!" << NORMAL_FORMAT << std::endl;
			throw std::runtime_error("ECS ERROR: Component not registered!");
		}

		return componentTypeToID[componentType];
	}

	//Add a component to entity. Returns a reference to that component
	template<typename T>
	T& AddComponent(Entity entity, T component)
	{
		//Make sure the entity exists
		if (!EntityExists(entity))
		{
			std::cout << ERROR_FORMAT << "ECS ERROR in AddComponent(): The entity you are trying to add the component to does not exist!" << NORMAL_FORMAT << std::endl;
			throw std::runtime_error("ECS ERROR: Entity does not exist!");
		}

		//Update the appropriate component array
		_GetComponentArray<T>()->AddComponent(entity, component);

		//Update the entity signature
		entitySignatures[entity].set(GetComponentID<T>());

		_OnEntitySignatureChanged(entity);
	}

	//Remove a component of type T from entity
	template<typename T>
	void RemoveComponent(Entity entity)
	{
		//Make sure the entity exists
		if (!EntityExists(entity))
		{
			std::cout << ERROR_FORMAT << "ECS ERROR in RemoveComponent(): The entity you are trying to remove the component from does not exist!" << NORMAL_FORMAT << std::endl;
			throw std::runtime_error("ECS ERROR: Entity does not exist!");
		}

		//Update the appropriate component array
		_GetComponentArray<T>()->RemoveComponent(entity);

		//Update the entity signature
		entitySignatures[entity].reset(GetComponentID<T>());

		_OnEntitySignatureChanged(entity);
	}
}