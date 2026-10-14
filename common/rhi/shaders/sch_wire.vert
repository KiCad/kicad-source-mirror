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
layout( location = 2 ) out float fragHalfLength;
layout( location = 3 ) out float fragHalfWidth;
layout( location = 4 ) out float fragAAPixelSize;

void main()
{
    float rotation = instRotScaleDepth.x;
    float scaleX   = instRotScaleDepth.y;
    float scaleY   = instRotScaleDepth.z;

    // GAL_MODERN line capsules encode param0=half-width, param1=half-length.
    float halfWidth  = instParams.x;
    float halfLength = instParams.y;

    float cosR = cos( rotation );
    float sinR = sin( rotation );
    mat2 rotMat = mat2( cosR, sinR, -sinR, cosR );

    // Extract pixels-per-unit from the projection matrix to avoid catastrophic
    // cancellation when probing two nearby world positions in float32.
    float pixelsPerUnit = length( vec2( projection[0][0] * viewport.x,
                                         projection[0][1] * viewport.y ) ) * 0.5 * scaleX;

    // Width added in pixels, so a stroke such as a selection shadow keeps it through a zoom
    if( pixelsPerUnit > 0.0 )
        halfWidth += float( instFlags.y ) / 32.0 / pixelsPerUnit;

    // Length added in pixels at each end, see RHI_INSTANCE_LENGTH_PAD
    if( ( instFlags.x & 0x4000u ) != 0u && pixelsPerUnit > 0.0 )
        halfLength += instParams.z / pixelsPerUnit;

    fragColor = applyLayerDim( instColor, ( instFlags.x & 0x100u ) != 0u );

    // FADE_SMALL strokes belong to text that the OpenGL canvas draws from a font texture. That
    // blurs into a faint patch below about six pixels of glyph height, given here in param 2.
    if( ( instFlags.x & 0x40u ) != 0u )
    {
        float glyphPixels = instParams.z * pixelsPerUnit;
        float legible = max( smoothstep( 4.0, 7.0, glyphPixels ), 0.3 * clamp( glyphPixels / 4.0, 0.0, 1.0 ) );

        fragColor.a *= legible;
    }

    if( elementLeavesNoMark( fragColor, instFlags.x ) )
    {
        gl_Position = vec4( 2.0, 2.0, 2.0, 1.0 );
        return;
    }

    vec2 center = instPosition;

    if( pixelsPerUnit > 0.0 )
    {
        vec2 axis = rotMat * vec2( halfLength * scaleX, 0.0 );

        snapStrokeToPixels( center, axis, halfWidth, pixelsPerUnit );

        float axisLength = length( axis );

        if( axisLength > 0.0 )
            rotMat = mat2( axis / axisLength, vec2( -axis.y, axis.x ) / axisLength );

        halfLength = axisLength / scaleX;
    }

    // The fragment shader clamps the stroke to a pixel
    float pixelSize = 1.0 / max( pixelsPerUnit, 1e-7 );
    float padding = max( halfWidth, 0.5 * pixelSize ) + QUAD_REACH_PIXELS * pixelSize;
    vec2 scaledPos = inPosition * vec2( halfLength + padding, padding );

    vec2 worldPos = rotMat * ( scaledPos * vec2( scaleX, scaleY ) ) + center;

    float depth = instRotScaleDepth.w;
    gl_Position = projection * vec4( worldPos, depth, 1.0 );

    fragLocalPos   = scaledPos;
    fragHalfLength = halfLength;
    fragHalfWidth  = halfWidth;
    fragAAPixelSize = edgePixelSize( ( pixelsPerUnit > 0.0 ) ? ( 1.0 / pixelsPerUnit ) : 0.001, instFlags.x );
}
