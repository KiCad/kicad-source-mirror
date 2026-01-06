/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2008 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <stack>
#include <vector>
#include <wx/filefn.h>
#include <wx/log.h>

#include <eda_item.h>
#include <id.h>
#include <string_utils.h>
#include <kiway.h>
#include <plotters/plotter.h>
#include <sch_plotter.h>
#include <project.h>
#include <project_sch.h>
#include <reporter.h>
#include <trace_helpers.h>
#include <sch_edit_frame.h>
#include <sch_item.h>

#include <libraries/legacy_symbol_library.h>
#include <connection_graph.h>
#include <junction_helpers.h>
#include <sch_commit.h>
#include <sch_pin.h>
#include <sch_symbol.h>
#include <sch_group.h>
#include <sch_junction.h>
#include <sch_line.h>
#include <sch_marker.h>
#include <sch_sheet.h>
#include <sch_sheet_pin.h>
#include <sch_text.h>
#include <schematic.h>
#include <symb_transforms_utils.h>
#include <tool/common_tools.h>
#include <sim/sim_model.h> // For V6 to V7 simulation model migration.
#include <locale_io.h>

#include <algorithm>
#include <math/vector3.h>
#include <memory>

// TODO(JE) Debugging only
#include <core/profile.h>
#include <libraries/symbol_library_adapter.h>

#include "sch_bus_entry.h"
#include "sch_shape.h"

/**
 * Flag to enable profiling of the TestDanglingEnds() function.
 *
 * @ingroup trace_env_vars
 */
static const wxChar DanglingProfileMask[] = wxT( "DANGLING_PROFILE" );


SCH_SCREEN::SCH_SCREEN( EDA_ITEM* aParent ) :
    BASE_SCREEN( aParent, SCH_SCREEN_T ),
    m_fileFormatVersionAtLoad( 0 ),
    m_paper( PAGE_SIZE_TYPE::A4 ),
    m_isReadOnly( false ),
    m_fileExists( false )
{
    m_modification_sync = 0;
    m_refCount = 0;
    m_zoomInitialized = false;
    m_LastZoomLevel = 1.0;

    // Suitable for schematic only. For symbol_editor and viewlib, must be set to true
    m_Center = false;

    InitDataPoints( m_paper.GetSizeIU( schIUScale.IU_PER_MILS ) );
}


SCH_SCREEN::~SCH_SCREEN()
{
    clearLibSymbols();
    FreeDrawList();
}


SCHEMATIC* SCH_SCREEN::Schematic() const
{
    wxCHECK_MSG( GetParent() && GetParent()->Type() == SCHEMATIC_T, nullptr,
                 wxT( "SCH_SCREEN must have a SCHEMATIC parent!" ) );

    return static_cast<SCHEMATIC*>( GetParent() );
}


void SCH_SCREEN::clearLibSymbols()
{
    for( const std::pair<const wxString, LIB_SYMBOL*>& libSymbol : m_libSymbols )
        delete libSymbol.second;

    m_libSymbols.clear();
}


void SCH_SCREEN::SetFileName( const wxString& aFileName )
{
    // Don't assert here.  We still call this after failing to load a file in order to show the
    // user what we *tried* to load.
    // wxASSERT( aFileName.IsEmpty() || wxIsAbsolutePath( aFileName ) );

    m_fileName = aFileName;
}


void SCH_SCREEN::IncRefCount()
{
    m_refCount++;
}


void SCH_SCREEN::DecRefCount()
{
    wxCHECK_RET( m_refCount != 0, wxT( "Screen reference count already zero.  Bad programmer!" ) );
    m_refCount--;
}


bool SCH_SCREEN::HasItems( KICAD_T aItemType ) const
{
    EE_RTREE::EE_TYPE sheets = m_rtree.OfType( aItemType );

    return sheets.begin() != sheets.end();
}


bool SCH_SCREEN::ClassOf( const EDA_ITEM* aItem )
{
    return aItem && SCH_SCREEN_T == aItem->Type();
}


void SCH_SCREEN::Append( SCH_ITEM* aItem, bool aUpdateLibSymbol )
{
    if( aItem->Type() != SCH_SHEET_PIN_T && aItem->Type() != SCH_FIELD_T )
    {
        // Ensure the item can reach the SCHEMATIC through this screen
        aItem->SetParent( this );

        if( aItem->Type() == SCH_SYMBOL_T && aUpdateLibSymbol )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( aItem );

            if( symbol->GetLibSymbolRef() )
            {
                symbol->GetLibSymbolRef()->GetDrawItems().sort();

                auto it = m_libSymbols.find( symbol->GetSchSymbolLibraryName() );

                if( it == m_libSymbols.end() || !it->second )
                {
                    m_libSymbols[symbol->GetSchSymbolLibraryName()] =
                            new LIB_SYMBOL( *symbol->GetLibSymbolRef() );
                }
                else
                {
                    // The original library symbol may have changed since the last time
                    // it was added to the schematic.  If it has changed, then a new name
                    // must be created for the library symbol list to prevent all of the
                    // other schematic symbols referencing that library symbol from changing.
                    LIB_SYMBOL* foundSymbol = it->second;

                    foundSymbol->GetDrawItems().sort();

                    if( *foundSymbol != *symbol->GetLibSymbolRef() )
                    {
                        wxString newName;
                        std::vector<wxString> matches;

                        getLibSymbolNameMatches( *symbol, matches );
                        foundSymbol = nullptr;

                        for( const wxString& libSymbolName : matches )
                        {
                            it = m_libSymbols.find( libSymbolName );

                            if( it == m_libSymbols.end() )
                                continue;

                            foundSymbol = it->second;

                            wxCHECK2( foundSymbol, continue );

                            wxString tmp = symbol->GetLibSymbolRef()->GetName();

                            // Temporarily update the new symbol library symbol name so it
                            // doesn't fail on the name comparison below.
                            symbol->GetLibSymbolRef()->SetName( foundSymbol->GetName() );

                            if( *foundSymbol == *symbol->GetLibSymbolRef() )
                            {
                                newName = libSymbolName;
                                symbol->GetLibSymbolRef()->SetName( tmp );
                                break;
                            }

                            symbol->GetLibSymbolRef()->SetName( tmp );
                            foundSymbol = nullptr;
                        }

                        if( !foundSymbol )
                        {
                            int cnt = 1;

                            newName.Printf( wxT( "%s_%d" ),
                                            symbol->GetLibId().GetUniStringLibItemName(),
                                            cnt );

                            while( m_libSymbols.find( newName ) != m_libSymbols.end() )
                            {
                                cnt += 1;
                                newName.Printf( wxT( "%s_%d" ),
                                                symbol->GetLibId().GetUniStringLibItemName(),
                                                cnt );
                            }
                        }

                        // Update the schematic symbol library link as this symbol only exists
                        // in the schematic.
                        symbol->SetSchSymbolLibraryName( newName );

                        if( !foundSymbol )
                        {
                            // Update the schematic symbol library link as this symbol does not
                            // exist in any symbol library.
                            LIB_ID newLibId( wxEmptyString, newName );
                            LIB_SYMBOL* newLibSymbol = new LIB_SYMBOL( *symbol->GetLibSymbolRef() );

                            newLibSymbol->SetLibId( newLibId );
                            newLibSymbol->SetName( newName );
                            symbol->SetLibSymbol( newLibSymbol->Flatten().release() );
                            m_libSymbols[newName] = newLibSymbol;
                        }
                    }
                }
            }
        }

        m_rtree.insert( aItem );
        --m_modification_sync;
    }
}


void SCH_SCREEN::Append( SCH_SCREEN* aScreen )
{
    wxCHECK_RET( aScreen, "Invalid screen object." );

    // No need to descend the hierarchy.  Once the top level screen is copied, all of its
    // children are copied as well.
    for( SCH_ITEM* aItem : aScreen->m_rtree )
        Append( aItem );

    aScreen->Clear( false );
}


void SCH_SCREEN::Clear( bool aFree )
{
    if( aFree )
    {
        FreeDrawList();
        clearLibSymbols();
    }
    else
    {
        m_rtree.clear();
    }

    // Clear the project settings
    m_virtualPageNumber = m_pageCount = 1;

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

    for( SCH_ITEM* item : delete_list )
        delete item;
}


void SCH_SCREEN::Update( SCH_ITEM* aItem, bool aUpdateLibSymbol )
{
    if( Remove( aItem, aUpdateLibSymbol ) )
        Append( aItem, aUpdateLibSymbol );
}


bool SCH_SCREEN::Remove( SCH_ITEM* aItem, bool aUpdateLibSymbol )
{
    bool retv = m_rtree.remove( aItem );

    // Check if the library symbol for the removed schematic symbol is still required.
    if( retv && aItem->Type() == SCH_SYMBOL_T && aUpdateLibSymbol )
    {
        SCH_SYMBOL* removedSymbol = static_cast<SCH_SYMBOL*>( aItem );

        bool removeUnusedLibSymbol = true;

        for( SCH_ITEM* item : Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

            if( removedSymbol->GetSchSymbolLibraryName() == symbol->GetSchSymbolLibraryName() )
            {
                removeUnusedLibSymbol = false;
                break;
            }
        }

        if( removeUnusedLibSymbol )
        {
            auto it = m_libSymbols.find( removedSymbol->GetSchSymbolLibraryName() );

            if( it != m_libSymbols.end() )
            {
                delete it->second;
                m_libSymbols.erase( it );
            }
        }
    }

    return retv;
}


void SCH_SCREEN::DeleteItem( SCH_ITEM* aItem )
{
    wxCHECK_RET( aItem, wxT( "Cannot delete invalid item from screen." ) );

    // Markers are not saved in the file, no need to flag as modified.
    // TODO: Maybe we should have a listing somewhere of items that aren't saved?
    if( aItem->Type() != SCH_MARKER_T )
        SetContentModified();

    Remove( aItem );

    if( aItem->Type() == SCH_SHEET_PIN_T )
    {
        // This structure is attached to a sheet, get the parent sheet object.
        SCH_SHEET_PIN* sheetPin = (SCH_SHEET_PIN*) aItem;
        SCH_SHEET* sheet = sheetPin->GetParent();
        wxCHECK_RET( sheet, wxT( "Sheet pin parent not properly set, bad programmer!" ) );
        sheet->RemovePin( sheetPin );
        return;
    }

    delete aItem;
}


bool SCH_SCREEN::CheckIfOnDrawList( const SCH_ITEM* aItem ) const
{
    return m_rtree.contains( aItem, true );
}


SCH_ITEM* SCH_SCREEN::GetItem( const VECTOR2I& aPosition, int aAccuracy, KICAD_T aType ) const
{
    BOX2I bbox;
    bbox.SetOrigin( aPosition );
    bbox.Inflate( aAccuracy );

    for( SCH_ITEM* item : Items().Overlapping( aType, bbox ) )
    {
        if( item->HitTest( aPosition, aAccuracy ) )
            return item;
    }

    return nullptr;
}


std::set<SCH_ITEM*> SCH_SCREEN::MarkConnections( SCH_ITEM* aItem, bool aSecondPass )
{
#define PROCESSED CANDIDATE     // Don't use SKIP_STRUCT; IsConnected() returns false if it's set.

    std::set<SCH_ITEM*>   retval;
    std::stack<SCH_ITEM*> toSearch;

    auto getItemEndpoints = []( SCH_ITEM* aCandidate ) -> std::vector<VECTOR2I>
    {
        if( !aCandidate )
            return {};

        if( aCandidate->Type() == SCH_LINE_T )
        {
            SCH_LINE* line = static_cast<SCH_LINE*>( aCandidate );
            return { line->GetStartPoint(), line->GetEndPoint() };
        }

        if( aCandidate->Type() == SCH_SHAPE_T )
        {
            SCH_SHAPE* shape = static_cast<SCH_SHAPE*>( aCandidate );

            if( shape->GetShape() == SHAPE_T::ARC || shape->GetShape() == SHAPE_T::BEZIER )
                return { shape->GetStart(), shape->GetEnd() };
            else if( shape->GetShape() == SHAPE_T::RECTANGLE )
                return shape->GetRectCorners();
            else if( shape->GetShape() == SHAPE_T::SEGMENT )
                return { shape->GetStart(), shape->GetEnd() };
            else if( shape->GetShape() == SHAPE_T::POLY )
                return shape->GetPolyPoints();
        }

        return {};
    };

    if( !aItem || getItemEndpoints( aItem ).empty() )
        return retval;

    toSearch.push( aItem );

    while( !toSearch.empty() )
    {
        SCH_ITEM* item = toSearch.top();
        toSearch.pop();

        if( item->HasFlag( PROCESSED ) )
            continue;

        item->SetFlags( PROCESSED );

        const BOX2I bbox = item->GetBoundingBox();

        for( KICAD_T type : { SCH_LINE_T, SCH_SHAPE_T } )
        {
            for( SCH_ITEM* candidate : Items().Overlapping( type, bbox ) )
            {
                if( candidate->HasFlag( PROCESSED ) )
                    continue;

                std::vector<VECTOR2I> endpoints = getItemEndpoints( candidate );

                if( endpoints.empty() )
                    continue;

                // Skip connecting items on different layers (e.g. buses)
                if( item->GetLayer() != candidate->GetLayer() )
                    continue;

                bool sharesEndpoint = false;

                for( const VECTOR2I& pt : endpoints )
                {
                    if( item->IsEndPoint( pt ) )
                    {
                        sharesEndpoint = true;

                        if( aSecondPass && item->IsConnected( pt ) )
                        {
                            SCH_ITEM* junction = GetItem( pt, 0, SCH_JUNCTION_T );

                            if( junction )
                                retval.insert( junction );
                        }
                    }
                }

                if( !sharesEndpoint )
                    continue;

                toSearch.push( candidate );
                retval.insert( candidate );
            }
        }
    }

    for( SCH_ITEM* item : Items() )
        item->ClearTempFlags();

    return retval;
}


bool SCH_SCREEN::IsJunction( const VECTOR2I& aPosition ) const
{
    const JUNCTION_HELPERS::POINT_INFO info =
            JUNCTION_HELPERS::AnalyzePoint( Items(), aPosition, false );
    return info.isJunction;
}


bool SCH_SCREEN::IsExplicitJunction( const VECTOR2I& aPosition ) const
{
    const JUNCTION_HELPERS::POINT_INFO info =
            JUNCTION_HELPERS::AnalyzePoint( Items(), aPosition, false );

    return info.isJunction && ( !info.hasBusEntry || info.hasBusEntryToMultipleWires );
}


bool SCH_SCREEN::IsExplicitJunctionNeeded( const VECTOR2I& aPosition ) const
{
    const JUNCTION_HELPERS::POINT_INFO info =
            JUNCTION_HELPERS::AnalyzePoint( Items(), aPosition, false );

    return info.isJunction && ( !info.hasBusEntry || info.hasBusEntryToMultipleWires )
           && !info.hasExplicitJunctionDot;
}


bool SCH_SCREEN::IsExplicitJunctionAllowed( const VECTOR2I& aPosition ) const
{
    const JUNCTION_HELPERS::POINT_INFO info =
            JUNCTION_HELPERS::AnalyzePoint( Items(), aPosition, true );

    return info.isJunction && (!info.hasBusEntry || info.hasBusEntryToMultipleWires );
}


SPIN_STYLE SCH_SCREEN::GetLabelOrientationForPoint( const VECTOR2I&       aPosition,
                                                    SPIN_STYLE            aDefaultOrientation,
                                                    const SCH_SHEET_PATH* aSheet ) const
{
    auto ret = aDefaultOrientation;

    for( SCH_ITEM* item : Items().Overlapping( aPosition ) )
    {
        if( item->GetEditFlags() & STRUCT_DELETED )
            continue;

        switch( item->Type() )
        {
        case SCH_BUS_WIRE_ENTRY_T:
        {
            auto busEntry = static_cast<const SCH_BUS_WIRE_ENTRY*>( item );
            if( busEntry->m_connected_bus_item )
            {
                // bus connected, take the bus direction into consideration only if it is
                // vertical or horizontal
                auto bus = static_cast<const SCH_LINE*>( busEntry->m_connected_bus_item );
                if( bus->Angle().AsDegrees() == 90.0 )
                {
                    // bus is vertical -> label shall be horizontal and
                    // shall be placed to the side where the bus entry is
                    if( aPosition.x < bus->GetPosition().x )
                        ret = SPIN_STYLE::LEFT;
                    else if( aPosition.x > bus->GetPosition().x )
                        ret = SPIN_STYLE::RIGHT;
                }
                else if( bus->Angle().AsDegrees() == 0.0 )
                {
                    // bus is horizontal -> label shall be vertical and
                    // shall be placed to the side where the bus entry is
                    if( aPosition.y < bus->GetPosition().y )
                        ret = SPIN_STYLE::UP;
                    else if( aPosition.y > bus->GetPosition().y )
                        ret = SPIN_STYLE::BOTTOM;
                }
            }
        }
        break;

        case SCH_LINE_T:
        {
            auto line = static_cast<const SCH_LINE*>( item );
            // line angles goes between -90 and 90 degrees, but normalize
            auto angle = line->Angle().Normalize90().AsDegrees();

            if( -45 < angle && angle <= 45 )
            {
                if( line->GetStartPoint().x <= line->GetEndPoint().x )
                    ret = line->GetEndPoint() == aPosition ? SPIN_STYLE::RIGHT : SPIN_STYLE::LEFT;
                else
                    ret = line->GetEndPoint() == aPosition ? SPIN_STYLE::LEFT : SPIN_STYLE::RIGHT;
            }
            else
            {
                if( line->GetStartPoint().y <= line->GetEndPoint().y )
                    ret = line->GetEndPoint() == aPosition ? SPIN_STYLE::BOTTOM : SPIN_STYLE::UP;
                else
                    ret = line->GetEndPoint() == aPosition ? SPIN_STYLE::UP : SPIN_STYLE::BOTTOM;
            }
        }
        break;

        case SCH_SYMBOL_T:
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

            for( SCH_PIN* pin : symbol->GetPins( aSheet ) )
            {
                if( pin->GetPosition() == aPosition )
                {
                    ret = GetPinSpinStyle( *pin, *symbol );
                    break;
                }
            }
        }
        break;

        default: break;
        }
    }

    return ret;
}


bool SCH_SCREEN::IsTerminalPoint( const VECTOR2I& aPosition, int aLayer ) const
{
    wxCHECK_MSG( aLayer == LAYER_NOTES || aLayer == LAYER_BUS || aLayer == LAYER_WIRE, false,
                 wxT( "Invalid layer type passed to SCH_SCREEN::IsTerminalPoint()." ) );

    SCH_SHEET_PIN* sheetPin;
    SCH_LABEL_BASE* label;

    switch( aLayer )
    {
    case LAYER_BUS:
        if( GetBus( aPosition ) )
            return true;

        sheetPin = GetSheetPin( aPosition );

        if( sheetPin && sheetPin->IsConnected( aPosition ) )
            return true;

        label = GetLabel( aPosition );

        if( label && !label->IsNew() && label->IsConnected( aPosition ) )
            return true;

        break;

    case LAYER_NOTES:
        if( GetLine( aPosition ) )
            return true;

        break;

    case LAYER_WIRE:
        if( GetItem( aPosition, 1, SCH_BUS_WIRE_ENTRY_T ) )
            return true;

        if( GetItem( aPosition, 1, SCH_JUNCTION_T ) )
            return true;

        if( GetPin( aPosition, nullptr, true ) )
            return true;

        if( GetWire( aPosition ) )
            return true;

        label = GetLabel( aPosition, 1 );

        if( label && !label->IsNew() && label->IsConnected( aPosition ) )
            return true;

        sheetPin = GetSheetPin( aPosition );

        if( sheetPin && sheetPin->IsConnected( aPosition ) )
            return true;

        break;

    default:
        break;
    }

    return false;
}


void SCH_SCREEN::UpdateSymbolLinks( REPORTER* aReporter )
{
    wxCHECK_RET( Schematic(), "Cannot call SCH_SCREEN::UpdateSymbolLinks with no SCHEMATIC" );

    wxString msg;
    std::vector<SCH_SYMBOL*> symbols;
    SYMBOL_LIBRARY_ADAPTER* libs = PROJECT_SCH::SymbolLibAdapter( &Schematic()->Project() );

    // This will be a nullptr if an s-expression schematic is loaded.
    LEGACY_SYMBOL_LIBS* legacyLibs = PROJECT_SCH::LegacySchLibs( &Schematic()->Project() );

    for( SCH_ITEM* item : Items().OfType( SCH_SYMBOL_T ) )
        symbols.push_back( static_cast<SCH_SYMBOL*>( item ) );

    // Remove them from the R tree.  Their bounding box size may change.
    for( SCH_SYMBOL* symbol : symbols )
        Remove( symbol );

    // Clear all existing symbol links.
    clearLibSymbols();

    for( SCH_SYMBOL* symbol : symbols )
    {
        LIB_SYMBOL* tmp = nullptr;

        // If the symbol is already in the internal library, map the symbol to it.
        auto it = m_libSymbols.find( symbol->GetSchSymbolLibraryName() );

        if( ( it != m_libSymbols.end() ) )
        {
            if( aReporter )
            {
                msg.Printf( _( "Setting schematic symbol '%s %s' library identifier to '%s'." ),
                            symbol->GetField( FIELD_T::REFERENCE )->GetText(),
                            symbol->GetField( FIELD_T::VALUE )->GetText(),
                            UnescapeString( symbol->GetLibId().Format() ) );
                aReporter->ReportTail( msg, RPT_SEVERITY_INFO );
            }

            // Internal library symbols are already flattened so just make a copy.
            symbol->SetLibSymbol( new LIB_SYMBOL( *it->second ) );
            continue;
        }

        if( !symbol->GetLibId().IsValid() )
        {
            if( aReporter )
            {
                msg.Printf( _( "Schematic symbol reference '%s' library identifier is not valid. "
                               "Unable to link library symbol." ),
                            UnescapeString( symbol->GetLibId().Format() ) );
                aReporter->ReportTail( msg, RPT_SEVERITY_WARNING );
            }

            continue;
        }

        // LIB_TABLE_BASE::LoadSymbol() throws an IO_ERROR if the library nickname
        // is not found in the table so check if the library still exists in the table
        // before attempting to load the symbol.
        if( !libs->HasLibrary( symbol->GetLibId().GetLibNickname() ) && !legacyLibs )
        {
            if( aReporter )
            {
                msg.Printf( _( "Symbol library '%s' not found and no fallback cache library "
                               "available.  Unable to link library symbol." ),
                            symbol->GetLibId().GetLibNickname().wx_str() );
                aReporter->ReportTail( msg, RPT_SEVERITY_WARNING );
            }

            continue;
        }

        if( libs->HasLibrary( symbol->GetLibId().GetLibNickname() ) )
        {
            try
            {
                tmp = libs->LoadSymbol( symbol->GetLibId() );
            }
            catch( const IO_ERROR& ioe )
            {
                if( aReporter )
                {
                    msg.Printf( _( "I/O error %s resolving library symbol %s" ), ioe.What(),
                                UnescapeString( symbol->GetLibId().Format() ) );
                    aReporter->ReportTail( msg, RPT_SEVERITY_ERROR );
                }
            }
        }

        if( !tmp && legacyLibs && legacyLibs->GetLibraryCount() )
        {
            LEGACY_SYMBOL_LIB& legacyCacheLib = legacyLibs->back();

            // It better be the cache library.
            wxCHECK2( legacyCacheLib.IsCache(), continue );

            wxString id = symbol->GetLibId().Format();

            id.Replace( ':', '_' );

            if( aReporter )
            {
                msg.Printf( _( "Falling back to cache to set symbol '%s:%s' link '%s'." ),
                            symbol->GetField( FIELD_T::REFERENCE )->GetText(),
                            symbol->GetField( FIELD_T::VALUE )->GetText(),
                            UnescapeString( id ) );
                aReporter->ReportTail( msg, RPT_SEVERITY_WARNING );
            }

            tmp = legacyCacheLib.FindSymbol( id );
        }

        if( tmp )
        {
            // We want a full symbol not just the top level child symbol.
            std::unique_ptr<LIB_SYMBOL> libSymbol = tmp->Flatten();
            libSymbol->SetParent();

            m_libSymbols.insert( { symbol->GetSchSymbolLibraryName(),
                                   new LIB_SYMBOL( *libSymbol ) } );

            if( aReporter )
            {
                msg.Printf( _( "Setting schematic symbol '%s %s' library identifier to '%s'." ),
                            symbol->GetField( FIELD_T::REFERENCE )->GetText(),
                            symbol->GetField( FIELD_T::VALUE )->GetText(),
                            UnescapeString( symbol->GetLibId().Format() ) );
                aReporter->ReportTail( msg, RPT_SEVERITY_INFO );
            }

            symbol->SetLibSymbol( libSymbol.release() );
        }
        else
        {
            if( aReporter )
            {
                msg.Printf( _( "No library symbol found for schematic symbol '%s %s'." ),
                            symbol->GetField( FIELD_T::REFERENCE )->GetText(),
                            symbol->GetField( FIELD_T::VALUE )->GetText() );
                aReporter->ReportTail( msg, RPT_SEVERITY_ERROR );
            }
        }
    }

    // Changing the symbol may adjust the bbox of the symbol.  This re-inserts the
    // item with the new bbox
    for( SCH_SYMBOL* symbol : symbols )
        Append( symbol );
}


void SCH_SCREEN::UpdateLocalLibSymbolLinks()
{
    std::vector<SCH_SYMBOL*> symbols;

    for( SCH_ITEM* item : Items().OfType( SCH_SYMBOL_T ) )
        symbols.push_back( static_cast<SCH_SYMBOL*>( item ) );

    for( SCH_SYMBOL* symbol : symbols )
    {
        // Changing the symbol may adjust the bbox of the symbol; remove and reinsert it afterwards.
        m_rtree.remove( symbol );

        auto it = m_libSymbols.find( symbol->GetSchSymbolLibraryName() );

        if( it != m_libSymbols.end() )
            symbol->SetLibSymbol( new LIB_SYMBOL( *it->second ) );
        else
            symbol->SetLibSymbol( nullptr );

        m_rtree.insert( symbol );
    }
}


void SCH_SCREEN::SetConnectivityDirty()
{
    for( SCH_ITEM* item : Items() )
        item->SetConnectivityDirty( true );
}


void SCH_SCREEN::Plot( PLOTTER* aPlotter, const SCH_PLOT_OPTS& aPlotOpts ) const
{
    std::vector<SCH_ITEM*> items;
    items.reserve( Items().size() );

    for( SCH_ITEM* item : Items() )
        items.push_back( item );

    Plot( aPlotter, aPlotOpts, items );
}


void SCH_SCREEN::Plot( PLOTTER* aPlotter, const SCH_PLOT_OPTS& aPlotOpts,
                       const std::vector<SCH_ITEM*>& aItems ) const
{
    // Ensure links are up to date, even if a library was reloaded for some reason:
    std::vector<SCH_ITEM*>   junctions;
    std::vector<SCH_ITEM*>   bitmaps;
    std::vector<SCH_SYMBOL*> symbols;
    std::vector<SCH_ITEM*>   other;

    for( SCH_ITEM* item : aItems )
    {
        if( item->IsMoving() )
            continue;

        if( item->Type() == SCH_JUNCTION_T )
            junctions.push_back( item );
        else if( item->Type() == SCH_BITMAP_T )
            bitmaps.push_back( item );
        else
            other.push_back( item );

        // Where the symbols overlap each other, we need to plot the text items a second
        // time to get them on top of the overlapping element.  This collection is in addition
        // to the symbols already collected in `other`
        if( item->Type() == SCH_SYMBOL_T )
        {
            for( SCH_ITEM* sym : m_rtree.Overlapping( SCH_SYMBOL_T, item->GetBoundingBox() ) )
            {
                if( sym != item )
                {
                    symbols.push_back( static_cast<SCH_SYMBOL*>( item ) );
                    break;
                }
            }
        }
    }

    /// Sort to ensure plot-order consistency with screen drawing
    std::sort( other.begin(), other.end(),
               []( const SCH_ITEM* a, const SCH_ITEM* b )
               {
                    if( a->Type() == b->Type() )
                        return a->GetLayer() > b->GetLayer();

                    return a->Type() > b->Type();
                } );

    auto* renderSettings = static_cast<SCH_RENDER_SETTINGS*>( aPlotter->RenderSettings() );
    constexpr bool background = true;

    // Bitmaps are drawn first to ensure they are in the background
    // This is particularly important for the wxPostscriptDC (used in *nix printers) as
    // the bitmap PS command clears the screen
    for( SCH_ITEM* item : bitmaps )
    {
        aPlotter->SetCurrentLineWidth( item->GetEffectivePenWidth( renderSettings ) );
        item->Plot( aPlotter, background, aPlotOpts, 0, 0, { 0, 0 }, false );
    }

    // Plot the background items
    for( SCH_ITEM* item : other )
    {
        aPlotter->SetCurrentLineWidth( item->GetEffectivePenWidth( renderSettings ) );
        item->Plot( aPlotter, background, aPlotOpts, 0, 0, { 0, 0 }, false );
    }

    // Plot the foreground items
    for( SCH_ITEM* item : other )
    {
        double   lineWidth = item->GetEffectivePenWidth( renderSettings );
        aPlotter->SetCurrentLineWidth( lineWidth );

        if( item->Type() != SCH_LINE_T )
            item->Plot( aPlotter, !background, aPlotOpts, 0, 0, { 0, 0 }, false );
        else
        {
            SCH_LINE* aLine = static_cast<SCH_LINE*>( item );

            if( !aLine->IsWire() || !aPlotOpts.m_plotHopOver )
            {
                item->Plot( aPlotter, !background, aPlotOpts, 0, 0, { 0, 0 }, false );
            }
            else
            {
                double arcRadius = lineWidth * aLine->Schematic()->Settings().m_HopOverScale;
                std::vector<VECTOR3I> curr_wire_shape = aLine->BuildWireWithHopShape( this, arcRadius );

                for( size_t ii = 1; ii < curr_wire_shape.size(); ii++ )
                {
                    VECTOR2I start( curr_wire_shape[ii-1].x, curr_wire_shape[ii-1].y );

                    if( curr_wire_shape[ii-1].z == 0 )  // This is the start point of a segment
                                                        // there are always 2 points in list for a segment
                    {
                        VECTOR2I end( curr_wire_shape[ii].x, curr_wire_shape[ii].y );

                        SCH_LINE curr_line( *aLine );
                        curr_line.SetStartPoint( start );
                        curr_line.SetEndPoint( end );
                        curr_line.Plot( aPlotter, !background, aPlotOpts, 0, 0, { 0, 0 }, false );
                    }
                    else   // This is the start point of a arc. there are always 3 points in list for an arc
                    {
                        VECTOR2I arc_middle( curr_wire_shape[ii].x, curr_wire_shape[ii].y );
                        ii++;
                        VECTOR2I arc_end( curr_wire_shape[ii].x, curr_wire_shape[ii].y );
                        ii++;

                        SCH_SHAPE arc( SHAPE_T::ARC, aLine->GetLayer(), lineWidth );

                        arc.SetArcGeometry( start, arc_middle, arc_end );
                        // Hop are a small arc, so use a solid line style gives best results
                        arc.SetLineStyle( LINE_STYLE::SOLID );
                        arc.Plot( aPlotter, !background, aPlotOpts, 0, 0, { 0, 0 }, false );
                    }
                }
            }
        }
    }

    // After plotting the symbols as a group above (in `other`), we need to overplot the pins
    // and symbols to ensure that they are always visible
    TRANSFORM savedTransform = renderSettings->m_Transform;

    for( const SCH_SYMBOL* sym :symbols )
    {
        renderSettings->m_Transform = sym->GetTransform();
        aPlotter->SetCurrentLineWidth( sym->GetEffectivePenWidth( renderSettings ) );

        for( SCH_FIELD field : sym->GetFields() )
        {
            field.ClearRenderCache();
            field.Plot( aPlotter, false, aPlotOpts, sym->GetUnit(), sym->GetBodyStyle(), { 0, 0 },
                        static_cast<const SYMBOL*>( sym )->GetDNP() );

            if( sym->IsSymbolLikePowerLocalLabel() && field.GetId() == FIELD_T::VALUE
                && ( field.IsVisible() || field.IsForceVisible() ) )
            {
                sym->PlotLocalPowerIconShape( aPlotter );
            }
        }

        sym->PlotPins( aPlotter );

        if( static_cast<const SYMBOL*>( sym )->GetDNP() )
            sym->PlotDNP( aPlotter );
    }

    renderSettings->m_Transform = savedTransform;

    for( SCH_ITEM* item : junctions )
    {
        aPlotter->SetCurrentLineWidth( item->GetEffectivePenWidth( renderSettings ) );
        item->Plot( aPlotter, !background, aPlotOpts, 0, 0, { 0, 0 }, false );
    }
}


void SCH_SCREEN::ClearDrawingState()
{
    for( SCH_ITEM* item : Items() )
        item->ClearTempFlags();
}


SCH_PIN* SCH_SCREEN::GetPin( const VECTOR2I& aPosition, SCH_SYMBOL** aSymbol,
                             bool aEndPointOnly ) const
{
    SCH_SYMBOL*  candidate = nullptr;
    SCH_PIN*     pin = nullptr;

    for( SCH_ITEM* item : Items().Overlapping( SCH_SYMBOL_T, aPosition ) )
    {
        candidate = static_cast<SCH_SYMBOL*>( item );

        if( aEndPointOnly )
        {
            pin = nullptr;

            if( !candidate->GetLibSymbolRef() )
                continue;

            for( SCH_PIN* test_pin : candidate->GetLibPins() )
            {
                if( candidate->GetPinPhysicalPosition( test_pin ) == aPosition )
                {
                    pin = test_pin;
                    break;
                }
            }

            if( pin )
                break;
        }
        else
        {
            pin = static_cast<SCH_PIN*>( candidate->GetDrawItem( aPosition, SCH_PIN_T ) );

            if( pin )
                break;
        }
    }

    if( pin && aSymbol )
        *aSymbol = candidate;

    return pin;
}


SCH_SHEET_PIN* SCH_SCREEN::GetSheetPin( const VECTOR2I& aPosition ) const
{
    SCH_SHEET_PIN* sheetPin = nullptr;

    for( SCH_ITEM* item : Items().Overlapping( SCH_SHEET_T, aPosition ) )
    {
        SCH_SHEET* sheet = static_cast<SCH_SHEET*>( item );

        sheetPin = sheet->GetPin( aPosition );

        if( sheetPin )
            break;
    }

    return sheetPin;
}


size_t SCH_SCREEN::CountConnectedItems( const VECTOR2I& aPos, bool aTestJunctions ) const
{
    size_t count = 0;

    for( const SCH_ITEM* item : Items().Overlapping( aPos ) )
    {
        if( ( item->Type() != SCH_JUNCTION_T || aTestJunctions ) && item->IsConnected( aPos ) )
            count++;
    }

    return count;
}


void SCH_SCREEN::ClearAnnotation( SCH_SHEET_PATH* aSheetPath, bool aResetPrefix )
{

    for( SCH_ITEM* item : Items().OfType( SCH_SYMBOL_T ) )
    {
        SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

        symbol->ClearAnnotation( aSheetPath, aResetPrefix );
    }
}


void SCH_SCREEN::EnsureAlternateReferencesExist()
{
    if( GetClientSheetPaths().size() <= 1 ) // No need for alternate reference
        return;

    for( SCH_ITEM* item : Items().OfType( SCH_SYMBOL_T ) )
    {
        SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

        // Add (when not existing) all sheet path entries
        for( const SCH_SHEET_PATH& sheet : GetClientSheetPaths() )
            symbol->AddSheetPathReferenceEntryIfMissing( sheet.Path() );
    }
}


void SCH_SCREEN::GetHierarchicalItems( std::vector<SCH_ITEM*>* aItems ) const
{
    static const std::vector<KICAD_T> hierarchicalTypes = { SCH_SYMBOL_T,
                                                            SCH_SHEET_T,
                                                            SCH_LABEL_LOCATE_ANY_T };

    for( SCH_ITEM* item : Items() )
    {
        if( item->IsType( hierarchicalTypes ) )
            aItems->push_back( item );
    }
}


void SCH_SCREEN::GetSheets( std::vector<SCH_ITEM*>* aItems ) const
{
    for( SCH_ITEM* item : Items().OfType( SCH_SHEET_T ) )
        aItems->push_back( item );

    std::sort( aItems->begin(), aItems->end(),
               []( EDA_ITEM* a, EDA_ITEM* b ) -> bool
               {
                   if( a->GetPosition().x == b->GetPosition().x )
                   {
                       // Ensure deterministic sort
                       if( a->GetPosition().y == b->GetPosition().y )
                           return a->m_Uuid < b->m_Uuid;

                       return a->GetPosition().y < b->GetPosition().y;
                   }
                   else
                   {
                       return a->GetPosition().x < b->GetPosition().x;
                   }
               } );
}


void SCH_SCREEN::TestDanglingEnds( const SCH_SHEET_PATH* aPath,
                                   std::function<void( SCH_ITEM* )>* aChangedHandler ) const
{
    PROF_TIMER timer( __FUNCTION__ );

    std::vector<DANGLING_END_ITEM> endPointsByPos;
    std::vector<DANGLING_END_ITEM> endPointsByType;

    auto get_ends =
            [&]( SCH_ITEM* item )
            {
                if( item->IsConnectable() )
                    item->GetEndPoints( endPointsByType );
            };

    auto update_state =
            [&]( SCH_ITEM* item )
            {
                if( item->UpdateDanglingState( endPointsByType, endPointsByPos, aPath ) )
                {
                    if( aChangedHandler )
                        ( *aChangedHandler )( item );
                }
            };

    for( SCH_ITEM* item : Items() )
    {
        get_ends( item );
        item->RunOnChildren( get_ends, RECURSE_MODE::NO_RECURSE );
    }

    PROF_TIMER sortTimer( "SCH_SCREEN::TestDanglingEnds pre-sort" );
    endPointsByPos = endPointsByType;
    DANGLING_END_ITEM_HELPER::sort_dangling_end_items( endPointsByType, endPointsByPos );
    sortTimer.Stop();

    if( wxLog::IsAllowedTraceMask( DanglingProfileMask ) )
        sortTimer.Show();

    for( SCH_ITEM* item : Items() )
    {
        update_state( item );
        item->RunOnChildren( update_state, RECURSE_MODE::NO_RECURSE );
    }

    if( wxLog::IsAllowedTraceMask( DanglingProfileMask ) )
        timer.Show();
}


SCH_LINE* SCH_SCREEN::GetLine( const VECTOR2I& aPosition, int aAccuracy, int aLayer,
                               SCH_LINE_TEST_T aSearchType ) const
{
    // an accuracy of 0 had problems with rounding errors; use at least 1
    aAccuracy = std::max( aAccuracy, 1 );

    for( SCH_ITEM* item : Items().Overlapping( aPosition, aAccuracy ) )
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

    return nullptr;
}


std::vector<SCH_LINE*> SCH_SCREEN::GetBusesAndWires( const VECTOR2I& aPosition,
                                                     bool            aIgnoreEndpoints ) const
{
    std::vector<SCH_LINE*> retVal;

    for( SCH_ITEM* item : Items().Overlapping( SCH_LINE_T, aPosition ) )
    {
        if( item->IsType( { SCH_ITEM_LOCATE_WIRE_T, SCH_ITEM_LOCATE_BUS_T } ) )
        {
            SCH_LINE* wire = static_cast<SCH_LINE*>( item );

            if( aIgnoreEndpoints && wire->IsEndPoint( aPosition ) )
                continue;

            if( IsPointOnSegment( wire->GetStartPoint(), wire->GetEndPoint(), aPosition ) )
                retVal.push_back( wire );
        }
    }

    return retVal;
}


std::vector<VECTOR2I> SCH_SCREEN::GetConnections() const
{
    std::vector<VECTOR2I> retval;

    for( SCH_ITEM* item : Items() )
    {
        // Avoid items that are changing
        if( !( item->GetEditFlags() & ( IS_MOVING | IS_DELETED ) ) )
        {
            std::vector<VECTOR2I> pts = item->GetConnectionPoints();
            retval.insert( retval.end(), pts.begin(), pts.end() );
        }
    }

    // We always have some overlapping connection points.  Drop duplicates here
    std::sort( retval.begin(), retval.end(),
               []( const VECTOR2I& a, const VECTOR2I& b ) -> bool
               {
                   return a.x < b.x || ( a.x == b.x && a.y < b.y );
               } );

    retval.erase( std::unique( retval.begin(), retval.end() ), retval.end() );

    return retval;
}


std::vector<VECTOR2I> SCH_SCREEN::GetNeededJunctions( const std::deque<EDA_ITEM*>& aItems ) const
{
    std::vector<VECTOR2I> pts;
    std::vector<VECTOR2I> connections = GetConnections();

    for( const EDA_ITEM* edaItem : aItems )
    {
        const SCH_ITEM* item = dynamic_cast<const SCH_ITEM*>( edaItem );

        if( !item || !item->IsConnectable() )
            continue;

        std::vector<VECTOR2I> new_pts = item->GetConnectionPoints();
        pts.insert( pts.end(), new_pts.begin(), new_pts.end() );

        // If the item is a line, we also add any connection points from the rest of the schematic
        // that terminate on the line after it is moved.
        if( item->Type() == SCH_LINE_T )
        {
            SCH_LINE* line = (SCH_LINE*) item;

            for( const VECTOR2I& pt : connections )
            {
                if( IsPointOnSegment( line->GetStartPoint(), line->GetEndPoint(), pt ) )
                    pts.push_back( pt );
            }
        }
    }

    // We always have some overlapping connection points.  Drop duplicates here
    std::sort( pts.begin(), pts.end(),
               []( const VECTOR2I& a, const VECTOR2I& b ) -> bool
               {
                   return a.x < b.x || ( a.x == b.x && a.y < b.y );
               } );

    pts.erase( unique( pts.begin(), pts.end() ), pts.end() );

    // We only want the needed junction points, remove all the others
    pts.erase( std::remove_if( pts.begin(), pts.end(),
                               [this]( const VECTOR2I& a ) -> bool
                               {
                                   return !IsExplicitJunctionNeeded( a );
                               } ),
               pts.end() );

    return pts;
}


SCH_LABEL_BASE* SCH_SCREEN::GetLabel( const VECTOR2I& aPosition, int aAccuracy ) const
{
    for( SCH_ITEM* item : Items().Overlapping( aPosition, aAccuracy ) )
    {
        switch( item->Type() )
        {
        case SCH_LABEL_T:
        case SCH_GLOBAL_LABEL_T:
        case SCH_HIER_LABEL_T:
        case SCH_DIRECTIVE_LABEL_T:
            if( item->HitTest( aPosition, aAccuracy ) )
                return static_cast<SCH_LABEL_BASE*>( item );

            break;

        default:
            ;
        }
    }

    return nullptr;
}


void SCH_SCREEN::AddLibSymbol( LIB_SYMBOL* aLibSymbol )
{
    wxCHECK( aLibSymbol, /* void */ );

    wxString libSymbolName = aLibSymbol->GetLibId().Format().wx_str();

    auto it = m_libSymbols.find( libSymbolName );

    if( it != m_libSymbols.end() )
    {
        delete it->second;
        m_libSymbols.erase( it );
    }

    m_libSymbols[libSymbolName] = aLibSymbol;
}


void SCH_SCREEN::FixupEmbeddedData()
{
    SCHEMATIC* schematic = Schematic();

    const std::vector<wxString>* embeddedFonts = schematic->GetEmbeddedFiles()->UpdateFontFiles();

    for( auto& [name, libSym] : m_libSymbols )
    {
        for( auto& [filename, embeddedFile] : libSym->EmbeddedFileMap() )
        {
            EMBEDDED_FILES::EMBEDDED_FILE* file = schematic->GetEmbeddedFile( filename );

            if( file )
            {
                embeddedFile->compressedEncodedData = file->compressedEncodedData;
                embeddedFile->decompressedData = file->decompressedData;
                embeddedFile->data_hash = file->data_hash;
                embeddedFile->is_valid = file->is_valid;
            }
        }

        libSym->RunOnChildren(
                [&]( SCH_ITEM* aChild )
                {
                    if( EDA_TEXT* textItem = dynamic_cast<EDA_TEXT*>( aChild ) )
                        textItem->ResolveFont( embeddedFonts );
                },
                RECURSE_MODE::NO_RECURSE );
    }

    std::vector<SCH_ITEM*> items_to_update;

    for( SCH_ITEM* item : Items() )
    {
        bool update = false;

        if( EDA_TEXT* textItem = dynamic_cast<EDA_TEXT*>( item ) )
            update |= textItem->ResolveFont( embeddedFonts );

        item->RunOnChildren(
                [&]( SCH_ITEM* aChild )
                {
                    if( EDA_TEXT* textItem = dynamic_cast<EDA_TEXT*>( aChild ) )
                        update |= textItem->ResolveFont( embeddedFonts );
                },
                RECURSE_MODE::NO_RECURSE );

        if( update )
            items_to_update.push_back( item );
    }

    for( SCH_ITEM* item : items_to_update )
        Update( item );
}


void SCH_SCREEN::AddBusAlias( std::shared_ptr<BUS_ALIAS> aAlias )
{
    if( SCHEMATIC* schematic = Schematic() )
        schematic->AddBusAlias( aAlias );
}


void SCH_SCREEN::SetLegacySymbolInstanceData()
{
    for( SCH_ITEM* item : Items().OfType( SCH_SYMBOL_T ) )
    {
        SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

        // Add missing value and footprint instance data for legacy schematics.
        for( const SCH_SYMBOL_INSTANCE& instance : symbol->GetInstances() )
        {
            symbol->AddHierarchicalReference( instance.m_Path, instance.m_Reference,
                                              instance.m_Unit );
        }
    }
}


void SCH_SCREEN::FixLegacyPowerSymbolMismatches()
{
    for( SCH_ITEM* item : Items().OfType( SCH_SYMBOL_T ) )
    {
        SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

        // Fix pre-8.0 legacy power symbols with invisible pins
        // that have mismatched pin names and value fields
        if( symbol->GetLibSymbolRef()
            && symbol->GetLibSymbolRef()->IsGlobalPower()
            && symbol->GetAllLibPins().size() > 0
            && symbol->GetAllLibPins()[0]->IsGlobalPower()
            && !symbol->GetAllLibPins()[0]->IsVisible() )
        {
            symbol->SetValueFieldText( symbol->GetAllLibPins()[0]->GetName() );
        }
    }
}


size_t SCH_SCREEN::getLibSymbolNameMatches( const SCH_SYMBOL& aSymbol,
                                            std::vector<wxString>& aMatches )
{
    wxString searchName = aSymbol.GetLibId().GetUniStringLibId();

    if( m_libSymbols.find( searchName ) != m_libSymbols.end() )
        aMatches.emplace_back( searchName );

    searchName = aSymbol.GetLibId().GetUniStringLibItemName() + wxS( "_" );

    long tmp;
    wxString suffix;

    for( auto& pair : m_libSymbols )
    {
        if( pair.first.StartsWith( searchName, &suffix ) && suffix.ToLong( &tmp ) )
            aMatches.emplace_back( pair.first );
    }

    return aMatches.size();
}


void SCH_SCREEN::PruneOrphanedSymbolInstances( const wxString& aProjectName,
                                               const SCH_SHEET_LIST& aValidSheetPaths )
{
    // The project name cannot be empty.  Projects older than 7.0 did not save project names
    // when saving instance data.  Running this algorithm with an empty project name would
    // clobber all instance data for projects other than the current one when a schematic
    // file is shared across multiple projects.  Because running the schematic editor in
    // stand alone mode can result in an empty project name, do not assert here.
    if( aProjectName.IsEmpty() )
        return;

    for( SCH_ITEM* item : Items().OfType( SCH_SYMBOL_T ) )
    {
        SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

        wxCHECK2( symbol, continue );

        std::set<KIID_PATH> pathsToPrune;
        const std::vector<SCH_SYMBOL_INSTANCE> instances = symbol->GetInstances();

        for( const SCH_SYMBOL_INSTANCE& instance : instances )
        {
            // Ignore instance paths from other projects.
            if( aProjectName != instance.m_ProjectName )
                continue;

            std::optional<SCH_SHEET_PATH> pathFound =
                    aValidSheetPaths.GetSheetPathByKIIDPath( instance.m_Path );

            // Check for paths that do not exist in the current project and paths that do
            // not contain the current symbol.
            if( !pathFound )
                pathsToPrune.emplace( instance.m_Path );
            else if( pathFound.value().LastScreen() != this )
                pathsToPrune.emplace( pathFound.value().Path() );
        }

        for( const KIID_PATH& sheetPath : pathsToPrune )
        {
            wxLogTrace( traceSchSheetPaths, wxS( "Pruning project '%s' symbol instance %s." ),
                        aProjectName, sheetPath.AsString() );
            symbol->RemoveInstance( sheetPath );
        }
    }
}


void SCH_SCREEN::PruneOrphanedSheetInstances( const wxString& aProjectName,
                                              const SCH_SHEET_LIST& aValidSheetPaths )
{
    // The project name cannot be empty.  Projects older than 7.0 did not save project names
    // when saving instance data.  Running this algorithm with an empty project name would
    // clobber all instance data for projects other than the current one when a schematic
    // file is shared across multiple projects.  Because running the schematic editor in
    // stand alone mode can result in an empty project name, do not assert here.
    if( aProjectName.IsEmpty() )
        return;

    for( SCH_ITEM* item : Items().OfType( SCH_SHEET_T ) )
    {
        SCH_SHEET* sheet = static_cast<SCH_SHEET*>( item );

        wxCHECK2( sheet, continue );

        std::set<KIID_PATH> pathsToPrune;
        const std::vector<SCH_SHEET_INSTANCE> instances = sheet->GetInstances();

        for( const SCH_SHEET_INSTANCE& instance : instances )
        {
            // Ignore instance paths from other projects.
            if( aProjectName != instance.m_ProjectName )
                continue;

            std::optional<SCH_SHEET_PATH> pathFound =
                    aValidSheetPaths.GetSheetPathByKIIDPath( instance.m_Path );

            // Check for paths that do not exist in the current project and paths that do
            // not contain the current symbol.
            if( !pathFound )
                pathsToPrune.emplace( instance.m_Path );
            else if( pathFound.value().LastScreen() != this )
                pathsToPrune.emplace( pathFound.value().Path() );
        }

        for( const KIID_PATH& sheetPath : pathsToPrune )
        {
            wxLogTrace( traceSchSheetPaths, wxS( "Pruning project '%s' sheet instance %s." ),
                        aProjectName, sheetPath.AsString() );
            sheet->RemoveInstance( sheetPath );
        }
    }
}


bool SCH_SCREEN::HasSymbolFieldNamesWithWhiteSpace() const
{
    wxString trimmedFieldName;

    for( const SCH_ITEM* item : Items().OfType( SCH_SYMBOL_T ) )
    {
        const SCH_SYMBOL* symbol = static_cast<const SCH_SYMBOL*>( item );

        wxCHECK2( symbol, continue );

        for( const SCH_FIELD& field : symbol->GetFields() )
        {
            trimmedFieldName = field.GetName();
            trimmedFieldName.Trim();
            trimmedFieldName.Trim( false );

            if( field.GetName() != trimmedFieldName )
                return true;
        }
    }

    return false;
}


std::set<wxString> SCH_SCREEN::GetSheetNames() const
{
    std::set<wxString> retv;

    for( SCH_ITEM* item : Items().OfType( SCH_SHEET_T ) )
    {
        SCH_SHEET* sheet = static_cast<SCH_SHEET*>( item );

        wxCHECK2( sheet, continue );

        retv.emplace( sheet->GetName() );
    }

    return retv;
}


bool SCH_SCREEN::HasInstanceDataFromOtherProjects() const
{
    wxCHECK( Schematic(), false );

    SCH_SHEET_LIST hierarchy = Schematic()->Hierarchy();

    for( const SCH_ITEM* item : Items().OfType( SCH_SYMBOL_T ) )
    {
        const SCH_SYMBOL* symbol = static_cast<const SCH_SYMBOL*>( item );

        const std::vector<SCH_SYMBOL_INSTANCE> symbolInstances = symbol->GetInstances();

        for( const SCH_SYMBOL_INSTANCE& instance : symbolInstances )
        {
            if( !hierarchy.HasPath( instance.m_Path ) )
                return true;
        }
    }

    return false;
}


wxString SCH_SCREEN::GroupsSanityCheck( bool repair )
{
    if( repair )
    {
        while( GroupsSanityCheckInternal( repair ) != wxEmptyString )
        {
        };

        return wxEmptyString;
    }
    return GroupsSanityCheckInternal( repair );
}


wxString SCH_SCREEN::GroupsSanityCheckInternal( bool repair )
{
    // Cycle detection
    //
    // Each group has at most one parent group.
    // So we start at group 0 and traverse the parent chain, marking groups seen along the way.
    // If we ever see a group that we've already marked, that's a cycle.
    // If we reach the end of the chain, we know all groups in that chain are not part of any cycle.
    //
    // Algorithm below is linear in the # of groups because each group is visited only once.
    // There may be extra time taken due to the container access calls and iterators.
    //
    // Groups we know are cycle free
    std::unordered_set<EDA_GROUP*> knownCycleFreeGroups;
    // Groups in the current chain we're exploring.
    std::unordered_set<EDA_GROUP*> currentChainGroups;
    // Groups we haven't checked yet.
    std::unordered_set<EDA_GROUP*> toCheckGroups;

    // Initialize set of groups and generators to check that could participate in a cycle.
    for( SCH_ITEM* item : Items().OfType( SCH_GROUP_T ) )
        toCheckGroups.insert( static_cast<SCH_GROUP*>( item ) );

    while( !toCheckGroups.empty() )
    {
        currentChainGroups.clear();
        EDA_GROUP* group = *toCheckGroups.begin();

        while( true )
        {
            if( currentChainGroups.find( group ) != currentChainGroups.end() )
            {
                if( repair )
                    Remove( static_cast<SCH_ITEM*>( group->AsEdaItem() ) );

                return "Cycle detected in group membership";
            }
            else if( knownCycleFreeGroups.find( group ) != knownCycleFreeGroups.end() )
            {
                // Parent is a group we know does not lead to a cycle
                break;
            }

            currentChainGroups.insert( group );
            // We haven't visited currIdx yet, so it must be in toCheckGroups
            toCheckGroups.erase( group );

            group = group->AsEdaItem()->GetParentGroup();

            if( !group )
            {
                // end of chain and no cycles found in this chain
                break;
            }
        }

        // No cycles found in chain, so add it to set of groups we know don't participate
        // in a cycle.
        knownCycleFreeGroups.insert( currentChainGroups.begin(), currentChainGroups.end() );
    }

    // Success
    return "";
}


bool SCH_SCREEN::InProjectPath() const
{
    wxCHECK( Schematic() && !m_fileName.IsEmpty(), false );

    wxFileName thisScreenFn( m_fileName );
    wxFileName thisProjectFn( Schematic()->Project().GetProjectFullName() );

    wxCHECK( thisProjectFn.IsAbsolute(), false );

    if( thisScreenFn.GetDirCount() < thisProjectFn.GetDirCount() )
        return false;

    while( thisProjectFn.GetDirCount() != thisScreenFn.GetDirCount() )
        thisScreenFn.RemoveLastDir();

    return thisScreenFn.GetPath() == thisProjectFn.GetPath();
}


std::set<wxString> SCH_SCREEN::GetVariantNames() const
{
    std::set<wxString> variantNames;

    for( const SCH_ITEM* item : Items().OfType( SCH_SYMBOL_T ) )
    {
        const SCH_SYMBOL* symbol = static_cast<const SCH_SYMBOL*>( item );

        wxCHECK2( symbol, continue );

        const std::vector<SCH_SYMBOL_INSTANCE> symbolInstances = symbol->GetInstances();

        for( const SCH_SYMBOL_INSTANCE& instance : symbolInstances )
        {
            for( const auto& [name, variant] : instance.m_Variants )
                variantNames.emplace( name );
        }
    }

    for( const SCH_ITEM* item : Items().OfType( SCH_SHEET_T ) )
    {
        const SCH_SHEET* sheet = static_cast<const SCH_SHEET*>( item );

        wxCHECK2( sheet, continue );

        const std::vector<SCH_SHEET_INSTANCE> sheetInstances = sheet->GetInstances();

        for( const SCH_SHEET_INSTANCE& instance : sheetInstances )
        {
            for( const auto& [name, variant] : instance.m_Variants )
                variantNames.emplace( name );
        }
    }

    return variantNames;
}


void SCH_SCREEN::DeleteVariant( const wxString& aVariantName, SCH_COMMIT* aCommit )
{
    wxCHECK( !aVariantName.IsEmpty(), /* void */ );

    for( SCH_ITEM* item : Items().OfType( SCH_SYMBOL_T ) )
    {
        SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

        wxCHECK2( symbol, continue );

        std::vector<SCH_SYMBOL_INSTANCE> symbolInstances = symbol->GetInstances();

        for( SCH_SYMBOL_INSTANCE& instance : symbolInstances )
        {
            if( instance.m_Variants.contains( aVariantName ) )
            {
                if( aCommit )
                    aCommit->Modify( item, this );

                symbol->DeleteVariant( instance.m_Path, aVariantName );
            }
        }
    }

    for( SCH_ITEM* item : Items().OfType( SCH_SHEET_T ) )
    {
        SCH_SHEET* sheet = static_cast<SCH_SHEET*>( item );

        wxCHECK2( sheet, continue );

        std::vector<SCH_SHEET_INSTANCE> sheetInstances = sheet->GetInstances();

        for( SCH_SHEET_INSTANCE& instance : sheetInstances )
        {
            if( instance.m_Variants.contains( aVariantName ) )
            {
                if( aCommit )
                    aCommit->Modify( item, this );

                sheet->DeleteVariant( instance.m_Path, aVariantName );
            }
        }
    }
}


void SCH_SCREEN::RenameVariant( const wxString& aOldName, const wxString& aNewName,
                                SCH_COMMIT* aCommit )
{
    wxCHECK( !aOldName.IsEmpty() && !aNewName.IsEmpty(), /* void */ );

    for( SCH_ITEM* item : Items().OfType( SCH_SYMBOL_T ) )
    {
        SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

        wxCHECK2( symbol, continue );

        std::vector<SCH_SYMBOL_INSTANCE> symbolInstances = symbol->GetInstances();

        for( SCH_SYMBOL_INSTANCE& instance : symbolInstances )
        {
            if( instance.m_Variants.contains( aOldName ) )
            {
                if( aCommit )
                    aCommit->Modify( item, this );

                symbol->RenameVariant( instance.m_Path, aOldName, aNewName );
            }
        }
    }

    for( SCH_ITEM* item : Items().OfType( SCH_SHEET_T ) )
    {
        SCH_SHEET* sheet = static_cast<SCH_SHEET*>( item );

        wxCHECK2( sheet, continue );

        std::vector<SCH_SHEET_INSTANCE> sheetInstances = sheet->GetInstances();

        for( SCH_SHEET_INSTANCE& instance : sheetInstances )
        {
            if( instance.m_Variants.contains( aOldName ) )
            {
                if( aCommit )
                    aCommit->Modify( item, this );

                sheet->RenameVariant( instance.m_Path, aOldName, aNewName );
            }
        }
    }
}


void SCH_SCREEN::CopyVariant( const wxString& aSourceVariant, const wxString& aNewVariant,
                              SCH_COMMIT* aCommit )
{
    wxCHECK( !aSourceVariant.IsEmpty() && !aNewVariant.IsEmpty(), /* void */ );

    for( SCH_ITEM* item : Items().OfType( SCH_SYMBOL_T ) )
    {
        SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

        wxCHECK2( symbol, continue );

        std::vector<SCH_SYMBOL_INSTANCE> symbolInstances = symbol->GetInstances();

        for( SCH_SYMBOL_INSTANCE& instance : symbolInstances )
        {
            if( instance.m_Variants.contains( aSourceVariant ) )
            {
                if( aCommit )
                    aCommit->Modify( item, this );

                symbol->CopyVariant( instance.m_Path, aSourceVariant, aNewVariant );
            }
        }
    }

    for( SCH_ITEM* item : Items().OfType( SCH_SHEET_T ) )
    {
        SCH_SHEET* sheet = static_cast<SCH_SHEET*>( item );

        wxCHECK2( sheet, continue );

        std::vector<SCH_SHEET_INSTANCE> sheetInstances = sheet->GetInstances();

        for( SCH_SHEET_INSTANCE& instance : sheetInstances )
        {
            if( instance.m_Variants.contains( aSourceVariant ) )
            {
                if( aCommit )
                    aCommit->Modify( item, this );

                sheet->CopyVariant( instance.m_Path, aSourceVariant, aNewVariant );
            }
        }
    }
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
    buildScreenList( aSheet );
}


SCH_SCREENS::~SCH_SCREENS()
{
}


SCH_SCREEN* SCH_SCREENS::GetFirst()
{
    m_index = 0;

    if( m_screens.size() > 0 )
        return m_screens[0];

    return nullptr;
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

    return nullptr;
}


SCH_SHEET* SCH_SCREENS::GetSheet( unsigned int aIndex ) const
{
    if( aIndex < m_sheets.size() )
        return m_sheets[ aIndex ];

    return nullptr;
}


void SCH_SCREENS::addScreenToList( SCH_SCREEN* aScreen, SCH_SHEET* aSheet )
{
    if( aScreen == nullptr )
        return;

    for( const SCH_SCREEN* screen : m_screens )
    {
        if( screen == aScreen )
            return;
    }

    m_screens.push_back( aScreen );
    m_sheets.push_back( aSheet );
}


void SCH_SCREENS::buildScreenList( SCH_SHEET* aSheet )
{
    if( aSheet && aSheet->Type() == SCH_SHEET_T )
    {
        SCH_SCREEN* screen = aSheet->GetScreen();

        if( !screen )
            return;

        addScreenToList( screen, aSheet );

        for( SCH_ITEM* item : screen->Items().OfType( SCH_SHEET_T ) )
            buildScreenList( static_cast<SCH_SHEET*>( item ) );
    }
}


void SCH_SCREENS::ClearAnnotationOfNewSheetPaths( SCH_SHEET_LIST& aInitialSheetPathList )
{
    SCH_SCREEN* first = GetFirst();

    if( !first )
        return;

    SCHEMATIC* sch = first->Schematic();

    wxCHECK_RET( sch, "Null schematic in SCH_SCREENS::ClearAnnotationOfNewSheetPaths" );

    // Clear the annotation for symbols inside new sheetpaths not already in aInitialSheetList
    SCH_SCREENS screensList( sch->Root() );     // The list of screens, shared by sheet paths
    screensList.BuildClientSheetPathList();     // build the shared by sheet paths, by screen

    // Search for new sheet paths, not existing in aInitialSheetPathList
    // and existing in sheetpathList
    for( SCH_SHEET_PATH& sheetpath : sch->Hierarchy() )
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
            curr_screen->ClearAnnotation( &sheetpath, false );
        }
    }
}


int SCH_SCREENS::ReplaceDuplicateTimeStamps()
{
    std::vector<SCH_ITEM*> items;
    int count = 0;

    auto timestamp_cmp = []( const EDA_ITEM* a, const EDA_ITEM* b ) -> bool
                         {
                             return a->m_Uuid < b->m_Uuid;
                         };

    std::set<EDA_ITEM*, decltype( timestamp_cmp )> unique_stamps( timestamp_cmp );

    // Collect ALL items from all screens to detect duplicate UUIDs.
    // This is essential for design blocks where multiple instances of the same content
    // are placed on the same sheet - each instance needs unique UUIDs for items like
    // wires, junctions, and groups, not just symbols and sheets.
    for( SCH_SCREEN* screen : m_screens )
    {
        for( SCH_ITEM* item : screen->Items() )
            items.push_back( item );
    }

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

            // @todo If the item is a sheet, we need to descend the hierarchy from the sheet
            //       and replace all instances of the changed UUID in sheet paths.  Otherwise,
            //       all instance paths with the sheet's UUID will get clobbered.
        }
    }

    return count;
}


void SCH_SCREENS::ClearEditFlags()
{
    for( SCH_SCREEN* screen = GetFirst(); screen; screen = GetNext() )
    {
        for( SCH_ITEM* item : screen->Items() )
            item->ClearEditFlags();
    }
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


void SCH_SCREENS::DeleteMarkers( enum MARKER_BASE::MARKER_T aMarkerType, int aErrorCode,
                                 bool aIncludeExclusions )
{
    for( SCH_SCREEN* screen = GetFirst(); screen; screen = GetNext() )
    {
        std::vector<SCH_ITEM*> markers;

        for( SCH_ITEM* item : screen->Items().OfType( SCH_MARKER_T ) )
        {
            SCH_MARKER* marker = static_cast<SCH_MARKER*>( item );
            std::shared_ptr<RC_ITEM>rcItem = marker->GetRCItem();

            if( marker->GetMarkerType() == aMarkerType
                    && ( aErrorCode == ERCE_UNSPECIFIED || rcItem->GetErrorCode() == aErrorCode )
                    && ( !marker->IsExcluded() || aIncludeExclusions ) )
            {
                markers.push_back( item );
            }
        }

        for( SCH_ITEM* marker : markers )
            screen->DeleteItem( marker );
    }
}


void SCH_SCREENS::DeleteAllMarkers( enum MARKER_BASE::MARKER_T aMarkerType,
                                    bool aIncludeExclusions )
{
    DeleteMarkers( aMarkerType, ERCE_UNSPECIFIED, aIncludeExclusions );
}


void SCH_SCREENS::UpdateSymbolLinks( REPORTER* aReporter )
{
    for( SCH_SCREEN* screen = GetFirst(); screen; screen = GetNext() )
        screen->UpdateSymbolLinks( aReporter );

    SCH_SCREEN* first = GetFirst();

    if( !first )
        return;

    SCHEMATIC* sch = first->Schematic();

    wxCHECK_RET( sch, "Null schematic in SCH_SCREENS::UpdateSymbolLinks" );

    SCH_SHEET_LIST sheets = sch->Hierarchy();

    // All of the library symbols have been replaced with copies so the connection graph
    // pointers are stale.
    if( sch->ConnectionGraph() )
        sch->ConnectionGraph()->Recalculate( sheets, true );
}


bool SCH_SCREENS::HasNoFullyDefinedLibIds()
{
    bool has_symbols = false;

    for( SCH_SCREEN* screen = GetFirst(); screen; screen = GetNext() )
    {
        for( SCH_ITEM* item : screen->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );
            has_symbols = true;

            if( !symbol->GetLibId().GetLibNickname().empty() )
                return false;
        }
    }

    // return true (i.e. has no fully defined symbol) only if at least one symbol is found
    return has_symbols ? true : false;
}


size_t SCH_SCREENS::GetLibNicknames( wxArrayString& aLibNicknames )
{
    for( SCH_SCREEN* screen = GetFirst(); screen; screen = GetNext() )
    {
        for( SCH_ITEM* item : screen->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* symbol   = static_cast<SCH_SYMBOL*>( item );
            const UTF8& nickname = symbol->GetLibId().GetLibNickname();

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
        for( SCH_ITEM* item : screen->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

            if( symbol->GetLibId().GetLibNickname().wx_str() != aFrom )
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
    SCH_SCREEN* first = GetFirst();

    if( !first )
        return;

    SCHEMATIC* sch = first->Schematic();

    wxCHECK_RET( sch, "Null schematic in SCH_SCREENS::BuildClientSheetPathList" );

    // Don't build until we have a hierarchy to work with.  This can be called before the hierarchy is built.
    if( !sch->HasHierarchy() )
        return;

    for( SCH_SCREEN* curr_screen = GetFirst(); curr_screen; curr_screen = GetNext() )
        curr_screen->GetClientSheetPaths().clear();

    for( SCH_SHEET_PATH& sheetpath : sch->Hierarchy() )
    {
        SCH_SCREEN* used_screen = sheetpath.LastScreen();

        // Search for the used_screen in list and add this unique sheet path:
        for( SCH_SCREEN* curr_screen = GetFirst(); curr_screen; curr_screen = GetNext() )
        {
            if( used_screen == curr_screen )
            {
                curr_screen->GetClientSheetPaths().push_back( sheetpath );
                break;
            }
        }
    }
}


void SCH_SCREENS::SetLegacySymbolInstanceData()
{
    for( SCH_SCREEN* screen = GetFirst(); screen; screen = GetNext() )
        screen->SetLegacySymbolInstanceData();
}


void SCH_SCREENS::FixLegacyPowerSymbolMismatches()
{
    for( SCH_SCREEN* screen = GetFirst(); screen; screen = GetNext() )
        screen->FixLegacyPowerSymbolMismatches();
}


void SCH_SCREEN::MigrateSimModels()
{
    LOCALE_IO toggle;

    // V6 schematics may specify model names in Value fields, which we don't do in V7.
    // Migrate by adding an equivalent model for these symbols.

    for( SCH_ITEM* item : Items().OfType( SCH_SYMBOL_T ) )
    {
        SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );
        SIM_MODEL::MigrateSimModel<SCH_SYMBOL>( *symbol, &Schematic()->Project() );
    }
}


void SCH_SCREENS::PruneOrphanedSymbolInstances( const wxString& aProjectName,
                                                const SCH_SHEET_LIST& aValidSheetPaths )
{
    if( aProjectName.IsEmpty() )
        return;

    for( SCH_SCREEN* screen = GetFirst(); screen; screen = GetNext() )
        screen->PruneOrphanedSymbolInstances( aProjectName, aValidSheetPaths );
}


void SCH_SCREENS::PruneOrphanedSheetInstances( const wxString& aProjectName,
                                               const SCH_SHEET_LIST& aValidSheetPaths )
{
    if( aProjectName.IsEmpty() )
        return;

    for( SCH_SCREEN* screen = GetFirst(); screen; screen = GetNext() )
        screen->PruneOrphanedSheetInstances( aProjectName, aValidSheetPaths );
}


bool SCH_SCREENS::HasSymbolFieldNamesWithWhiteSpace() const
{
    for( const SCH_SCREEN* screen : m_screens )
    {
        if( screen->HasSymbolFieldNamesWithWhiteSpace() )
            return true;
    }

    return false;
}


std::set<wxString> SCH_SCREENS::GetVariantNames() const
{
    std::set<wxString> variantNames;

    for( const SCH_SCREEN* screen : m_screens )
    {
        for( const wxString& variantName : screen->GetVariantNames() )
            variantNames.emplace( variantName );
    }

    return variantNames;
}


void SCH_SCREENS::DeleteVariant( const wxString& aVariantName, SCH_COMMIT* aCommit )
{
    wxCHECK( !aVariantName.IsEmpty(), /* void */ );

    for( SCH_SCREEN* screen : m_screens )
        screen->DeleteVariant( aVariantName, aCommit );
}


void SCH_SCREENS::RenameVariant( const wxString& aOldName, const wxString& aNewName,
                                 SCH_COMMIT* aCommit )
{
    wxCHECK( !aOldName.IsEmpty() && !aNewName.IsEmpty(), /* void */ );

    for( SCH_SCREEN* screen : m_screens )
        screen->RenameVariant( aOldName, aNewName, aCommit );
}


void SCH_SCREENS::CopyVariant( const wxString& aSourceVariant, const wxString& aNewVariant,
                               SCH_COMMIT* aCommit )
{
    wxCHECK( !aSourceVariant.IsEmpty() && !aNewVariant.IsEmpty(), /* void */ );

    for( SCH_SCREEN* screen : m_screens )
        screen->CopyVariant( aSourceVariant, aNewVariant, aCommit );
}
