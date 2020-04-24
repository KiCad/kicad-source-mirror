/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2008 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 1992-2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <common.h>
#include <eda_rect.h>
#include <fctsys.h>
#include <gr_text.h>
#include <id.h>
#include <kicad_string.h>
#include <kiway.h>
#include <pgm_base.h>
#include <plotter.h>
#include <project.h>
#include <sch_draw_panel.h>
#include <sch_edit_frame.h>
#include <sch_item.h>

#include <class_library.h>
#include <connection_graph.h>
#include <lib_pin.h>
#include <netlist_object.h>
#include <sch_component.h>
#include <sch_junction.h>
#include <sch_line.h>
#include <sch_marker.h>
#include <sch_sheet.h>
#include <sch_text.h>
#include <symbol_lib_table.h>
#include <tool/common_tools.h>

#include <thread>
#include <algorithm>
#include <future>

// TODO(JE) Debugging only
#include <profile.h>

#define EESCHEMA_FILE_STAMP   "EESchema"
#define ZOOM_FACTOR( x )       ( x * IU_PER_MILS )


/* Default zoom values. Limited to these values to keep a decent size
 * to menus
 */
static double SchematicZoomList[] =
{
    ZOOM_FACTOR( 0.5 ),
    ZOOM_FACTOR( 0.7 ),
    ZOOM_FACTOR( 1.0 ),
    ZOOM_FACTOR( 1.5 ),
    ZOOM_FACTOR( 2.0 ),
    ZOOM_FACTOR( 3.0 ),
    ZOOM_FACTOR( 4.0 ),
    ZOOM_FACTOR( 6.0 ),
    ZOOM_FACTOR( 8.0 ),
    ZOOM_FACTOR( 11.0 ),
    ZOOM_FACTOR( 13.0 ),
    ZOOM_FACTOR( 16.0 ),
    ZOOM_FACTOR( 20.0 ),
    ZOOM_FACTOR( 26.0 ),
    ZOOM_FACTOR( 32.0 ),
    ZOOM_FACTOR( 48.0 ),
    ZOOM_FACTOR( 64.0 ),
    ZOOM_FACTOR( 80.0 ),
    ZOOM_FACTOR( 128.0 )
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
    { ID_POPUP_GRID_LEVEL_100, wxRealPoint( Mils2iu( 100 ), Mils2iu( 100 ) ) },
    { ID_POPUP_GRID_LEVEL_50, wxRealPoint( Mils2iu( 50 ), Mils2iu( 50 ) ) },
    { ID_POPUP_GRID_LEVEL_25, wxRealPoint( Mils2iu( 25 ), Mils2iu( 25 ) ) },
    { ID_POPUP_GRID_LEVEL_10, wxRealPoint( Mils2iu( 10 ), Mils2iu( 10 ) ) },
    { ID_POPUP_GRID_LEVEL_5, wxRealPoint( Mils2iu( 5 ), Mils2iu( 5 ) ) },
    { ID_POPUP_GRID_LEVEL_2, wxRealPoint( Mils2iu( 2 ), Mils2iu( 2 ) ) },
    { ID_POPUP_GRID_LEVEL_1, wxRealPoint( Mils2iu( 1 ), Mils2iu( 1 ) ) },
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
    SetGrid( wxRealPoint( Mils2iu( 50 ), Mils2iu( 50 ) ) );

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


void SCH_SCREEN::Append( SCH_ITEM* aItem )
{
    if( aItem->Type() != SCH_SHEET_PIN_T && aItem->Type() != SCH_FIELD_T )
    {
        m_rtree.insert( aItem );
        --m_modification_sync;
    }
}


void SCH_SCREEN::Append( SCH_SCREEN* aScreen )
{
    wxCHECK_RET( aScreen, "Invalid screen object." );

    // No need to descend the hierarchy.  Once the top level screen is copied, all of it's
    // children are copied as well.
    for( auto aItem : aScreen->m_rtree )
        Append( aItem );

    aScreen->Clear( false );
}


void SCH_SCREEN::Clear( bool aFree )
{
    if( aFree )
        FreeDrawList();
    else
        m_rtree.clear();

    // Clear the project settings
    m_ScreenNumber = m_NumberOfScreens = 1;

    m_titles.Clear();
}


void SCH_SCREEN::FreeDrawList()
{
    // We don't know which order we will encounter dependent items (e.g. pins or fields), so
    // we store the items to be deleted until we've fully cleared the tree before deleting
    std::vector<SCH_ITEM*> delete_list;

    std::copy_if( m_rtree.begin(), m_rtree.end(), std::back_inserter( delete_list ),
            []( SCH_ITEM* aItem )
            {
                return ( aItem->Type() != SCH_SHEET_PIN_T && aItem->Type() != SCH_FIELD_T );
            } );

    m_rtree.clear();

    for( auto item : delete_list )
        delete item;
}


void SCH_SCREEN::Update( SCH_ITEM* aItem )
{
    if( Remove( aItem ) )
        Append( aItem );
}


bool SCH_SCREEN::Remove( SCH_ITEM* aItem )
{
    return m_rtree.remove( aItem );
}


void SCH_SCREEN::DeleteItem( SCH_ITEM* aItem )
{
    wxCHECK_RET( aItem, wxT( "Cannot delete invalid item from screen." ) );

    SetModify();
    Remove( aItem );

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
        delete aItem;
    }
}


bool SCH_SCREEN::CheckIfOnDrawList( SCH_ITEM* aItem )
{
    return m_rtree.contains( aItem, true );
}


SCH_ITEM* SCH_SCREEN::GetItem( const wxPoint& aPosition, int aAccuracy, KICAD_T aType )
{
    EDA_RECT bbox;
    bbox.SetOrigin( aPosition );
    bbox.Inflate( aAccuracy );

    for( auto item : Items().Overlapping( aType, bbox ) )
    {
        if( item->HitTest( aPosition, aAccuracy ) )
            return item;
    }

    return nullptr;
}


std::set<SCH_ITEM*> SCH_SCREEN::MarkConnections( SCH_LINE* aSegment )
{
    std::set<SCH_ITEM*>   retval;
    std::stack<SCH_LINE*> to_search;

    wxCHECK_MSG( ( aSegment ) && ( aSegment->Type() == SCH_LINE_T ), retval,
            wxT( "Invalid object pointer." ) );

    to_search.push( aSegment );

    while( !to_search.empty() )
    {
        auto test_item = to_search.top();
        to_search.pop();

        for( auto item : Items().Overlapping( SCH_JUNCTION_T, test_item->GetBoundingBox() ) )
        {
            if( test_item->IsEndPoint( item->GetPosition() ) )
                retval.insert( item );
        }

        for( auto item : Items().Overlapping( SCH_LINE_T, test_item->GetBoundingBox() ) )
        {
            // Skip connecting lines on different layers (e.g. buses)
            if( test_item->GetLayer() != item->GetLayer() )
                continue;

            auto line = static_cast<SCH_LINE*>( item );

            if( ( test_item->IsEndPoint( line->GetStartPoint() )
                        && !GetPin( line->GetStartPoint(), NULL, true ) )
                    || ( test_item->IsEndPoint( line->GetEndPoint() )
                               && !GetPin( line->GetEndPoint(), nullptr, true ) ) )
            {
                auto result = retval.insert( line );

                if( result.second )
                    to_search.push( line );
            }
        }
    }

    return retval;
}


bool SCH_SCREEN::IsJunctionNeeded( const wxPoint& aPosition, bool aNew )
{
    enum { WIRES, BUSES } layers;

    bool    has_nonparallel[ sizeof( layers ) ] = { false };
    int     end_count[ sizeof( layers ) ] = { 0 };
    int     pin_count = 0;

    std::vector<SCH_LINE*> lines[ sizeof( layers ) ];

    for( auto item : Items().Overlapping( aPosition ) )
    {
        if( item->GetEditFlags() & STRUCT_DELETED )
            continue;

        if( aNew && ( item->Type() == SCH_JUNCTION_T ) && ( item->HitTest( aPosition ) ) )
            return false;

        if( ( item->Type() == SCH_LINE_T ) && ( item->HitTest( aPosition, 0 ) ) )
        {
            if( item->GetLayer() == LAYER_WIRE )
                lines[WIRES].push_back( (SCH_LINE*) item );
            else if( item->GetLayer() == LAYER_BUS )
                lines[BUSES].push_back( (SCH_LINE*) item );
        }

        if( ( ( item->Type() == SCH_COMPONENT_T ) || ( item->Type() == SCH_SHEET_T ) )
                && ( item->IsConnected( aPosition ) ) )
            pin_count++;
    }

    for( int i : { WIRES, BUSES } )
    {
        bool removed_overlapping = false;
        bool mid_point = false;

        for( auto line = lines[i].begin(); line < lines[i].end(); line++ )
        {
            if( !(*line)->IsEndPoint( aPosition ) )
                mid_point = true;
            else
                end_count[i]++;

            for( auto second_line = lines[i].end() - 1; second_line > line; second_line-- )
            {
                if( !(*line)->IsParallel( *second_line ) )
                    has_nonparallel[i] = true;
                else if( !removed_overlapping
                         && (*line)->IsSameQuadrant( *second_line, aPosition ) )
                {
                    removed_overlapping = true;
                }
            }
        }

        /// A line with a midpoint should be counted as two endpoints for this calculation
        /// because the junction will split the line into two if there is another item
        /// present at the point.
        if( mid_point )
            end_count[i] += 2;

        ///Overlapping lines that point in the same direction should not be counted
        /// as extra end_points.
        if( removed_overlapping )
            end_count[i]--;
    }

    // If there are three or more endpoints
    if( pin_count && pin_count + end_count[WIRES] > 2 )
        return true;

    // If there is at least one segment that ends on a non-parallel line or
    // junction of two other lines
    if( has_nonparallel[WIRES] && end_count[WIRES] > 2 )
        return true;

    // Check for bus - bus junction requirements
    if( has_nonparallel[BUSES] && end_count[BUSES] > 2 )
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
        if( GetItem( aPosition, Mils2iu( 6 ), SCH_BUS_WIRE_ENTRY_T) )
            return true;

        if( GetItem( aPosition, Mils2iu( 6 ), SCH_BUS_BUS_ENTRY_T) )
            return true;

        if( GetItem( aPosition, SCH_JUNCTION::GetSymbolSize(), SCH_JUNCTION_T ) )
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
    if( !IsEmpty() )
    {
        std::vector<SCH_COMPONENT*> cmps;
        SYMBOL_LIB_TABLE* libs = Prj().SchSymbolLibTable();
        int mod_hash = libs->GetModifyHash();

        for( SCH_ITEM* aItem : Items().OfType( SCH_COMPONENT_T ) )
            cmps.push_back( static_cast<SCH_COMPONENT*>( aItem ) );

        for( SCH_COMPONENT* cmp : cmps )
            Remove( cmp );

        // Must we resolve?
        if( (m_modification_sync != mod_hash) || aForce )
        {
            SCH_COMPONENT::ResolveAll( cmps, *libs, Prj().SchLibs()->GetCacheLibrary() );

            m_modification_sync = mod_hash;     // note the last mod_hash
        }
        // Resolving will update the pin caches but we must ensure that this happens
        // even if the libraries don't change.
        else
        {
            for( SCH_COMPONENT* cmp : cmps )
                cmp->UpdatePins();
        }

        // Changing the symbol may adjust the bbox of the symbol.  This re-inserts the
        // item with the new bbox
        for( SCH_COMPONENT* cmp : cmps )
            Append( cmp );
    }
}


void SCH_SCREEN::Print( RENDER_SETTINGS* aSettings )
{
    // Ensure links are up to date, even if a library was reloaded for some reason:
    std::vector< SCH_ITEM* > junctions;
    std::vector<SCH_ITEM*>   bitmaps;
    std::vector<SCH_ITEM*>   other;

    // Ensure links are up to date, even if a library was reloaded for some reason:
    UpdateSymbolLinks();

    for( auto item : Items() )
    {
        if( item->IsMoving() || item->IsResized() )
            continue;

        if( item->Type() == SCH_JUNCTION_T )
            junctions.push_back( item );
        else if( item->Type() == SCH_BITMAP_T )
            bitmaps.push_back( item );
        else
            other.push_back( item );
    }

    /// Sort to ensure plot-order consistency with screen drawing
    std::sort( other.begin(), other.end(),
               []( const SCH_ITEM* a, const SCH_ITEM* b )
               {
                    if( a->Type() == b->Type() )
                        return a->GetLayer() > b->GetLayer();

                    return a->Type() > b->Type();
               } );

    for( auto item : bitmaps )
        item->Print( aSettings, wxPoint( 0, 0 ) );

    for( auto item : other )
        item->Print( aSettings, wxPoint( 0, 0 ) );

    for( auto item : junctions )
        item->Print( aSettings, wxPoint( 0, 0 ) );
}


void SCH_SCREEN::Plot( PLOTTER* aPlotter )
{
    // Ensure links are up to date, even if a library was reloaded for some reason:
    std::vector< SCH_ITEM* > junctions;
    std::vector< SCH_ITEM* > bitmaps;
    std::vector< SCH_ITEM* > other;

    // Ensure links are up to date, even if a library was reloaded for some reason:
    UpdateSymbolLinks();

    for( auto item : Items() )
    {
        if( item->IsMoving() || item->IsResized() )
            continue;

        if( item->Type() == SCH_JUNCTION_T )
            junctions.push_back( item );
        else if( item->Type() == SCH_BITMAP_T )
            bitmaps.push_back( item );
        else
            other.push_back( item );
    }

    /// Sort to ensure plot-order consistency with screen drawing
    std::sort( other.begin(), other.end(), []( const SCH_ITEM* a, const SCH_ITEM* b ) {
        if( a->Type() == b->Type() )
            return a->GetLayer() > b->GetLayer();

        return a->Type() > b->Type();
    } );

    int defaultPenWidth = aPlotter->RenderSettings()->GetDefaultPenWidth();

    // Bitmaps are drawn first to ensure they are in the background
    // This is particularly important for the wxPostscriptDC (used in *nix printers) as
    // the bitmap PS command clears the screen
    for( auto item : bitmaps )
    {
        aPlotter->SetCurrentLineWidth( std::max( item->GetPenWidth(), defaultPenWidth ) );
        item->Plot( aPlotter );
    }

    for( auto item : other )
    {
        aPlotter->SetCurrentLineWidth( std::max( item->GetPenWidth(), defaultPenWidth ) );
        item->Plot( aPlotter );
    }

    for( auto item : junctions )
    {
        aPlotter->SetCurrentLineWidth( std::max( item->GetPenWidth(), defaultPenWidth ) );
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
    for( auto item : Items() )
        item->ClearTempFlags();
}


LIB_PIN* SCH_SCREEN::GetPin( const wxPoint& aPosition, SCH_COMPONENT** aComponent,
                             bool aEndPointOnly )
{
    SCH_COMPONENT*  component = NULL;
    LIB_PIN*        pin = NULL;

    for( SCH_ITEM* item : Items().Overlapping( SCH_COMPONENT_T, aPosition ) )
    {
        component = static_cast<SCH_COMPONENT*>( item );

        if( aEndPointOnly )
        {
            pin = NULL;

            if( !component->GetPartRef() )
                continue;

            for( pin = component->GetPartRef()->GetNextPin(); pin;
                 pin = component->GetPartRef()->GetNextPin( pin ) )
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


SCH_SHEET_PIN* SCH_SCREEN::GetSheetLabel( const wxPoint& aPosition )
{
    SCH_SHEET_PIN* sheetPin = nullptr;

    for( SCH_ITEM* item : Items().OfType( SCH_SHEET_T ) )
    {
        auto sheet = static_cast<SCH_SHEET*>( item );

        sheetPin = sheet->GetPin( aPosition );

        if( sheetPin )
            break;
    }

    return sheetPin;
}


size_t SCH_SCREEN::CountConnectedItems( const wxPoint& aPos, bool aTestJunctions )
{
    size_t count = 0;

    for( SCH_ITEM* item : Items() )
    {
        if( ( item->Type() != SCH_JUNCTION_T || aTestJunctions ) && item->IsConnected( aPos ) )
            count++;
    }

    return count;
}


void SCH_SCREEN::ClearAnnotation( SCH_SHEET_PATH* aSheetPath )
{

    for( SCH_ITEM* item : Items().OfType( SCH_COMPONENT_T ) )
    {
        SCH_COMPONENT* component = static_cast<SCH_COMPONENT*>( item );

        component->ClearAnnotation( aSheetPath );

        // Clear the modified component flag set by component->ClearAnnotation
        // because we do not use it here and we should not leave this flag set,
        // when an editing is finished:
        component->ClearFlags();
    }
}


void SCH_SCREEN::EnsureAlternateReferencesExist()
{
    if( GetClientSheetPathsCount() <= 1 )   // No need for alternate reference
        return;

    for( SCH_ITEM* item : Items().OfType( SCH_COMPONENT_T ) )
    {
        auto component = static_cast<SCH_COMPONENT*>( item );

        // Add (when not existing) all sheet path entries
        for( unsigned int ii = 0; ii < m_clientSheetPathList.GetCount(); ii++ )
            component->AddSheetPathReferenceEntryIfMissing( m_clientSheetPathList[ii] );
    }
}


void SCH_SCREEN::GetHierarchicalItems( EDA_ITEMS& aItems )
{
    for( SCH_ITEM* item : Items() )
    {
        if( ( item->Type() == SCH_SHEET_T ) || ( item->Type() == SCH_COMPONENT_T ) )
            aItems.push_back( item );
    }
}


bool SCH_SCREEN::TestDanglingEnds( const SCH_SHEET_PATH* aPath )
{
    std::vector< DANGLING_END_ITEM > endPoints;
    bool hasStateChanged = false;

    for( SCH_ITEM* item : Items() )
        item->GetEndPoints( endPoints );

    for( SCH_ITEM* item : Items() )
    {
        if( item->UpdateDanglingState( endPoints, aPath ) )
            hasStateChanged = true;
    }

    return hasStateChanged;
}


SCH_LINE* SCH_SCREEN::GetLine( const wxPoint& aPosition, int aAccuracy, int aLayer,
                               SCH_LINE_TEST_T aSearchType )
{
    // an accuracy of 0 had problems with rounding errors; use at least 1
    aAccuracy = std::max( aAccuracy, 1 );

    for( SCH_ITEM* item : Items() )
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
    for( SCH_ITEM* item : Items().Overlapping( aPosition, aAccuracy ) )
    {
        switch( item->Type() )
        {
        case SCH_LABEL_T:
        case SCH_GLOBAL_LABEL_T:
        case SCH_HIER_LABEL_T:
            if( item->HitTest( aPosition, aAccuracy ) )
                return (SCH_TEXT*) item;

            break;

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

    for( SCH_ITEM* item : Items().OfType( SCH_COMPONENT_T ) )
    {
        component = static_cast<SCH_COMPONENT*>( item );

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
                    fpfield->Offset( wxPoint( 0, Mils2iu( 100 ) ) );
                else
                    fpfield->Offset( wxPoint( Mils2iu( 100 ), 0 ) );
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
        for( const auto& alias : aSheets[i].LastScreen()->GetBusAliases() )
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

    for( const SCH_ITEM* item : Items() )
        item->Show( nestLevel + 1, os );

    NestedSpace( nestLevel, os ) << "</" << GetClass().Lower().mb_str() << ">\n";
}
#endif


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

    for( const SCH_SCREEN* screen : m_screens )
    {
        if( screen == aScreen )
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

        for( SCH_ITEM* item : screen->Items().OfType( SCH_SHEET_T ) )
            buildScreenList( static_cast<SCH_SHEET*>( item ) );
    }
}


void SCH_SCREENS::ClearAnnotation()
{
    for( SCH_SCREEN* screen : m_screens )
        screen->ClearAnnotation( NULL );
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
    int count = 0;

    auto timestamp_cmp = []( const EDA_ITEM* a, const EDA_ITEM* b ) -> bool
                         {
                             return a->m_Uuid < b->m_Uuid;
                         };

    std::set<EDA_ITEM*, decltype( timestamp_cmp )> unique_stamps( timestamp_cmp );

    for( SCH_SCREEN* screen : m_screens )
        screen->GetHierarchicalItems( items );

    if( items.size() < 2 )
        return 0;

    for( EDA_ITEM* item : items )
    {
        if( !unique_stamps.insert( item ).second )
        {
            // Reset to fully random UUID.  This may lose reference, but better to be
            // deterministic about it rather than to have duplicate UUIDs with random
            // side-effects.
            const_cast<KIID&>( item->m_Uuid ) = KIID();
            count++;
        }
    }

    return count;
}


void SCH_SCREENS::DeleteMarker( SCH_MARKER* aMarker )
{
    for( SCH_SCREEN* screen = GetFirst(); screen; screen = GetNext() )
    {
        for( SCH_ITEM* item : screen->Items().OfType( SCH_MARKER_T ) )
        {
            if( item == aMarker )
            {
                screen->DeleteItem( item );
                return;
            }
        }
    }
}


void SCH_SCREENS::DeleteMarkers( enum MARKER_BASE::TYPEMARKER aMarkerType, int aErrorCode )
{
    for( SCH_SCREEN* screen = GetFirst(); screen; screen = GetNext() )
    {
        std::vector<SCH_ITEM*> markers;

        for( SCH_ITEM* item : screen->Items().OfType( SCH_MARKER_T ) )
        {
            SCH_MARKER* marker = static_cast<SCH_MARKER*>( item );
            RC_ITEM*    rcItem = marker->GetRCItem();

            if( marker->GetMarkerType() == aMarkerType &&
                    ( aErrorCode == ERCE_UNSPECIFIED || rcItem->GetErrorCode() == aErrorCode ) )
            {
                markers.push_back( item );
            }
        }

        for( SCH_ITEM* marker : markers )
            screen->DeleteItem( marker );
    }
}


void SCH_SCREENS::DeleteAllMarkers( enum MARKER_BASE::TYPEMARKER aMarkerType )
{
    DeleteMarkers( aMarkerType, ERCE_UNSPECIFIED );
}


void SCH_SCREENS::UpdateSymbolLinks( bool aForce )
{
    for( SCH_SCREEN* screen = GetFirst(); screen; screen = GetNext() )
        screen->UpdateSymbolLinks( aForce );

    SCH_SHEET_LIST sheets( g_RootSheet );

    // All of the library symbols have been replaced with copies so the connection graph
    // pointer are stale.
    if( g_ConnectionGraph )
        g_ConnectionGraph->Recalculate( sheets, true );
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
    SCH_SCREEN* screen;
    unsigned cnt = 0;

    for( screen = GetFirst(); screen; screen = GetNext() )
    {
        for( auto item : screen->Items().OfType( SCH_COMPONENT_T ) )
        {
            cnt++;
            auto symbol = static_cast<SCH_COMPONENT*>( item );

            if( !symbol->GetLibId().GetLibNickname().empty() )
                return false;
        }
    }

    return cnt != 0;
}


size_t SCH_SCREENS::GetLibNicknames( wxArrayString& aLibNicknames )
{
    for( SCH_SCREEN* screen = GetFirst(); screen; screen = GetNext() )
    {
        for( auto item : screen->Items().OfType( SCH_COMPONENT_T ) )
        {
            auto  symbol   = static_cast<SCH_COMPONENT*>( item );
            auto& nickname = symbol->GetLibId().GetLibNickname();

            if( !nickname.empty() && ( aLibNicknames.Index( nickname ) == wxNOT_FOUND ) )
                aLibNicknames.Add( nickname );
        }
    }

    return aLibNicknames.GetCount();
}


int SCH_SCREENS::ChangeSymbolLibNickname( const wxString& aFrom, const wxString& aTo )
{
    SCH_SCREEN* screen;
    int cnt = 0;

    for( screen = GetFirst(); screen; screen = GetNext() )
    {
        for( auto item : screen->Items().OfType( SCH_COMPONENT_T ) )
        {
            auto symbol = static_cast<SCH_COMPONENT*>( item );

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


bool SCH_SCREENS::CanCauseCaseSensitivityIssue( const wxString& aSchematicFileName ) const
{
    wxString   lhsLower;
    wxString   rhsLower;
    wxFileName lhs;
    wxFileName rhs = aSchematicFileName;

    wxCHECK( rhs.IsAbsolute(), false );

    for( const SCH_SCREEN* screen : m_screens )
    {
        lhs = screen->GetFileName();

        if( lhs.GetPath() != rhs.GetPath() )
            continue;

        lhsLower = lhs.GetFullName().Lower();
        rhsLower = rhs.GetFullName().Lower();

        if( lhsLower == rhsLower && lhs.GetFullName() != rhs.GetFullName() )
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
                curr_screen->GetClientSheetPaths().Add( sheetpath.PathAsString() );
                break;
            }
        }
    }
}
