#version 150

uniform sampler2D	uTexNormal;
uniform sampler2D	uTexAudio;

uniform bool		uEnableFallOff;
uniform bool        uEnableLines;
uniform float       uLineWidth;
uniform vec3        uLineColor1;
uniform vec3        uLineColor2;
uniform float        uLineGapAlpha;
uniform vec3        uVolumeColor;
uniform vec3        uFalloffColor;

uniform mat3 ciNormalMatrix;

in vec2 vTexCoord0;

// interpolated surface normal from vertex shader
in vec3 vNormal;
in vec3 vColor;

out vec4 oColor;

void main(){
    
    vec3 uLineColor = mix(uLineColor1,uLineColor2,vColor.g);
    
    float uVolume = texture( uTexAudio, vTexCoord0.xy ).r;
    uLineColor = mix(uLineColor, uVolumeColor, uVolume);
    
    // retrieve normal from texture
    vec3 Nmap = texture( uTexNormal, vTexCoord0.xy ).rgb;

    // modify it with the original surface normal
    const vec3 Ndirection = vec3(0.0, 1.0, 0.0);	// see: normal_map.frag (y-direction)
    vec3 Nfinal = ciNormalMatrix * normalize( vNormal + Nmap - Ndirection );

    // perform some falloff magic
    float falloff = sin( max( dot( Nfinal, vec3(0.25, 1.0, 0.25) ), 0.0) * 2.25);
    float alpha = 0.01 + ( uEnableFallOff ? 0.3 * pow( falloff, 25.0 ) : 0.3 );
    
    if (uEnableLines){
        // 2 point adaptation of https://codea.io/talk/discussion/3170/render-a-mesh-as-wireframe
        float a = vColor.r;
        float lineWidth = uLineWidth * pow( falloff, 25.0 );
        a = smoothstep( 1.0 - lineWidth, 1, a );
        oColor = vec4( uLineColor, mix(uLineGapAlpha,1,a) );
    }else{
        oColor = vec4( uLineColor, 1.0 );
    }
    
    oColor.rgb = mix( uFalloffColor, oColor.rgb, alpha);
    
}