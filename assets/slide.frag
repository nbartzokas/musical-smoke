#version 150

in vec2				TexCoord0;

out vec4			outputColor;

uniform sampler2D	uTex0;
uniform float       dx;

float fadeSpeed = 0.0001f;

void main(){
    
    vec2 v = TexCoord0.xy;
    
    v.x += dx;
    
    outputColor = texture( uTex0, v);
    
    if (outputColor.r >= 0.01) outputColor.r -= fadeSpeed;
    else outputColor.r = 0.0;
    if (outputColor.b >= 0.01) outputColor.b -= fadeSpeed;
    else outputColor.b = 0.0;
    if (outputColor.g >= 0.01) outputColor.g -= fadeSpeed;
    else outputColor.g = 0.0;
    
}