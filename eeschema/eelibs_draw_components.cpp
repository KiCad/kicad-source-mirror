/****************************************/
/* Modules to handle component drawing. */
/****************************************/

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "class_drawpanel.h"

#include "program.h"
#include "general.h"
#include "protos.h"
#include "class_library.h"

#include <boost/foreach.hpp>


//#define DRAW_ARC_WITH_ANGLE       // Used to select function to draw arcs


/***************************************************************************/
/** Function TransformCoordinate
 * Calculate the wew coordinate from the old one, according to the transform
 * matrix.
 * @param aTransformMatrix = rotation, mirror .. matrix
 * @param aPosition = the position to transform
 * @return the new coordinate
 */
/***************************************************************************/
wxPoint TransformCoordinate( const int      aTransformMatrix[2][2],
                             const wxPoint& aPosition )
{
    wxPoint new_pos;

    new_pos.x = ( aTransformMatrix[0][0] * aPosition.x ) +
        ( aTransformMatrix[0][1] * aPosition.y );
    new_pos.y = ( aTransformMatrix[1][0] * aPosition.x ) +
        ( aTransformMatrix[1][1] * aPosition.y );

    return new_pos;
}


/*****************************************************************************
 * Routine to rotate the given angular direction by the given Transformation. *
 * Input (and output) angles must be as follows:                              *
 *  Unit is 0.1 degre                                                         *
 * Angle1 in [0..3600], Angle2 > Angle1 in [0..7200]. Arc is assumed to be    *
 * less  than 180.0 degrees.                                                  *
 * Algorithm:                                                                 *
 * Map the angles to a point on the unit circle which is mapped using the     *
 * transform (only mirror and rotate so it remains on the unit circle) to     *
 * a new point which is used to detect new angle.                             *
 *****************************************************************************/
bool MapAngles( int* Angle1, int* Angle2, const int TransMat[2][2] )
{
    int    Angle, Delta;
    double x, y, t;
    bool   swap = FALSE;

    Delta = *Angle2 - *Angle1;
    if( Delta >= 1800 )
    {
        *Angle1 -= 1;
        *Angle2 += 1;
    }

    x = cos( *Angle1 * M_PI / 1800.0 );
    y = sin( *Angle1 * M_PI / 1800.0 );
    t = x * TransMat[0][0] + y * TransMat[0][1];
    y = x * TransMat[1][0] + y * TransMat[1][1];
    x = t;
    *Angle1 = (int) ( atan2( y, x ) * 1800.0 / M_PI + 0.5 );

    x = cos( *Angle2 * M_PI / 1800.0 );
    y = sin( *Angle2 * M_PI / 1800.0 );
    t = x * TransMat[0][0] + y * TransMat[0][1];
    y = x * TransMat[1][0] + y * TransMat[1][1];
    x = t;
    *Angle2 = (int) ( atan2( y, x ) * 1800.0 / M_PI + 0.5 );

    NORMALIZE_ANGLE( *Angle1 );
    NORMALIZE_ANGLE( *Angle2 );
    if( *Angle2 < *Angle1 )
        *Angle2 += 3600;

    if( *Angle2 - *Angle1 > 1800 ) /* Need to swap the two angles. */
    {
        Angle   = (*Angle1);
        *Angle1 = (*Angle2);
        *Angle2 = Angle;

        NORMALIZE_ANGLE( *Angle1 );
        NORMALIZE_ANGLE( *Angle2 );
        if( *Angle2 < *Angle1 )
            *Angle2 += 3600;
        swap = TRUE;
    }

    if( Delta >= 1800 )
    {
        *Angle1 += 1;
        *Angle2 -= 1;
    }

    return swap;
}
