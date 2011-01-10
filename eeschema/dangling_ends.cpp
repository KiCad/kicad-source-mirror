/*********************/
/* dangling_ends.cpp */
/*********************/

#include "fctsys.h"
#include "gr_basic.h"
#include "sch_item_struct.h"
#include "wxEeschemaStruct.h"
#include "class_sch_screen.h"

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
            item->Draw( DrawPanel, aDC, wxPoint( 0, 0 ), g_XorMode );
            item->Draw( DrawPanel, aDC, wxPoint( 0, 0 ), GR_DEFAULT_DRAWMODE );
        }
    }
}
