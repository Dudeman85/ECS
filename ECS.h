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
#include <cassert>
#include <iostream>

//Allow max components to be determined outside this file
#ifndef ECS_MAX_COMPONENTS
#define ECS_MAX_COMPONENTS 100
#endif

namespace ecs
{
	//Entities as IDs, 0 will never be a valid ID
	using Entity = uint32_t;

	//Signatures as bitsets, where each component has its own bit
	using Signature = std::bitset<ECS_MAX_COMPONENTS>;

	//A list of all currently available Entity IDs, this is dynamically increased 100 at a time
	std::stack<Entity> availableEntities;
	//A list of all the currently in use entities
	std::set<Entity> usedEntities;
	//A map of an Entity's ID to its signature
	std::unordered_map<Entity, Signature> entitySignatures;
	//How many entities are currently reserved
	uint32_t entityCount = 0;

	//A map of every component's type name to it's corresponding component array
	std::unordered_map<const char*, IComponentArray> componentArrays;

	//Create a new entity as a unique ID
	//Returns 0 on failure
	Entity NewEntity()
	{
		//Make sure there are not too many entities
		if (entityCount < UINT32_MAX)
		{
			std::cout << "ECS WARNING in NewEntity(): Too many Entities! You likely have an infinite loop!" << std::endl;
			return 0;
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
	//Returns true on success
	bool DestroyEntity(Entity entity)
	{
		//Make sure entity is exists
		if (usedEntities.find(entity) == usedEntities.end())
		{
			std::cout << "ECS WARNING in DestroyEntity(): The Entity you are trying to delete does not exist!" << std::endl;
			return false;
		}

		//Set the entity as available and update relevant trackers
		entitySignatures.erase(entity);
		usedEntities.erase(entity);
		availableEntities.push(entity);
		entityCount--;

		//TODO Delete components

		return true;
	}

	//Checks if the entity exists
	bool EntityExists(Entity entity)
	{
		return usedEntities.find(entity) == usedEntities.end();
	}

	//Generic component array interface for component arrays
	class IComponentArray {};
	//A component array class is created for each type of component
	template<typename T>
	class ComponentArray : private IComponentArray
	{
		//A map of entity to its component
		std::unordered_map<Entity, T> components;
	};
}