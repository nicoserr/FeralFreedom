#version 330

layout (location = 0) in vec4 vertex;
out vec2 TexCoords;

uniform mat4 projection;
uniform mat4 transform;

void main()
{
	gl_Position = projection * transform * vec4(vertex.xy, 0.0, 1.0);
	TexCoords = vertex.zw;
}