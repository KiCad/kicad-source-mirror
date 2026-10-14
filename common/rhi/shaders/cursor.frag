#version 450

layout( set = 0, binding = 1 ) uniform CursorUBO
{
    vec2  cursorPos;        // Cursor position in NDC [-1, 1]
    vec2  viewport;
    vec4  crosshairColor;
    vec4  snapColor;
    float crosshairWidth;   // Width in pixels
    float snapRadius;       // Snap indicator radius in pixels
    int   showSnap;
    int   cursorType;       // 0 = small cross, 1 = fullscreen, 2 = diagonal 45°
    float cursorSize;       // Small cross extent in pixels
};

layout( location = 0 ) in vec2 fragUV;
layout( location = 0 ) out vec4 outColor;

void main()
{
    // Measured from a corner, pixel centres sit on half pixels for any viewport size. Snap to the centre
    // because an arm's end is a tie that interpolation error would break.
    vec2 pixelPos = floor( fragUV * viewport ) + 0.5;
    vec2 cursorPixelPos = ( cursorPos * 0.5 + 0.5 ) * viewport;
    cursorPixelPos = floor( cursorPixelPos ) + 0.5;

    vec2 diff = pixelPos - cursorPixelPos;

    float halfW = crosshairWidth * 0.5;
    float crossAlpha = 0.0;

    if( cursorType == 0 )
    {
        // Small crosshair of H/V lines limited to cursorSize pixels around the cursor
        float halfSize = cursorSize * 0.5;
        float hExtent = step( abs( diff.x ), halfSize );
        float vExtent = step( abs( diff.y ), halfSize );
        float alphaH = step( abs( diff.y ), halfW ) * hExtent;
        float alphaV = step( abs( diff.x ), halfW ) * vExtent;
        crossAlpha = max( alphaH, alphaV );
    }
    else if( cursorType == 2 )
    {
        // Fullscreen 45° diagonal crosshair
        float distPos = abs( diff.x - diff.y ) * 0.70710678;
        float distNeg = abs( diff.x + diff.y ) * 0.70710678;
        float alphaPos = step( distPos, halfW );
        float alphaNeg = step( distNeg, halfW );
        crossAlpha = max( alphaPos, alphaNeg );
    }
    else
    {
        // Fullscreen horizontal/vertical crosshair (default)
        float alphaH = step( abs( diff.y ), halfW );
        float alphaV = step( abs( diff.x ), halfW );
        crossAlpha = max( alphaH, alphaV );
    }

    // Snap indicator (circle at cursor position)
    float snapAlpha = 0.0;

    if( showSnap != 0 )
    {
        float dist = length( diff );
        snapAlpha = 1.0 - smoothstep( snapRadius - 1.0, snapRadius + 1.0, dist );
    }

    if( snapAlpha > crossAlpha )
        outColor = vec4( snapColor.rgb, snapColor.a * snapAlpha );
    else if( crossAlpha > 0.004 )
        outColor = vec4( crosshairColor.rgb, crosshairColor.a * crossAlpha );
    else
        discard;
}
