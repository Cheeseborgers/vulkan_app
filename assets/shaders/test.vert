#version 460

struct VertexData {
    float x, y, z;
    float u, v;
};

layout(binding = 0) readonly buffer Vertices { VertexData data[]; }
in_Vertices;

layout(binding = 1) readonly uniform UniformBuffer { mat4 WVP; }
ubo;

layout(location = 0) out vec2 texCoord;

void main()
{
    VertexData vtx = in_Vertices.data[gl_VertexIndex];

    vec3 pos = vec3(vtx.x, vtx.y, vtx.z);

    gl_Position = ubo.WVP * vec4(pos, 1.0);

    texCoord = vec2(vtx.u, vtx.v);
}