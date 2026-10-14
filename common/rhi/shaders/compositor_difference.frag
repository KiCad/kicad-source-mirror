#version 450

#extension GL_GOOGLE_include_directive : require
#include "compositor_push.glsl"

layout( set = 0, binding = 0 ) uniform sampler2D layerTexture;
layout( set = 0, binding = 1 ) uniform sampler2D belowTexture;

layout( location = 0 ) in vec2 fragUV;
layout( location = 0 ) out vec4 outColor;

void main()
{
    vec4 layer = texture( layerTexture, fragUV ) * layerAlpha;
    vec4 below = texture( belowTexture, fragUV );

    // The OpenGL diff layer (xor_diff_frag.glsl) applied to premultiplied colour, so identical content
    // cancels to black
    outColor = vec4( abs( layer.rgb - below.rgb ), max( layer.a, below.a ) );
}
