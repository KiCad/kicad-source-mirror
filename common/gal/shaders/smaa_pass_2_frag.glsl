varying vec2 texcoord;
varying vec2 pixcoord;
varying vec4 offset[3];
uniform sampler2D edgesTex;
uniform sampler2D areaTex;
uniform sampler2D searchTex;

void main()
{
    gl_FragColor = SMAABlendingWeightCalculationPS(texcoord, pixcoord, offset, edgesTex, areaTex, searchTex, vec4(0.,0.,0.,0.));
}