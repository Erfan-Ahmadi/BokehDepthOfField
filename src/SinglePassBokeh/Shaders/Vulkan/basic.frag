#version 450 core

layout(location = 0) in vec2 TexCoords;

layout (set = 0, binding = 1) uniform texture2D		Texture;
layout (set = 0, binding = 2) uniform sampler		uSampler0;

layout(location = 0) out vec4 outColor;

void main()
{
	outColor = texture(sampler2D(Texture, uSampler0), TexCoords);
}