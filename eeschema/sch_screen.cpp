/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2008 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 1992-2017 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <kiway.h>
#include <sch_draw_panel.h>
#include <sch_item.h>
#include <gr_text.h>
#include <sch_edit_frame.h>
#include <plotter.h>

#include <netlist.h>
#include <netlist_object.h>
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
#include <symbol_lib_table.h>
#include <tool/common_tools.h>

#include <thread>
#include <algorithm>
#include <future>
#include <array>

// TODO(JE) Debugging only
#include <profile.h>

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

/* Default grid sizes for the schematic editor.
 * Do NOT add others values (mainly grid values in mm), because they
 * can break the schematic: Because wires and pins are considered as
 * connected when the are to the same coordinate we cannot mix
 * coordinates in mils (internal units) and mm (that cannot exactly
 * converted in mils in many cases).  In fact schematic must only use
 * 50 and 25 mils to place labels, wires and components others values
 * are useful only for graphic items (mainly in library editor) so use
 * integer values in mils only.  The 100 mil grid is added to help
 * conform to the KiCad Library Convention. Which states: "Using a
 * 100mil grid, pin ends and origin must lie on grid nodes IEC-60617"
*/
static GRID_TYPE SchematicGridList[] = {
    { ID_POPUP_GRID_LEVEL_100, wxRealPoint( 100, 100 ) },
    { ID_POPUP_GRID_LEVEL_50, wxRealPoint( 50, 50 ) },
    { ID_POPUP_GRID_LEVEL_25, wxRealPoint( 25, 25 ) },
    { ID_POPUP_GRID_LEVEL_10, wxRealPoint( 10, 10 ) },
    { ID_POPUP_GRID_LEVEL_5, wxRealPoint( 5, 5 ) },
    { ID_POPUP_GRID_LEVEL_2, wxRealPoint( 2, 2 ) },
    { ID_POPUP_GRID_LEVEL_1, wxRealPoint( 1, 1 ) },
};


SCH_SCREEN::SCH_SCREEN( KIWAY* aKiway ) :
    BASE_SCREEN( SCH_SCREEN_T ),
    KIWAY_HOLDER( aKiway, KIWAY_HOLDER::HOLDER_TYPE::SCREEN ),
    m_paper( wxT( "A4" ) )
{
    m_modification_sync = 0;

    SetZoom( 32 );

    for( unsigned zoom : SchematicZoomList )
        m_ZoomList.push_back( zoom );

    for( GRID_TYPE grid : SchematicGridList )
        AddGrid( grid );

    // Set the default grid size, now that the grid list is populated
    SetGrid( wxRealPoint( 50, 50 ) );

    m_refCount = 0;

    // Suitable for schematic only. For libedit and viewlib, must be set to true
    m_Center = false;

    InitDataPoints( m_paper.GetSizeIU() );
}


SCH_SCREEN::~SCH_SCREEN()
{
    ClearUndoRedoList();

    // Now delete items in draw list. We do that only if the list is not empty,
    // because if the list was appended to another list (see SCH_SCREEN::Append( SCH_SCREEN* aScreen )
    // it is empty but as no longer the ownership (m_drawList.meOwner == false) of items, and calling
    // FreeDrawList() with m_drawList.meOwner == false will generate a debug alert in debug mode
    if( GetDrawItems() )
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


void SCH_SCREEN::Append( SCH_SCREEN* aScreen )
{
    wxCHECK_RET( aScreen, "Invalid screen object." );

    // No need to decend the hierarchy.  Once the top level screen is copied, all of it's
    // children are copied as well.
    m_drawList.Append( aScreen->m_drawList );

    // This screen owns the objects now.  This prevents the object from being delete when
    // aSheet is deleted.
    aScreen->m_drawList.SetOwnership( false );
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
        wxCHECK_RET( sheet, wxT( "Sheet label parent not properly set, bad programmer!" ) );
        sheet->RemovePin( sheetPin );
        return;
    }
    else
    {
        m_drawList.Remove( aItem );
        delete aItem;
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
    KICAD_T types[] = { aType, EOT };

    for( SCH_ITEM* item = m_drawList.begin(); item; item = item->Next() )
    {
        switch( item->Type() )
        {
        case SCH_COMPONENT_T:
        {
            SCH_COMPONENT* component = (SCH_COMPONENT*) item;

            for( int i = REFERENCE; i < component->GetFieldCount(); i++ )
            {
                SCH_FIELD* field = component->GetField( i );

                if( field->IsType( types ) && field->HitTest( aPosition, aAccuracy ) )
                    return field;
            }

            break;
        }
        case SCH_SHEET_T:
        {
            SCH_SHEET* sheet = (SCH_SHEET*)item;

            SCH_SHEET_PIN* pin = sheet->GetPin( aPosition );

            if( pin && pin->IsType( types ) )
                return pin;

            break;
        }
        default:
            break;
        }

        if( item->IsType( types ) && item->HitTest( aPosition, aAccuracy ) )
            return item;
    }

    return NULL;
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


bool SCH_SCREEN::IsJunctionNeeded( const wxPoint& aPosition, bool aNew )
{
    enum { WIRES, BUSSES } layers;

    bool    has_nonparallel[ sizeof( layers ) ] = { false };
    int     end_count[ sizeof( layers ) ] = { 0 };
    int     pin_count = 0;

    std::vector<SCH_LINE*> lines[ sizeof( layers ) ];

    for( SCH_ITEM* item = m_drawList.begin(); item; item = item->Next() )
    {
        if( item->GetEditFlags() & STRUCT_DELETED )
            continue;

        if( aNew && ( item->Type() == SCH_JUNCTION_T ) && ( item->HitTest( aPosition ) ) )
            return false;

        if( ( item->Type() == SCH_LINE_T ) && ( item->HitTest( aPosition, 0 ) ) )
        {
            if( item->GetLayer() == LAYER_WIRE )
                lines[ WIRES ].push_back( (SCH_LINE*) item );
            else if( item->GetLayer() == LAYER_BUS )
                lines[ BUSSES ].push_back( (SCH_LINE*) item );
        }

        if( ( item->Type() == SCH_COMPONENT_T ) && ( item->IsConnected( aPosition ) ) )
            pin_count++;
    }

    for( int i : { WIRES, BUSSES } )
    {
        bool removed_overlapping = false;
        end_count[i] = lines[i].size();

        for( auto line = lines[i].begin(); line < lines[i].end(); line++ )
        {
            // Consider ending on a line to be equivalent to two endpoints because
            // we will want to split the line if anything else connects
            if( !(*line)->IsEndPoint( aPosition ) )
                end_count[i]++;

            for( auto second_line = lines[i].end() - 1; second_line > line; second_line-- )
            {
                if( !(*line)->IsParallel( *second_line ) )
                    has_nonparallel[i] = true;
                else if( !removed_overlapping
                         && (*line)->IsSameQuadrant( *second_line, aPosition ) )
                {
                    /**
                     * Overlapping lines that point in the same direction should not be counted
                     * as extra end_points.  We remove the overlapping lines, being careful to only
                     * remove them once.
                     */
                    removed_overlapping = true;
                    end_count[i]--;
                }
            }
        }
    }

    //

    // If there are three or more endpoints
    if( pin_count + end_count[0] > 2 )
        return true;

    // If there is at least one segment that ends on a non-parallel line or
    // junction of two other lines
    if( has_nonparallel[0] && end_count[0] > 2 )
        return true;

    // Check for bus - bus junction requirements
    if( has_nonparallel[1] && end_count[1] > 2 )
        return true;

    return false;
}


bool SCH_SCREEN::IsTerminalPoint( const wxPoint& aPosition, int aLayer )
{
    wxCHECK_MSG( aLayer == LAYER_NOTES || aLayer == LAYER_BUS || aLayer == LAYER_WIRE, false,
                 wxT( "Invalid layer type passed to SCH_SCREEN::IsTerminalPoint()." ) );

    SCH_SHEET_PIN* label;
    SCH_TEXT*      text;
    SCH_CONNECTION conn;

    switch( aLayer )
    {
    case LAYER_BUS:

        if( GetBus( aPosition ) )
            return true;

        label = GetSheetLabel( aPosition );

        if( label && conn.IsBusLabel( label->GetText() ) && label->IsConnected( aPosition ) )
            return true;

        text = GetLabel( aPosition );

        if( text && conn.IsBusLabel( text->GetText() ) && text->IsConnected( aPosition )
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

        if( text && text->IsConnected( aPosition ) && !conn.IsBusLabel( text->GetText() ) )
            return true;

        label = GetSheetLabel( aPosition );

        if( label && label->IsConnected( aPosition ) && !conn.IsBusLabel( label->GetText() ) )
            return true;

        break;

    default:
        break;
    }

    return false;
}


void SCH_SCREEN::UpdateSymbolLinks( bool aForce )
{
    // Initialize or reinitialize the pointer to the LIB_PART for each component
    // found in m_drawList, but only if needed (change in lib or schematic)
    // therefore the calculation time is usually very low.
    if( m_drawList.GetCount() )
    {
        SYMBOL_LIB_TABLE* libs = Prj().SchSymbolLibTable();
        int mod_hash = libs->GetModifyHash();
        EE_TYPE_COLLECTOR c;

        c.Collect( GetDrawItems(), EE_COLLECTOR::ComponentsOnly );

        // Must we resolve?
        if( (m_modification_sync != mod_hash) || aForce )
        {
            SCH_COMPONENT::ResolveAll( c, *libs, Prj().SchLibs()->GetCacheLibrary() );

            m_modification_sync = mod_hash;     // note the last mod_hash
        }
        // Resolving will update the pin caches but we must ensure that this happens
        // even if the libraries don't change.
        else
            SCH_COMPONENT::UpdatePins( c );
    }
}


void SCH_SCREEN::Print( wxDC* aDC )
{
    std::vector< SCH_ITEM* > junctions;

    // Ensure links are up to date, even if a library was reloaded for some reason:
    UpdateSymbolLinks();

    for( SCH_ITEM* item = m_drawList.begin(); item; item = item->Next() )
    {
        if( item->IsMoving() || item->IsResized() )
            continue;

        if( item->Type() == SCH_JUNCTION_T )
            junctions.push_back( item );
        else
            item->Print( aDC, wxPoint( 0, 0 ) );
    }

    for( auto item : junctions )
        item->Print( aDC, wxPoint( 0, 0 ) );
}


void SCH_SCREEN::Plot( PLOTTER* aPlotter )
{
    // Ensure links are up to date, even if a library was reloaded for some reason:
    std::vector< SCH_ITEM* > junctions;
    std::vector< SCH_ITEM* > bitmaps;
    std::vector< SCH_ITEM* > other;

    // Ensure links are up to date, even if a library was reloaded for some reason:
    UpdateSymbolLinks();

    for( SCH_ITEM* item = m_drawList.begin(); item; item = item->Next() )
    {
        if( item->IsMoving() || item->IsResized() )
            continue;

        if( item->Type() == SCH_JUNCTION_T )
            junctions.push_back( item );
        else if( item->Type() == SCH_BITMAP_T )
            bitmaps.push_back( item );
        else
            // uncomment line below when there is a virtual EDA_ITEM::GetBoundingBox()
            // if( panel->GetClipBox().Intersects( item->GetBoundingBox() ) )
            other.push_back( item );
    }

    // Bitmaps are drawn first to ensure they are in the background
    // This is particularly important for the wxPostscriptDC (used in *nix printers) as
    // the bitmap PS command clears the screen
    for( auto item : bitmaps )
    {
        aPlotter->SetCurrentLineWidth( item->GetPenSize() );
        item->Plot( aPlotter );
    }

    for( auto item : other )
    {
        aPlotter->SetCurrentLineWidth( item->GetPenSize() );
        item->Plot( aPlotter );
    }

    for( auto item : junctions )
    {
        aPlotter->SetCurrentLineWidth( item->GetPenSize() );
        item->Plot( aPlotter );
    }
}


void SCH_SCREEN::ClearUndoORRedoList( UNDO_REDO_CONTAINER& aList, int aItemCount )
{
    if( aItemCount == 0 )
        return;

    for( auto& command : aList.m_CommandsList )
    {
        command->ClearListAndDeleteItems();
        delete command;
    }

    aList.m_CommandsList.clear();
}


void SCH_SCREEN::ClearDrawingState()
{
    for( SCH_ITEM* item = m_drawList.begin(); item; item = item->Next() )
        item->ClearTempFlags();
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

            auto part = component->GetPartRef().lock();

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
            // when an editing is finished:
            component->ClearFlags();
        }
    }
}


void SCH_SCREEN::EnsureAlternateReferencesExist()
{
    if( GetClientSheetPathsCount() <= 1 )   // No need for alternate reference
        return;

    for( SCH_ITEM* item = m_drawList.begin(); item; item = item->Next() )
    {
        if( item->Type() != SCH_COMPONENT_T )
            continue;

        // Add (when not existing) all sheet path entries
        for( unsigned int ii = 0; ii < m_clientSheetPathList.GetCount(); ii++ )
            ((SCH_COMPONENT*)item)->AddSheetPathReferenceEntryIfMissing( m_clientSheetPathList[ii] );
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


bool SCH_SCREEN::TestDanglingEnds()
{
    SCH_ITEM* item;
    std::vector< DANGLING_END_ITEM > endPoints;
    bool hasStateChanged = false;

    for( item = m_drawList.begin(); item; item = item->Next() )
        item->GetEndPoints( endPoints );

    for( item = m_drawList.begin(); item; item = item->Next() )
    {
        if( item->UpdateDanglingState( endPoints ) )
        {
            hasStateChanged = true;
        }
    }

    return hasStateChanged;
}


SCH_LINE* SCH_SCREEN::GetWireOrBus( const wxPoint& aPosition )
{
    static KICAD_T types[] = { SCH_LINE_LOCATE_WIRE_T, SCH_LINE_LOCATE_BUS_T, EOT };

    for( SCH_ITEM* item = m_drawList.begin(); item; item = item->Next() )
    {
        if( item->IsType( types ) && item->HitTest( aPosition ) )
            return (SCH_LINE*) item;
    }

    return nullptr;
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
        case SCH_HIER_LABEL_T:
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
              && ( fpfield->GetTextPos() == component->GetPosition() ) )
            {
                fpfield->SetTextAngle( component->GetField( VALUE )->GetTextAngle() );
                fpfield->SetTextPos( component->GetField( VALUE )->GetTextPos() );
                fpfield->SetTextSize( component->GetField( VALUE )->GetTextSize() );

                if( fpfield->GetTextAngle() == 0.0 )
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


void SCH_SCREEN::AddBusAlias( std::shared_ptr<BUS_ALIAS> aAlias )
{
    m_aliases.insert( aAlias );
}


bool SCH_SCREEN::IsBusAlias( const wxString& aLabel )
{
    SCH_SHEET_LIST aSheets( g_RootSheet );
    for( unsigned i = 0; i < aSheets.size(); i++ )
    {
        for( auto alias : aSheets[i].LastScreen()->GetBusAliases() )
        {
            if( alias->GetName() == aLabel )
            {
                return true;
            }
        }
    }

    return false;
}


std::shared_ptr<BUS_ALIAS> SCH_SCREEN::GetBusAlias( const wxString& aLabel )
{
    SCH_SHEET_LIST aSheets( g_RootSheet );
    for( unsigned i = 0; i < aSheets.size(); i++ )
    {
        for( auto alias : aSheets[i].LastScreen()->GetBusAliases() )
        {
            if( alias->GetName() == aLabel )
            {
                return alias;
            }
        }
    }

    return NULL;
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


/**
 * Sort a list of schematic items by time stamp and type.
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


SCH_SCREENS::SCH_SCREENS( SCH_SHEET* aSheet )
{
    m_index = 0;
    buildScreenList( ( !aSheet ) ? g_RootSheet : aSheet );
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


void SCH_SCREENS::addScreenToList( SCH_SCREEN* aScreen )
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


void SCH_SCREENS::buildScreenList( SCH_SHEET* aSheet )
{
    if( aSheet && aSheet->Type() == SCH_SHEET_T )
    {
        SCH_SCREEN* screen = aSheet->GetScreen();

        addScreenToList( screen );

        EDA_ITEM* strct = screen->GetDrawItems();

        while( strct )
        {
            if( strct->Type() == SCH_SHEET_T )
            {
                buildScreenList( ( SCH_SHEET* )strct );
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


void SCH_SCREENS::ClearAnnotationOfNewSheetPaths( SCH_SHEET_LIST& aInitialSheetPathList )
{
    // Clear the annotation for the components inside new sheetpaths
    // not already in aInitialSheetList
    SCH_SCREENS screensList( g_RootSheet );     // The list of screens, shared by sheet paths
    screensList.BuildClientSheetPathList();     // build the shared by sheet paths, by screen

    // Search for new sheet paths, not existing in aInitialSheetPathList
    // and existing in sheetpathList
    SCH_SHEET_LIST sheetpathList( g_RootSheet );

    for( SCH_SHEET_PATH& sheetpath: sheetpathList )
    {
        bool path_exists = false;

        for( const SCH_SHEET_PATH& existing_sheetpath: aInitialSheetPathList )
        {
            if( existing_sheetpath.Path() == sheetpath.Path() )
            {
                path_exists = true;
                break;
            }
        }

        if( !path_exists )
        {
            // A new sheet path is found: clear the annotation corresponding to this new path:
            SCH_SCREEN* curr_screen = sheetpath.LastScreen();

            // Clear annotation and create the AR for this path, if not exists,
            // when the screen is shared by sheet paths.
            // Otherwise ClearAnnotation do nothing, because the F1 field is used as
            // reference default value and takes the latest displayed value
            curr_screen->EnsureAlternateReferencesExist();
            curr_screen->ClearAnnotation( &sheetpath );
        }
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


void SCH_SCREENS::DeleteAllMarkers( enum MARKER_BASE::TYPEMARKER aMarkerType )
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


int SCH_SCREENS::GetMarkerCount( enum MARKER_BASE::TYPEMARKER aMarkerType,
                                 enum MARKER_BASE::MARKER_SEVERITY aSeverity )
{
    int count = 0;

    for( SCH_SCREEN* screen = GetFirst(); screen; screen = GetNext() )
    {
        for( SCH_ITEM* item = screen->GetDrawItems(); item; item = item->Next() )
        {
            if( item->Type() != SCH_MARKER_T )
                continue;

            SCH_MARKER* marker = (SCH_MARKER*) item;

            if( ( aMarkerType != MARKER_BASE::MARKER_UNSPEC ) &&
                ( marker->GetMarkerType() != aMarkerType ) )
                continue;

            if( aSeverity == MARKER_BASE::MARKER_SEVERITY_UNSPEC ||
                aSeverity == marker->GetErrorLevel() )
                count++;
        }
    }

    return count;
}


void SCH_SCREENS::UpdateSymbolLinks( bool aForce )
{
    for( SCH_SCREEN* screen = GetFirst(); screen; screen = GetNext() )
        screen->UpdateSymbolLinks( aForce );
}


void SCH_SCREENS::TestDanglingEnds()
{
    std::vector<SCH_SCREEN*> screens;
    for( SCH_SCREEN* screen = GetFirst(); screen; screen = GetNext() )
        screens.push_back( screen );

    size_t parallelThreadCount = std::min<size_t>( std::thread::hardware_concurrency(),
            screens.size() );

    std::atomic<size_t> nextScreen( 0 );
    std::vector<std::future<size_t>> returns( parallelThreadCount );

    auto update_lambda = [&screens, &nextScreen]() -> size_t
    {
        for( auto i = nextScreen++; i < screens.size(); i = nextScreen++ )
            screens[i]->TestDanglingEnds();

        return 1;
    };

    if( parallelThreadCount == 1 )
        update_lambda();
    else
    {
        for( size_t ii = 0; ii < parallelThreadCount; ++ii )
            returns[ii] = std::async( std::launch::async, update_lambda );

        // Finalize the threads
        for( size_t ii = 0; ii < parallelThreadCount; ++ii )
            returns[ii].wait();
    }

}


bool SCH_SCREENS::HasNoFullyDefinedLibIds()
{
    SCH_COMPONENT* symbol;
    SCH_ITEM* item;
    SCH_ITEM* nextItem;
    SCH_SCREEN* screen;
    unsigned cnt = 0;

    for( screen = GetFirst(); screen; screen = GetNext() )
    {
        for( item = screen->GetDrawItems(); item; item = nextItem )
        {
            nextItem = item->Next();

            if( item->Type() != SCH_COMPONENT_T )
                continue;

            cnt += 1;
            symbol = dynamic_cast< SCH_COMPONENT* >( item );
            wxASSERT( symbol );

            if( !symbol->GetLibId().GetLibNickname().empty() )
                return false;
        }
    }

    if( cnt == 0 )
        return false;

    return true;
}


size_t SCH_SCREENS::GetLibNicknames( wxArrayString& aLibNicknames )
{
    SCH_COMPONENT* symbol;
    SCH_ITEM* item;
    SCH_ITEM* nextItem;
    SCH_SCREEN* screen;
    wxString nickname;

    for( screen = GetFirst(); screen; screen = GetNext() )
    {
        for( item = screen->GetDrawItems(); item; item = nextItem )
        {
            nextItem = item->Next();

            if( item->Type() != SCH_COMPONENT_T )
                continue;

            symbol = dynamic_cast< SCH_COMPONENT* >( item );
            wxASSERT( symbol );

            if( !symbol )
                continue;

            nickname = symbol->GetLibId().GetLibNickname();

            if( !nickname.empty() && ( aLibNicknames.Index( nickname ) == wxNOT_FOUND ) )
                aLibNicknames.Add( nickname );;
        }
    }

    return aLibNicknames.GetCount();
}


int SCH_SCREENS::ChangeSymbolLibNickname( const wxString& aFrom, const wxString& aTo )
{
    SCH_COMPONENT* symbol;
    SCH_ITEM* item;
    SCH_ITEM* nextItem;
    SCH_SCREEN* screen;
    int cnt = 0;

    for( screen = GetFirst(); screen; screen = GetNext() )
    {
        for( item = screen->GetDrawItems(); item; item = nextItem )
        {
            nextItem = item->Next();

            if( item->Type() != SCH_COMPONENT_T )
                continue;

            symbol = dynamic_cast< SCH_COMPONENT* >( item );
            wxASSERT( symbol );

            if( symbol->GetLibId().GetLibNickname() != aFrom )
                continue;

            LIB_ID id = symbol->GetLibId();
            id.SetLibNickname( aTo );
            symbol->SetLibId( id );
            cnt++;
        }
    }

    return cnt;
}


bool SCH_SCREENS::HasSchematic( const wxString& aSchematicFileName )
{
    for( const SCH_SCREEN* screen = GetFirst(); screen; screen = GetNext() )
    {
        if( screen->GetFileName() == aSchematicFileName )
            return true;
    }

    return false;
}


void SCH_SCREENS::BuildClientSheetPathList()
{
    SCH_SHEET_LIST sheetList( g_RootSheet );

    for( SCH_SCREEN* curr_screen = GetFirst(); curr_screen; curr_screen = GetNext() )
        curr_screen->GetClientSheetPaths().Clear();

    for( SCH_SHEET_PATH& sheetpath: sheetList )
    {
        SCH_SCREEN* used_screen = sheetpath.LastScreen();

        // SEarch for the used_screen in list and add this unique sheet path:
        for( SCH_SCREEN* curr_screen = GetFirst(); curr_screen; curr_screen = GetNext() )
        {
            if( used_screen == curr_screen )
            {
                curr_screen->GetClientSheetPaths().Add( sheetpath.Path() );
                break;
            }
        }
    }
}
