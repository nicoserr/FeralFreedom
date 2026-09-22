#version 330 core
layout (location = 1) in vec2 position;
layout (location = 2) in vec4 color;
layout (location = 3) in float size;

uniform mat3 projection;
uniform mat3 cameraTransform;
uniform mat3 transform;

out vec4 fragColor;

void main() {
    fragColor = color;
    vec3 worldPosition = transform * vec3(position, 1.0); 
    vec3 transformedPosition = cameraTransform * worldPosition; 
    vec3 projectedPosition = projection * transformedPosition;
    gl_Position = vec4(projectedPosition.xy, 1, 1);
    gl_PointSize = 50;
}