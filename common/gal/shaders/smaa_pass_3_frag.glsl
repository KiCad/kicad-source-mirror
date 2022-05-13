varying vec2 texcoord;
varying vec4 offset;
uniform sampler2D colorTex;
uniform sampler2D blendTex;

void main()
{
    gl_FragColor = SMAANeighborhoodBlendingPS(texcoord, offset, colorTex, blendTex);
}