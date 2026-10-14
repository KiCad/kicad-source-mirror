#version 450

layout( location = 0 ) in vec2  fragLocalPos;
layout( location = 1 ) in vec4  fragColor;
layout( location = 2 ) in float fragHalfLength;
layout( location = 3 ) in float fragHalfWidth;
layout( location = 4 ) in float fragAAPixelSize;

layout( location = 0 ) out vec4 outColor;

float sdSegment( vec2 p, vec2 a, vec2 b )
{
    vec2 pa = p - a;
    vec2 ba = b - a;
    float denom = dot( ba, ba );

    // A zero length segment is a dot, and 0 / 0 would leave it undefined
    float h = denom > 0.0 ? clamp( dot( pa, ba ) / denom, 0.0, 1.0 ) : 0.0;
    return length( pa - ba * h );
}

void main()
{
    vec2 endA = vec2( -fragHalfLength, 0.0 );
    vec2 endB = vec2(  fragHalfLength, 0.0 );

    // Enforce minimum 1px line width so thin wires remain visible at any zoom
    float minHalfWidth = max( fragHalfWidth, abs( fragAAPixelSize ) * 0.5 );
    float dist = sdSegment( fragLocalPos, endA, endB ) - minHalfWidth;

    // Analytical anti-aliasing via smoothstep over 1 pixel transition
    float aaWidth = abs( fragAAPixelSize ) * 0.5;
    float alpha = 1.0 - smoothstep( -aaWidth, aaWidth, dist );

    // A layer that takes each pixel once cannot have partly covered edges
    if( fragAAPixelSize < 0.0 )
        alpha = step( 0.5, alpha );

    if( alpha < 0.004 )
        discard;

    outColor = vec4( fragColor.rgb, fragColor.a * alpha );
}
