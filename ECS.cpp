#include <iostream>

#define ECS_MAX_COMPONENTS 100
#include "ECS.h"

int main()
{
    ecs::Signature signature;
    std::cout << signature.size();

    ecs::Entity e = ecs::NewEntity();
    std::cout << ecs::availableEntities.size();
    std::cout << std::endl << e;
    ecs::availableEntities;
}