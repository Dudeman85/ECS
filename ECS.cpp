#include <iostream>
#include <chrono>

#define ECS_MAX_COMPONENTS 100
#include "ECS.h"

//Foo component
struct Foo
{
	float a;
};
//Bar Component
struct Bar
{
	int a;
};

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

			std::cout << foo.a << " ";

			if (bar.a == 5)
			{
				ecs::DestroyEntity(entity);
			}
		}

		std::cout << std::endl;
	}
};

int main()
{
	//Start tests
	std::cout << "Start Test:\n";
	auto start = std::chrono::high_resolution_clock::now();


	//Register Foo and Bar components
	ecs::RegisterComponent<Foo>();
	ecs::RegisterComponent<Bar>();

	std::shared_ptr<TestSystem> testSystem = ecs::RegisterSystem<TestSystem>();
	ecs::Signature testSignature;
	testSignature.set(ecs::GetComponentID<Foo>());
	testSignature.set(ecs::GetComponentID<Bar>());
	ecs::SetSystemSignature<TestSystem>(testSignature);

	//Make a bunch of entities
	for (int i = 0; i < 100; i++)
	{
		//Create a new entity
		ecs::Entity e = ecs::NewEntity();
		//Add the Foo and Bar components to the entity
		//They are also initialized here using designated initializers
		ecs::AddComponent(e, Foo{.a = (float)i});
		ecs::AddComponent(e, Bar{.a = i % 10 });

		if (i % 3 == 0)
		{
			ecs::RemoveComponent<Foo>(e);
		}
	}

	//Call the TestSystem System's Update function
	testSystem->Update();
	testSystem->Update();


	//End tests
	auto end = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
	std::cout << "End Test: Took " << duration << "\n";
}