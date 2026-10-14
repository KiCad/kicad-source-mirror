#version 450

#extension GL_GOOGLE_include_directive : require

#include "frame_ubo.glsl"
#include "layer_dim.glsl"

layout( location = 0 ) in vec2 inPosition;
layout( location = 1 ) in vec4 inColor;
// x is the layer depth so fills depth sort with the instanced primitives, and y is non-zero for an element
// its layer's dim must not fade.
layout( location = 2 ) in vec2 inTexCoord;

layout( location = 0 ) out vec4 fragColor;

void main()
{
    gl_Position = projection * vec4( inPosition, inTexCoord.x, 1.0 );
    fragColor = applyLayerDim( inColor, inTexCoord.y != 0.0 );
}
