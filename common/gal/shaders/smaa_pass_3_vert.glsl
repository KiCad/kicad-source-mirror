varying vec4 offset;
varying vec2 texcoord;

void main()
{
    texcoord = gl_MultiTexCoord0.st;
    SMAANeighborhoodBlendingVS( texcoord, offset );
    gl_Position = ftransform();
}