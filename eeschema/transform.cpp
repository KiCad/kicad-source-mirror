
#include "macros.h"
#include "transform.h"


TRANSFORM& TRANSFORM::operator=( const TRANSFORM& aTransform )
{
    if( this == &aTransform )       // Check for self assingnemt;
        return *this;

    x1 = aTransform.x1;
    y1 = aTransform.y1;
    x2 = aTransform.x2;
    y2 = aTransform.y2;
    return *this;
}


bool TRANSFORM::operator==( const TRANSFORM& aTransform ) const
{
    return ( x1 == aTransform.x1 &&
             y1 == aTransform.y1 &&
             x2 == aTransform.x2 &&
             y2 == aTransform.y2 );
}


wxPoint TRANSFORM::TransformCoordinate( const wxPoint& aPoint ) const
{
    return wxPoint( ( x1 * aPoint.x ) + ( y1 * aPoint.y ),
                    ( x2 * aPoint.x ) + ( y2 * aPoint.y ) );
}


bool TRANSFORM::MapAngles( int* aAngle1, int* aAngle2 ) const
{
    wxCHECK_MSG( aAngle1 != NULL && aAngle2 != NULL, false,
                 wxT( "Cannot map NULL point angles." ) );

    int    Angle, Delta;
    double x, y, t;
    bool   swap = false;

    Delta = *aAngle2 - *aAngle1;
    if( Delta >= 1800 )
    {
        *aAngle1 -= 1;
        *aAngle2 += 1;
    }

    x = cos( *aAngle1 * M_PI / 1800.0 );
    y = sin( *aAngle1 * M_PI / 1800.0 );
    t = x * x1 + y * y1;
    y = x * x2 + y * y2;
    x = t;
    *aAngle1 = (int) ( atan2( y, x ) * 1800.0 / M_PI + 0.5 );

    x = cos( *aAngle2 * M_PI / 1800.0 );
    y = sin( *aAngle2 * M_PI / 1800.0 );
    t = x * x1 + y * y1;
    y = x * x2 + y * y2;
    x = t;
    *aAngle2 = (int) ( atan2( y, x ) * 1800.0 / M_PI + 0.5 );

    NORMALIZE_ANGLE( *aAngle1 );
    NORMALIZE_ANGLE( *aAngle2 );
    if( *aAngle2 < *aAngle1 )
        *aAngle2 += 3600;

    if( *aAngle2 - *aAngle1 > 1800 ) /* Need to swap the two angles. */
    {
        Angle   = (*aAngle1);
        *aAngle1 = (*aAngle2);
        *aAngle2 = Angle;

        NORMALIZE_ANGLE( *aAngle1 );
        NORMALIZE_ANGLE( *aAngle2 );
        if( *aAngle2 < *aAngle1 )
            *aAngle2 += 3600;
        swap = true;
    }

    if( Delta >= 1800 )
    {
        *aAngle1 += 1;
        *aAngle2 -= 1;
    }

    return swap;
}
