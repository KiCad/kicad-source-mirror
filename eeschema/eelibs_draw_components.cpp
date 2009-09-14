/****************************************/
/* Modules to handle component drawing. */
/****************************************/

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "class_drawpanel.h"
#include "program.h"
#include "libcmp.h"
#include "general.h"
#include "protos.h"

//#define DRAW_ARC_WITH_ANGLE       // Used to select function to draw arcs


/* Local functions */


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


/*****************************************************************************/
/*
 *  Routine to find a part in one of the libraries given its name.
 *  Name = Name of part.
 *  LibName = Name of Lib; if "": seach in all libs
 *  Alias = Flag: si flag != 0, retourne un pointeur sur une part ou un alias
 *                si flag = 0, retourne un pointeur sur une part meme si le nom
 *                correspond a un alias
 *  Alias = FIND_ROOT, ou Alias = FIND_ALIAS
 */
/*****************************************************************************/
LibCmpEntry* FindLibPart( const wxChar* Name, const wxString& LibName,
                          LibrEntryType type )
{
    LibCmpEntry*   Entry = NULL;
    LibraryStruct* Lib = g_LibraryList;

    FindLibName.Empty();

    while( Lib )
    {
        if( !LibName.IsEmpty() )
        {
            if( Lib->m_Name != LibName )
            {
                Lib = Lib->m_Pnext;
                continue;
            }
        }

        if( Lib == NULL )
            break;

        if( type == ROOT )
            Entry = (LibCmpEntry*) Lib->FindComponent( Name );
        else
            Entry = Lib->FindEntry( Name );

        if( Entry != NULL )
        {
            FindLibName = Lib->m_Name;
            break;
        }

        Lib = Lib->m_Pnext;
    }

    return Entry;
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


/*****************************************************************************
* Routine to display an outline version of given library entry.              *
* This routine is applied by the PlaceLibItem routine above.                 *
*****************************************************************************/
void DrawingLibInGhost( WinEDA_DrawPanel* panel, wxDC* DC,
                        EDA_LibComponentStruct* LibEntry,
                        SCH_COMPONENT* DrawLibItem, int PartX, int PartY,
                        int multi, int convert, int Color, bool DrawPinText )
{
    int DrawMode = g_XorMode;

    DrawLibPartAux( panel, DC, DrawLibItem, LibEntry, wxPoint( PartX, PartY ),
                    DrawLibItem->m_Transform, multi, convert, DrawMode, Color,
                    DrawPinText );
}
