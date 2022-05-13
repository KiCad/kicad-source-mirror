varying vec4 offset[3];
varying vec2 texcoord;

void main()
{
    texcoord = gl_MultiTexCoord0.st;
    SMAAEdgeDetectionVS( texcoord, offset);
    gl_Position   = ftransform();

}