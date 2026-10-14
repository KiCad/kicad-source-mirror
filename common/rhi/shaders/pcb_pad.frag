#version 450

layout( location = 0 ) in vec2  fragLocalPos;
layout( location = 1 ) in vec4  fragColor;
layout( location = 2 ) in float fragHalfWidth;
layout( location = 3 ) in float fragHalfHeight;
layout( location = 4 ) in float fragCornerRadius;
layout( location = 5 ) in float fragShapeType;
layout( location = 6 ) in float fragAAPixelSize;
layout( location = 7 ) flat in float fragOverlayAlpha;

layout( location = 0 ) out vec4 outColor;

// Shape type constants matching CPU-side encoding
const float SHAPE_CIRCLE    = 0.0;
const float SHAPE_RECT      = 1.0;
const float SHAPE_ROUNDRECT = 2.0;
const float SHAPE_CHAMFERED = 3.0;
const float SHAPE_OVAL      = 4.0;
const float SHAPE_TRAPEZOID = 5.0;

float sdCircle( vec2 p, float r )
{
    return length( p ) - r;
}

float sdBox( vec2 p, vec2 b )
{
    vec2 d = abs( p ) - b;
    return length( max( d, 0.0 ) ) + min( max( d.x, d.y ), 0.0 );
}

float sdRoundedBox( vec2 p, vec2 b, float r )
{
    vec2 q = abs( p ) - b + r;
    return length( max( q, 0.0 ) ) + min( max( q.x, q.y ), 0.0 ) - r;
}

// Capsule/oval along X axis
float sdCapsule( vec2 p, float halfLen, float radius )
{
    p.x = p.x - clamp( p.x, -halfLen, halfLen );
    return length( p ) - radius;
}

// Box with 45-degree chamfer cuts at corners
float sdChamferedBox( vec2 p, vec2 b, float chamfer )
{
    vec2 ap = abs( p );
    float boxDist = sdBox( p, b );

    // Each corner is cut by the plane x + y = b.x + b.y - 2 * chamfer
    float chamferDist = ( ap.x + ap.y - ( b.x + b.y - 2.0 * chamfer ) ) * 0.7071068;

    return max( boxDist, chamferDist );
}

// Trapezoid centered at origin, wider at bottom
float sdTrapezoid( vec2 p, float halfWidthTop, float halfWidthBot, float halfHeight )
{
    float t = clamp( ( p.y + halfHeight ) / ( 2.0 * halfHeight ), 0.0, 1.0 );
    float halfW = mix( halfWidthBot, halfWidthTop, t );
    vec2 b = vec2( halfW, halfHeight );
    vec2 d = abs( p ) - b;
    return length( max( d, 0.0 ) ) + min( max( d.x, d.y ), 0.0 );
}

void main()
{
    float hw = fragHalfWidth;
    float hh = fragHalfHeight;
    float cr = fragCornerRadius;
    int shape = int( fragShapeType + 0.5 );

    float dist;

    switch( shape )
    {
    case 0: // CIRCLE
        dist = sdCircle( fragLocalPos, hw );
        break;

    case 1: // RECT
        dist = sdBox( fragLocalPos, vec2( hw, hh ) );
        break;

    case 2: // ROUNDRECT
        dist = sdRoundedBox( fragLocalPos, vec2( hw, hh ), cr );
        break;

    case 3: // CHAMFERED
        dist = sdChamferedBox( fragLocalPos, vec2( hw, hh ), cr );
        break;

    case 4: // OVAL
    {
        // Oval = capsule along the longer axis
        if( hw > hh )
            dist = sdCapsule( fragLocalPos, hw - hh, hh );
        else
            dist = sdCapsule( fragLocalPos.yx, hh - hw, hw );
        break;
    }

    case 5: // TRAPEZOID
    {
        // cr encodes the top-width ratio relative to bottom
        float halfWidthTop = hw * cr;
        dist = sdTrapezoid( fragLocalPos, halfWidthTop, hw, hh );
        break;
    }

    default:
        dist = sdBox( fragLocalPos, vec2( hw, hh ) );
        break;
    }

    // One pixel of transition, as tracks and arcs have. A wider one blurs the pad edge
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
