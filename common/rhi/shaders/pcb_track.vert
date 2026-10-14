#version 450

#extension GL_GOOGLE_include_directive : require

#include "frame_ubo.glsl"
#include "layer_dim.glsl"
#include "stroke_snap.glsl"
#include "element_cull.glsl"
#include "sdf_quad.glsl"
#include "instance_inputs.glsl"
#include "glyph_atlas.glsl"


layout( location = 0 ) out vec2 fragLocalPos;
layout( location = 1 ) out vec4 fragColor;
layout( location = 2 ) out float fragHalfWidth;
layout( location = 3 ) out float fragHalfLength;
layout( location = 4 ) out float fragAAPixelSize;
layout( location = 5 ) flat out float fragOverlayAlpha;
layout( location = 6 ) flat out float fragGlyph;

void main()
{
    // A stroke past its level of detail is collapsed to a point off screen, so it is neither
    // binned nor shaded, see RHI_INSTANCE_SCALE_LOD
    if( ( instFlags.x & 0x800u ) != 0u && !( instParams.w < viewScale ) )
    {
        gl_Position = vec4( 2.0, 2.0, 2.0, 1.0 );
        return;
    }

    float rotation = instRotScaleDepth.x;
    float scaleX   = instRotScaleDepth.y;
    float scaleY   = instRotScaleDepth.z;
    float depth    = instRotScaleDepth.w;

    float halfWidth  = instParams.x;
    float halfLength = instParams.y;

    float cosR = cos( rotation );
    float sinR = sin( rotation );
    mat2 rotMat = mat2( cosR, sinR, -sinR, cosR );

    // Read pixels per unit from the projection, as a 1nm probe vanishes in float32 at PCB coordinates of
    // around 1e8 nm.
    float pixelsPerUnit = length( vec2( projection[0][0] * viewport.x,
                                         projection[0][1] * viewport.y ) ) * 0.5 * scaleX;
    float aaPixelSize = ( pixelsPerUnit > 0.0 ) ? ( 1.0 / pixelsPerUnit ) : 0.001;

    // SCREEN_SPACE params are pixel counts, converted with the live projection so the size holds through a zoom
    uint flags = instFlags.x;

    if( ( flags & 0x10u ) != 0u )
    {
        halfWidth  *= aaPixelSize;
        halfLength *= aaPixelSize;
    }

    // Width added in pixels, so a stroke such as a selection shadow keeps it through a zoom
    halfWidth += float( instFlags.y ) / 32.0 * aaPixelSize;

    // Length added in pixels at each end, see RHI_INSTANCE_LENGTH_PAD
    if( ( flags & 0x4000u ) != 0u )
        halfLength += instParams.z * aaPixelSize;

    fragColor = applyLayerDim( instColor, ( instFlags.x & 0x100u ) != 0u );

    // FADE_SMALL strokes belong to text that the OpenGL canvas draws from a font texture. That
    // blurs into a faint patch below about six pixels of glyph height, given here in param 2.
    if( ( flags & 0x40u ) != 0u )
    {
        float glyphPixels = instParams.z * pixelsPerUnit;
        float legible = max( smoothstep( 4.0, 7.0, glyphPixels ), 0.3 * clamp( glyphPixels / 4.0, 0.0, 1.0 ) );

        fragColor.a *= legible;
    }

    if( elementLeavesNoMark( fragColor, flags ) )
    {
        gl_Position = vec4( 2.0, 2.0, 2.0, 1.0 );
        return;
    }

    fragGlyph = 0.0;

    // Param 0 and 1 are the glyph quad's half sizes and param 2 the glyph. Atlas texels per target texel
    // are constant over the quad, so the filter width is worked out here.
    if( ( flags & 0x8000u ) != 0u )
    {
        vec4 rect = glyphRects[min( uint( instParams.z + 0.5 ), GLYPH_TABLE_SIZE - 1u )];
        vec2 quadHalf = vec2( halfWidth, halfLength );

        vec2 worldPos = rotMat * ( inPosition * quadHalf * vec2( scaleX, scaleY ) ) + instPosition;
        gl_Position = projection * vec4( worldPos, depth, 1.0 );

        fragLocalPos = rect.xy + rect.zw * vec2( 0.5 + 0.5 * inPosition.x, 0.5 - 0.5 * inPosition.y );
        fragHalfWidth = 0.25 * rect.z * atlasSize.x * aaPixelSize / ( 2.0 * halfWidth * max( renderScale, 1.0 ) );
        fragHalfLength = 0.0;
        fragAAPixelSize = ( flags & 0x80u ) != 0u ? -aaPixelSize : aaPixelSize;
        fragOverlayAlpha = ( flags & 0x400u ) != 0u ? 1.0 : 0.0;
        fragGlyph = 1.0;
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

    // Expand halfWidth for bounding quad so subpixel strokes get rasterized
    float effectiveHalfWidth = max( halfWidth, aaPixelSize * 0.5 );
    float padding = aaPixelSize * QUAD_REACH_PIXELS;

    vec2 scaledPos = inPosition * vec2( halfLength + effectiveHalfWidth + padding,
                                         effectiveHalfWidth + padding );

    vec2 worldPos = rotMat * ( scaledPos * vec2( scaleX, scaleY ) ) + center;
    gl_Position = projection * vec4( worldPos, depth, 1.0 );

    fragLocalPos = inPosition * vec2( halfLength + effectiveHalfWidth + padding,
                                       effectiveHalfWidth + padding );
    fragHalfWidth = halfWidth;
    fragHalfLength = halfLength;
    fragAAPixelSize = edgePixelSize( aaPixelSize, instFlags.x );

    fragOverlayAlpha = ( instFlags.x & 0x400u ) != 0u ? 1.0 : 0.0;
}
