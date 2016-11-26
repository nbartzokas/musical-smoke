#version 150 core

in vec3 VertexPosition;
in float VertexStartTime;
in vec4 VertexColor;

out float Transp; // To Fragment Shader
out vec2 vPosition;
out float vSize;

uniform float MinParticleSize;
uniform float MaxParticleSize;

uniform float Time; 
uniform float ParticleLifetime;
uniform float Volume;

uniform mat4 ciModelViewProjection;
uniform vec4 ciPosition;

void main() {
	float age = Time - VertexStartTime;
	Transp = 0.0;
    vPosition = ciPosition.xy;
	gl_Position = ciModelViewProjection * vec4( VertexPosition.x + sin( Time - VertexStartTime ), VertexPosition.y + 0.2 * sin( Time + VertexStartTime ), VertexPosition.z , 1.0);
	if( Time >= VertexStartTime ) {
		float agePct = age / ParticleLifetime;
		Transp = 1.0 - agePct;
        vSize = mix( MinParticleSize, MaxParticleSize, agePct );
        gl_PointSize = vSize + 10.0 * sin( Time + VertexStartTime ) + 50.*Volume;
	}
}