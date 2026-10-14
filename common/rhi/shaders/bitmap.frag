#version 450

layout( set = 1, binding = 0 ) uniform sampler2D bitmapTexture;

layout( location = 0 ) in vec2 fragUV;
layout( location = 1 ) in float fragAlpha;

layout( location = 0 ) out vec4 outColor;

void main()
{
    vec4 texel = texture( bitmapTexture, fragUV );
    outColor = vec4( texel.rgb, texel.a * fragAlpha );
}
