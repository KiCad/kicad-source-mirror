varying vec4 offset[3];
varying vec2 texcoord;
varying vec2 pixcoord;

void main()
{
    texcoord = gl_MultiTexCoord0.st;
    SMAABlendingWeightCalculationVS( texcoord, pixcoord, offset );
    gl_Position = ftransform();
}