#version 150

uniform mat4 ciModelViewProjection;

in vec4 ciPosition;
in vec2 ciTexCoord0;


out vec2 vTexCoord0;

void main()
{
	vTexCoord0 = ciTexCoord0;

	// simply pass position and texture coordinate on to the fragment shader
	gl_Position = ciModelViewProjection * ciPosition;
}