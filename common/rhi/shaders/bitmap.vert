#version 450

#extension GL_GOOGLE_include_directive : require

#include "frame_ubo.glsl"

layout( location = 0 ) in vec2 inPosition;
layout( location = 1 ) in vec4 inColor;
layout( location = 2 ) in vec2 inTexCoord;

layout( location = 0 ) out vec2 fragUV;
layout( location = 1 ) out float fragAlpha;

void main()
{
    // The OpenGL canvas draws an image without the offset of its supersampled target
    gl_Position = withoutSupersampleOffset( projection * vec4( inPosition, 0.0, 1.0 ) );
    fragUV = inTexCoord;
    fragAlpha = inColor.a;
}
