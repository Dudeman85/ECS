# Entity Component System (ECS)

*This is a very basic header-only implementation of Entity Component System, it is not meant to be used to make actual games.*

**ECS** or **Entity Component System** is an approach to designing game objects by having an **entity** represent the object, **components** which contain all the data of the object, and **systems** which are collections of functions and operate on the data in components.

This is generally much better than the traditional inheritance approach because:
- It is much more flexible in the ways you can make game objects.
- It is easier to understand how specific game objects work just by looking at their components.
- It achieves superior performance by making better use of the CPU cache.

## Basic Consepts
The ECS architectures consists of entities, components, and systems:
- **Entities can be though of as base game objects.** They don't contain any data and are simply numeric IDs.
- **Components store data attached to entities.** They are aggregate structs and have no functions.
- **Systems operate upon the data in components.** Each system has required components it needs to operate; if an entity has every required component it will be operated upon by the system.

## Usage
This is a header-only implementation, therefore simply include "ECS.h" in any files that use ECS. All types and functions are in the ecs namespace.<br>
**It also requires C++17 or higher!**

### Functions
Function | Example | Description
---|---|---
Entity NewEntity() | Entity e = NewEntity(); | Creates a new entity with no components.
bool EntityExists(Entity entity) | if(EntityExists(e)) | Returns true if an entity exists.
void DestroyEntity(Entity entity) | DestroyEntity(e); | Deletes an entity along with all its components. 
void RegisterComponent\<T\>() | RegisterComponent\<Position\>(); | Registers an aggregate struct as a component.
bool HasComponent\<T\>(Entity entity) | if(HasComponent\<Position\>(e)) | Returns true if an entity has a component of type T.
T& GetComponent\<T\>(Entity entity) | Position& pos = GetComponent\<Position\>(e); | Get a reference to an entity's component of type T.
uint16_t GetComponentID<T>() | uint16_t id = GetComponentID<Position>() | Get the ID of a component. Used in setting system signatures.
T& AddComponent(Entity entity, T component) | AddComponent(e, Position{ .x = 10.0f, .y = 25.25f }); | Add a component to an Entity. Also returns a reference to that component.
void RemoveComponent<T>(Entity entity) | RemoveComponent<Position>(e) | Remove a component of type T from an entity and delete all its data.
shared_ptr\<T\> RegisterSystem\<T\>() | auto gs = RegisterSystem\<GravitySystem\>(); | Registers a class as a system for ECS, the class must inherit from System.
void SetSystemSignature(Signature signature) | SetSystemSignature\<GravitySystem\>(gravitySystemSignature); | Sets the signature of required components for a system.

### Example
```cpp
#include "ECS.h"

//Example component	
struct Position
{
    float x, y, z;
};

//Example Gravity System, must inherit from ecs::System
//Requires Position component
class GravitySystem : public ecs::System
{
public:
    //The Update function of the Gravity system
    //This is not required, but is often useful
    void Update(double deltaTime)
    {
        //Loop through each entity available to this system
        //The loop should be done like this to avoid invalid iterators when deleting entities
        for (auto itr = entities.begin(); itr != entities.end();)
        {
            //Get the entity and increment the iterator
            ecs::Entity entity = *itr++;

            //Get the Position component of the entity
            Position& position = ecs::GetComponent<Position>(entity);

            //Operate upon the data in the entity's Position component
            position.y += gravity * deltaTime;
        }
    }

    //Systems can have members, but they cannot be entity specific
    float gravity = -9.81;
};

int main()
{
    //Register the Position component
    ecs::RegisterComponent<Position>();

    //Register the GravitySystem
    std::shared_ptr<GravitySystem> gravitySystem = ecs::RegisterSystem<GravitySystem>();

    //Create a new signature and set it to require the Position component
    ecs::Signature gravitySystemSignature;
    gravitySystemSignature.set(ecs::GetComponentID<Position>());

    //Assign the signature to the GravitySystem
    ecs::SetSystemSignature<GravitySystem>(gravitySystemSignature);

    //Create an entity
    ecs::Entity player = ecs::NewEntity();
    //Add the Position component to the player entity
    //This will also return a reference to that component
    Position& playerPosition = ecs::AddComponent(player, Position{ .x = 10.0f, .y = 25.25f, .z = 0.0f});

    //Game Loop
    while (true)
    {
        //Now you can use the gravity system
        //It will automatically operate upon every entity with the Position component
        gravitySystem->Update(0.016);
    }
}
```

Look in main.cpp for another example program.

## Design Goals
1. Maximum performance at run time.
2. Simple header only implementation.
3. Easy and simple to work with for the end user.

## TODO:
- Update docs
- Fix macro registration and add an init function