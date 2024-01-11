#define ENTITIES 10000
#define ITERATIONS 10000

//OLD IMPLEMENTATION

#include "ECSCore.h"
#include <iostream>
#include <chrono>

namespace t2 {

	ECS ecs;

	//Foo component
	struct Foo
	{
		float a;
	};
	//Bar Component
	struct Bar
	{
		int a;
		float b;
	};

	//TestSystem system requires Foo and Bar
	class TestSystem : public System
	{
	public:
		void Update()
		{
			//Loop through each entity available to this system
			for (auto itr = entities.begin(); itr != entities.end();)
			{
				//Get the entity and increment the iterator
				Entity entity = *itr++;

				//Get the relevant components
				Foo& foo = ecs.getComponent<Foo>(entity);
				Bar& bar = ecs.getComponent<Bar>(entity);

				//Operate upon the data in those components
				float c = foo.a * bar.a;
				bar.b = c;
			}
		}
	};

	int run()
	{
		//Start tests
		std::cout << "Start Test:\n";
		auto start = std::chrono::high_resolution_clock::now();

		//Register Foo and Bar components
		ecs.registerComponent<Foo>();
		ecs.registerComponent<Bar>();

		//Register the TestSystem system and set its signature
		std::shared_ptr<TestSystem> testSystem = ecs.registerSystem<TestSystem>();
		Signature testSignature;
		testSignature.set(ecs.getComponentId<Foo>());
		testSignature.set(ecs.getComponentId<Bar>());
		ecs.setSystemSignature<TestSystem>(testSignature);

		//Make a bunch of entities
		for (int i = 0; i < ENTITIES; i++)
		{
			//Create a new entity
			Entity e = ecs.newEntity();
			//Add the Foo and Bar components to the entity
			//They are also initialized here using designated initializers
			ecs.addComponent(e, Foo{ .a = (float)i });
			ecs.addComponent(e, Bar{ .a = i % 10 });

			if (i % 3 == 0)
			{
				ecs.removeComponent<Foo>(e);
			}
		}

		//Call the TestSystem System's Update function
		for (int i = 0; i < ITERATIONS; i++)
		{
			testSystem->Update();
		}

		//End tests
		auto end = std::chrono::high_resolution_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
		std::cout << "End Test: Took " << duration << "\n";
	}
}



//NEW IMPLEMENTATION

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
	float b;
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
			float c = foo.a * bar.a;
			bar.b = c;
		}
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

	//Register the TestSystem system and set its signature
	std::shared_ptr<TestSystem> testSystem = ecs::RegisterSystem<TestSystem>();
	ecs::Signature testSignature;
	testSignature.set(ecs::GetComponentID<Foo>());
	testSignature.set(ecs::GetComponentID<Bar>());
	ecs::SetSystemSignature<TestSystem>(testSignature);

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

	//Call the TestSystem System's Update function
	for (int i = 0; i < ITERATIONS; i++)
	{
		testSystem->Update();

	}


	//End tests
	auto end = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
	std::cout << "End Test: Took " << duration << "\n";

	t2::run();
}
