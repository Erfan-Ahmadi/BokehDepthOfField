#version 450 core

layout(location = 0) in vec4 Position;
layout(location = 1) in vec4 Normal;
layout(location = 2) in vec2 TexCoords;

layout(UPDATE_FREQ_PER_FRAME, binding = 0) uniform cbPerPass
{
	uniform mat4 proj;
	uniform mat4 view;
};

layout(std140, UPDATE_FREQ_NONE, binding = 0) uniform cbPerProp 
{
	uniform mat4		world;
};

layout(location = 0) out vec4 outNormal;
layout(location = 1) out vec2 outUV;
layout(location = 2) out vec4 outFragPos;

void main()
{
	outFragPos = world * Position;
	gl_Position = proj * view * outFragPos;
	outUV = TexCoords;
	outNormal = transpose(inverse(world)) * Normal;
}