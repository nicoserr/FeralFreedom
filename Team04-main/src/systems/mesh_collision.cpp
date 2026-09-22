#include "mesh_collision.hpp"
#include <iostream>

// REFERENCE: Used https://code.tutsplus.com/collision-detection-using-the-separating-axis-theorem--gamedev-169t and https://github.com/winstxnhdw/2d-separating-axis-theorem/tree/master as a guide to understand and implement SAT

// Generate normals for the edges of the shape (SAT requires edge normals)
std::vector<vec2> MeshCollision::getNormals(const std::vector<vec2>& vertices) {
    std::vector<vec2> normals;
    for (size_t i = 0; i < vertices.size(); ++i) {
        vec2 edge = vertices[(i + 1) % vertices.size()] - vertices[i];
        if (length(edge) == 0) {
            continue; // skip zero-length edges
        }
        vec2 normal = vec2(-edge.y, edge.x); // perpendicular vector
        normals.push_back(normalize(normal));
    }
    return normals;
}

// Project vertices onto an axis and get min and max projections
void MeshCollision::projectVertices(const std::vector<vec2>& vertices, const vec2& axis, float& min, float& max) {
    min = dot(vertices[0], axis);
    max = min;
    for (const auto& vertex : vertices) {
        float projection = dot(vertex, axis);
        if (projection < min) min = projection;
        if (projection > max) max = projection;
    }
}

// Check overlap on a single axis
bool MeshCollision::isOverlapping(const std::vector<vec2>& shape1, const std::vector<vec2>& shape2, const vec2& axis) {
    float min1, max1, min2, max2;
    projectVertices(shape1, axis, min1, max1);
    projectVertices(shape2, axis, min2, max2);
    return !(max1 < min2 || max2 < min1);
}

// Perform SAT collision check between two convex shapes
bool MeshCollision::checkSATCollision(const std::vector<vec2>& shape1, const std::vector<vec2>& shape2) {
    if (shape1.empty() || shape2.empty()) {
        std::cerr << "Error: One of the shapes is empty. Shape1 size: " << shape1.size() << ", Shape2 size: " << shape2.size() << std::endl;
        return false;
    }
    std::vector<vec2> normals1 = getNormals(shape1);
    std::vector<vec2> normals2 = getNormals(shape2);

    // check all normals from shape1
    for (const vec2& axis : normals1) {
        if (!isOverlapping(shape1, shape2, axis)) {
            return false; // separation found, no collision
        }
    }

    // check all normals from shape2
    for (const vec2& axis : normals2) {
        if (!isOverlapping(shape1, shape2, axis)) {
            return false; // separation found, no collision
        }
    }

    // intersects if all projections overlap
    return true;
}

// This is no longer needed. I left it for reference and possible future use
bool MeshCollision::checkTriangleAABBCollision(const vec2& v0, const vec2& v1, const vec2& v2, const vec2& aabb_min, const vec2& aabb_max) {
    std::vector<vec2> triangle_vertices = {
        vec2(v0.x, v0.y),
        vec2(v1.x, v1.y),
        vec2(v2.x, v2.y)
    };

    std::vector<vec2> aabb_vertices = {
        aabb_min,
        vec2(aabb_max.x, aabb_min.y),
        aabb_max,
        vec2(aabb_min.x, aabb_max.y)
    };

    std::vector<vec2> axes = getNormals(triangle_vertices);
    std::vector<vec2> aabb_axes = getNormals(aabb_vertices);
    axes.insert(axes.end(), aabb_axes.begin(), aabb_axes.end());

    // SAT on all axes
    for (const vec2& axis : axes) {
        float minTriangle, maxTriangle, minAABB, maxAABB;
        projectVertices(triangle_vertices, axis, minTriangle, maxTriangle);
        projectVertices(aabb_vertices, axis, minAABB, maxAABB);

        // Check for separating axis
        if (maxTriangle < minAABB || maxAABB < minTriangle) {
            return false;
        }
    }
    return true;
}