#version 450 core

layout(UPDATE_FREQ_PER_FRAME, binding = 0) uniform cbPerPass
{
	uniform mat4 proj;
	uniform mat4 view;
};

layout(location = 0) in vec3 Position;
layout(location = 1) in vec3 InstancePosition;
layout(location = 2) in vec3 InstanceColor;

layout(location = 0) out vec3 outColor;

void main()
{
	vec4 worldPos = vec4(InstancePosition + 0.5f * Position, 1.0f); 
	gl_Position = proj * view * worldPos;
	outColor = InstanceColor;
}