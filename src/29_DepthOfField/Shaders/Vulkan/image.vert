#version 450 core

layout(location = 0) out vec2 outUV;

void main()
{
	outUV = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
	gl_Position = vec4(vec2(outUV * 2.0f - 1.0f) * vec2(1, -1), 0.0f, 1.0f);
}