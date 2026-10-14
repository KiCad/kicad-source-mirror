#version 450

layout( location = 0 ) in vec2  fragLocalPos;
layout( location = 1 ) in vec4  fragColor;
layout( location = 2 ) in float fragRadius;
layout( location = 3 ) in float fragHalfWidth;
layout( location = 4 ) in float fragStartAngle;
layout( location = 5 ) in float fragSweepAngle;
layout( location = 6 ) in float fragAAPixelSize;
layout( location = 7 ) flat in float fragOverlayAlpha;

layout( location = 0 ) out vec4 outColor;

const float PI = 3.14159265359;

// Signed distance from p to an arc stroke centred at the origin
float sdArc( vec2 p, float radius, float startAngle, float sweepAngle, float halfWidth )
{
    float ringDist = abs( length( p ) - radius ) - halfWidth;

    // A clockwise full circle would otherwise reduce to a zero span, see the full circle test in pcb_arc.vert
    if( abs( sweepAngle ) >= 2.0 * PI - 2e-4 )
        return ringDist;

    float angle = atan( p.y, p.x );

    float endAngle = startAngle + sweepAngle;

    float normStart = mod( startAngle, 2.0 * PI );

    if( normStart < 0.0 )
        normStart += 2.0 * PI;

    float normEnd = normStart + abs( sweepAngle );

    float normAngle = mod( angle, 2.0 * PI );

    if( normAngle < 0.0 )
        normAngle += 2.0 * PI;

    // The span may wrap past 2 * PI
    bool inArc;

    if( sweepAngle >= 0.0 )
    {
        if( normEnd <= 2.0 * PI )
            inArc = ( normAngle >= normStart && normAngle <= normEnd );
        else
            inArc = ( normAngle >= normStart || normAngle <= normEnd - 2.0 * PI );
    }
    else
    {
        // A negative sweep means the arc goes clockwise
        float ns = mod( startAngle + sweepAngle, 2.0 * PI );

        if( ns < 0.0 )
            ns += 2.0 * PI;

        float ne = normStart;

        if( ne >= ns )
            inArc = ( normAngle >= ns && normAngle <= ne );
        else
            inArc = ( normAngle >= ns || normAngle <= ne );
    }

    if( inArc )
        return ringDist;

    // Outside the angular range, compute distance to the arc endpoints (rounded caps)
    float cosS = cos( startAngle );
    float sinS = sin( startAngle );
    vec2 endpointA = radius * vec2( cosS, sinS );

    float cosE = cos( endAngle );
    float sinE = sin( endAngle );
    vec2 endpointB = radius * vec2( cosE, sinE );

    float distA = length( p - endpointA ) - halfWidth;
    float distB = length( p - endpointB ) - halfWidth;

    return min( distA, distB );
}

void main()
{
    float minHalfWidth = max( fragHalfWidth, abs( fragAAPixelSize ) * 0.5 );
    float dist = sdArc( fragLocalPos, fragRadius, fragStartAngle, fragSweepAngle, minHalfWidth );

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
