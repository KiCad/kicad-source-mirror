/******************************************************/
/* Routines for locating an element of a schematic.   */
/******************************************************/

#include "fctsys.h"
#include "common.h"
#include "program.h"
#include "trigo.h"
#include "macros.h"

#include "general.h"
#include "class_marker_sch.h"
#include "protos.h"
#include "class_library.h"


static bool IsItemInBox(EDA_Rect& aBox, SCH_ITEM* DrawStruct );

static SCH_ITEM* LastSnappedStruct = NULL;
static bool SnapPoint2( const wxPoint& aPosRef, int SearchMask,
                        SCH_ITEM* DrawList, double aScaleFactor );


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

    DrawList = Screen->EEDrawList;

    while( DrawList )
    {
        if( ( SnapPoint2( Screen->m_MousePosition, LIBITEM,
                          DrawList, Screen->GetZoom() ) ) == FALSE )
        {
            if( ( SnapPoint2( Screen->m_Curseur, LIBITEM,
                              DrawList, Screen->GetScalingFactor() ) ) == FALSE )
                break;
        }
        component = (SCH_COMPONENT*) LastSnappedStruct;
        DrawList  = component->Next();
        if( lastcomponent == NULL )  // First time a component is located
        {
            lastcomponent = component;
            BoundaryBox   = lastcomponent->GetBoundaryBox();
            sizeref = ABS( (float) BoundaryBox.GetWidth() *
                           BoundaryBox.GetHeight() );
        }
        else
        {
            BoundaryBox = component->GetBoundaryBox();
            sizecurr    = ABS( (float) BoundaryBox.GetWidth() *
                               BoundaryBox.GetHeight() );
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
SCH_ITEM* PickStruct( const wxPoint& refpos,
                      BASE_SCREEN*   screen,
                      int            SearchMask )
{
    bool Snapped;

    if( screen == NULL || screen->EEDrawList == NULL )
        return NULL;

    if( ( Snapped = SnapPoint2( refpos, SearchMask,
                                screen->EEDrawList,
                                screen->GetScalingFactor() ) ) != FALSE )
    {
        return LastSnappedStruct;
    }
    return NULL;
}


/** Function PickStruct
 * Search items in a block
 * @return items count
 * @param aBlock a BLOCK_SELECTOR that gives the search area boundary
 * list of items is stored in aBlock
 */
int PickItemsInBlock( BLOCK_SELECTOR& aBlock, BASE_SCREEN* aScreen )
{
    int itemcount = 0;

    if( aScreen == NULL )
        return itemcount;

    EDA_Rect area;
    area.SetOrigin( aBlock.GetOrigin());
    area.SetSize( aBlock.GetSize() );

    area.Normalize();

    ITEM_PICKER picker;
    SCH_ITEM*   DrawStruct = aScreen->EEDrawList;
    for( ; DrawStruct != NULL; DrawStruct = DrawStruct->Next() )
    {
        if( IsItemInBox( area, DrawStruct ) )
        {
            /* Put this structure in the picked list: */
            picker.m_PickedItem     = DrawStruct;
            picker.m_PickedItemType = DrawStruct->Type();
            aBlock.PushItem( picker );
            itemcount++;
        }
    }

    return itemcount;
}


/*****************************************************************************
* Routine to search all objects for the closest point to a given point, in   *
* drawing space, and snap it to that points if closer than SnapDistance.     *
* Note we use L1 norm as distance measure, as it is the fastest.             *
* This routine updates LastSnappedStruct to the last object used in to snap  *
* a point. This variable is global to this module only (see above).          *
* The routine returns true if point was snapped.                             *
*****************************************************************************/
bool SnapPoint2( const wxPoint& aPosRef, int SearchMask,
                 SCH_ITEM* DrawList, double aScaleFactor )
{
    for( ; DrawList != NULL; DrawList = DrawList->Next() )
    {
        int hitminDist = MAX( g_DrawDefaultLineThickness, 3 );
        switch( DrawList->Type() )
        {
        case DRAW_POLYLINE_STRUCT_TYPE:
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

        case DRAW_SEGMENT_STRUCT_TYPE:
            #undef  STRUCT
            #define STRUCT ( (SCH_LINE*) DrawList )
            if( !( SearchMask & (DRAWITEM | WIREITEM | BUSITEM) ) )
                break;

            if( TestSegmentHit( aPosRef, STRUCT->m_Start, STRUCT->m_End, 0 ) )
            {
                if( ( ( SearchMask & DRAWITEM )
                     && ( STRUCT->GetLayer() == LAYER_NOTES ) )
                   || ( ( SearchMask & WIREITEM )
                       && ( STRUCT->GetLayer() == LAYER_WIRE ) )
                   || ( ( SearchMask & BUSITEM )
                       && ( STRUCT->GetLayer() == LAYER_BUS ) )
                    )
                {
                    if( SearchMask & EXCLUDE_WIRE_BUS_ENDPOINTS )
                    {
                        if( aPosRef == STRUCT->m_Start
                            || aPosRef == STRUCT->m_End )
                            break;
                    }

                    if( SearchMask & WIRE_BUS_ENDPOINTS_ONLY )
                    {
                        if( !STRUCT->IsOneEndPointAt( aPosRef ) )
                            break;
                    }

                    LastSnappedStruct = DrawList;
                    return true;
                }
            }
            break;


        case DRAW_BUSENTRY_STRUCT_TYPE:
            #undef  STRUCT
            #define STRUCT ( (SCH_BUS_ENTRY*) DrawList )
            if( !( SearchMask & (RACCORDITEM) ) )
                break;

            if( TestSegmentHit( aPosRef, STRUCT->m_Pos, STRUCT->m_End(),
                                hitminDist ) )
            {
                LastSnappedStruct = DrawList;
                return true;
            }
            break;

        case DRAW_JUNCTION_STRUCT_TYPE:
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

        case DRAW_NOCONNECT_STRUCT_TYPE:
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

        case TYPE_SCH_MARKER:
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

        case TYPE_SCH_TEXT:
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


        case TYPE_SCH_LABEL:
        case TYPE_SCH_GLOBALLABEL:
        case TYPE_SCH_HIERLABEL:
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

        case TYPE_SCH_COMPONENT:
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

                    EDA_Rect BoundaryBox = field->GetBoundaryBox();
                    if( BoundaryBox.Inside( aPosRef ) )
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
                EDA_Rect BoundaryBox = STRUCT->GetBoundaryBox();
                if( BoundaryBox.Inside( aPosRef ) )
                {
                    LastSnappedStruct = DrawList;
                    return true;
                }
            }
            break;

        case DRAW_SHEET_STRUCT_TYPE:
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


/*****************************************************************************
* Routine to test if an object has non empty intersection with the box       *
* defined by x1/y1 and x2/y2 (x1 < x2, y1 < y2), and return true if so. This *
* routine is used to pick all points in a given box.                         *
*****************************************************************************/
bool IsItemInBox( EDA_Rect& aBox, SCH_ITEM* DrawStruct )
{
    EDA_Rect BoundaryBox;

    switch( DrawStruct->Type() )
    {
    case DRAW_POLYLINE_STRUCT_TYPE:
        #undef  STRUCT
        #define STRUCT ( (SCH_POLYLINE*) DrawStruct )
        for( unsigned i = 0; i < STRUCT->GetCornerCount(); i++ )
        {
            if( aBox.Inside(STRUCT->m_PolyPoints[i]) )
                return true;
        }

        break;

    case DRAW_SEGMENT_STRUCT_TYPE:
        #undef STRUCT
        #define STRUCT ( (SCH_LINE*) DrawStruct )
        if( aBox.Inside(STRUCT->m_Start) )
            return true;
        if( aBox.Inside(STRUCT->m_End) )
            return true;
        break;

    case DRAW_BUSENTRY_STRUCT_TYPE:
        #undef STRUCT
        #define STRUCT ( (SCH_BUS_ENTRY*) DrawStruct )
        if( aBox.Inside(STRUCT->m_Pos) )
            return true;
        if( aBox.Inside(STRUCT->m_End() ) )
            return true;
        break;

    case DRAW_JUNCTION_STRUCT_TYPE:
    case DRAW_NOCONNECT_STRUCT_TYPE:
    case TYPE_SCH_LABEL:
    case TYPE_SCH_TEXT:
    case TYPE_SCH_HIERLABEL:
    case TYPE_SCH_GLOBALLABEL:
    case TYPE_SCH_COMPONENT:
    case DRAW_SHEET_STRUCT_TYPE:
    case TYPE_SCH_MARKER:
        BoundaryBox = DrawStruct->GetBoundingBox();
        if( aBox.Intersects( BoundaryBox ) )
            return true;
        break;

    case DRAW_HIERARCHICAL_PIN_SHEET_STRUCT_TYPE:
        break;

    default:
    {
        wxString msg;

        msg.Printf( wxT( "IsItemInBox() Err: unexpected StructType %d (" ),
                    DrawStruct->Type() );
        msg << DrawStruct->GetClass() << wxT( ")" );
        wxMessageBox( msg );
        break;
    }
    }

    return FALSE;
}


SCH_SHEET_PIN* LocateSheetLabel( SCH_SHEET* Sheet, const wxPoint& pos )
{
    int size, dy, minx, maxx;
    SCH_SHEET_PIN* SheetLabel;

    SheetLabel = Sheet->m_Label;
    while( SheetLabel
           && SheetLabel->Type() == DRAW_HIERARCHICAL_PIN_SHEET_STRUCT_TYPE )
    {
        size = ( SheetLabel->GetLength() + 1 ) * SheetLabel->m_Size.x;
        if( SheetLabel->m_Edge )
            size = -size;
        minx = SheetLabel->m_Pos.x; maxx = SheetLabel->m_Pos.x + size;
        if( maxx < minx )
            EXCHG( maxx, minx );
        dy = SheetLabel->m_Size.x / 2;
        if( (ABS( pos.y - SheetLabel->m_Pos.y ) <= dy )
           && (pos.x <= maxx)
           && (pos.x >= minx) )
            return SheetLabel;
        SheetLabel = SheetLabel->Next();
    }

    return NULL;
}


LIB_PIN* LocateAnyPin( SCH_ITEM* DrawList, const wxPoint& RefPos,
                       SCH_COMPONENT** libpart )
{
    SCH_ITEM* DrawStruct;
    LIB_COMPONENT* Entry;
    SCH_COMPONENT* schItem = NULL;
    LIB_PIN* Pin = NULL;

    for( DrawStruct = DrawList; DrawStruct != NULL;
        DrawStruct = DrawStruct->Next() )
    {
        if( DrawStruct->Type() != TYPE_SCH_COMPONENT )
            continue;
        schItem = (SCH_COMPONENT*) DrawStruct;
        Entry   = CMP_LIBRARY::FindLibraryComponent( schItem->m_ChipName );

        if( Entry == NULL )
            continue;

        /* we use LocateDrawItem to locate pins. but this function suppose a
         * component.
         * at 0,0 location
         * So we must calculate the ref position relative to the component
         */
        wxPoint libPos = RefPos - schItem->m_Pos;
        Pin = (LIB_PIN*) Entry->LocateDrawItem( schItem->m_Multi,
                                                schItem->m_Convert,
                                                COMPONENT_PIN_DRAW_TYPE,
                                                libPos, schItem->m_Transform );
        if( Pin )
            break;
    }

    if( libpart )
        *libpart = schItem;
    return Pin;
}


SCH_SHEET_PIN* LocateAnyPinSheet( const wxPoint& RefPos, SCH_ITEM* DrawList )
{
    SCH_ITEM* DrawStruct;
    SCH_SHEET_PIN* PinSheet = NULL;

    for( DrawStruct = DrawList; DrawStruct != NULL;
        DrawStruct = DrawStruct->Next() )
    {
        if( DrawStruct->Type() != DRAW_SHEET_STRUCT_TYPE )
            continue;

        PinSheet = LocateSheetLabel( (SCH_SHEET*) DrawStruct, RefPos );
        if( PinSheet )
            break;
    }

    return PinSheet;
}
