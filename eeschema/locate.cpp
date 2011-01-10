/******************************************************/
/* Routines for locating an element of a schematic.   */
/******************************************************/

#include "fctsys.h"
#include "common.h"
#include "trigo.h"
#include "macros.h"
#include "class_sch_screen.h"

#include "general.h"
#include "protos.h"
#include "class_library.h"
#include "sch_bus_entry.h"
#include "sch_marker.h"
#include "sch_items.h"
#include "sch_component.h"
#include "sch_line.h"
#include "sch_no_connect.h"
#include "sch_polyline.h"
#include "sch_sheet.h"
#include "lib_pin.h"
#include "template_fieldnames.h"


static SCH_ITEM* LastSnappedStruct = NULL;
static bool SnapPoint2( const wxPoint& aPosRef, int SearchMask, SCH_ITEM* DrawList );


/**
 * Search the smaller (considering its area) component under the mouse
 * cursor or the pcb cursor
 *
 * If more than 1 component is found, a pointer to the smaller component is
 * returned
 */
SCH_COMPONENT* LocateSmallestComponent( SCH_SCREEN* Screen )
{
    SCH_COMPONENT* component = NULL, * lastcomponent = NULL;
    SCH_ITEM*      DrawList;
    EDA_Rect       BoundaryBox;
    float          sizeref = 0, sizecurr;

    DrawList = Screen->GetDrawItems();

    while( DrawList )
    {
        if( !SnapPoint2( Screen->m_MousePosition, LIBITEM, DrawList ) )
        {
            if( !SnapPoint2( Screen->m_Curseur, LIBITEM, DrawList ) )
                break;
        }

        component = (SCH_COMPONENT*) LastSnappedStruct;
        DrawList  = component->Next();

        if( lastcomponent == NULL )  // First time a component is located
        {
            lastcomponent = component;
            BoundaryBox   = lastcomponent->GetBoundingBox();
            sizeref = ABS( (float) BoundaryBox.GetWidth() * BoundaryBox.GetHeight() );
        }
        else
        {
            BoundaryBox = component->GetBoundingBox();
            sizecurr    = ABS( (float) BoundaryBox.GetWidth() * BoundaryBox.GetHeight() );

            if( sizeref > sizecurr )   // a smallest component is found
            {
                sizeref = sizecurr;
                lastcomponent = component;
            }
        }
    }

    return lastcomponent;
}


/* Search an item at pos refpos
 *  SearchMask = (bitwise OR):
 *  LIBITEM
 *  WIREITEM
 *  BUSITEM
 *  RACCORDITEM
 *  JUNCTIONITEM
 *  DRAWITEM
 *  TEXTITEM
 *  LABELITEM
 *  SHEETITEM
 *  MARKERITEM
 *  NOCONNECTITEM
 *  SEARCH_PINITEM
 *  SHEETLABELITEM
 *  FIELDCMPITEM
 *
 *  if EXCLUDE_WIRE_BUS_ENDPOINTS is set, in wire or bus search and locate,
 *  start and end points are not included in search
 *  if WIRE_BUS_ENDPOINTS_ONLY is set, in wire or bus search and locate,
 *  only start and end points are included in search
 *
 *
 *  Return:
 *          pointer on item found or NULL
 *
 */
SCH_ITEM* PickStruct( const wxPoint& refpos, SCH_SCREEN* screen, int SearchMask )
{
    if( screen == NULL || screen->GetDrawItems() == NULL )
        return NULL;

    if( SnapPoint2( refpos, SearchMask, screen->GetDrawItems() ) )
    {
        return LastSnappedStruct;
    }

    return NULL;
}


/*****************************************************************************
* Routine to search all objects for the closest point to a given point, in   *
* drawing space, and snap it to that points if closer than SnapDistance.     *
* Note we use L1 norm as distance measure, as it is the fastest.             *
* This routine updates LastSnappedStruct to the last object used in to snap  *
* a point. This variable is global to this module only (see above).          *
* The routine returns true if point was snapped.                             *
*****************************************************************************/
bool SnapPoint2( const wxPoint& aPosRef, int SearchMask, SCH_ITEM* DrawList )
{
    for( ; DrawList != NULL; DrawList = DrawList->Next() )
    {
        int hitminDist = MAX( g_DrawDefaultLineThickness, 3 );

        switch( DrawList->Type() )
        {
        case SCH_POLYLINE_T:
            #undef  STRUCT
            #define STRUCT ( (SCH_POLYLINE*) DrawList )
            if( !( SearchMask & (DRAWITEM | WIREITEM | BUSITEM) ) )
                break;

            for( unsigned i = 0; i < STRUCT->GetCornerCount() - 1; i++ )
            {
                if( TestSegmentHit( aPosRef, STRUCT->m_PolyPoints[i],
                                    STRUCT->m_PolyPoints[i + 1], hitminDist ) )
                {
                    LastSnappedStruct = DrawList;
                    return true;
                }
            }

            break;

        case SCH_LINE_T:
            #undef  STRUCT
            #define STRUCT ( (SCH_LINE*) DrawList )
            if( !( SearchMask & (DRAWITEM | WIREITEM | BUSITEM) ) )
                break;

            if( TestSegmentHit( aPosRef, STRUCT->m_Start, STRUCT->m_End, 0 ) )
            {
                if( ( ( SearchMask & DRAWITEM ) && ( STRUCT->GetLayer() == LAYER_NOTES ) )
                   || ( ( SearchMask & WIREITEM ) && ( STRUCT->GetLayer() == LAYER_WIRE ) )
                   || ( ( SearchMask & BUSITEM ) && ( STRUCT->GetLayer() == LAYER_BUS ) ) )
                {
                    if( SearchMask & EXCLUDE_WIRE_BUS_ENDPOINTS && STRUCT->IsEndPoint( aPosRef ) )
                        break;

                    if( SearchMask & WIRE_BUS_ENDPOINTS_ONLY && !STRUCT->IsEndPoint( aPosRef ) )
                        break;

                    LastSnappedStruct = DrawList;
                    return true;
                }
            }
            break;


        case SCH_BUS_ENTRY_T:
            #undef  STRUCT
            #define STRUCT ( (SCH_BUS_ENTRY*) DrawList )
            if( !( SearchMask & (RACCORDITEM) ) )
                break;

            if( TestSegmentHit( aPosRef, STRUCT->m_Pos, STRUCT->m_End(), hitminDist ) )
            {
                LastSnappedStruct = DrawList;
                return true;
            }
            break;

        case SCH_JUNCTION_T:
            #undef  STRUCT
            #define STRUCT ( (SCH_JUNCTION*) DrawList )
            if( !(SearchMask & JUNCTIONITEM) )
                break;
            if( STRUCT->HitTest( aPosRef ) )
            {
                LastSnappedStruct = DrawList;
                return true;
            }
            break;

        case SCH_NO_CONNECT_T:
            #undef  STRUCT
            #define STRUCT ( (SCH_NO_CONNECT*) DrawList )
            if( !(SearchMask & NOCONNECTITEM) )
                break;
            if( STRUCT->HitTest( aPosRef ) )
            {
                LastSnappedStruct = DrawList;
                return true;
            }
            break;

        case SCH_MARKER_T:
        {
            #undef  STRUCT
            #define STRUCT ( (SCH_MARKER*) DrawList )
            if( !(SearchMask & MARKERITEM) )
                break;
            if( STRUCT->HitTest( aPosRef ) )
            {
                LastSnappedStruct = DrawList;
                return true;
            }
            break;
        }

        case SCH_TEXT_T:
            #undef  STRUCT
            #define STRUCT ( (SCH_TEXT*) DrawList )
            if( !( SearchMask & TEXTITEM) )
                break;
            if( STRUCT->HitTest( aPosRef ) )
            {
                LastSnappedStruct = DrawList;
                return true;
            }
            break;


        case SCH_LABEL_T:
        case SCH_GLOBAL_LABEL_T:
        case SCH_HIERARCHICAL_LABEL_T:
            #undef  STRUCT
            #define STRUCT ( (SCH_TEXT*) DrawList ) // SCH_TEXT is the base
                                                    // class of these labels
            if( !(SearchMask & LABELITEM) )
                break;
            if( STRUCT->HitTest( aPosRef ) )
            {
                LastSnappedStruct = DrawList;
                return true;
            }
            break;

        case SCH_COMPONENT_T:
            if( !( SearchMask & (LIBITEM | FIELDCMPITEM) ) )
                break;

            if( SearchMask & FIELDCMPITEM )
            {
                SCH_COMPONENT* DrawLibItem = (SCH_COMPONENT*) DrawList;
                for( int i = REFERENCE; i < DrawLibItem->GetFieldCount(); i++ )
                {
                    SCH_FIELD* field = DrawLibItem->GetField( i );

                    if( field->m_Attributs & TEXT_NO_VISIBLE )
                        continue;

                    if( field->IsVoid() )
                        continue;

                    EDA_Rect BoundaryBox = field->GetBoundingBox();

                    if( BoundaryBox.Contains( aPosRef ) )
                    {
                        LastSnappedStruct = field;
                        return true;
                    }
                }
            }
            else
            {
                #undef  STRUCT
                #define STRUCT ( (SCH_COMPONENT*) DrawList )
                EDA_Rect BoundaryBox = STRUCT->GetBoundingBox();

                if( BoundaryBox.Contains( aPosRef ) )
                {
                    LastSnappedStruct = DrawList;
                    return true;
                }
            }
            break;

        case SCH_SHEET_T:
            #undef STRUCT
            #define STRUCT ( (SCH_SHEET*) DrawList )
            if( !(SearchMask & SHEETITEM) )
                break;
            if( STRUCT->HitTest( aPosRef ) )
            {
                LastSnappedStruct = DrawList;
                return true;
            }
            break;

        default:
        {
            wxString msg;
            msg.Printf( wxT( "SnapPoint2() error: unexpected struct type %d (" ),
                        DrawList->Type() );
            msg << DrawList->GetClass() << wxT( ")" );
            wxMessageBox( msg );
            break;
        }
        }
    }

    return FALSE;
}


SCH_SHEET_PIN* LocateSheetLabel( SCH_SHEET* Sheet, const wxPoint& pos )
{
    return Sheet->GetLabel( pos );
}


LIB_PIN* LocateAnyPin( SCH_ITEM* DrawList, const wxPoint& RefPos, SCH_COMPONENT** libpart )
{
    SCH_ITEM* item;
    SCH_COMPONENT* component = NULL;
    LIB_PIN* pin = NULL;

    for( item = DrawList; item != NULL; item = item->Next() )
    {
        if( item->Type() != SCH_COMPONENT_T )
            continue;

        component = (SCH_COMPONENT*) item;

        pin = (LIB_PIN*) component->GetDrawItem( RefPos, LIB_PIN_T );

        if( pin )
            break;
    }

    if( libpart )
        *libpart = component;

    return pin;
}


SCH_SHEET_PIN* LocateAnyPinSheet( const wxPoint& RefPos, SCH_ITEM* DrawList )
{
    SCH_ITEM* DrawStruct;
    SCH_SHEET_PIN* PinSheet = NULL;

    for( DrawStruct = DrawList; DrawStruct != NULL; DrawStruct = DrawStruct->Next() )
    {
        if( DrawStruct->Type() != SCH_SHEET_T )
            continue;

        PinSheet = LocateSheetLabel( (SCH_SHEET*) DrawStruct, RefPos );

        if( PinSheet )
            break;
    }

    return PinSheet;
}
