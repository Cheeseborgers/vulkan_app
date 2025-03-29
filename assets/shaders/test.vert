#version 460

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inUV;
layout(location = 2) in vec4 inColour;

layout(location = 3) in vec3 instancePosition;
layout(location = 4) in float instanceScale;
layout(location = 5) in float instanceRotation;

layout(location = 0) out vec2 texCoord;
layout(location = 1) out vec4 colour;

layout(binding = 0) uniform UniformBufferObject { mat4 WVP; }
ubo;

void main()
{
    vec3 pos = inPosition;
    float cosR = cos(instanceRotation);
    float sinR = sin(instanceRotation);
    vec2 rotatedPos = vec2(pos.x * cosR - pos.y * sinR, pos.x * sinR + pos.y * cosR);
    vec2 scaledPos = rotatedPos * instanceScale;
    vec3 finalPos = vec3(scaledPos + instancePosition.xy, pos.z + instancePosition.z);

    gl_Position = ubo.WVP * vec4(finalPos, 1.0);
    texCoord = inUV;
    colour = inColour;
}