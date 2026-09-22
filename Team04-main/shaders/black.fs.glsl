#version 330

uniform sampler2D screen_texture;
uniform sampler2D shadow_map;
uniform float redness_timer;
uniform float vignette_amount;

uniform vec4 overlay_color;

uniform bool is_paused;
uniform bool is_game_over;
uniform vec2 boxCenter; // Center of the pause box
uniform vec2 boxSize;   // Size of the pause box
uniform vec4 boxColor;  // Color of the pause overlay (e.g., semi-transparent black)

in vec2 texcoord;
layout(location = 0) out vec4 color;

vec4 color_shift(vec4 in_color, vec2 uv) 
{
    if (redness_timer >= 0) {
        float vignette = 1 - 2*((uv.x - uv.x * uv.x) + (uv.y - uv.y * uv.y));
        in_color *= vec4(1.0, (1.0 - (sin(redness_timer/100.0)+1.0)/2.0  * vignette), (1.0 - (sin(redness_timer/100.0)+1.0)/2.0  * vignette), 1.0);
    }

    if (is_paused) {
        // Calculate the box region (normalized coordinates)
        vec2 halfSize = boxSize * 0.5;
        vec2 boxMin = boxCenter - halfSize;
        vec2 boxMax = boxCenter + halfSize;

        if (texcoord.x > boxMin.x && texcoord.x < boxMax.x && texcoord.y > boxMin.y && texcoord.y < boxMax.y) {
            // Blend the box color with the screen texture color within the box
            in_color = mix(in_color, boxColor, boxColor.a);
        }
    }

    if (is_game_over) {
        in_color.rgb *= 0.3; // Darken the screen by 70% if game over
    }

    in_color = vec4(vec3(overlay_color.rgb * overlay_color.a + in_color.rgb * (1-overlay_color.a)), 1.0);

	return in_color;
}

float blur(vec2 uv) {
    float offset = 1.0 / 900.0;
    vec2 offsets[9] = vec2[](
        vec2(-offset,  offset), // top-left
        vec2( 0.0f,    offset), // top-center
        vec2( offset,  offset), // top-right
        vec2(-offset,  0.0f),   // center-left
        vec2( 0.0f,    0.0f),   // center-center
        vec2( offset,  0.0f),   // center-right
        vec2(-offset, -offset), // bottom-left
        vec2( 0.0f,   -offset), // bottom-center
        vec2( offset, -offset)  // bottom-right    
    );

    float kernel[9] = float[](
        1, 2, 1,
        2,  4, 2,
        1, 2, 1
    );

    float sampleTex[9];
    for(int i = 0; i < 9; i++)
    {
        sampleTex[i] = min(texture(shadow_map, uv.xy + offsets[i]).r + max(0.2, 1.0f - vignette_amount), 1.0);
    }
    float col = 0.0;
    for(int i = 0; i < 9; i++)
        col += sampleTex[i] * kernel[i] / 16.0;
    
    return col;
}

void main()
{
	vec2 coord = texcoord;
    vec2 shadow_coord = texcoord;

    vec4 in_color = texture(screen_texture, coord);

    in_color = color_shift(in_color, texcoord);

    float vignette = 2.0f*((texcoord.x * texcoord.x - texcoord.x + 0.25f) + (texcoord.y * texcoord.y - texcoord.y + 0.25f));
    float shadow_mask = blur(shadow_coord) * (1.0f - vignette * vignette_amount);
    in_color = vec4(shadow_mask * max((1.0f - vignette_amount), 0.2) * in_color.xyz, in_color.w);

    color = in_color;
}