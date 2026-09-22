#version 330 core
in vec4 fragColor;
out vec4 outColor;

uniform sampler2D particleTexture;

void main() {
    vec4 texColor = texture(particleTexture, gl_PointCoord);
    outColor = fragColor * texColor;
    if (outColor.a < 0.1) discard;
}