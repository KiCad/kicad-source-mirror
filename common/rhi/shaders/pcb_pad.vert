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
layout( location = 2 ) out float fragHalfWidth;
layout( location = 3 ) out float fragHalfHeight;
layout( location = 4 ) out float fragCornerRadius;
layout( location = 5 ) out float fragShapeType;
layout( location = 6 ) out float fragAAPixelSize;
layout( location = 7 ) flat out float fragOverlayAlpha;

void main()
{
    float rotation = instRotScaleDepth.x;
    float scaleX   = instRotScaleDepth.y;
    float scaleY   = instRotScaleDepth.z;

    float halfWidth    = instParams.x;
    float halfHeight   = instParams.y;
    float cornerRadius = instParams.z;
    float shapeType    = instParams.w;

    float cosR = cos( rotation );
    float sinR = sin( rotation );
    mat2 rotMat = mat2( cosR, sinR, -sinR, cosR );

    float pixelsPerUnit = length( vec2( projection[0][0] * viewport.x,
                                         projection[0][1] * viewport.y ) ) * 0.5 * scaleX;
    float aaPixelSize = ( pixelsPerUnit > 0.0 ) ? ( 1.0 / pixelsPerUnit ) : 0.001;

    // Size added in pixels, so a shape such as a selection shadow keeps it through a zoom
    halfWidth  += float( instFlags.y ) / 32.0 * aaPixelSize;
    halfHeight += float( instFlags.y ) / 32.0 * aaPixelSize;

    vec2 center = instPosition;

    fragColor = applyLayerDim( instColor, ( instFlags.x & 0x100u ) != 0u );

    if( elementLeavesNoMark( fragColor, instFlags.x ) )
    {
        gl_Position = vec4( 2.0, 2.0, 2.0, 1.0 );
        return;
    }

    // A circle lands on whole pixels as on the OpenGL canvas, so a hole matches its wall
    if( ( instFlags.x & 0x200u ) != 0u )
    {
        halfWidth = snapCircleToPixels( center, halfWidth, pixelsPerUnit ) * aaPixelSize;
        halfHeight = halfWidth;
    }

    float extent = max( halfWidth, halfHeight );
    float padding = aaPixelSize * QUAD_REACH_PIXELS;
    vec2 scaledPos = inPosition * ( extent + padding );

    vec2 worldPos = rotMat * ( scaledPos * vec2( scaleX, scaleY ) ) + center;

    float depth = instRotScaleDepth.w;
    gl_Position = projection * vec4( worldPos, depth, 1.0 );

    fragLocalPos    = inPosition * ( extent + padding );
    fragHalfWidth   = halfWidth;
    fragHalfHeight  = halfHeight;
    fragCornerRadius = cornerRadius;
    fragShapeType   = shapeType;
    fragAAPixelSize = edgePixelSize( aaPixelSize, instFlags.x );

    fragOverlayAlpha = ( instFlags.x & 0x400u ) != 0u ? 1.0 : 0.0;
}
