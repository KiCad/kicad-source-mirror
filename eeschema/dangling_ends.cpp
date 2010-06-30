/*********************/
/* dangling_ends.cpp */
/*********************/

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"

#include "program.h"
#include "general.h"
#include "netlist.h"
#include "protos.h"
#include "class_library.h"
#include "class_pin.h"


enum End_Type {
    UNKNOWN = 0,
    WIRE_START_END,
    WIRE_END_END,
    BUS_START_END,
    BUS_END_END,
    JUNCTION_END,
    PIN_END,
    LABEL_END,
    ENTRY_END,
    SHEET_LABEL_END
};

class DanglingEndHandle
{
public:
    const void*        m_Item;
    wxPoint            m_Pos;
    int                m_Type;
    DanglingEndHandle* m_Pnext;

    DanglingEndHandle( int type )
    {
        m_Item  = NULL;
        m_Type  = type;
        m_Pnext = NULL;
    }
};

DanglingEndHandle* ItemList;

static void        TestWireForDangling( SCH_LINE*              DrawRef,
                                        WinEDA_SchematicFrame* frame,
                                        wxDC*                  DC );
void               TestLabelForDangling( SCH_TEXT*              label,
                                         WinEDA_SchematicFrame* frame,
                                         wxDC*                  DC );
DanglingEndHandle* RebuildEndList( EDA_BaseStruct* DrawList );


/* Returns TRUE if the point P is on the segment S.
 * The segment is assumed horizontal or vertical.
 */
bool SegmentIntersect( int Sx1, int Sy1, int Sx2, int Sy2,
                       int Px1, int Py1 )
{
    int Sxmin, Sxmax, Symin, Symax;

    if( Sx1 == Sx2 )          /* Line S is vertical. */
    {
        Symin = MIN( Sy1, Sy2 );
        Symax = MAX( Sy1, Sy2 );

        if( Px1 != Sx1 )
            return FALSE;

        if( Py1 >= Symin  &&  Py1 <= Symax )
            return TRUE;
        else
            return FALSE;
    }
    else if( Sy1 == Sy2 )    /* Line S is horizontal. */
    {
        Sxmin = MIN( Sx1, Sx2 );
        Sxmax = MAX( Sx1, Sx2 );

        if( Py1 != Sy1 )
            return FALSE;

        if( Px1 >= Sxmin && Px1 <= Sxmax )
            return TRUE;
        else
            return FALSE;
    }
    else
        return FALSE;
}


void WinEDA_SchematicFrame::TestDanglingEnds( SCH_ITEM* DrawList, wxDC* DC )
{
    if( ItemList )
    {
        const DanglingEndHandle* DanglingItem;
        const DanglingEndHandle* nextitem;

        for( DanglingItem = ItemList;
             DanglingItem != NULL;
             DanglingItem = nextitem )
        {
            nextitem = DanglingItem->m_Pnext;
            SAFE_DELETE( DanglingItem );
        }
    }

    ItemList = RebuildEndList( DrawList );

    for( SCH_ITEM* item = DrawList; item; item = item->Next() )
    {
        switch( item->Type() )
        {
        case TYPE_SCH_GLOBALLABEL:
        case TYPE_SCH_HIERLABEL:
        case TYPE_SCH_LABEL:
            TestLabelForDangling( (SCH_LABEL*) item, this, DC );
            break;

        case DRAW_SEGMENT_STRUCT_TYPE:
            #undef STRUCT
            #define STRUCT ( (SCH_LINE*) item )
            if( STRUCT->GetLayer() == LAYER_WIRE )
            {
                TestWireForDangling( STRUCT, this, DC );
                break;
            }
            if( STRUCT->GetLayer() == LAYER_NOTES )
                break;
            if( STRUCT->GetLayer() == LAYER_BUS )
            {
                STRUCT->m_StartIsDangling = STRUCT->m_EndIsDangling = FALSE;
                break;
            }
            break;

        default:
            ;
        }
    }
}


/**
 * Test if point pos is on a pin end.
 *
 * @param DrawList = List of SCH_ITEMs to check.
 * @return a LIB_PIN pointer to the located pin or NULL if no pin was found.
 */
LIB_PIN* WinEDA_SchematicFrame::LocatePinEnd( SCH_ITEM*      DrawList,
                                              const wxPoint& pos )
{
    SCH_COMPONENT* DrawLibItem;
    LIB_PIN* Pin;
    wxPoint pinpos;

    Pin = LocateAnyPin( DrawList, pos, &DrawLibItem );
    if( !Pin )
        return NULL;

    pinpos = Pin->m_Pos;

    if( DrawLibItem == NULL )
        NEGATE( pinpos.y );     // In libraries Y axis is bottom to top
                                // and in schematic Y axis is top to bottom

    else    // calculate the pin position in schematic
        pinpos = TransformCoordinate( DrawLibItem->m_Transform, pinpos )
                + DrawLibItem->m_Pos;

    if( pos == pinpos )
        return Pin;
    return NULL;
}


void TestWireForDangling( SCH_LINE* DrawRef, WinEDA_SchematicFrame* frame,
                          wxDC* DC )
{
    DanglingEndHandle* terminal_item;
    bool Sdangstate = TRUE, Edangstate = TRUE;

    for( terminal_item = ItemList; terminal_item != NULL;
         terminal_item = terminal_item->m_Pnext )
    {
        if( terminal_item->m_Item == DrawRef )
            continue;

        if( (DrawRef->m_Start.x == terminal_item->m_Pos.x)
           && (DrawRef->m_Start.y == terminal_item->m_Pos.y) )
            Sdangstate = FALSE;

        if( (DrawRef->m_End.x == terminal_item->m_Pos.x)
           && (DrawRef->m_End.y == terminal_item->m_Pos.y) )
            Edangstate = FALSE;

        if( (Sdangstate == FALSE) && (Edangstate == FALSE) )
            break;
    }

    if( (Sdangstate != DrawRef->m_StartIsDangling)
       || (Edangstate != DrawRef->m_EndIsDangling) )
    {
        if( DC )
            RedrawOneStruct( frame->DrawPanel, DC, DrawRef, g_XorMode );
        DrawRef->m_StartIsDangling = Sdangstate;
        DrawRef->m_EndIsDangling   = Edangstate;
        if( DC )
            RedrawOneStruct( frame->DrawPanel, DC, DrawRef,
                             GR_DEFAULT_DRAWMODE );
    }
}


void TestLabelForDangling( SCH_TEXT* label, WinEDA_SchematicFrame* frame,
                           wxDC* DC )
{
    DanglingEndHandle* terminal_item;
    bool dangstate = TRUE;

    for( terminal_item = ItemList; terminal_item != NULL;
         terminal_item = terminal_item->m_Pnext )
    {
        if( terminal_item->m_Item == label )
            continue;

        switch( terminal_item->m_Type )
        {
        case PIN_END:
        case LABEL_END:
        case SHEET_LABEL_END:
            if( ( label->m_Pos.x == terminal_item->m_Pos.x )
               && ( label->m_Pos.y == terminal_item->m_Pos.y ) )
                dangstate = FALSE;
            break;

        case WIRE_START_END:
        case BUS_START_END:
            dangstate = !SegmentIntersect( terminal_item->m_Pos.x,
                                           terminal_item->m_Pos.y,
                                           terminal_item->m_Pnext->m_Pos.x,
                                           terminal_item->m_Pnext->m_Pos.y,
                                           label->m_Pos.x, label->m_Pos.y );
            terminal_item = terminal_item->m_Pnext;
            break;

        case UNKNOWN:
        case JUNCTION_END:
        case ENTRY_END:
        case WIRE_END_END:
        case BUS_END_END:
            break;
        }

        if( dangstate == FALSE )
            break;
    }

    if( dangstate != label->m_IsDangling )
    {
        if( DC )
            RedrawOneStruct( frame->DrawPanel, DC, label, g_XorMode );
        label->m_IsDangling = dangstate;
        if( DC )
            RedrawOneStruct( frame->DrawPanel, DC, label, GR_DEFAULT_DRAWMODE );
    }
}


/* Returns the physical position of the pin relative to the component
 * orientation. */
wxPoint ReturnPinPhysicalPosition( LIB_PIN* Pin, SCH_COMPONENT* DrawLibItem )
{
    wxPoint PinPos = Pin->m_Pos;

    if( DrawLibItem == NULL )
        NEGATE( PinPos.y );

    else
        PinPos = TransformCoordinate( DrawLibItem->m_Transform,
                                      Pin->m_Pos ) + DrawLibItem->m_Pos;

    return PinPos;
}


DanglingEndHandle* RebuildEndList( EDA_BaseStruct* DrawList )
{
    DanglingEndHandle* StartList = NULL, * item, * lastitem = NULL;
    EDA_BaseStruct* DrawItem;

    for( DrawItem = DrawList; DrawItem != NULL; DrawItem = DrawItem->Next() )
    {
        switch( DrawItem->Type() )
        {
        case TYPE_SCH_LABEL:
        case TYPE_SCH_GLOBALLABEL:
        case TYPE_SCH_HIERLABEL:
                #undef STRUCT
                #define STRUCT ( (SCH_LABEL*) DrawItem )
            item = new DanglingEndHandle( LABEL_END );

            item->m_Item = DrawItem;
            item->m_Pos  = STRUCT->m_Pos;
            if( lastitem )
                lastitem->m_Pnext = item;
            else
                StartList = item;
            lastitem = item;
            break;

        case DRAW_SEGMENT_STRUCT_TYPE:
                #undef STRUCT
                #define STRUCT ( (SCH_LINE*) DrawItem )
            if( STRUCT->GetLayer() == LAYER_NOTES )
                break;
            if( ( STRUCT->GetLayer() == LAYER_BUS )
               || (STRUCT->GetLayer() == LAYER_WIRE ) )
            {
                item = new DanglingEndHandle(
                     (STRUCT->GetLayer() == LAYER_BUS) ?
                    BUS_START_END : WIRE_START_END );

                item->m_Item = DrawItem;
                item->m_Pos  = STRUCT->m_Start;
                if( lastitem )
                    lastitem->m_Pnext = item;
                else
                    StartList = item;
                lastitem = item;
                item     =
                    new DanglingEndHandle( (STRUCT->GetLayer() == LAYER_BUS) ?
                                           BUS_END_END : WIRE_END_END );

                item->m_Item = DrawItem;
                item->m_Pos  = STRUCT->m_End;
                lastitem->m_Pnext = item;
                lastitem = item;
            }
            break;

        case DRAW_JUNCTION_STRUCT_TYPE:
                #undef STRUCT
                #define STRUCT ( (SCH_JUNCTION*) DrawItem )
            item = new DanglingEndHandle( JUNCTION_END );

            item->m_Item = DrawItem;
            item->m_Pos  = STRUCT->m_Pos;
            if( lastitem )
                lastitem->m_Pnext = item;
            else
                StartList = item;
            lastitem = item;
            break;

        case DRAW_BUSENTRY_STRUCT_TYPE:
                #undef STRUCT
                #define STRUCT ( (SCH_BUS_ENTRY*) DrawItem )
            item = new DanglingEndHandle( ENTRY_END );

            item->m_Item = DrawItem;
            item->m_Pos  = STRUCT->m_Pos;
            if( lastitem )
                lastitem->m_Pnext = item;
            else
                StartList = item;
            lastitem = item;
            item     = new DanglingEndHandle( ENTRY_END );

            item->m_Item = DrawItem;
            item->m_Pos  = STRUCT->m_End();
            lastitem->m_Pnext = item;
            lastitem = item;
            break;

        case TYPE_SCH_COMPONENT:
        {
            #undef STRUCT
            #define STRUCT ( (SCH_COMPONENT*) DrawItem )
            LIB_COMPONENT* Entry;
            Entry = CMP_LIBRARY::FindLibraryComponent( STRUCT->m_ChipName );
            if( Entry == NULL )
                break;

            for( LIB_PIN* Pin = Entry->GetNextPin(); Pin != NULL;
                 Pin = Entry->GetNextPin( Pin ) )
            {
                wxASSERT( Pin->Type() == COMPONENT_PIN_DRAW_TYPE );

                if( Pin->m_Unit && STRUCT->m_Multi
                   && ( STRUCT->m_Multi != Pin->m_Unit ) )
                    continue;

                if( Pin->m_Convert && STRUCT->m_Convert
                   && ( STRUCT->m_Convert != Pin->m_Convert ) )
                    continue;

                item = new DanglingEndHandle( PIN_END );

                item->m_Item = Pin;
                item->m_Pos  = ReturnPinPhysicalPosition( Pin, STRUCT );
                if( lastitem )
                    lastitem->m_Pnext = item;
                else
                    StartList = item;
                lastitem = item;
            }

            break;
        }

        case DRAW_SHEET_STRUCT_TYPE:
        {
            SCH_SHEET* sheet = (SCH_SHEET*) DrawItem;

            BOOST_FOREACH( SCH_SHEET_PIN pinsheet, sheet->GetSheetPins() )
            {
                wxASSERT( pinsheet.Type() == DRAW_HIERARCHICAL_PIN_SHEET_STRUCT_TYPE );

                item = new DanglingEndHandle( SHEET_LABEL_END );
                item->m_Item = &pinsheet;
                item->m_Pos  = pinsheet.m_Pos;

                if( lastitem )
                    lastitem->m_Pnext = item;
                else
                    StartList = item;

                lastitem = item;
            }
        }
        break;

        default:
            ;
        }
    }

    return StartList;
}
