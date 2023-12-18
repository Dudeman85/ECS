#include <iostream>

#define ECS_MAX_COMPONENTS 200
#include "ECS.h"

int main()
{
    ecs::Signature signature;
    std::cout << signature.size();
    std::cout << "Hello World!\n";

    ecs::Entity e = ecs::NewEntity();
    std::cout << ecs::availableEntities.size();
    std::cout << std::endl << e;
}