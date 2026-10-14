#version 450

layout( set = 0, binding = 1 ) uniform GridUBO
{
    mat4  invProjection;
    vec2  viewport;
    vec2  gridOrigin;
    vec2  gridSpacing;
    float gridAngle;
    int   gridStyle;
    vec4  gridColor;
    vec4  axisColorX;
    vec4  axisColorY;
    float minorLineWidth;
    float majorLineWorld;   // world units, as the OpenGL canvas gives its line shader
    int   gridTick;
    float renderScale;      // target texels per viewport pixel, above 1 in a supersampled target
    float worldPixelSize;   // world units, as the OpenGL canvas gives its line shader
};

layout( location = 0 ) in vec2 fragUV;
layout( location = 0 ) out vec4 outColor;

vec2 screenToWorld( vec2 uv )
{
    vec2 ndc = uv * 2.0 - 1.0;
    vec4 world = invProjection * vec4( ndc, 0.0, 1.0 );
    return world.xy / world.w;
}

// Distance in pixels to a grid line snapped to the pixels the OpenGL canvas gives it, so no line shows
// moiré. toward is the side a wide odd line moves to.
float pixelAlignDist( float signedPixelDist, float fragmentOffset, float lineWidth, float toward )
{
    // A line half way between two pixels goes toward that side too. The little over keeps every
    // pixel of such a line on the same answer.
    float along = signedPixelDist * toward + 0.001;
    float snapped = floor( along + 0.5 );

    if( renderScale <= 1.0 && lineWidth > 1.5 )
        snapped = mod( lineWidth, 2.0 ) < 0.5 ? floor( along + 1.0 ) - 0.5 : floor( along ) + 1.0;

    return abs( snapped * toward - fragmentOffset );
}

vec2 pixelAlignDist( vec2 signedPixelDist, vec2 fragmentOffset, float lineWidth, vec2 toward )
{
    return vec2( pixelAlignDist( signedPixelDist.x, fragmentOffset.x, lineWidth, toward.x ),
                 pixelAlignDist( signedPixelDist.y, fragmentOffset.y, lineWidth, toward.y ) );
}

// The average of a supersampled target is its antialiasing, so a line is cut hard in it
float lineAlpha( float dist, float halfWidth, float pxSize )
{
    if( renderScale > 1.0 )
        return step( dist, halfWidth );

    return 1.0 - smoothstep( halfWidth - 0.5 * pxSize, halfWidth + 0.5 * pxSize, dist );
}

void main()
{
    // A line is whole pixels wide, as the line shader of the OpenGL canvas rounds its width
    float minorWidth = max( floor( minorLineWidth + 0.5 ), 1.0 );
    float majorWidth = max( floor( majorLineWorld / worldPixelSize + 0.5 ), 1.0 );

    // Lines snap to the pixels of the picture, of which a supersampled target has several
    // fragments each
    vec2 pixelUV = renderScale > 1.0 ? ( floor( fragUV * viewport ) + 0.5 ) / viewport : fragUV;
    vec2 worldPos = screenToWorld( pixelUV );

    vec2 worldPosR = screenToWorld( pixelUV + vec2( 1.0 / viewport.x, 0.0 ) );
    float pixelSize = length( worldPosR - worldPos );
    vec2 fragOff = mat2( invProjection ) * ( ( fragUV - pixelUV ) * 2.0 ) / pixelSize;

    // Right and up on the screen, along the world axes
    vec2 toward = vec2( invProjection[0][0] < 0.0 ? -1.0 : 1.0, invProjection[1][1] < 0.0 ? 1.0 : -1.0 );

    // Axes go through the world origin at the minor width, with or without the grid beneath
    float axisDistX = pixelAlignDist( -worldPos.y / pixelSize, fragOff.y, minorWidth, toward.y ) * pixelSize;
    float axisDistY = pixelAlignDist( -worldPos.x / pixelSize, fragOff.x, minorWidth, toward.x ) * pixelSize;
    float axisHalfW = minorWidth * pixelSize * 0.5;

    // Fold the colour alpha in so a hidden axis falls through to the grid beneath it
    float axisAlphaX = lineAlpha( axisDistX, axisHalfW, pixelSize ) * axisColorX.a;
    float axisAlphaY = lineAlpha( axisDistY, axisHalfW, pixelSize ) * axisColorY.a;

    if( axisAlphaX > 0.0 || axisAlphaY > 0.0 )
    {
        if( axisAlphaX >= axisAlphaY )
            outColor = vec4( axisColorX.rgb, axisAlphaX );
        else
            outColor = vec4( axisColorY.rgb, axisAlphaY );

        return;
    }

    float gridPixels = min( gridSpacing.x, gridSpacing.y ) / max( pixelSize, 0.0001 );

    if( gridColor.a <= 0.0 || gridPixels < 2.0 )
        discard;

    vec2 p = worldPos - gridOrigin;

    if( abs( gridAngle ) > 0.001 )
    {
        float c = cos( -gridAngle );
        float s = sin( -gridAngle );
        p = vec2( c * p.x - s * p.y, s * p.x + c * p.y );
        fragOff = vec2( c * fragOff.x - s * fragOff.y, s * fragOff.x + c * fragOff.y );
    }

    float alpha = 0.0;
    vec2 majorSpacing = gridSpacing * float( gridTick );

    if( gridStyle == 0 )
    {
        float nearMinX = round( p.x / gridSpacing.x ) * gridSpacing.x;
        float nearMinY = round( p.y / gridSpacing.y ) * gridSpacing.y;
        float dMinX = pixelAlignDist( ( nearMinX - p.x ) / pixelSize, fragOff.x, minorWidth, toward.x ) * pixelSize;
        float dMinY = pixelAlignDist( ( nearMinY - p.y ) / pixelSize, fragOff.y, minorWidth, toward.y ) * pixelSize;
        float dMin  = min( dMinX, dMinY );
        float halfMinW = minorWidth * pixelSize * 0.5;
        float minorAlpha = lineAlpha( dMin, halfMinW, pixelSize );

        float nearMajX = round( p.x / majorSpacing.x ) * majorSpacing.x;
        float nearMajY = round( p.y / majorSpacing.y ) * majorSpacing.y;
        float dMajX = pixelAlignDist( ( nearMajX - p.x ) / pixelSize, fragOff.x, majorWidth, toward.x ) * pixelSize;
        float dMajY = pixelAlignDist( ( nearMajY - p.y ) / pixelSize, fragOff.y, majorWidth, toward.y ) * pixelSize;
        float dMaj  = min( dMajX, dMajY );
        float halfMajW = majorWidth * pixelSize * 0.5;
        float majorAlpha = lineAlpha( dMaj, halfMajW, pixelSize );

        alpha = max( minorAlpha, majorAlpha );
    }
    else if( gridStyle == 1 )
    {
        vec2 minorCell = round( p / gridSpacing ) * gridSpacing;
        vec2 minorOff  = ( minorCell - p ) / pixelSize;
        vec2 dMinor = pixelAlignDist( minorOff, fragOff, minorWidth, toward ) * pixelSize;
        float minorHalf = minorWidth * pixelSize * 0.5;
        float minorAlpha = lineAlpha( max( dMinor.x, dMinor.y ), minorHalf, pixelSize );

        vec2 majorCell = round( p / majorSpacing ) * majorSpacing;
        vec2 majorOff  = ( majorCell - p ) / pixelSize;
        vec2 dMajor = pixelAlignDist( majorOff, fragOff, majorWidth, toward ) * pixelSize;
        float majorHalf = majorWidth * pixelSize * 0.5;
        float majorAlpha = lineAlpha( max( dMajor.x, dMajor.y ), majorHalf, pixelSize );

        alpha = max( minorAlpha, majorAlpha );
    }
    else if( gridStyle == 2 )
    {
        vec2 minorCell = round( p / gridSpacing ) * gridSpacing;
        vec2 minorOff  = ( minorCell - p ) / pixelSize;
        vec2 dMinor = pixelAlignDist( minorOff, fragOff, minorWidth, toward ) * pixelSize;
        float minorHalf = minorWidth * pixelSize * 0.5;
        float minorArm  = minorLineWidth * pixelSize * 2.0;

        float minorAlpha = 0.0;

        if( dMinor.y < minorHalf + 0.5 * pixelSize && dMinor.x < minorArm + 0.5 * pixelSize )
        {
            float aY = lineAlpha( dMinor.y, minorHalf, pixelSize );
            float aX = lineAlpha( dMinor.x, minorArm, pixelSize );
            minorAlpha = max( minorAlpha, min( aY, aX ) );
        }

        if( dMinor.x < minorHalf + 0.5 * pixelSize && dMinor.y < minorArm + 0.5 * pixelSize )
        {
            float aX = lineAlpha( dMinor.x, minorHalf, pixelSize );
            float aY = lineAlpha( dMinor.y, minorArm, pixelSize );
            minorAlpha = max( minorAlpha, min( aX, aY ) );
        }

        vec2 majorCell = round( p / majorSpacing ) * majorSpacing;
        vec2 majorOff  = ( majorCell - p ) / pixelSize;
        vec2 dMajor = pixelAlignDist( majorOff, fragOff, majorWidth, toward ) * pixelSize;
        float majorHalf = majorWidth * pixelSize * 0.5;
        float majorArm  = minorLineWidth * pixelSize * 4.0;

        float majorAlpha = 0.0;

        if( dMajor.y < majorHalf + 0.5 * pixelSize && dMajor.x < majorArm + 0.5 * pixelSize )
        {
            float aY = lineAlpha( dMajor.y, majorHalf, pixelSize );
            float aX = lineAlpha( dMajor.x, majorArm, pixelSize );
            majorAlpha = max( majorAlpha, min( aY, aX ) );
        }

        if( dMajor.x < majorHalf + 0.5 * pixelSize && dMajor.y < majorArm + 0.5 * pixelSize )
        {
            float aX = lineAlpha( dMajor.x, majorHalf, pixelSize );
            float aY = lineAlpha( dMajor.y, majorArm, pixelSize );
            majorAlpha = max( majorAlpha, min( aX, aY ) );
        }

        alpha = max( minorAlpha, majorAlpha );
    }

    if( alpha < 0.004 )
        discard;

    outColor = vec4( gridColor.rgb, gridColor.a * alpha );
}
