#version 450

#extension GL_GOOGLE_include_directive : require

#include "frame_ubo.glsl"
#include "layer_dim.glsl"
#include "stroke_snap.glsl"
#include "element_cull.glsl"
#include "sdf_quad.glsl"
#include "instance_inputs.glsl"


layout( location = 0 ) out vec2  fragLocalPos;
layout( location = 1 ) out vec4  fragColor;
layout( location = 2 ) out float fragRadius;
layout( location = 3 ) out float fragHalfWidth;
layout( location = 4 ) out float fragStartAngle;
layout( location = 5 ) out float fragSweepAngle;
layout( location = 6 ) out float fragAAPixelSize;
layout( location = 7 ) flat out float fragOverlayAlpha;

const float PI = 3.14159265359;

// Box over an arc in its own frame. Quarter turn extremes inside the sweep widen it past the ends, and
// the sweep is padded a hair so an extreme at an end is never missed.
void arcBounds( float aRadius, float aStart, float aSweep, out vec2 aLow, out vec2 aHigh )
{
    float from = min( aStart, aStart + aSweep );
    float to   = max( aStart, aStart + aSweep );

    vec2 endA = aRadius * vec2( cos( aStart ), sin( aStart ) );
    vec2 endB = aRadius * vec2( cos( aStart + aSweep ), sin( aStart + aSweep ) );

    aLow  = min( endA, endB );
    aHigh = max( endA, endB );

    if( to - from >= 2.0 * PI - 2e-4 )
    {
        aLow  = vec2( -aRadius );
        aHigh = vec2( aRadius );
        return;
    }

    const float quarter = 0.5 * PI;
    float       first = ceil( ( from - 1e-4 ) / quarter );

    for( int ii = 0; ii < 4; ++ii )
    {
        float angle = ( first + float( ii ) ) * quarter;

        if( angle > to + 1e-4 )
            break;

        vec2 point = aRadius * vec2( cos( angle ), sin( angle ) );

        aLow  = min( aLow, point );
        aHigh = max( aHigh, point );
    }
}

void main()
{
    float scaleX = instRotScaleDepth.y;
    float scaleY = instRotScaleDepth.z;
    float depth  = instRotScaleDepth.w;

    float radius    = instParams.x;
    float halfWidth = instParams.y;

    // Read pixels per unit from the projection, as a 1nm probe vanishes in float32 at PCB coordinates of
    // around 1e8 nm.
    float pixelsPerUnit = length( vec2( projection[0][0] * viewport.x,
                                         projection[0][1] * viewport.y ) ) * 0.5 * scaleX;
    float aaPixelSize = ( pixelsPerUnit > 0.0 ) ? ( 1.0 / pixelsPerUnit ) : 0.001;

    // Width added in pixels, so a stroke such as a selection shadow keeps it through a zoom
    halfWidth += float( instFlags.y ) / 32.0 * aaPixelSize;

    vec2 center = instPosition;

    // A circle lands on whole pixels and strokes at least one pixel wide, as on the OpenGL canvas
    if( ( instFlags.x & 0x200u ) != 0u )
    {
        float widthPx = floor( 2.0 * halfWidth * pixelsPerUnit + 0.5 );
        float innerPx;

        // A hole wall reaches half a pixel under its hole so the antialiased hole edge never shows copper
        if( ( instFlags.x & 0x20u ) != 0u )
        {
            widthPx = max( widthPx, 1.0 ) + 0.5;
            innerPx = snapCircleToPixels( center, radius - halfWidth, pixelsPerUnit ) - 0.5;
        }
        else
        {
            float outerPx = snapCircleToPixels( center, radius, pixelsPerUnit ) + widthPx * 0.5;

            widthPx = max( widthPx, 1.0 );
            innerPx = outerPx - widthPx;
        }

        radius = ( innerPx + widthPx * 0.5 ) * aaPixelSize;
        halfWidth = widthPx * 0.5 * aaPixelSize;
    }

    fragColor = applyLayerDim( instColor, ( instFlags.x & 0x100u ) != 0u );

    if( elementLeavesNoMark( fragColor, instFlags.x ) )
    {
        gl_Position = vec4( 2.0, 2.0, 2.0, 1.0 );
        return;
    }

    // Expand halfWidth for bounding quad so subpixel strokes get rasterized
    float effectiveHalfWidth = max( halfWidth, aaPixelSize * 0.5 );

    float margin = effectiveHalfWidth + aaPixelSize * QUAD_REACH_PIXELS;

    vec2 low;
    vec2 high;
    arcBounds( radius, instParams.z, instParams.w, low, high );

    vec2 localPos = mix( low - margin, high + margin, inPosition * 0.5 + 0.5 );

    vec2 worldPos = localPos * vec2( scaleX, scaleY ) + center;
    gl_Position = projection * vec4( worldPos, depth, 1.0 );

    fragLocalPos   = localPos;
    fragRadius     = radius;
    fragHalfWidth  = halfWidth;
    fragStartAngle = instParams.z;
    fragSweepAngle = instParams.w;
    fragAAPixelSize = edgePixelSize( aaPixelSize, instFlags.x );

    fragOverlayAlpha = ( instFlags.x & 0x400u ) != 0u ? 1.0 : 0.0;
}
