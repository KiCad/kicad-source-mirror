/*********************/
/* dangling_ends.cpp */
/*********************/

#include "fctsys.h"
#include "gr_basic.h"
#include "sch_item_struct.h"
#include "wxEeschemaStruct.h"

#include "general.h"
#include "protos.h"
#include "class_libentry.h"
#include "lib_pin.h"
#include "sch_component.h"


/* Returns true if the point P is on the segment S. */
bool SegmentIntersect( wxPoint aSegStart, wxPoint aSegEnd, wxPoint aTestPoint )
{
    wxPoint vectSeg   = aSegEnd - aSegStart;    // Vector from S1 to S2
    wxPoint vectPoint = aTestPoint - aSegStart; // Vector from S1 to P

    // Use long long here to avoid overflow in calculations
    if( (long long) vectSeg.x * vectPoint.y - (long long) vectSeg.y * vectPoint.x )
        return false;        /* Cross product non-zero, vectors not parallel */

    if( ( (long long) vectSeg.x * vectPoint.x + (long long) vectSeg.y * vectPoint.y ) <
        ( (long long) vectPoint.x * vectPoint.x + (long long) vectPoint.y * vectPoint.y ) )
        return false;          /* Point not on segment */

    return true;
}


void SCH_EDIT_FRAME::TestDanglingEnds( SCH_ITEM* aDrawList, wxDC* aDC )
{
    SCH_ITEM* item;
    std::vector< DANGLING_END_ITEM > endPoints;

    for( item = aDrawList; item != NULL; item = item->Next() )
        item->GetEndPoints( endPoints );

    for( item = aDrawList; item; item = item->Next() )
    {
        if( item->IsDanglingStateChanged( endPoints ) && aDC != NULL )
        {
            RedrawOneStruct( DrawPanel, aDC, item, g_XorMode );
            RedrawOneStruct( DrawPanel, aDC, item, GR_DEFAULT_DRAWMODE );
        }
    }
}


/**
 * Test if point pos is on a pin end.
 *
 * @param DrawList = List of SCH_ITEMs to check.
 * @param pos - Position of pin end to locate.
 * @return a LIB_PIN pointer to the located pin or NULL if no pin was found.
 */
LIB_PIN* SCH_EDIT_FRAME::LocatePinEnd( SCH_ITEM* DrawList, const wxPoint& pos )
{
    SCH_COMPONENT* DrawLibItem;
    LIB_PIN* Pin;
    wxPoint pinpos;

    Pin = LocateAnyPin( DrawList, pos, &DrawLibItem );

    if( !Pin )
        return NULL;

    pinpos = Pin->GetPosition();

    if( DrawLibItem == NULL )
        NEGATE( pinpos.y );     // In libraries Y axis is bottom to top
                                // and in schematic Y axis is top to bottom

    else                        // calculate the pin position in schematic
        pinpos = DrawLibItem->GetTransform().TransformCoordinate( pinpos ) + DrawLibItem->m_Pos;

    if( pos == pinpos )
        return Pin;

    return NULL;
}
