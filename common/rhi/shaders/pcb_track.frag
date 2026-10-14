#version 450

layout( location = 0 ) in vec2 fragLocalPos;
layout( location = 1 ) in vec4 fragColor;
layout( location = 2 ) in float fragHalfWidth;
layout( location = 3 ) in float fragHalfLength;
layout( location = 4 ) in float fragAAPixelSize;
layout( location = 5 ) flat in float fragOverlayAlpha;
layout( location = 6 ) flat in float fragGlyph;

layout( set = 1, binding = 0 ) uniform sampler2D glyphAtlas;

layout( location = 0 ) out vec4 outColor;

// Signed distance from point p to a line segment from a to b
float sdSegment( vec2 p, vec2 a, vec2 b )
{
    vec2 pa = p - a;
    vec2 ba = b - a;
    float denom = dot( ba, ba );

    // A zero length segment is a dot, and 0 / 0 would leave it undefined
    float h = denom > 0.0 ? clamp( dot( pa, ba ) / denom, 0.0, 1.0 ) : 0.0;
    return length( pa - ba * h );
}

float median( vec3 aValue )
{
    return max( min( aValue.r, aValue.g ), min( max( aValue.r, aValue.g ), aValue.b ) );
}

void main()
{
    // A glyph of the bitmap font is filtered as the OpenGL canvas does, fragHalfWidth being the
    // texels a pixel spans over four
    if( fragGlyph != 0.0 )
    {
        float edge = max( fragHalfWidth, 1e-4 );
        float cover = smoothstep( 0.5 - edge, 0.5 + edge, median( textureLod( glyphAtlas, fragLocalPos, 0.0 ).rgb ) );

        if( fragAAPixelSize < 0.0 )
            cover = step( 0.5, cover );

        if( cover < 0.004 )
            discard;

        if( fragOverlayAlpha != 0.0 )
        {
            float a = fragColor.a;
            outColor = vec4( fragColor.rgb * a * cover, a * a * cover );
            return;
        }

        outColor = vec4( fragColor.rgb, fragColor.a * cover );
        return;
    }

    // The track is a capsule along the local X axis
    vec2 endA = vec2( -fragHalfLength, 0.0 );
    vec2 endB = vec2(  fragHalfLength, 0.0 );

    // Enforce minimum 1px line width so thin tracks remain visible at any zoom
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

    // Premultiplied with alpha squared and both scaled by coverage, which is what the OpenGL overlay leaves,
    // see RHI_INSTANCE_OVERLAY_ALPHA.
    if( fragOverlayAlpha != 0.0 )
    {
        float a = fragColor.a;
        outColor = vec4( fragColor.rgb * a * alpha, a * a * alpha );
        return;
    }

    outColor = vec4( fragColor.rgb, fragColor.a * alpha );
}
