/**************************************/
/* Code to handle schematic clean up. */
/**************************************/

#include "fctsys.h"
#include "common.h"
#include "trigo.h"
#include "confirm.h"
#include "macros.h"

#include "program.h"
#include "general.h"
#include "protos.h"
#include "netlist.h"


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

    DrawList = Screen->EEDrawList;
    while( DrawList )
    {
        switch( DrawList->Type() )
        {
        case DRAW_JUNCTION_STRUCT_TYPE:
            #undef STRUCT
            #define STRUCT ( (SCH_JUNCTION*) DrawList )
            BreakSegment( Screen, STRUCT->m_Pos );
            break;

        case DRAW_BUSENTRY_STRUCT_TYPE:
            #undef STRUCT
            #define STRUCT ( (SCH_BUS_ENTRY*) DrawList )
            BreakSegment( Screen, STRUCT->m_Pos );
            BreakSegment( Screen, STRUCT->m_End() );
            break;

        case DRAW_SEGMENT_STRUCT_TYPE:
        case DRAW_NOCONNECT_STRUCT_TYPE:
        case TYPE_SCH_LABEL:
        case TYPE_SCH_GLOBALLABEL:
        case TYPE_SCH_HIERLABEL:
        case TYPE_SCH_COMPONENT:
        case DRAW_POLYLINE_STRUCT_TYPE:
        case TYPE_SCH_MARKER:
        case TYPE_SCH_TEXT:
        case DRAW_SHEET_STRUCT_TYPE:
        case DRAW_HIERARCHICAL_PIN_SHEET_STRUCT_TYPE:
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

    for( SCH_ITEM* DrawList = aScreen->EEDrawList; DrawList;
         DrawList = DrawList->Next() )
    {
        if( DrawList->Type() != DRAW_SEGMENT_STRUCT_TYPE )
            continue;

        segment = (SCH_LINE*) DrawList;

        if( !TestSegmentHit( aBreakpoint, segment->m_Start, segment->m_End, 0 ) )
            continue;

        /* ???
         * Segment connecte: doit etre coupe en 2 si px,py
         * n'est
         *  pas une extremite */
        if( ( segment->m_Start == aBreakpoint )
           || ( segment->m_End == aBreakpoint ) )
            continue;
        /* Here we must cut the segment into 2. */
        NewSegment = segment->GenCopy();
        NewSegment->m_Start = aBreakpoint;
        segment->m_End = NewSegment->m_Start;
        NewSegment->SetNext( segment->Next() );
        segment->SetNext( NewSegment );
        DrawList = NewSegment;
    }
}
