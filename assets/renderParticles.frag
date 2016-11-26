#version 150
#extension all : warn

uniform sampler2D ParticleTex;

uniform float Volume;
uniform float r1;
uniform float r2;
uniform float g1;
uniform float g2;
uniform float b1;
uniform float b2;
uniform float a1;
uniform float a2;

in float Transp;
in vec2 vPosition;
in float vSize;

out vec4 FragColor;

void main() {
    
	FragColor = texture( ParticleTex, gl_PointCoord );
    
    vec3 fragColorInverted = vec3(1 - FragColor.r, 1 - FragColor.g, 1 - FragColor.b);
    
    float d = 2 * distance( vec2(0.5,0.5), gl_PointCoord );
    
    fragColorInverted.r = 1 - smoothstep(r1,r2,d);
    fragColorInverted.g = 1 - smoothstep(g1,g2,d);
    fragColorInverted.b = 1 - smoothstep(b1,b2,d);
    
    FragColor = vec4( fragColorInverted, FragColor.a );
    
    FragColor = mix( FragColor, vec4(1,1,1,FragColor.a), Volume );
    
	FragColor.a *= (Transp);
	FragColor.a -= .1;
    
}