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
		virtual void HasComponent(Entity entity) = 0;
	};
	//A map of every components name to it's corresponding component array
	std::unordered_map<const char*, std::shared_ptr<IComponentArray>> componentArrays;
	//A map from a components name to its ID
	std::unordered_map<const char*, uint16_t> componentTypeToID;
	//The amount of components registered. Also the next available component ID
	uint16_t componentCount = 0;


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

	//Makes the Entity available and destroys all its components
	void DestroyEntity(Entity entity)
	{
		//Make sure entity is exists
		if (usedEntities.find(entity) == usedEntities.end())
		{
			std::cout << WARNING_FORMAT << "ECS WARNING in DestroyEntity(): The Entity you are trying to destroy does not exist!" << NORMAL_FORMAT << std::endl;
			return;
		}

		//Set the entity as available and update relevant trackers
		entitySignatures.erase(entity);
		usedEntities.erase(entity);
		availableEntities.push(entity);
		entityCount--;

		//TODO Delete components

	}

	//Checks if the entity exists
	bool EntityExists(Entity entity)
	{
		return usedEntities.find(entity) != usedEntities.end();
	}

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
			if (!HasComponent(entity))
			{
				std::cout << WARNING_FORMAT << "ECS WARNING in AddComponent(): Entity already has the component you are trying to add!" << NORMAL_FORMAT << std::endl;
				return;
			}

			components[entity] = component;
		}

		//Removes a component from entity and deletes all its data
		void RemoveComponent(Entity entity)
		{
			//Make sure the entity has the component
			if (HasComponent(entity))
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
			if (HasComponent(entity))
			{
				std::cout << ERROR_FORMAT << "ECS ERROR in GetComponent(): Entity does not have the desired component!" << NORMAL_FORMAT << std::endl;
				throw std::runtime_error("ECS ERROR: Entity does not have the desired component!");
			}

			return components[entity];
		}

		//Returns true if the entity has this typoe of component
		bool HasComponent(Entity entity) override
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
		componentArrays[componentType] = std::make_shared<ComponentArray<T>>();

		componentCount++;
	}
	
	//This is an implementation internal function
	//Returns the component array of type T
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
	
	//Add a component to entity
	//Returns a reference to that component
	template<typename T>
	T& AddComponent(Entity entity, T component)
	{
		//Make sure the entity exists
		if (!EntityExists(entity))
		{
			std::cout << ERROR_FORMAT << "ECS ERROR in AddComponent(): The entity you are trying to add the component to does not exist!" << NORMAL_FORMAT << std::endl;
			throw std::runtime_error("ECS ERROR: Entity does not exist!");
		}

		//Get the appropriate component array
		std::shared_ptr<ComponentArray<T>> componentArray = _GetComponentArray<T>();

		componentArray->AddComponent(entity, component);

		//TODO update entity signatures
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

		//Get the appropriate component array
		std::shared_ptr<ComponentArray<T>> componentArray = _GetComponentArray<T>();

		componentArray->RemoveComponent(entity, component);

		//TODO update entity signatures
	}
}