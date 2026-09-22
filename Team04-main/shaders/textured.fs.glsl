#version 330

// From vertex shader
in vec2 texcoord;

// Application data
uniform sampler2D sampler0;
uniform vec3 fcolor;
uniform vec2 uTexCoordMin = vec2(0.0, 0.0);
uniform vec2 uTexCoordMax = vec2(1.0, 1.0);
uniform float alpha = 1;

// Output color
layout(location = 0) out vec4 color;

void main()
{
    vec2 frameTexCoord = mix(uTexCoordMin, uTexCoordMax, texcoord);
    vec4 textureColor = texture(sampler0, frameTexCoord);
    color = vec4(textureColor.rgb, textureColor.a * alpha);
}
