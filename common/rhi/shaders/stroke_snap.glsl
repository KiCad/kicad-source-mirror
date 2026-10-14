// Shared by the vertex shaders that snap strokes and circles to pixels. Include after frame_ubo.glsl.

// Pixel position the OpenGL canvas gives a stroke axis point, or a circle centre for width 0. Odd widths stay
// off pixel corners so horizontal and vertical strokes stay sharp.
vec2 snapStrokePoint( vec2 aPixel, float aWidthPx )
{
    if( supersampled() )
        return vec2( floor( aPixel.x ), ceil( aPixel.y - 1.0 ) ) + 0.5;

    if( aWidthPx > 0.5 && aWidthPx < 1.5 )
        return floor( aPixel ) + 0.5;

    vec2 corner = vec2( floor( aPixel.x + 0.5 ), aWidthPx > 0.5 ? ceil( aPixel.y - 0.5 ) : floor( aPixel.y + 0.5 ) );

    return corner + mod( aWidthPx, 2.0 ) * vec2( 0.5, -0.5 );
}


// Move both ends of a stroke onto the pixel grid and round its width to whole pixels, as the
// OpenGL canvas does for lines. aAxis runs from the centre to one end.
void snapStrokeToPixels( inout vec2 aCenter, inout vec2 aAxis, inout float aHalfWidth, float aPixelsPerUnit )
{
    mat2 toPixel = mat2( projection[0].xy * viewport * 0.5, projection[1].xy * viewport * 0.5 );

    if( abs( determinant( toPixel ) ) < 1e-30 )
        return;

    mat2  toWorld = inverse( toPixel );
    float widthPx = max( floor( 2.0 * aHalfWidth * aPixelsPerUnit + 0.5 ), 1.0 );

    vec2 endA = aCenter - aAxis;
    vec2 endB = aCenter + aAxis;

    vec2 pixelA = ( ( projection * vec4( endA, 0.0, 1.0 ) ).xy * 0.5 + 0.5 ) * viewport;
    vec2 pixelB = ( ( projection * vec4( endB, 0.0, 1.0 ) ).xy * 0.5 + 0.5 ) * viewport;

    vec2 snappedA = snapStrokePoint( pixelA, widthPx );
    vec2 snappedB = snapStrokePoint( pixelB, widthPx );
    vec2 snappedMid = snapStrokePoint( ( pixelA + pixelB ) * 0.5, widthPx );

    // Ends derived from a 16 bit angle can be a thousandth of a radian off, so rounding them apart would tilt
    // a horizontal or vertical stroke.
    vec2 span = abs( pixelB - pixelA );

    if( span.x < max( 0.5, 0.001 * span.y ) )
        snappedA.x = snappedB.x = snappedMid.x;

    if( span.y < max( 0.5, 0.001 * span.x ) )
        snappedA.y = snappedB.y = snappedMid.y;

    endA += toWorld * ( snappedA - pixelA );
    endB += toWorld * ( snappedB - pixelB );

    aCenter    = ( endA + endB ) * 0.5;
    aAxis      = ( endB - endA ) * 0.5;
    aHalfWidth = widthPx * 0.5 / aPixelsPerUnit;
}


// Put a circle's centre on a pixel corner and return its radius rounded to whole pixels, as the
// OpenGL canvas draws circles. A hole and the wall drawn under it then round alike.
float snapCircleToPixels( inout vec2 aCenter, float aRadius, float aPixelsPerUnit )
{
    mat2 toPixel = mat2( projection[0].xy * viewport * 0.5, projection[1].xy * viewport * 0.5 );

    if( abs( determinant( toPixel ) ) >= 1e-30 )
    {
        vec2 pixel = ( ( projection * vec4( aCenter, 0.0, 1.0 ) ).xy * 0.5 + 0.5 ) * viewport;

        aCenter += inverse( toPixel ) * ( snapStrokePoint( pixel, 0.0 ) - pixel );
    }

    return floor( aRadius * aPixelsPerUnit + 0.5 );
}
