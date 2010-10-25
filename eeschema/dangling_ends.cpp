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
#include "lib_pin.h"


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

// A helper class to store a list of items that can be connected to something:
class DANGLING_END_ITEM
{
public:
    const void* m_Item;         // a pointer to the parent
    wxPoint     m_Pos;          // the position of the connecting point
    int         m_Type;         // type of parent

    DANGLING_END_ITEM( int type, const void* aItem )
    {
        m_Item = aItem;
        m_Type = type;
    }
};

static void TestWireForDangling( std::vector <DANGLING_END_ITEM>& aItemList,
                                 SCH_LINE*                        DrawRef,
                                 WinEDA_SchematicFrame*           frame,
                                 wxDC*                            aDC );
void        TestLabelForDangling( std::vector <DANGLING_END_ITEM>& aItemList,
                                  SCH_TEXT*                        aLabel,
                                  WinEDA_SchematicFrame*           aFrame,
                                  wxDC*                            aDC );
void        RebuildEndPointsList( std::vector <DANGLING_END_ITEM>& aItemList, SCH_ITEM* aDrawList );


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


void WinEDA_SchematicFrame::TestDanglingEnds( SCH_ITEM* DrawList, wxDC* DC )
{
    // this list is static to avoid many useles memory allocation.
    std::vector <DANGLING_END_ITEM> itemList;

    RebuildEndPointsList( itemList, DrawList );

    for( SCH_ITEM* item = DrawList; item; item = item->Next() )
    {
        switch( item->Type() )
        {
        case TYPE_SCH_GLOBALLABEL:
        case TYPE_SCH_HIERLABEL:
        case TYPE_SCH_LABEL:
            TestLabelForDangling( itemList, (SCH_LABEL*) item, this, DC );
            break;

        case DRAW_SHEET_STRUCT_TYPE:
        {
             SCH_SHEET* sheet = (SCH_SHEET*) item;
           // Read the hierarchical pins list and teast for dangling pins:
            BOOST_FOREACH( SCH_SHEET_PIN & pinsheet, sheet->GetSheetPins() )
            {
                TestLabelForDangling( itemList, &pinsheet, this, DC );
            }
        }
            break;

        case DRAW_SEGMENT_STRUCT_TYPE:
            #undef STRUCT
            #define STRUCT ( (SCH_LINE*) item )
            if( STRUCT->GetLayer() == LAYER_WIRE )
            {
                TestWireForDangling( itemList, STRUCT, this, DC );
                break;
            }
            if( STRUCT->GetLayer() == LAYER_NOTES )
                break;
            if( STRUCT->GetLayer() == LAYER_BUS )
            {
                STRUCT->m_StartIsDangling = STRUCT->m_EndIsDangling = false;
                break;
            }
            break;

        default:
            break;
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

    else                        // calculate the pin position in schematic
        pinpos = DrawLibItem->m_Transform.TransformCoordinate( pinpos ) + DrawLibItem->m_Pos;

    if( pos == pinpos )
        return Pin;
    return NULL;
}


void TestWireForDangling( std::vector <DANGLING_END_ITEM>& aItemList,
                          SCH_LINE*                        DrawRef,
                          WinEDA_SchematicFrame*           frame,
                          wxDC*                            DC )
{
    bool Sdangstate = true, Edangstate = true;

    BOOST_FOREACH( DANGLING_END_ITEM terminal_item, aItemList )
    {
        if( terminal_item.m_Item == DrawRef )
            continue;

        if( DrawRef->m_Start == terminal_item.m_Pos )
            Sdangstate = false;

        if( DrawRef->m_End == terminal_item.m_Pos )
            Edangstate = false;

        if( (Sdangstate == false) && (Edangstate == false) )
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


void TestLabelForDangling( std::vector <DANGLING_END_ITEM>& aItemList,
                           SCH_TEXT*                        aLabel,
                           WinEDA_SchematicFrame*           aFrame,
                           wxDC*                            aDC )
{
    bool dangstate = true;

    for( unsigned ii = 0; ii < aItemList.size(); ii++ )
    {
        DANGLING_END_ITEM & terminal_item = aItemList[ii];

        if( terminal_item.m_Item == aLabel )
            continue;

        switch( terminal_item.m_Type )
        {
        case PIN_END:
        case LABEL_END:
        case SHEET_LABEL_END:
            if( aLabel->m_Pos == terminal_item.m_Pos )
                dangstate = false;
            break;

        case WIRE_START_END:
        case BUS_START_END:
        {
            // these schematic items have created 2 DANGLING_END_ITEM
            // one per end.
            ii++;
            DANGLING_END_ITEM & next_terminal = aItemList[ii];
            dangstate = !SegmentIntersect( terminal_item.m_Pos,
                                           next_terminal.m_Pos,
                                           aLabel->m_Pos );
        }
            break;

        case UNKNOWN:
        case JUNCTION_END:
        case ENTRY_END:
        case WIRE_END_END:
        case BUS_END_END:
            break;
        }

        if( dangstate == false )
            break;
    }

    if( dangstate != aLabel->m_IsDangling )
    {
        if( aDC )
            RedrawOneStruct( aFrame->DrawPanel, aDC, aLabel, g_XorMode );
        aLabel->m_IsDangling = dangstate;
        if( aDC )
            RedrawOneStruct( aFrame->DrawPanel, aDC, aLabel, GR_DEFAULT_DRAWMODE );
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
        PinPos = DrawLibItem->m_Transform.TransformCoordinate( Pin->m_Pos ) + DrawLibItem->m_Pos;

    return PinPos;
}


void RebuildEndPointsList( std::vector <DANGLING_END_ITEM>& aItemList, SCH_ITEM* aDrawList )
{
    SCH_ITEM* schItem;

    aItemList.clear();

    for( schItem = aDrawList; schItem != NULL; schItem = schItem->Next() )
    {
        switch( schItem->Type() )
        {
        case TYPE_SCH_LABEL:
        case TYPE_SCH_GLOBALLABEL:
        case TYPE_SCH_HIERLABEL:
        {
            #undef STRUCT
            #define STRUCT ( (SCH_LABEL*) schItem )
            DANGLING_END_ITEM item( LABEL_END, schItem );
            item.m_Pos  = STRUCT->m_Pos;
            aItemList.push_back( item );
        }
        break;

        case DRAW_SEGMENT_STRUCT_TYPE:
            #undef STRUCT
            #define STRUCT ( (SCH_LINE*) schItem )
            if( STRUCT->GetLayer() == LAYER_NOTES )
                break;
            if( ( STRUCT->GetLayer() == LAYER_BUS )
               || (STRUCT->GetLayer() == LAYER_WIRE ) )
            {
                DANGLING_END_ITEM item( (STRUCT->GetLayer() == LAYER_BUS) ?
                                       BUS_START_END : WIRE_START_END, schItem );
                item.m_Pos  = STRUCT->m_Start;
                DANGLING_END_ITEM item1( (STRUCT->GetLayer() == LAYER_BUS) ?
                                          BUS_END_END : WIRE_END_END, schItem );
                item1.m_Pos  = STRUCT->m_End;

                aItemList.push_back( item );
                aItemList.push_back( item1 );
            }
            break;

        case DRAW_JUNCTION_STRUCT_TYPE:
        {
            #undef STRUCT
            #define STRUCT ( (SCH_JUNCTION*) schItem )
            DANGLING_END_ITEM item( JUNCTION_END, schItem );
            item.m_Pos  = STRUCT->m_Pos;
            aItemList.push_back( item );
        }
        break;

        case DRAW_BUSENTRY_STRUCT_TYPE:
        {
            #undef STRUCT
            #define STRUCT ( (SCH_BUS_ENTRY*) schItem )
            DANGLING_END_ITEM item( ENTRY_END, schItem );
            item.m_Pos  = STRUCT->m_Pos;

            DANGLING_END_ITEM item1( ENTRY_END, schItem );
            item1.m_Pos  = STRUCT->m_End();
            aItemList.push_back( item );
            aItemList.push_back( item1 );
        }
        break;

        case TYPE_SCH_COMPONENT:
        {
            #undef STRUCT
            #define STRUCT ( (SCH_COMPONENT*) schItem )
            LIB_COMPONENT* Entry;
            Entry = CMP_LIBRARY::FindLibraryComponent( STRUCT->m_ChipName );
            if( Entry == NULL )
                break;

            for( LIB_PIN* Pin = Entry->GetNextPin(); Pin != NULL;
                Pin = Entry->GetNextPin( Pin ) )
            {
                wxASSERT( Pin->Type() == COMPONENT_PIN_DRAW_TYPE );

                if( Pin->GetUnit() && STRUCT->m_Multi && ( STRUCT->m_Multi != Pin->GetUnit() ) )
                    continue;

                if( Pin->GetConvert() && STRUCT->m_Convert
                    && ( STRUCT->m_Convert != Pin->GetConvert() ) )
                    continue;

                DANGLING_END_ITEM item( PIN_END, Pin );
                item.m_Pos  = ReturnPinPhysicalPosition( Pin, STRUCT );
                aItemList.push_back( item );
            }

            break;
        }

        case DRAW_SHEET_STRUCT_TYPE:
        {
            SCH_SHEET* sheet = (SCH_SHEET*) schItem;

            // Using BOOST_FOREACH here creates problems (bad pointer value to pinsheet).
            // I do not know why.
            for( unsigned ii = 0; ii < sheet->GetSheetPins().size(); ii++ )
            {
                SCH_SHEET_PIN &pinsheet = sheet->GetSheetPins()[ii];
                wxASSERT( pinsheet.Type() == DRAW_HIERARCHICAL_PIN_SHEET_STRUCT_TYPE );

                DANGLING_END_ITEM item( SHEET_LABEL_END, &pinsheet );
                item.m_Pos  = pinsheet.m_Pos;
                aItemList.push_back( item );
            }
        }
        break;

        default:
            ;
        }
    }
}
