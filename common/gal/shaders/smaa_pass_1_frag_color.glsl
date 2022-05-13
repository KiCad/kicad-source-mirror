varying vec2 texcoord;
varying vec4 offset[3];
uniform sampler2D colorTex;

void main()
{
    gl_FragColor.xy = SMAAColorEdgeDetectionPS(texcoord, offset, colorTex).xy;
}