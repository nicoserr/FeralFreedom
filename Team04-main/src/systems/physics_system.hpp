#pragma once

#include "../common.hpp"
#include "core/ecs.hpp"
#include "core/components.hpp"

// A simple physics system that moves rigid bodies and checks for collision
class PhysicsSystem
{
public:
    void step (float elapsed_ms);

    PhysicsSystem()
    {
    }
};