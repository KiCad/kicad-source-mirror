#version 450

layout( location = 0 ) in vec2 inPosition;
layout( location = 1 ) in vec4 inColor;
layout( location = 2 ) in vec2 inTexCoord;

layout( location = 0 ) out vec2 fragUV;

void main()
{
    gl_Position = vec4( inPosition, 0.0, 1.0 );
    fragUV = inPosition * 0.5 + 0.5;
}
