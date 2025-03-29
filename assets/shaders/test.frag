#version 460

// Inputs from vertex shader
layout(location = 0) in vec2 texCoord;
layout(location = 1) in vec4 colour;

// Output color
layout(location = 0) out vec4 outColour;

// Texture sampler
layout(binding = 3) uniform sampler2D texSampler;

void main()
{
    vec4 texColor = texture(texSampler, texCoord);
    outColour = texColor * colour;
}