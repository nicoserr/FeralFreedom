#version 330

// Input attributes
in vec3 vertex;

// Application data
uniform vec2 light_position;
uniform mat3 projection;
uniform mat3 cameraTransform;

void main()
{
    vec3 temp = cameraTransform * vec3(vertex.xy - vertex.z * light_position, 1);
    temp = projection * temp;
    gl_Position = vec4(temp.xy, 0.0, 1.0-vertex.z);
}