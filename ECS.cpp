#include <iostream>
#include <chrono>

#define ECS_MAX_COMPONENTS 100
#include "ECS.h"

#ifdef _DEBUG
#define ITERATIONS 1000
#else
#define ITERATIONS 10000
#endif
#define ENTITIES 10000


ECS_REGISTER_COMPONENT(Foo)
//Foo struct, this will become a component once it is registered as one
struct Foo : ecs::Component
{
	float a;
};

ECS_REGISTER_COMPONENT(Bar)
//Bar struct, this will become a component once it is registered as one
struct Bar : ecs::Component
{
	int a;
	float b;
};

ECS_REGISTER_SYSTEM(TestSystem, Foo, Bar)
//TestSystem system requires Foo and Bar
class TestSystem : public ecs::System
{
public:
	void Update()
	{
		//Loop through each entity available to this system
		for (auto itr = entities.begin(); itr != entities.end();)
		{
			//Get the entity and increment the iterator
			ecs::Entity entity = *itr++;

			//Get the relevant components
			Foo& foo = ecs::GetComponent<Foo>(entity);
			Bar& bar = ecs::GetComponent<Bar>(entity);

			//Operate upon the data in those components
			float c = foo.a * bar.a;
			bar.b = c;
		}
	}
};

int main()
{
	//Start test 1
	std::cout << "Start Test 1:\n";
	auto start = std::chrono::high_resolution_clock::now();

	//Register the Foo and Bar components
	//ecs::RegisterComponent<Bar>();
	//ecs::RegisterComponent<Foo>();

	//Register the TestSystem system and set its signature
	//std::shared_ptr<TestSystem> ts = ecs::RegisterSystem<TestSystem, Foo, Bar>();
	std::shared_ptr<TestSystem> ts = ecs::GetSystem<TestSystem>();

	//Make a bunch of entities
	for (int i = 0; i < ENTITIES; i++)
	{
		//Create a new entity
		ecs::Entity e = ecs::NewEntity();
		//Add the Foo and Bar components to the entity
		//They are also initialized here using designated initializers
		ecs::AddComponent(e, Foo{ .a = (float)i });
		ecs::AddComponent(e, Bar{ .a = i % 10 });

		if (i % 3 == 0)
		{
			ecs::RemoveComponent<Foo>(e);
		}
	}

	//End test 1
	auto end = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
	std::cout << "End Test 1: Took " << duration << "\n";
	//Start test 2
	std::cout << "Start Test 2:\n";
	start = std::chrono::high_resolution_clock::now();

	//Call the TestSystem System's Update function
	for (int i = 0; i < ITERATIONS; i++)
	{
		ts->Update();
	}

	//End tests
	end = std::chrono::high_resolution_clock::now();
	duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
	std::cout << "End Test 2: Took " << duration << "\n";
}
