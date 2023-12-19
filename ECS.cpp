#include <iostream>

#define ECS_MAX_COMPONENTS 100
#include "ECS.h"

struct foo
{
    bool a;
};
struct bar
{
    int b;
};

int main()
{
    std::cout << "Start Test:\n";

    ecs::RegisterComponent<foo>();
    ecs::RegisterComponent<bar>();

    ecs::Entity e = ecs::NewEntity();
    ecs::AddComponent(e, foo());



    ecs::availableEntities;

    std::cout << "End Test:\n";
}