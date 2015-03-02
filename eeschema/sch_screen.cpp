/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2008-2015 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2015 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file sch_screen.cpp
 * @brief Implementation of SCH_SCREEN and SCH_SCREENS classes.
 */

#include <fctsys.h>
#include <gr_basic.h>
#include <common.h>
#include <kicad_string.h>
#include <eeschema_id.h>
#include <pgm_base.h>
#include <class_drawpanel.h>
#include <sch_item_struct.h>
#include <schframe.h>
#include <plot_common.h>

#include <netlist.h>
#include <class_netlist_object.h>
#include <class_library.h>
#include <sch_junction.h>
#include <sch_bus_entry.h>
#include <sch_line.h>
#include <sch_marker.h>
#include <sch_no_connect.h>
#include <sch_sheet.h>
#include <sch_component.h>
#include <sch_text.h>
#include <lib_pin.h>

#include <boost/foreach.hpp>

#define EESCHEMA_FILE_STAMP   "EESchema"

/* Default zoom values. Limited to these values to keep a decent size
 * to menus
 */
static double SchematicZoomList[] =
{
    0.5, 0.7, 1.0, 1.5, 2.0, 3.0, 4.0, 6.0, 8.0, 11.0,
    13.0, 16.0, 20.0, 26.0, 32.0, 48.0, 64.0, 80.0, 128.0
};

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


SCH_SCREEN::SCH_SCREEN( KIWAY* aKiway ) :
    BASE_SCREEN( SCH_SCREEN_T ),
    KIWAY_HOLDER( aKiway ),
    m_paper( wxT( "A4" ) )
{
    m_modification_sync = 0;

    SetZoom( 32 );

    for( unsigned i = 0; i < DIM( SchematicZoomList ); i++ )
        m_ZoomList.push_back( SchematicZoomList[i] );

    for( unsigned i = 0; i < DIM( SchematicGridList ); i++ )
        AddGrid( SchematicGridList[i] );

    SetGrid( wxRealPoint( 50, 50 ) );   // Default grid size.
    m_refCount = 0;

    // Suitable for schematic only. For libedit and viewlib, must be set to true
    m_Center = false;

    InitDataPoints( m_paper.GetSizeIU() );
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


void SCH_SCREEN::Clear()
{
    FreeDrawList();

    // Clear the project settings
    m_ScreenNumber = m_NumberOfScreens = 1;

    m_titles.Clear();
}


void SCH_SCREEN::FreeDrawList()
{
    m_drawList.DeleteAll();
}


void SCH_SCREEN::Remove( SCH_ITEM* aItem )
{
    m_drawList.Remove( aItem );
}


void SCH_SCREEN::DeleteItem( SCH_ITEM* aItem )
{
    wxCHECK_RET( aItem, wxT( "Cannot delete invalid item from screen." ) );

    SetModify();

    if( aItem->Type() == SCH_SHEET_PIN_T )
    {
        // This structure is attached to a sheet, get the parent sheet object.
        SCH_SHEET_PIN* sheetPin = (SCH_SHEET_PIN*) aItem;
        SCH_SHEET* sheet = sheetPin->GetParent();
        wxCHECK_RET( sheet,
                     wxT( "Sheet label parent not properly set, bad programmer!" ) );
        sheet->RemovePin( sheetPin );
        return;
    }
    else
    {
        delete m_drawList.Remove( aItem );
    }
}


bool SCH_SCREEN::CheckIfOnDrawList( SCH_ITEM* aItem )
{
    SCH_ITEM* itemList = m_drawList.begin();

    while( itemList )
    {
        if( itemList == aItem )
            return true;

        itemList = itemList->Next();
    }

    return false;
}


SCH_ITEM* SCH_SCREEN::GetItem( const wxPoint& aPosition, int aAccuracy, KICAD_T aType ) const
{
    for( SCH_ITEM* item = m_drawList.begin(); item; item = item->Next() )
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


void SCH_SCREEN::ExtractWires( DLIST< SCH_ITEM >& aList, bool aCreateCopy )
{
    SCH_ITEM* item;
    SCH_ITEM* next_item;

    for( item = m_drawList.begin(); item; item = next_item )
    {
        next_item = item->Next();

        switch( item->Type() )
        {
        case SCH_JUNCTION_T:
        case SCH_LINE_T:
            m_drawList.Remove( item );
            aList.Append( item );

            if( aCreateCopy )
                m_drawList.Insert( (SCH_ITEM*) item->Clone(), next_item );

            break;

        default:
            break;
        }
    }
}


void SCH_SCREEN::ReplaceWires( DLIST< SCH_ITEM >& aWireList )
{
    SCH_ITEM* item;
    SCH_ITEM* next_item;

    for( item = m_drawList.begin(); item; item = next_item )
    {
        next_item = item->Next();

        switch( item->Type() )
        {
        case SCH_JUNCTION_T:
        case SCH_LINE_T:
            Remove( item );
            delete item;
            break;

        default:
            break;
        }
    }

    m_drawList.Append( aWireList );
}


void SCH_SCREEN::MarkConnections( SCH_LINE* aSegment )
{
    wxCHECK_RET( (aSegment) && (aSegment->Type() == SCH_LINE_T),
                 wxT( "Invalid object pointer." ) );

    for( SCH_ITEM* item = m_drawList.begin(); item; item = item->Next() )
    {
        if( item->GetFlags() & CANDIDATE )
            continue;

        if( item->Type() == SCH_JUNCTION_T )
        {
            SCH_JUNCTION* junction = (SCH_JUNCTION*) item;

            if( aSegment->IsEndPoint( junction->GetPosition() ) )
                item->SetFlags( CANDIDATE );

            continue;
        }

        if( item->Type() != SCH_LINE_T )
            continue;

        SCH_LINE* segment = (SCH_LINE*) item;

        if( aSegment->IsEndPoint( segment->GetStartPoint() )
            && !GetPin( segment->GetStartPoint(), NULL, true ) )
        {
            item->SetFlags( CANDIDATE );
            MarkConnections( segment );
        }

        if( aSegment->IsEndPoint( segment->GetEndPoint() )
            && !GetPin( segment->GetEndPoint(), NULL, true ) )
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

        if( label && IsBusLabel( label->GetText() ) && label->IsConnected( aPosition ) )
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
        if( GetItem( aPosition, std::max( GetDefaultLineThickness(), 3 ), SCH_BUS_WIRE_ENTRY_T) )
            return true;

        if( GetItem( aPosition, std::max( GetDefaultLineThickness(), 3 ), SCH_BUS_BUS_ENTRY_T) )
            return true;

        if( GetItem( aPosition, std::max( GetDefaultLineThickness(), 3 ), SCH_JUNCTION_T ) )
            return true;

        if( GetPin( aPosition, NULL, true ) )
            return true;

        if( GetWire( aPosition ) )
            return true;

        text = GetLabel( aPosition );

        if( text && text->IsConnected( aPosition ) && !IsBusLabel( text->GetText() ) )
            return true;

        label = GetSheetLabel( aPosition );

        if( label && label->IsConnected( aPosition ) && !IsBusLabel( label->GetText() ) )
            return true;

        break;

    default:
        break;
    }

    return false;
}


bool SCH_SCREEN::SchematicCleanUp( EDA_DRAW_PANEL* aCanvas, wxDC* aDC )
{
    SCH_ITEM* item, * testItem;
    bool      modified = false;

    item = m_drawList.begin();

    for( ; item; item = item->Next() )
    {
        if( ( item->Type() != SCH_LINE_T ) && ( item->Type() != SCH_JUNCTION_T ) )
            continue;

        testItem = item->Next();

        while( testItem )
        {
            if( ( item->Type() == SCH_LINE_T ) && ( testItem->Type() == SCH_LINE_T ) )
            {
                SCH_LINE* line = (SCH_LINE*) item;

                if( line->MergeOverlap( (SCH_LINE*) testItem ) )
                {
                    // Keep the current flags, because the deleted segment can be flagged.
                    item->SetFlags( testItem->GetFlags() );
                    DeleteItem( testItem );
                    testItem = m_drawList.begin();
                    modified = true;
                }
                else
                {
                    testItem = testItem->Next();
                }
            }
            else if ( ( ( item->Type() == SCH_JUNCTION_T ) && ( testItem->Type() == SCH_JUNCTION_T ) ) && ( testItem != item ) )
            {
                if ( testItem->HitTest( item->GetPosition() ) )
                {
                    // Keep the current flags, because the deleted segment can be flagged.
                    item->SetFlags( testItem->GetFlags() );
                    DeleteItem( testItem );
                    testItem = m_drawList.begin();
                    modified = true;
                }
                else
                {
                    testItem = testItem->Next();
                }
            }
            else
            {
                testItem = testItem->Next();
            }
        }
    }

    TestDanglingEnds( aCanvas, aDC );

    if( aCanvas && modified )
        aCanvas->Refresh();

    return modified;
}


bool SCH_SCREEN::Save( FILE* aFile ) const
{
    // Creates header
    if( fprintf( aFile, "%s %s %d\n", EESCHEMA_FILE_STAMP,
                 SCHEMATIC_HEAD_STRING, EESCHEMA_VERSION ) < 0 )
        return false;

    BOOST_FOREACH( const PART_LIB& lib, *Prj().SchLibs() )
    {
        if( fprintf( aFile, "LIBS:%s\n", TO_UTF8( lib.GetName() ) ) < 0 )
            return false;
    }

    // This section is not used, but written for file compatibility
    if( fprintf( aFile, "EELAYER %d %d\n", LAYERSCH_ID_COUNT, 0 ) < 0
        || fprintf( aFile, "EELAYER END\n" ) < 0 )
        return false;

    /* Write page info, ScreenNumber and NumberOfScreen; not very meaningful for
     * SheetNumber and Sheet Count in a complex hierarchy, but useful in
     * simple hierarchy and flat hierarchy.  Used also to search the root
     * sheet ( ScreenNumber = 1 ) within the files
     */
    const TITLE_BLOCK& tb = GetTitleBlock();

    if( fprintf( aFile, "$Descr %s %d %d%s\n", TO_UTF8( m_paper.GetType() ),
                 m_paper.GetWidthMils(),
                 m_paper.GetHeightMils(),
                 !m_paper.IsCustom() && m_paper.IsPortrait() ?
                    " portrait" : ""
                 ) < 0
        || fprintf( aFile, "encoding utf-8\n") < 0
        || fprintf( aFile, "Sheet %d %d\n", m_ScreenNumber, m_NumberOfScreens ) < 0
        || fprintf( aFile, "Title %s\n",    EscapedUTF8( tb.GetTitle() ).c_str() ) < 0
        || fprintf( aFile, "Date %s\n",     EscapedUTF8( tb.GetDate() ).c_str() ) < 0
        || fprintf( aFile, "Rev %s\n",      EscapedUTF8( tb.GetRevision() ).c_str() ) < 0
        || fprintf( aFile, "Comp %s\n",     EscapedUTF8( tb.GetCompany() ).c_str() ) < 0
        || fprintf( aFile, "Comment1 %s\n", EscapedUTF8( tb.GetComment1() ).c_str() ) < 0
        || fprintf( aFile, "Comment2 %s\n", EscapedUTF8( tb.GetComment2() ).c_str() ) < 0
        || fprintf( aFile, "Comment3 %s\n", EscapedUTF8( tb.GetComment3() ).c_str() ) < 0
        || fprintf( aFile, "Comment4 %s\n", EscapedUTF8( tb.GetComment4() ).c_str() ) < 0
        || fprintf( aFile, "$EndDescr\n" ) < 0 )
        return false;

    for( SCH_ITEM* item = m_drawList.begin(); item; item = item->Next() )
    {
        if( !item->Save( aFile ) )
            return false;
    }

    if( fprintf( aFile, "$EndSCHEMATC\n" ) < 0 )
        return false;

    return true;
}

void SCH_SCREEN::BuildSchCmpLinksToLibCmp()
{
    // Initialize or reinitialize the pointer to the LIB_PART for each component
    // found in m_drawList, but only if needed (change in lib or schematic)
    // therefore the calculation time is usually very low.

    if( m_drawList.GetCount() )
    {
        PART_LIBS*  libs = Prj().SchLibs();
        int         mod_hash = libs->GetModifyHash();

        // Must we resolve?
        if( m_modification_sync != mod_hash )
        {
            SCH_TYPE_COLLECTOR c;

            c.Collect( GetDrawItems(), SCH_COLLECTOR::ComponentsOnly );

            SCH_COMPONENT::ResolveAll( c, libs );

            m_modification_sync = mod_hash;     // note the last mod_hash
        }
    }
}



void SCH_SCREEN::Draw( EDA_DRAW_PANEL* aCanvas, wxDC* aDC, GR_DRAWMODE aDrawMode, EDA_COLOR_T aColor )
{
    /* note: SCH_SCREEN::Draw is useful only for schematic.
     * library editor and library viewer do not use m_drawList, and therefore
     * their SCH_SCREEN::Draw() draws nothing
     */

    BuildSchCmpLinksToLibCmp();

    for( SCH_ITEM* item = m_drawList.begin(); item; item = item->Next() )
    {
        if( item->IsMoving() || item->IsResized() )
            continue;

        // uncomment line below when there is a virtual
        // EDA_ITEM::GetBoundingBox()
        //      if( panel->GetClipBox().Intersects( Structs->GetBoundingBox()
        // ) )
        item->Draw( aCanvas, aDC, wxPoint( 0, 0 ), aDrawMode, aColor );
    }
}


/* note: SCH_SCREEN::Plot is useful only for schematic.
 * library editor and library viewer do not use a draw list, and therefore
 * SCH_SCREEN::Plot plots nothing
 */
void SCH_SCREEN::Plot( PLOTTER* aPlotter )
{
    BuildSchCmpLinksToLibCmp();

    for( SCH_ITEM* item = m_drawList.begin();  item;  item = item->Next() )
    {
        aPlotter->SetCurrentLineWidth( item->GetPenSize() );
        item->Plot( aPlotter );
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
    for( SCH_ITEM* item = m_drawList.begin(); item; item = item->Next() )
        item->ClearFlags();
}


LIB_PIN* SCH_SCREEN::GetPin( const wxPoint& aPosition, SCH_COMPONENT** aComponent,
                             bool aEndPointOnly ) const
{
    SCH_ITEM*       item;
    SCH_COMPONENT*  component = NULL;
    LIB_PIN*        pin = NULL;

    for( item = m_drawList.begin(); item; item = item->Next() )
    {
        if( item->Type() != SCH_COMPONENT_T )
            continue;

        component = (SCH_COMPONENT*) item;

        if( aEndPointOnly )
        {
            pin = NULL;

            LIB_PART* part = Prj().SchLibs()->FindLibPart( component->GetPartName() );

            if( !part )
                continue;

            for( pin = part->GetNextPin(); pin; pin = part->GetNextPin( pin ) )
            {
                // Skip items not used for this part.
                if( component->GetUnit() && pin->GetUnit() &&
                    ( pin->GetUnit() != component->GetUnit() ) )
                    continue;

                if( component->GetConvert() && pin->GetConvert() &&
                    ( pin->GetConvert() != component->GetConvert() ) )
                    continue;

                if(component->GetPinPhysicalPosition( pin ) == aPosition )
                    break;
            }
            if( pin )
                break;
        }
        else
        {
            pin = (LIB_PIN*) component->GetDrawItem( aPosition, LIB_PIN_T );

            if( pin )
                break;
        }
    }

    if( pin && aComponent )
        *aComponent = component;

    return pin;
}


SCH_SHEET* SCH_SCREEN::GetSheet( const wxString& aName )
{
    for( SCH_ITEM* item = m_drawList.begin(); item; item = item->Next() )
    {
        if( item->Type() != SCH_SHEET_T )
            continue;

        SCH_SHEET* sheet = (SCH_SHEET*) item;

        if( aName.CmpNoCase( sheet->GetName() ) == 0 )
            return sheet;
    }

    return NULL;
}


SCH_SHEET_PIN* SCH_SCREEN::GetSheetLabel( const wxPoint& aPosition )
{
    SCH_SHEET_PIN* sheetPin = NULL;

    for( SCH_ITEM* item = m_drawList.begin(); item; item = item->Next() )
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

    for( item = m_drawList.begin(); item; item = item->Next() )
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
    for( SCH_ITEM* item = m_drawList.begin(); item; item = item->Next() )
    {
        if( item->Type() == SCH_COMPONENT_T )
        {
            SCH_COMPONENT* component = (SCH_COMPONENT*) item;

            component->ClearAnnotation( aSheetPath );

            // Clear the modified component flag set by component->ClearAnnotation
            // because we do not use it here and we should not leave this flag set,
            // when an edition is finished:
            component->ClearFlags();
        }
    }
}


void SCH_SCREEN::GetHierarchicalItems( EDA_ITEMS& aItems )
{
    SCH_ITEM* item = m_drawList.begin();

    while( item )
    {
        if( ( item->Type() == SCH_SHEET_T ) || ( item->Type() == SCH_COMPONENT_T ) )
            aItems.push_back( item );

        item = item->Next();
    }
}


void SCH_SCREEN::SelectBlockItems()
{
    PICKED_ITEMS_LIST* pickedlist = &m_BlockLocate.GetItems();

    if( pickedlist->GetCount() == 0 )
        return;

    ClearDrawingState();

    for( unsigned ii = 0; ii < pickedlist->GetCount(); ii++ )
    {
        SCH_ITEM* item = (SCH_ITEM*) pickedlist->GetPickedItem( ii );
        item->SetFlags( SELECTED );
    }

    if( !m_BlockLocate.IsDragging() )
        return;

    // Select all the items in the screen connected to the items in the block.
    // be sure end lines that are on the block limits are seen inside this block
    m_BlockLocate.Inflate( 1 );
    unsigned last_select_id = pickedlist->GetCount();

    for( unsigned ii = 0; ii < last_select_id; ii++ )
    {
        SCH_ITEM* item = (SCH_ITEM*)pickedlist->GetPickedItem( ii );
        item->SetFlags( IS_DRAGGED );

        if( item->Type() == SCH_LINE_T )
        {
            item->IsSelectStateChanged( m_BlockLocate );

            if( !item->IsSelected() )
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

            for( size_t jj = 0; jj < connections.size(); jj++ )
                addConnectedItemsToBlock( connections[jj] );
        }
    }

    m_BlockLocate.Inflate( -1 );
}


void SCH_SCREEN::addConnectedItemsToBlock( const wxPoint& position )
{
    SCH_ITEM* item;
    ITEM_PICKER picker;
    bool addinlist = true;

    for( item = m_drawList.begin(); item; item = item->Next() )
    {
        picker.SetItem( item );

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

            if( line->GetStartPoint() == position )
                item->ClearFlags( STARTPOINT );
            else if( line->GetEndPoint() == position )
                item->ClearFlags( ENDPOINT );
        }
        else
            item->SetFlags( SELECTED );

        if( addinlist )
        {
            picker.SetFlags( item->GetFlags() );
            m_BlockLocate.GetItems().PushItem( picker );
        }
    }
}


int SCH_SCREEN::UpdatePickList()
{
    ITEM_PICKER picker;
    EDA_RECT area;
    unsigned count;

    area.SetOrigin( m_BlockLocate.GetOrigin() );
    area.SetSize( m_BlockLocate.GetSize() );
    area.Normalize();

    for( SCH_ITEM* item = m_drawList.begin(); item; item = item->Next() )
    {
        // An item is picked if its bounding box intersects the reference area.
        if( item->HitTest( area ) )
        {
            picker.SetItem( item );
            m_BlockLocate.PushItem( picker );
        }
    }

    // if the block is composed of one item,
    // select it as the current item
    count =  m_BlockLocate.GetCount();
    if( count == 1 )
    {
        SetCurItem( (SCH_ITEM*) m_BlockLocate.GetItem( 0 ) );
    }
    else
    {
        SetCurItem( NULL );
    }

    return count;
}


bool SCH_SCREEN::TestDanglingEnds( EDA_DRAW_PANEL* aCanvas, wxDC* aDC )
{
    SCH_ITEM* item;
    std::vector< DANGLING_END_ITEM > endPoints;
    bool hasDanglingEnds = false;

    for( item = m_drawList.begin(); item; item = item->Next() )
        item->GetEndPoints( endPoints );

    for( item = m_drawList.begin(); item; item = item->Next() )
    {
        if( item->IsDanglingStateChanged( endPoints ) && ( aCanvas ) && ( aDC ) )
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

    for( SCH_ITEM* item = m_drawList.begin(); item; item = item->Next() )
    {
        if( (item->Type() != SCH_LINE_T) || (item->GetLayer() == LAYER_NOTES) )
            continue;

        segment = (SCH_LINE*) item;

        if( !segment->HitTest( aPoint, 0 ) || segment->IsEndPoint( aPoint ) )
            continue;

        // Break the segment at aPoint and create a new segment.
        newSegment = new SCH_LINE( *segment );
        newSegment->SetStartPoint( aPoint );
        segment->SetEndPoint( aPoint );
        m_drawList.Insert( newSegment, segment->Next() );
        item = newSegment;
        brokenSegments = true;
    }

    return brokenSegments;
}


bool SCH_SCREEN::BreakSegmentsOnJunctions()
{
    bool brokenSegments = false;

    for( SCH_ITEM* item = m_drawList.begin(); item; item = item->Next() )
    {
        if( item->Type() == SCH_JUNCTION_T )
        {
            SCH_JUNCTION* junction = ( SCH_JUNCTION* ) item;

            if( BreakSegment( junction->GetPosition() ) )
                brokenSegments = true;
        }
        else
        {
            SCH_BUS_ENTRY_BASE* busEntry = dynamic_cast<SCH_BUS_ENTRY_BASE*>( item );
            if( busEntry )
            {
                if( BreakSegment( busEntry->GetPosition() )
                 || BreakSegment( busEntry->m_End() ) )
                    brokenSegments = true;
            }
        }
    }

    return brokenSegments;
}


int SCH_SCREEN::GetNode( const wxPoint& aPosition, EDA_ITEMS& aList )
{
    for( SCH_ITEM* item = m_drawList.begin(); item; item = item->Next() )
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
    for( SCH_ITEM* item = m_drawList.begin(); item; item = item->Next() )
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
    for( SCH_ITEM* item = m_drawList.begin(); item; item = item->Next() )
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
    for( SCH_ITEM* item = m_drawList.begin(); item; item = item->Next() )
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

    for( SCH_ITEM* item = m_drawList.begin(); item; item = item->Next() )
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
            SCH_FIELD * fpfield = component->GetField( FOOTPRINT );
            if( fpfield->GetText().IsEmpty()
              && ( fpfield->GetTextPosition() == component->GetPosition() ) )
            {
                fpfield->SetOrientation( component->GetField( VALUE )->GetOrientation() );
                fpfield->SetTextPosition( component->GetField( VALUE )->GetTextPosition() );
                fpfield->SetSize( component->GetField( VALUE )->GetSize() );

                if( fpfield->GetOrientation() == 0 )
                    fpfield->Offset( wxPoint( 0, 100 ) );
                else
                    fpfield->Offset( wxPoint( 100, 0 ) );
            }

            fpfield->SetText( aFootPrint );
            fpfield->SetVisible( aSetVisible );

            found = true;
        }
    }

    return found;
}


int SCH_SCREEN::GetConnection( const wxPoint& aPosition, PICKED_ITEMS_LIST& aList,
                               bool aFullConnection )
{
    SCH_ITEM* item;
    EDA_ITEM* tmp;
    EDA_ITEMS list;

    // Clear flags member for all items.
    ClearDrawingState();
    BreakSegmentsOnJunctions();

    if( GetNode( aPosition, list ) == 0 )
        return 0;

    for( size_t i = 0;  i < list.size();  i++ )
    {
        item = (SCH_ITEM*) list[ i ];
        item->SetFlags( SELECTEDNODE | STRUCT_DELETED );

        /* Put this structure in the picked list: */
        ITEM_PICKER picker( item, UR_DELETED );
        aList.PushItem( picker );
    }

    // Mark all wires, junctions, .. connected to the item(s) found.
    if( aFullConnection )
    {
        SCH_LINE* segment;

        for( item = m_drawList.begin(); item; item = item->Next() )
        {
            if( !(item->GetFlags() & SELECTEDNODE) )
                continue;

            if( item->Type() != SCH_LINE_T )
                continue;

            MarkConnections( (SCH_LINE*) item );
        }

        // Search all attached wires (i.e wire with one new dangling end )
        for( item = m_drawList.begin(); item; item = item->Next() )
        {
            bool noconnect = false;

            if( item->GetFlags() & STRUCT_DELETED )
                continue;                                   // Already seen

            if( !(item->GetFlags() & CANDIDATE) )
                continue;                                   // not a candidate

            if( item->Type() != SCH_LINE_T )
                continue;

            item->SetFlags( SKIP_STRUCT );

            segment = (SCH_LINE*) item;

            /* If the wire start point is connected to a wire that was already found
             * and now is not connected, add the wire to the list. */
            for( tmp = m_drawList.begin(); tmp; tmp = tmp->Next() )
            {
                // Ensure tmp is a previously deleted segment:
                if( ( tmp->GetFlags() & STRUCT_DELETED ) == 0 )
                    continue;

                if( tmp->Type() != SCH_LINE_T )
                    continue;

                SCH_LINE* testSegment = (SCH_LINE*) tmp;

               // Test for segment connected to the previously deleted segment:
                if( testSegment->IsEndPoint( segment->GetStartPoint() ) )
                    break;
            }

            // when tmp != NULL, segment is a new candidate:
            // put it in deleted list if
            // the start point is not connected to an other item (like pin)
            if( tmp && !CountConnectedItems( segment->GetStartPoint(), true ) )
                noconnect = true;

            /* If the wire end point is connected to a wire that has already been found
             * and now is not connected, add the wire to the list. */
            for( tmp = m_drawList.begin(); tmp; tmp = tmp->Next() )
            {
                // Ensure tmp is a previously deleted segment:
                if( ( tmp->GetFlags() & STRUCT_DELETED ) == 0 )
                    continue;

                if( tmp->Type() != SCH_LINE_T )
                    continue;

                SCH_LINE* testSegment = (SCH_LINE*) tmp;

                // Test for segment connected to the previously deleted segment:
                if( testSegment->IsEndPoint( segment->GetEndPoint() ) )
                    break;
            }

            // when tmp != NULL, segment is a new candidate:
            // put it in deleted list if
            // the end point is not connected to an other item (like pin)
            if( tmp && !CountConnectedItems( segment->GetEndPoint(), true ) )
                noconnect = true;

            item->ClearFlags( SKIP_STRUCT );

            if( noconnect )
            {
                item->SetFlags( STRUCT_DELETED );

                ITEM_PICKER picker( item, UR_DELETED );
                aList.PushItem( picker );

                item = m_drawList.begin();
            }
        }

        // Get redundant junctions (junctions which connect < 3 end wires
        // and no pin)
        for( item = m_drawList.begin(); item; item = item->Next() )
        {
            if( item->GetFlags() & STRUCT_DELETED )
                continue;

            if( !(item->GetFlags() & CANDIDATE) )
                continue;

            if( item->Type() != SCH_JUNCTION_T )
                continue;

            SCH_JUNCTION* junction = (SCH_JUNCTION*) item;

            if( CountConnectedItems( junction->GetPosition(), false ) <= 2 )
            {
                item->SetFlags( STRUCT_DELETED );

                ITEM_PICKER picker( item, UR_DELETED );
                aList.PushItem( picker );
            }
        }

        for( item = m_drawList.begin(); item;  item = item->Next() )
        {
            if( item->GetFlags() & STRUCT_DELETED )
                continue;

            if( item->Type() != SCH_LABEL_T )
                continue;

            tmp = GetWireOrBus( ( (SCH_TEXT*) item )->GetPosition() );

            if( tmp && tmp->GetFlags() & STRUCT_DELETED )
            {
                item->SetFlags( STRUCT_DELETED );

                ITEM_PICKER picker( item, UR_DELETED );
                aList.PushItem( picker );
            }
        }
    }

    ClearDrawingState();

    return aList.GetCount();
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
    int ii = item1->GetTimeStamp() - item2->GetTimeStamp();

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


SCH_SCREEN* SCH_SCREENS::GetScreen( unsigned int aIndex ) const
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

        // Ensure each component has its pointer to its part lib LIB_PART
        // up to date (the cost is low if this is the case)
        // We do this update here, because most of time this function is called
        // to create a netlist, or an ERC, which need this update
        screen->BuildSchCmpLinksToLibCmp();

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

        if( item->GetTimeStamp() == nextItem->GetTimeStamp() )
        {
            count++;

            // for a component, update its Time stamp and its paths
            // (m_PathsAndReferences field)
            if( item->Type() == SCH_COMPONENT_T )
                ( (SCH_COMPONENT*) item )->SetTimeStamp( GetNewTimeStamp() );

            // for a sheet, update only its time stamp (annotation of its
            // components will be lost)
            // @todo: see how to change sheet paths for its cmp list (can
            //        be possible in most cases)
            else
                item->SetTimeStamp( GetNewTimeStamp() );
        }
    }

    return count;
}


void SCH_SCREENS::DeleteAllMarkers( int aMarkerType )
{
    SCH_ITEM* item;
    SCH_ITEM* nextItem;
    SCH_MARKER* marker;
    SCH_SCREEN* screen;

    for( screen = GetFirst(); screen; screen = GetNext() )
    {
        for( item = screen->GetDrawItems(); item; item = nextItem )
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


int SCH_SCREENS::GetMarkerCount( int aMarkerType )
{
    SCH_ITEM* item;
    SCH_ITEM* nextItem;
    SCH_MARKER* marker;
    SCH_SCREEN* screen;
    int count = 0;

    for( screen = GetFirst(); screen; screen = GetNext() )
    {
        for( item = screen->GetDrawItems(); item; item = nextItem )
        {
            nextItem = item->Next();

            if( item->Type() != SCH_MARKER_T )
                continue;

            marker = (SCH_MARKER*) item;

            if( (aMarkerType != -1) && (marker->GetMarkerType() != aMarkerType) )
                continue;

            count++;
        }
    }

    return count;
}

#if defined(DEBUG)
void SCH_SCREEN::Show( int nestLevel, std::ostream& os ) const
{
    // for now, make it look like XML, expand on this later.
    NestedSpace( nestLevel, os ) << '<' << GetClass().Lower().mb_str() << ">\n";

    for( EDA_ITEM* item = m_drawList.begin();  item;  item = item->Next() )
    {
        item->Show( nestLevel+1, os );
    }

    NestedSpace( nestLevel, os ) << "</" << GetClass().Lower().mb_str() << ">\n";
}
#endif
