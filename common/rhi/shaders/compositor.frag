#version 450

#extension GL_GOOGLE_include_directive : require
#include "compositor_push.glsl"

layout( set = 0, binding = 0 ) uniform sampler2D layerTexture;

layout( location = 0 ) in vec2 fragUV;
layout( location = 0 ) out vec4 outColor;

void main()
{
    vec4 layer = texture( layerTexture, fragUV );
    // A layer target starts transparent and is filled by blending, so its colour is already
    // premultiplied and takes only the layer's own alpha
    outColor = layer * layerAlpha;
}
