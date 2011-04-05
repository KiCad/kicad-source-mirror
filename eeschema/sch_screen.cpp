
#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "kicad_string.h"
#include "eeschema_id.h"
#include "appl_wxstruct.h"
#include "class_drawpanel.h"
#include "sch_item_struct.h"
#include "class_sch_screen.h"
#include "wxEeschemaStruct.h"

#include "general.h"
#include "protos.h"
#include "netlist.h"
#include "class_library.h"
#include "sch_junction.h"
#include "sch_bus_entry.h"
#include "sch_line.h"
#include "sch_marker.h"
#include "sch_no_connect.h"
#include "sch_sheet.h"
#include "sch_component.h"
#include "sch_collectors.h"
#include "sch_text.h"

#include <boost/foreach.hpp>


/* Default EESchema zoom values. Limited to 17 values to keep a decent size
 * to menus
 */
#if defined( __WINDOWS__ ) && !wxCHECK_VERSION(2, 9, 1)
/* Please, note: wxWidgets (wxMSW) before version 2.9 seems have
 * problems with scale values < 1
 * because  scale value is SchematicZoomList[x] / 10
 * ( in fact is SchematicZoomList[x] / m_ZoomScalar )
 * do not use values smaller than 10 without testing them under wxWidgets versions < 2.9
 * value like 0.5 seems work
 */
static int SchematicZoomList[] =
{
    5, 10, 15, 20, 30, 40, 60, 80, 120, 160, 230, 320, 480, 640, 800, 1280
};

#else
static int SchematicZoomList[] =
{
    5, 7, 10, 15, 20, 30, 40, 60, 80, 120, 160, 230, 320, 480, 640, 800, 1280
};
#endif

#define SCHEMATIC_ZOOM_LIST_CNT  ( sizeof( SchematicZoomList ) / sizeof( int ) )
#define MM_TO_SCH_UNITS 1000.0 / 25.4       //schematic internal unites are mils


/* Default grid sizes for the schematic editor.
 * Do NOT add others values (mainly grid values in mm),
 * because they can break the schematic:
 * because wires and pins are considered as connected when the are to the same coordinate
 * we cannot mix coordinates in mils (internal units) and mm
 * (that cannot exactly converted in mils in many cases
 * in fact schematic must only use 50 and 25 mils to place labels, wires and components
 * others values are useful only for graphic items (mainly in library editor)
 * so use integer values in mils only.
*/
static GRID_TYPE SchematicGridList[] = {
    { ID_POPUP_GRID_LEVEL_50, wxRealPoint( 50, 50 ) },
    { ID_POPUP_GRID_LEVEL_25, wxRealPoint( 25, 25 ) },
    { ID_POPUP_GRID_LEVEL_10, wxRealPoint( 10, 10 ) },
    { ID_POPUP_GRID_LEVEL_5, wxRealPoint( 5, 5 ) },
    { ID_POPUP_GRID_LEVEL_2, wxRealPoint( 2, 2 ) },
    { ID_POPUP_GRID_LEVEL_1, wxRealPoint( 1, 1 ) },
};

#define SCHEMATIC_GRID_LIST_CNT ( sizeof( SchematicGridList ) / sizeof( GRID_TYPE ) )


SCH_SCREEN::SCH_SCREEN( KICAD_T type ) : BASE_SCREEN( type )
{
    size_t i;

    SetDrawItems( NULL );                  /* Schematic items list */
    m_Zoom = 32;

    for( i = 0; i < SCHEMATIC_ZOOM_LIST_CNT; i++ )
        m_ZoomList.Add( SchematicZoomList[i] );

    for( i = 0; i < SCHEMATIC_GRID_LIST_CNT; i++ )
        AddGrid( SchematicGridList[i] );

    SetGrid( wxRealPoint( 50, 50 ) );   /* Default grid size. */
    m_refCount = 0;
    m_Center = false;                   /* Suitable for schematic only. For
                                         * libedit and viewlib, must be set
                                         * to true */
    InitDatas();
}


SCH_SCREEN::~SCH_SCREEN()
{
    ClearUndoRedoList();
    FreeDrawList();
}


void SCH_SCREEN::IncRefCount()
{
    m_refCount++;
}


void SCH_SCREEN::DecRefCount()
{
    wxCHECK_RET( m_refCount != 0,
                 wxT( "Screen reference count already zero.  Bad programmer!" ) );
    m_refCount--;
}


void SCH_SCREEN::FreeDrawList()
{
    SCH_ITEM* item;

    while( GetDrawItems() != NULL )
    {
        item = GetDrawItems();
        SetDrawItems( GetDrawItems()->Next() );
        SAFE_DELETE( item );
    }

    SetDrawItems( NULL );
}


void SCH_SCREEN::RemoveFromDrawList( SCH_ITEM* aItem )
{
    if( aItem == GetDrawItems() )
    {
        SetDrawItems( GetDrawItems()->Next() );
    }
    else
    {
        EDA_ITEM* DrawList = GetDrawItems();

        while( DrawList && DrawList->Next() )
        {
            if( DrawList->Next() == aItem )
            {
                DrawList->SetNext( DrawList->Next()->Next() );
                break;
            }

            DrawList = DrawList->Next();
        }
    }
}


void SCH_SCREEN::DeleteItem( SCH_ITEM* aItem )
{
    wxCHECK_RET( aItem != NULL, wxT( "Cannot delete invaled item from screen." ) );

    SetModify();

    if( aItem->Type() == SCH_SHEET_PIN_T )
    {
        // This structure is attached to a sheet, get the parent sheet object.
        SCH_SHEET_PIN* sheetPin = (SCH_SHEET_PIN*) aItem;
        SCH_SHEET* sheet = sheetPin->GetParent();
        wxCHECK_RET( sheet != NULL,
                     wxT( "Sheet label parent not properly set, bad programmer!" ) );
        sheet->RemovePin( sheetPin );
        return;
    }
    else
    {
        if( aItem == GetDrawItems() )
        {
            SetDrawItems( aItem->Next() );
            SAFE_DELETE( aItem );
        }
        else
        {
            SCH_ITEM* itemList = GetDrawItems();

            while( itemList && itemList->Next() )
            {
                if( itemList->Next() == aItem )
                {
                    itemList->SetNext( aItem->Next() );
                    SAFE_DELETE( aItem );
                    return;
                }

                itemList = itemList->Next();
            }
        }
    }
}


bool SCH_SCREEN::CheckIfOnDrawList( SCH_ITEM* aItem )
{
    SCH_ITEM* itemList = GetDrawItems();

    while( itemList )
    {
        if( itemList == aItem )
            return true;

        itemList = itemList->Next();
    }

    return false;
}


void SCH_SCREEN::AddToDrawList( SCH_ITEM* aItem )
{
    aItem->SetNext( GetDrawItems() );
    SetDrawItems( aItem );
}


SCH_ITEM* SCH_SCREEN::GetItem( const wxPoint& aPosition, int aAccuracy, KICAD_T aType ) const
{
    for( SCH_ITEM* item = GetDrawItems(); item != NULL; item = item->Next() )
    {
        if( item->HitTest( aPosition, aAccuracy ) && (aType == NOT_USED) )
            return item;

        if( (aType == SCH_FIELD_T) && (item->Type() == SCH_COMPONENT_T) )
        {
            SCH_COMPONENT* component = (SCH_COMPONENT*) item;

            for( int i = REFERENCE; i < component->GetFieldCount(); i++ )
            {
                SCH_FIELD* field = component->GetField( i );

                if( field->HitTest( aPosition, aAccuracy ) )
                    return (SCH_ITEM*) field;
            }
        }
        else if( (aType == SCH_SHEET_PIN_T) && (item->Type() == SCH_SHEET_T) )
        {
            SCH_SHEET* sheet = (SCH_SHEET*)item;

            SCH_SHEET_PIN* label = sheet->GetPin( aPosition );

            if( label )
                return (SCH_ITEM*) label;
        }
        else if( (item->Type() == aType) && item->HitTest( aPosition, aAccuracy ) )
        {
            return item;
        }
    }

    return NULL;
}


SCH_ITEM* SCH_SCREEN::ExtractWires( bool CreateCopy )
{
    SCH_ITEM* item, * next_item, * new_item, * List = NULL;

    for( item = GetDrawItems(); item != NULL; item = next_item )
    {
        next_item = item->Next();

        switch( item->Type() )
        {
        case SCH_JUNCTION_T:
        case SCH_LINE_T:
            RemoveFromDrawList( item );
            item->SetNext( List );
            List = item;

            if( CreateCopy )
            {
                new_item = item->Clone();
                new_item->SetNext( GetDrawItems() );
                SetDrawItems( new_item );
            }
            break;

        default:
            break;
        }
    }

    return List;
}


void SCH_SCREEN::ReplaceWires( SCH_ITEM* aWireList )
{
    SCH_ITEM* item;
    SCH_ITEM* next_item;

    for( item = GetDrawItems(); item != NULL; item = next_item )
    {
        next_item = item->Next();

        switch( item->Type() )
        {
        case SCH_JUNCTION_T:
        case SCH_LINE_T:
            RemoveFromDrawList( item );
            delete item;
            break;

        default:
            break;
        }
    }

    while( aWireList )
    {
        next_item = aWireList->Next();

        aWireList->SetNext( GetDrawItems() );
        SetDrawItems( aWireList );
        aWireList = next_item;
    }
}


void SCH_SCREEN::MarkConnections( SCH_LINE* aSegment )
{
    wxCHECK_RET( (aSegment != NULL) && (aSegment->Type() == SCH_LINE_T),
                 wxT( "Invalid object pointer." ) );

    for( SCH_ITEM* item = GetDrawItems(); item != NULL; item = item->Next() )
    {
        if( item->GetFlags() & CANDIDATE )
            continue;

        if( item->Type() == SCH_JUNCTION_T )
        {
            SCH_JUNCTION* junction = (SCH_JUNCTION*) item;

            if( aSegment->IsEndPoint( junction->m_Pos ) )
                item->SetFlags( CANDIDATE );

            continue;
        }

        if( item->Type() != SCH_LINE_T )
            continue;

        SCH_LINE* segment = (SCH_LINE*) item;

        if( aSegment->IsEndPoint( segment->m_Start ) && !GetPin( segment->m_Start, NULL, true ) )
        {
            item->SetFlags( CANDIDATE );
            MarkConnections( segment );
        }

        if( aSegment->IsEndPoint( segment->m_End ) && !GetPin( segment->m_End, NULL, true ) )
        {
            item->SetFlags( CANDIDATE );
            MarkConnections( segment );
        }
    }
}


bool SCH_SCREEN::IsJunctionNeeded( const wxPoint& aPosition )
{
    if( GetItem( aPosition, 0, SCH_JUNCTION_T ) )
        return false;

    if( GetWire( aPosition, 0, EXCLUDE_END_POINTS_T ) )
    {
        if( GetWire( aPosition, 0, END_POINTS_ONLY_T ) )
            return true;

        if( GetPin( aPosition, NULL, true ) )
            return true;
    }

    return false;
}


bool SCH_SCREEN::IsTerminalPoint( const wxPoint& aPosition, int aLayer )
{
    wxCHECK_MSG( aLayer == LAYER_NOTES || aLayer == LAYER_BUS || aLayer == LAYER_WIRE, false,
                 wxT( "Invalid layer type passed to SCH_SCREEN::IsTerminalPoint()." ) );

    SCH_SHEET_PIN* label;
    SCH_TEXT*      text;

    switch( aLayer )
    {
    case LAYER_BUS:

        if( GetBus( aPosition ) )
            return true;

        label = GetSheetLabel( aPosition );

        if( label && IsBusLabel( label->m_Text ) && label->IsConnected( aPosition ) )
            return true;

        text = GetLabel( aPosition );

        if( text && IsBusLabel( text->GetText() ) && text->IsConnected( aPosition )
            && (text->Type() != SCH_LABEL_T) )
            return true;

        break;

    case LAYER_NOTES:

        if( GetLine( aPosition ) )
            return true;

        break;

    case LAYER_WIRE:
        if( GetItem( aPosition, MAX( g_DrawDefaultLineThickness, 3 ), SCH_BUS_ENTRY_T ) )
            return true;

        if( GetItem( aPosition, MAX( g_DrawDefaultLineThickness, 3 ), SCH_JUNCTION_T ) )
            return true;

        if( GetPin( aPosition, NULL, true ) )
            return true;

        if( GetWire( aPosition ) )
            return true;

        text = GetLabel( aPosition );

        if( text && text->IsConnected( aPosition ) && !IsBusLabel( text->GetText() ) )
            return true;

        label = GetSheetLabel( aPosition );

        if( label && label->IsConnected( aPosition ) && !IsBusLabel( label->m_Text ) )
            return true;

        break;

    default:
        break;
    }

    return false;
}


bool SCH_SCREEN::SchematicCleanUp( EDA_DRAW_PANEL* aCanvas, wxDC* aDC )
{
    SCH_ITEM* DrawList, * TstDrawList;
    bool      Modify = false;

    DrawList = GetDrawItems();

    for( ; DrawList != NULL; DrawList = DrawList->Next() )
    {
        if( DrawList->Type() == SCH_LINE_T )
        {
            TstDrawList = DrawList->Next();

            while( TstDrawList )
            {
                if( TstDrawList->Type() == SCH_LINE_T )
                {
                    SCH_LINE* line = (SCH_LINE*) DrawList;

                    if( line->MergeOverlap( (SCH_LINE*) TstDrawList ) )
                    {
                        // Keep the current flags, because the deleted segment can be flagged.
                        DrawList->SetFlags( TstDrawList->GetFlags() );
                        DeleteItem( TstDrawList );
                        TstDrawList = GetDrawItems();
                        Modify = true;
                    }
                    else
                    {
                        TstDrawList = TstDrawList->Next();
                    }
                }
                else
                {
                    TstDrawList = TstDrawList->Next();
                }
            }
        }
    }

    TestDanglingEnds( aCanvas, aDC );

    if( aCanvas && Modify )
        aCanvas->Refresh();

    return Modify;
}


bool SCH_SCREEN::Save( FILE* aFile ) const
{
    // Creates header
    if( fprintf( aFile, "%s %s %d", EESCHEMA_FILE_STAMP,
                 SCHEMATIC_HEAD_STRING, EESCHEMA_VERSION ) < 0
        || fprintf( aFile, "  date %s\n", TO_UTF8( DateAndTime() ) ) < 0 )
        return false;

    BOOST_FOREACH( const CMP_LIBRARY& lib, CMP_LIBRARY::GetLibraryList() )
    {
        if( fprintf( aFile, "LIBS:%s\n", TO_UTF8( lib.GetName() ) ) < 0 )
            return false;
    }

    if( fprintf( aFile, "EELAYER %2d %2d\n", g_LayerDescr.NumberOfLayers,
                 g_LayerDescr.CurrentLayer ) < 0
        || fprintf( aFile, "EELAYER END\n" ) < 0 )
        return false;

    /* Write page info, ScreenNumber and NumberOfScreen; not very meaningful for
     * SheetNumber and Sheet Count in a complex hierarchy, but useful in
     * simple hierarchy and flat hierarchy.  Used also to search the root
     * sheet ( ScreenNumber = 1 ) within the files
     */

    if( fprintf( aFile, "$Descr %s %d %d\n", TO_UTF8( m_CurrentSheetDesc->m_Name ),
                 m_CurrentSheetDesc->m_Size.x, m_CurrentSheetDesc->m_Size.y ) < 0
        || fprintf( aFile, "encoding utf-8\n") < 0
        || fprintf( aFile, "Sheet %d %d\n", m_ScreenNumber, m_NumberOfScreen ) < 0
        || fprintf( aFile, "Title %s\n",    EscapedUTF8( m_Title ).c_str() ) < 0
        || fprintf( aFile, "Date %s\n",     EscapedUTF8( m_Date ).c_str() ) < 0
        || fprintf( aFile, "Rev %s\n",      EscapedUTF8( m_Revision ).c_str() ) < 0
        || fprintf( aFile, "Comp %s\n",     EscapedUTF8( m_Company ).c_str() ) < 0
        || fprintf( aFile, "Comment1 %s\n", EscapedUTF8( m_Commentaire1 ).c_str() ) < 0
        || fprintf( aFile, "Comment2 %s\n", EscapedUTF8( m_Commentaire2 ).c_str() ) < 0
        || fprintf( aFile, "Comment3 %s\n", EscapedUTF8( m_Commentaire3 ).c_str() ) < 0
        || fprintf( aFile, "Comment4 %s\n", EscapedUTF8( m_Commentaire4 ).c_str() ) < 0
        || fprintf( aFile, "$EndDescr\n" ) < 0 )
        return false;

    for( SCH_ITEM* item = GetDrawItems(); item; item = item->Next() )
    {
        if( !item->Save( aFile ) )
            return false;
    }

    if( fprintf( aFile, "$EndSCHEMATC\n" ) < 0 )
        return false;

    return true;
}


void SCH_SCREEN::Draw( EDA_DRAW_PANEL* aCanvas, wxDC* aDC, int aDrawMode, int aColor )
{
    for( SCH_ITEM* item = GetDrawItems(); item != NULL; item = item->Next() )
    {
        if( item->IsMoving() )
            continue;

        // uncomment line below when there is a virtual
        // EDA_ITEM::GetBoundingBox()
        //      if( panel->m_ClipBox.Intersects( Structs->GetBoundingBox()
        // ) )
        item->Draw( aCanvas, aDC, wxPoint( 0, 0 ), aDrawMode, aColor );
    }
}


void SCH_SCREEN::ClearUndoORRedoList( UNDO_REDO_CONTAINER& aList, int aItemCount )
{
    if( aItemCount == 0 )
        return;

    unsigned icnt = aList.m_CommandsList.size();

    if( aItemCount > 0 )
        icnt = aItemCount;

    for( unsigned ii = 0; ii < icnt; ii++ )
    {
        if( aList.m_CommandsList.size() == 0 )
            break;

        PICKED_ITEMS_LIST* curr_cmd = aList.m_CommandsList[0];
        aList.m_CommandsList.erase( aList.m_CommandsList.begin() );

        curr_cmd->ClearListAndDeleteItems();
        delete curr_cmd;    // Delete command
    }
}


void SCH_SCREEN::ClearDrawingState()
{
    for( SCH_ITEM* item = GetDrawItems(); item != NULL; item = item->Next() )
        item->ClearFlags();
}


LIB_PIN* SCH_SCREEN::GetPin( const wxPoint& aPosition, SCH_COMPONENT** aComponent,
                             bool aEndPointOnly ) const
{
    SCH_ITEM* item;
    SCH_COMPONENT* component = NULL;
    LIB_PIN* pin = NULL;

    for( item = GetDrawItems(); item != NULL; item = item->Next() )
    {
        if( item->Type() != SCH_COMPONENT_T )
            continue;

        component = (SCH_COMPONENT*) item;

        pin = (LIB_PIN*) component->GetDrawItem( aPosition, LIB_PIN_T );

        if( pin )
            break;
    }

    if( pin && aEndPointOnly && ( component->GetPinPhysicalPosition( pin ) != aPosition ) )
    {
        wxPoint endpt = component->GetPinPhysicalPosition( pin );
        wxLogDebug( wxString::Format( wxT( "Pin end point at (%d, %d) not (%d, %d )." ),
                                      endpt.x, endpt.y, aPosition.x, aPosition.y ) );
        pin = NULL;
    }

    if( aComponent )
        *aComponent = component;

    return pin;
}


SCH_SHEET_PIN* SCH_SCREEN::GetSheetLabel( const wxPoint& aPosition )
{
    SCH_SHEET_PIN* sheetPin = NULL;

    for( SCH_ITEM* item = GetDrawItems(); item != NULL; item = item->Next() )
    {
        if( item->Type() != SCH_SHEET_T )
            continue;

        SCH_SHEET* sheet = (SCH_SHEET*) item;
        sheetPin = sheet->GetPin( aPosition );

        if( sheetPin )
            break;
    }

    return sheetPin;
}


int SCH_SCREEN::CountConnectedItems( const wxPoint& aPos, bool aTestJunctions ) const
{
    SCH_ITEM* item;
    int       count = 0;

    for( item = GetDrawItems(); item != NULL; item = item->Next() )
    {
        if( item->Type() == SCH_JUNCTION_T  && !aTestJunctions )
            continue;

        if( item->IsConnected( aPos ) )
            count++;
    }

    return count;
}


void SCH_SCREEN::ClearAnnotation( SCH_SHEET_PATH* aSheetPath )
{
    for( SCH_ITEM* item = GetDrawItems(); item != NULL; item = item->Next() )
    {
        if( item->Type() == SCH_COMPONENT_T )
        {
            SCH_COMPONENT* component = (SCH_COMPONENT*) item;

            component->ClearAnnotation( aSheetPath );
        }
    }
}


void SCH_SCREEN::GetHierarchicalItems( EDA_ITEMS& aItems )
{
    SCH_ITEM* item = GetDrawItems();

    while( item )
    {
        if( ( item->Type() == SCH_SHEET_T ) || ( item->Type() == SCH_COMPONENT_T ) )
            aItems.push_back( item );

        item = item->Next();
    }
}


void SCH_SCREEN::SelectBlockItems()
{
    SCH_ITEM*          item;

    PICKED_ITEMS_LIST* pickedlist = &m_BlockLocate.m_ItemsSelection;

    if( pickedlist->GetCount() == 0 )
        return;

    ClearDrawingState();

    for( unsigned ii = 0; ii < pickedlist->GetCount(); ii++ )
    {
        item = (SCH_ITEM*) pickedlist->GetPickedItem( ii );
        item->SetFlags( SELECTED );
    }

    if( !m_BlockLocate.IsDragging() )
        return;

    // Select all the items in the screen connected to the items in the block.
    // be sure end lines that are on the block limits are seen inside this block
    m_BlockLocate.Inflate(1);
    unsigned last_select_id = pickedlist->GetCount();
    unsigned ii = 0;

    for( ; ii < last_select_id; ii++ )
    {
        item = (SCH_ITEM*)pickedlist->GetPickedItem( ii );

        if( item->Type() == SCH_LINE_T )
        {
            item->IsSelectStateChanged( m_BlockLocate );

            if( ( item->GetFlags() & SELECTED ) == 0 )
            {   // This is a special case:
                // this selected wire has no ends in block.
                // But it was selected (because it intersects the selecting area),
                // so we must keep it selected and select items connected to it
                // Note: an other option could be: remove it from drag list
                item->SetFlags( SELECTED | SKIP_STRUCT );
                std::vector< wxPoint > connections;
                item->GetConnectionPoints( connections );

                for( size_t i = 0; i < connections.size(); i++ )
                    addConnectedItemsToBlock( connections[i] );
            }

            pickedlist->SetPickerFlags( item->GetFlags(), ii );
        }
        else if( item->IsConnectable() )
        {
            std::vector< wxPoint > connections;

            item->GetConnectionPoints( connections );

            for( size_t i = 0; i < connections.size(); i++ )
                addConnectedItemsToBlock( connections[i] );
        }
    }

    m_BlockLocate.Inflate(-1);
}


void SCH_SCREEN::addConnectedItemsToBlock( const wxPoint& position )
{
    SCH_ITEM* item;
    ITEM_PICKER picker;
    bool addinlist = true;

    for( item = GetDrawItems(); item != NULL; item = item->Next() )
    {
        picker.SetItem( item );
        picker.SetItemType( item->Type() );

        if( !item->IsConnectable() || !item->IsConnected( position )
            || (item->GetFlags() & SKIP_STRUCT) )
            continue;

        if( item->IsSelected() && item->Type() != SCH_LINE_T )
            continue;

        // A line having 2 ends, it can be tested twice: one time per end
        if( item->Type() == SCH_LINE_T )
        {
            if( ! item->IsSelected() )      // First time this line is tested
                item->SetFlags( SELECTED | STARTPOINT | ENDPOINT );
            else      // second time (or more) this line is tested
                addinlist = false;

            SCH_LINE* line = (SCH_LINE*) item;

            if( line->m_Start == position )
                item->ClearFlags( STARTPOINT );
            else if( line->m_End == position )
                item->ClearFlags( ENDPOINT );
        }
        else
            item->SetFlags( SELECTED );

        if( addinlist )
        {
            picker.m_PickerFlags = item->GetFlags();
            m_BlockLocate.m_ItemsSelection.PushItem( picker );
        }
    }
}


int SCH_SCREEN::UpdatePickList()
{
    ITEM_PICKER picker;
    EDA_RECT area;
    area.SetOrigin( m_BlockLocate.GetOrigin());
    area.SetSize( m_BlockLocate.GetSize() );
    area.Normalize();

    for( SCH_ITEM* item = GetDrawItems(); item != NULL; item = item->Next() )
    {
        // An item is picked if its bounding box intersects the reference area.
        if( item->HitTest( area ) )
        {
            picker.SetItem( item );
            picker.SetItemType( item->Type() );
            m_BlockLocate.PushItem( picker );
        }
    }

    return m_BlockLocate.GetCount();
}


bool SCH_SCREEN::TestDanglingEnds( EDA_DRAW_PANEL* aCanvas, wxDC* aDC )
{
    SCH_ITEM* item;
    std::vector< DANGLING_END_ITEM > endPoints;
    bool hasDanglingEnds = false;

    for( item = GetDrawItems(); item != NULL; item = item->Next() )
        item->GetEndPoints( endPoints );

    for( item = GetDrawItems(); item; item = item->Next() )
    {
        if( item->IsDanglingStateChanged( endPoints ) && ( aCanvas != NULL ) && ( aDC != NULL ) )
        {
            item->Draw( aCanvas, aDC, wxPoint( 0, 0 ), g_XorMode );
            item->Draw( aCanvas, aDC, wxPoint( 0, 0 ), GR_DEFAULT_DRAWMODE );
        }

        if( item->IsDangling() )
            hasDanglingEnds = true;
    }

    return hasDanglingEnds;
}


bool SCH_SCREEN::BreakSegment( const wxPoint& aPoint )
{
    SCH_LINE* segment;
    SCH_LINE* newSegment;
    bool brokenSegments = false;

    for( SCH_ITEM* item = GetDrawItems(); item != NULL; item = item->Next() )
    {
        if( (item->Type() != SCH_LINE_T) || (item->GetLayer() == LAYER_NOTES) )
            continue;

        segment = (SCH_LINE*) item;

        if( !segment->HitTest( aPoint, 0 ) || segment->IsEndPoint( aPoint ) )
            continue;

        // Break the segment at aPoint and create a new segment.
        newSegment = new SCH_LINE( *segment );
        newSegment->m_Start = aPoint;
        segment->m_End = newSegment->m_Start;
        newSegment->SetNext( segment->Next() );
        segment->SetNext( newSegment );
        item = newSegment;
        brokenSegments = true;
    }

    return brokenSegments;
}


bool SCH_SCREEN::BreakSegmentsOnJunctions()
{
    bool brokenSegments = false;

    for( SCH_ITEM* item = GetDrawItems(); item != NULL; item = item->Next() )
    {
        if( item->Type() == SCH_JUNCTION_T )
        {
            SCH_JUNCTION* junction = ( SCH_JUNCTION* ) item;

            if( BreakSegment( junction->m_Pos ) )
                brokenSegments = true;
        }
        else if( item->Type() == SCH_BUS_ENTRY_T )
        {
            SCH_BUS_ENTRY* busEntry = ( SCH_BUS_ENTRY* ) item;

            if( BreakSegment( busEntry->m_Pos ) || BreakSegment( busEntry->m_End() ) )
                brokenSegments = true;
        }
    }

    return brokenSegments;
}


int SCH_SCREEN::GetNode( const wxPoint& aPosition, EDA_ITEMS& aList )
{
    for( SCH_ITEM* item = GetDrawItems(); item != NULL; item = item->Next() )
    {
        if( item->Type() == SCH_LINE_T && item->HitTest( aPosition )
            && (item->GetLayer() == LAYER_BUS || item->GetLayer() == LAYER_WIRE) )
        {
            aList.push_back( item );
        }
        else if( item->Type() == SCH_JUNCTION_T && item->HitTest( aPosition ) )
        {
            aList.push_back( item );
        }
    }

    return (int) aList.size();
}


SCH_LINE* SCH_SCREEN::GetWireOrBus( const wxPoint& aPosition )
{
    for( SCH_ITEM* item = GetDrawItems(); item != NULL; item = item->Next() )
    {
        if( (item->Type() == SCH_LINE_T) && item->HitTest( aPosition )
            && (item->GetLayer() == LAYER_BUS || item->GetLayer() == LAYER_WIRE) )
        {
            return (SCH_LINE*) item;
        }
    }

    return NULL;
}


SCH_LINE* SCH_SCREEN::GetLine( const wxPoint& aPosition, int aAccuracy, int aLayer,
                               SCH_LINE_TEST_T aSearchType )
{
    for( SCH_ITEM* item = GetDrawItems(); item != NULL; item = item->Next() )
    {
        if( item->Type() != SCH_LINE_T )
            continue;

        if( item->GetLayer() != aLayer )
            continue;

        if( !item->HitTest( aPosition, aAccuracy ) )
            continue;

        switch( aSearchType )
        {
        case ENTIRE_LENGTH_T:
            return (SCH_LINE*) item;

        case EXCLUDE_END_POINTS_T:
            if( !( (SCH_LINE*) item )->IsEndPoint( aPosition ) )
                return (SCH_LINE*) item;
            break;

        case END_POINTS_ONLY_T:
            if( ( (SCH_LINE*) item )->IsEndPoint( aPosition ) )
                return (SCH_LINE*) item;
        }
    }

    return NULL;
}


SCH_TEXT* SCH_SCREEN::GetLabel( const wxPoint& aPosition, int aAccuracy )
{
    for( SCH_ITEM* item = GetDrawItems(); item != NULL; item = item->Next() )
    {
        switch( item->Type() )
        {
        case SCH_LABEL_T:
        case SCH_GLOBAL_LABEL_T:
        case SCH_HIERARCHICAL_LABEL_T:
            if( item->HitTest( aPosition, aAccuracy ) )
                return (SCH_TEXT*) item;

        default:
            ;
        }
    }

    return NULL;
}


bool SCH_SCREEN::SetComponentFootprint( SCH_SHEET_PATH* aSheetPath, const wxString& aReference,
                                        const wxString& aFootPrint, bool aSetVisible )
{
    SCH_COMPONENT* component;
    bool           found = false;

    for( SCH_ITEM* item = GetDrawItems(); item != NULL; item = item->Next() )
    {
        if( item->Type() != SCH_COMPONENT_T )
            continue;

        component = (SCH_COMPONENT*) item;

        if( aReference.CmpNoCase( component->GetRef( aSheetPath ) ) == 0 )
        {
            // Found: Init Footprint Field

            /* Give a reasonable value to the field position and
             * orientation, if the text is empty at position 0, because
             * it is probably not yet initialized
             */
            if( component->GetField( FOOTPRINT )->m_Text.IsEmpty()
                && ( component->GetField( FOOTPRINT )->m_Pos == wxPoint( 0, 0 ) ) )
            {
                component->GetField( FOOTPRINT )->m_Orient = component->GetField( VALUE )->m_Orient;
                component->GetField( FOOTPRINT )->m_Pos    = component->GetField( VALUE )->m_Pos;
                component->GetField( FOOTPRINT )->m_Pos.y -= 100;
            }

            component->GetField( FOOTPRINT )->m_Text = aFootPrint;

            if( aSetVisible )
                component->GetField( FOOTPRINT )->m_Attributs &= ~TEXT_NO_VISIBLE;
            else
                component->GetField( FOOTPRINT )->m_Attributs |= TEXT_NO_VISIBLE;

            found = true;
        }
    }

    return found;
}


/******************************************************************/
/* Class SCH_SCREENS to handle the list of screens in a hierarchy */
/******************************************************************/

/**
 * Function SortByTimeStamp
 * sorts a list of schematic items by time stamp and type.
 */
static bool SortByTimeStamp( const EDA_ITEM* item1, const EDA_ITEM* item2 )
{
    int ii = item1->m_TimeStamp - item2->m_TimeStamp;

    /* If the time stamps are the same, compare type in order to have component objects
     * before sheet object. This is done because changing the sheet time stamp
     * before the component time stamp could cause the current annotation to be lost.
     */
    if( ( ii == 0 && ( item1->Type() != item2->Type() ) ) && ( item1->Type() == SCH_SHEET_T ) )
        ii = -1;

    return ii < 0;
}


SCH_SCREENS::SCH_SCREENS()
{
    m_index = 0;
    BuildScreenList( g_RootSheet );
}


SCH_SCREENS::~SCH_SCREENS()
{
}


SCH_SCREEN* SCH_SCREENS::GetFirst()
{
    m_index = 0;

    if( m_screens.size() > 0 )
        return m_screens[0];

    return NULL;
}


SCH_SCREEN* SCH_SCREENS::GetNext()
{
    if( m_index < m_screens.size() )
        m_index++;

    return GetScreen( m_index );
}


SCH_SCREEN* SCH_SCREENS::GetScreen( unsigned int aIndex )
{
    if( aIndex < m_screens.size() )
        return m_screens[ aIndex ];

    return NULL;
}


void SCH_SCREENS::AddScreenToList( SCH_SCREEN* aScreen )
{
    if( aScreen == NULL )
        return;

    for( unsigned int i = 0; i < m_screens.size(); i++ )
    {
        if( m_screens[i] == aScreen )
            return;
    }

    m_screens.push_back( aScreen );
}


void SCH_SCREENS::BuildScreenList( EDA_ITEM* aItem )
{
    if( aItem && aItem->Type() == SCH_SHEET_T )
    {
        SCH_SHEET* ds = (SCH_SHEET*) aItem;
        aItem = ds->GetScreen();
    }

    if( aItem && aItem->Type() == SCH_SCREEN_T )
    {
        SCH_SCREEN*     screen = (SCH_SCREEN*) aItem;
        AddScreenToList( screen );
        EDA_ITEM* strct = screen->GetDrawItems();

        while( strct )
        {
            if( strct->Type() == SCH_SHEET_T )
            {
                BuildScreenList( strct );
            }

            strct = strct->Next();
        }
    }
}


void SCH_SCREENS::ClearAnnotation()
{
    for( size_t i = 0;  i < m_screens.size();  i++ )
        m_screens[i]->ClearAnnotation( NULL );
}


void SCH_SCREENS::SchematicCleanUp()
{
    for( size_t i = 0;  i < m_screens.size();  i++ )
    {
        // if wire list has changed, delete the undo/redo list to avoid
        // pointer problems with deleted data.
        if( m_screens[i]->SchematicCleanUp() )
            m_screens[i]->ClearUndoRedoList();
    }
}


int SCH_SCREENS::ReplaceDuplicateTimeStamps()
{
    EDA_ITEMS items;
    SCH_ITEM* item;

    for( size_t i = 0;  i < m_screens.size();  i++ )
        m_screens[i]->GetHierarchicalItems( items );

    if( items.size() < 2 )
        return 0;

    sort( items.begin(), items.end(), SortByTimeStamp );

    int count = 0;

    for( size_t ii = 0;  ii < items.size() - 1;  ii++ )
    {
        item = (SCH_ITEM*)items[ii];

        SCH_ITEM* nextItem = (SCH_ITEM*)items[ii + 1];

        if( item->m_TimeStamp == nextItem->m_TimeStamp )
        {
            count++;

            // for a component, update its Time stamp and its paths
            // (m_PathsAndReferences field)
            if( item->Type() == SCH_COMPONENT_T )
                ( (SCH_COMPONENT*) item )->SetTimeStamp( GetTimeStamp() );

            // for a sheet, update only its time stamp (annotation of its
            // components will be lost)
            // @todo: see how to change sheet paths for its cmp list (can
            //        be possible in most cases)
            else
                item->m_TimeStamp = GetTimeStamp();
        }
    }

    return count;
}


void SCH_SCREENS::SetDate( const wxString& aDate )
{
    for( size_t i = 0;  i < m_screens.size();  i++ )
        m_screens[i]->m_Date = aDate;
}


void SCH_SCREENS::DeleteAllMarkers( int aMarkerType )
{
    SCH_ITEM* item;
    SCH_ITEM* nextItem;
    SCH_MARKER* marker;
    SCH_SCREEN* screen;

    for( screen = GetFirst(); screen != NULL; screen = GetNext() )
    {
        for( item = screen->GetDrawItems(); item != NULL; item = nextItem )
        {
            nextItem = item->Next();

            if( item->Type() != SCH_MARKER_T )
                continue;

            marker = (SCH_MARKER*) item;

            if( marker->GetMarkerType() != aMarkerType )
                continue;

            screen->DeleteItem( marker );
        }
    }
}
