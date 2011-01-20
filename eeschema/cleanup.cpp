/**************************************/
/* Code to handle schematic clean up. */
/**************************************/

#include "fctsys.h"
#include "common.h"
#include "trigo.h"
#include "confirm.h"
#include "macros.h"
#include "class_sch_screen.h"

#include "general.h"
#include "protos.h"
#include "netlist.h"
#include "sch_bus_entry.h"
#include "sch_junction.h"
#include "sch_line.h"


/* Routine to start/end segment (BUS or wires) on junctions.
 */
void BreakSegmentOnJunction( SCH_SCREEN* Screen )
{
    SCH_ITEM* DrawList;

    if( Screen == NULL )
    {
        DisplayError( NULL,
                      wxT( "BreakSegmentOnJunction() error: NULL screen" ) );
        return;
    }

    DrawList = Screen->GetDrawItems();

    while( DrawList )
    {
        switch( DrawList->Type() )
        {
        case SCH_JUNCTION_T:
            #undef STRUCT
            #define STRUCT ( (SCH_JUNCTION*) DrawList )
            BreakSegment( Screen, STRUCT->m_Pos );
            break;

        case SCH_BUS_ENTRY_T:
            #undef STRUCT
            #define STRUCT ( (SCH_BUS_ENTRY*) DrawList )
            BreakSegment( Screen, STRUCT->m_Pos );
            BreakSegment( Screen, STRUCT->m_End() );
            break;

        case SCH_LINE_T:
        case SCH_NO_CONNECT_T:
        case SCH_LABEL_T:
        case SCH_GLOBAL_LABEL_T:
        case SCH_HIERARCHICAL_LABEL_T:
        case SCH_COMPONENT_T:
        case SCH_POLYLINE_T:
        case SCH_MARKER_T:
        case SCH_TEXT_T:
        case SCH_SHEET_T:
        case SCH_SHEET_LABEL_T:
            break;

        default:
            break;
        }
        DrawList = DrawList->Next();
    }
}


/* Break a segment ( BUS, WIRE ) int 2 segments at location aBreakpoint,
 * if aBreakpoint in on segment segment
 * ( excluding ends)
 * fill aPicklist with modified items if non null
 */
void BreakSegment( SCH_SCREEN* aScreen, wxPoint aBreakpoint )
{
    SCH_LINE* segment, * NewSegment;

    for( SCH_ITEM* DrawList = aScreen->GetDrawItems(); DrawList; DrawList = DrawList->Next() )
    {
        if( DrawList->Type() != SCH_LINE_T )
            continue;

        segment = (SCH_LINE*) DrawList;

        if( !TestSegmentHit( aBreakpoint, segment->m_Start, segment->m_End, 0 ) )
            continue;

        /* A segment is found
         * It will be cut if aBreakpoint is not a segment end */
        if( ( segment->m_Start == aBreakpoint ) || ( segment->m_End == aBreakpoint ) )
            continue;

        /* Here we must cut the segment and create a new segment. */
        NewSegment = new SCH_LINE( *segment );
        NewSegment->m_Start = aBreakpoint;
        segment->m_End = NewSegment->m_Start;
        NewSegment->SetNext( segment->Next() );
        segment->SetNext( NewSegment );
        DrawList = NewSegment;
    }
}
