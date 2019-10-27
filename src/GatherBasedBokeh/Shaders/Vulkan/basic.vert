#version 450 core

layout(location = 0) in vec3 InPosition;
layout(location = 1) in vec3 InNormal;
layout(location = 2) in vec2 InUV;

layout(std140, UPDATE_FREQ_PER_FRAME, binding = 0) uniform cbPerPass 
{
	uniform mat4		projView;
};

layout(std140, UPDATE_FREQ_NONE, binding = 0) uniform cbPerProp 
{
	uniform mat4		world;
};

layout(location = 0) out vec3 OutNormal;
layout(location = 1) out vec2 OutUV;

void main()
{
	gl_Position = projView * world * vec4(InPosition.xyz, 1.0f);
	OutNormal = mat3(world) * InNormal.xyz;
	OutUV = InUV;
}
