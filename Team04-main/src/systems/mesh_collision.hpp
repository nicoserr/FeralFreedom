#pragma once
#include "common.hpp"
#include <vector>

class MeshCollision {
public:
    // checks collision between two convex shapes represented by their vertices
    static bool checkSATCollision(const std::vector<vec2>& shape1, const std::vector<vec2>& shape2);

    // check collision between a triangle and an AABB
    static bool checkTriangleAABBCollision(const vec2& v0, const vec2& v1, const vec2& v2, const vec2& aabb_min, const vec2& aabb_max);

private:
    // Helper methods for SAT
    static std::vector<vec2> getNormals(const std::vector<vec2>& vertices);
    static void projectVertices(const std::vector<vec2>& vertices, const vec2& axis, float& min, float& max);
    static bool isOverlapping(const std::vector<vec2>& shape1, const std::vector<vec2>& shape2, const vec2& axis);
};
