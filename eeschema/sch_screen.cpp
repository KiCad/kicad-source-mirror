/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2008 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 1992-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <wx/filefn.h>

#include <eda_item.h>
#include <eda_rect.h>
#include <id.h>
#include <kicad_string.h>
#include <kiway.h>
#include <plotter.h>
#include <project.h>
#include <reporter.h>
#include <sch_draw_panel.h>
#include <sch_edit_frame.h>
#include <sch_item.h>

#include <class_library.h>
#include <connection_graph.h>
#include <lib_pin.h>
#include <sch_symbol.h>
#include <sch_junction.h>
#include <sch_line.h>
#include <sch_marker.h>
#include <sch_sheet.h>
#include <sch_sheet_pin.h>
#include <sch_text.h>
#include <schematic.h>
#include <symbol_lib_table.h>
#include <tool/common_tools.h>

#include <algorithm>

// TODO(JE) Debugging only
#include <profile.h>
#include "sch_bus_entry.h"

SCH_SCREEN::SCH_SCREEN( EDA_ITEM* aParent ) :
    BASE_SCREEN( aParent, SCH_SCREEN_T ),
    m_fileFormatVersionAtLoad( 0 ),
    m_paper( wxT( "A4" ) )
{
    m_modification_sync = 0;
    m_refCount = 0;
    m_zoomInitialized = false;
    m_LastZoomLevel = 1.0;

    // Suitable for schematic only. For symbol_editor and viewlib, must be set to true
    m_Center = false;

    InitDataPoints( m_paper.GetSizeIU() );
}


SCH_SCREEN::~SCH_SCREEN()
{
    clearLibSymbols();
    FreeDrawList();
}


SCHEMATIC* SCH_SCREEN::Schematic() const
{
    wxCHECK_MSG( GetParent() && GetParent()->Type() == SCHEMATIC_T, nullptr,
            "SCH_SCREEN must have a SCHEMATIC parent!" );

    return static_cast<SCHEMATIC*>( GetParent() );
}


void SCH_SCREEN::clearLibSymbols()
{
    for( auto libSymbol : m_libSymbols )
        delete libSymbol.second;

    m_libSymbols.clear();
}


void SCH_SCREEN::SetFileName( const wxString& aFileName )
{
    wxASSERT( aFileName.IsEmpty() || wxIsAbsolutePath( aFileName ) );

    m_fileName = aFileName;
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


bool SCH_SCREEN::HasItems( KICAD_T aItemType ) const
{
    EE_RTREE::EE_TYPE sheets = m_rtree.OfType( aItemType );

    return sheets.begin() != sheets.end();
}


bool SCH_SCREEN::ClassOf( const EDA_ITEM* aItem )
{
    return aItem && SCH_SCREEN_T == aItem->Type();
}


void SCH_SCREEN::Append( SCH_ITEM* aItem )
{
    if( aItem->Type() != SCH_SHEET_PIN_T && aItem->Type() != SCH_FIELD_T )
    {
        // Ensure the item can reach the SCHEMATIC through this screen
        aItem->SetParent( this );

        if( aItem->Type() == SCH_SYMBOL_T )
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
                        int cnt = 1;
                        wxString newName;

                        newName.Printf( "%s_%d", symbol->GetLibId().Format().wx_str(), cnt );

                        while( m_libSymbols.find( newName ) != m_libSymbols.end() )
                        {
                            cnt += 1;
                            newName.Printf( "%s_%d", symbol->GetLibId().Format().wx_str(), cnt );
                        }

                        symbol->SetSchSymbolLibraryName( newName );
                        m_libSymbols[newName] = new LIB_SYMBOL( *symbol->GetLibSymbolRef() );
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

    // No need to descend the hierarchy.  Once the top level screen is copied, all of it's
    // children are copied as well.
    for( auto aItem : aScreen->m_rtree )
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
    bool retv = m_rtree.remove( aItem );

    // Check if the library symbol for the removed schematic symbol is still required.
    if( retv && aItem->Type() == SCH_SYMBOL_T )
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


SCH_ITEM* SCH_SCREEN::GetItem( const wxPoint& aPosition, int aAccuracy, KICAD_T aType ) const
{
    EDA_RECT bbox;
    bbox.SetOrigin( aPosition );
    bbox.Inflate( aAccuracy );

    for( SCH_ITEM* item : Items().Overlapping( aType, bbox ) )
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

    wxCHECK_MSG( aSegment && aSegment->Type() == SCH_LINE_T, retval, wxT( "Invalid pointer." ) );

    to_search.push( aSegment );

    while( !to_search.empty() )
    {
        SCH_LINE* test_item = to_search.top();
        to_search.pop();

        for( SCH_ITEM* item : Items().Overlapping( SCH_JUNCTION_T, test_item->GetBoundingBox() ) )
        {
            if( test_item->IsEndPoint( item->GetPosition() ) )
                retval.insert( item );
        }

        for( SCH_ITEM* item : Items().Overlapping( SCH_LINE_T, test_item->GetBoundingBox() ) )
        {
            // Skip connecting lines on different layers (e.g. buses)
            if( test_item->GetLayer() != item->GetLayer() )
                continue;

            SCH_LINE* line = static_cast<SCH_LINE*>( item );

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


bool SCH_SCREEN::IsJunctionNeeded( const wxPoint& aPosition, bool aNew ) const
{
    enum layers { WIRES = 0, BUSES };

    bool                          breakLines[ 2 ] = { false };
    std::unordered_set<int>       exitAngles[ 2 ];
    std::vector<const SCH_LINE*>  midPointLines[ 2 ];

    // A pin at 90º still shouldn't match a line at 90º so just give pins unique numbers
    int                           uniqueAngle = 10000;

    for( const SCH_ITEM* item : Items().Overlapping( aPosition ) )
    {
        if( item->GetEditFlags() & STRUCT_DELETED )
            continue;

        switch( item->Type() )
        {
        case SCH_JUNCTION_T:
            if( aNew && item->HitTest( aPosition, -1 ) )
                return false;

            break;

        case SCH_LINE_T:
        {
            const SCH_LINE* line = static_cast<const SCH_LINE*>( item );
            int             layer;

            if( line->GetStartPoint() == line->GetEndPoint() )
                break;
            else if( line->GetLayer() == LAYER_WIRE )
                layer = WIRES;
            else if( line->GetLayer() == LAYER_BUS )
                layer = BUSES;
            else
                break;

            if( line->IsConnected( aPosition ) )
            {
                breakLines[ layer ] = true;
                exitAngles[ layer ].insert( line->GetAngleFrom( aPosition ) );
            }
            else if( line->HitTest( aPosition, -1 ) )
            {
                // Defer any line midpoints until we know whether or not we're breaking them
                midPointLines[ layer ].push_back( line );
            }
        }
            break;

        case SCH_BUS_WIRE_ENTRY_T:
        case SCH_SYMBOL_T:
        case SCH_SHEET_T:
            if( item->IsConnected( aPosition ) )
            {
                breakLines[ WIRES ] = true;
                exitAngles[ WIRES ].insert( uniqueAngle++ );
            }

            break;

        default:
            break;
        }
    }

    for( int layer : { WIRES, BUSES } )
    {
        if( breakLines[ layer ] )
        {
            for( const SCH_LINE* line : midPointLines[ layer ] )
            {
                exitAngles[ layer ].insert( line->GetAngleFrom( aPosition ) );
                exitAngles[ layer ].insert( line->GetReverseAngleFrom( aPosition ) );
            }
        }
    }

    return exitAngles[ WIRES ].size() >= 3 || exitAngles[ BUSES ].size() >= 3;
}


bool SCH_SCREEN::IsTerminalPoint( const wxPoint& aPosition, int aLayer ) const
{
    wxCHECK_MSG( aLayer == LAYER_NOTES || aLayer == LAYER_BUS || aLayer == LAYER_WIRE, false,
                 wxT( "Invalid layer type passed to SCH_SCREEN::IsTerminalPoint()." ) );

    switch( aLayer )
    {
    case LAYER_BUS:
    {
        if( GetBus( aPosition ) )
            return true;

        SCH_SHEET_PIN* sheetPin = GetSheetPin( aPosition );

        if( sheetPin && sheetPin->IsConnected( aPosition ) )
            return true;

        SCH_TEXT* label = GetLabel( aPosition );

        if( label && label->IsConnected( aPosition ) )
            return true;
    }
        break;

    case LAYER_NOTES:
    {
        if( GetLine( aPosition ) )
            return true;
    }
        break;

    case LAYER_WIRE:
    {
        if( GetItem( aPosition, 1, SCH_BUS_WIRE_ENTRY_T) )
            return true;

        if( GetItem( aPosition, 1, SCH_JUNCTION_T ) )
            return true;

        if( GetPin( aPosition, NULL, true ) )
            return true;

        if( GetWire( aPosition ) )
            return true;

        SCH_TEXT* label = GetLabel( aPosition, 1 );

        if( label && label->IsConnected( aPosition ) )
            return true;

        SCH_SHEET_PIN* sheetPin = GetSheetPin( aPosition );

        if( sheetPin && sheetPin->IsConnected( aPosition ) )
            return true;
    }
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
    std::unique_ptr< LIB_SYMBOL > libSymbol;
    std::vector<SCH_SYMBOL*> symbols;
    SYMBOL_LIB_TABLE* libs = Schematic()->Prj().SchSymbolLibTable();

    // This will be a nullptr if an s-expression schematic is loaded.
    SYMBOL_LIBS* legacyLibs = Schematic()->Prj().SchLibs();

    for( auto item : Items().OfType( SCH_SYMBOL_T ) )
        symbols.push_back( static_cast<SCH_SYMBOL*>( item ) );

    // Remove them from the R tree.  There bounding box size may change.
    for( auto symbol : symbols )
        Remove( symbol );

    // Clear all existing symbol links.
    clearLibSymbols();

    for( auto symbol : symbols )
    {
        LIB_SYMBOL* tmp = nullptr;
        libSymbol.reset();

        // If the symbol is already in the internal library, map the symbol to it.
        auto it = m_libSymbols.find( symbol->GetSchSymbolLibraryName() );

        if( ( it != m_libSymbols.end() ) )
        {
            if( aReporter )
            {
                msg.Printf( _( "Setting schematic symbol '%s %s' library identifier "
                               "to '%s'. " ),
                            symbol->GetField( REFERENCE_FIELD )->GetText(),
                            symbol->GetField( VALUE_FIELD )->GetText(),
                            symbol->GetLibId().Format().wx_str() );
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
                msg.Printf( _( "Schematic symbol reference '%s' library identifier is not "
                               "valid.  Unable to link library symbol." ),
                            symbol->GetLibId().Format().wx_str() );
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
                msg.Printf( _( "Symbol library '%s' not found and no fallback cache "
                               "library available.  Unable to link library symbol." ),
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
                                symbol->GetLibId().Format().wx_str() );
                    aReporter->ReportTail( msg, RPT_SEVERITY_ERROR );
                }
            }
        }

        if( !tmp && legacyLibs && legacyLibs->GetLibraryCount() )
        {
            SYMBOL_LIB& legacyCacheLib = legacyLibs->at( 0 );

            // It better be the cache library.
            wxCHECK2( legacyCacheLib.IsCache(), continue );

            wxString id = symbol->GetLibId().Format();

            id.Replace( ':', '_' );

            if( aReporter )
            {
                msg.Printf( _( "Falling back to cache to set symbol '%s:%s' link '%s'." ),
                            symbol->GetField( REFERENCE_FIELD )->GetText(),
                            symbol->GetField( VALUE_FIELD )->GetText(),
                            id );
                aReporter->ReportTail( msg, RPT_SEVERITY_WARNING );
            }

            tmp = legacyCacheLib.FindSymbol( id );
        }

        if( tmp )
        {
            // We want a full symbol not just the top level child symbol.
            libSymbol = tmp->Flatten();
            libSymbol->SetParent();

            m_libSymbols.insert( { symbol->GetSchSymbolLibraryName(),
                                   new LIB_SYMBOL( *libSymbol.get() ) } );

            if( aReporter )
            {
                msg.Printf( _( "Setting schematic symbol '%s %s' library identifier to '%s'." ),
                            symbol->GetField( REFERENCE_FIELD )->GetText(),
                            symbol->GetField( VALUE_FIELD )->GetText(),
                            symbol->GetLibId().Format().wx_str() );
                aReporter->ReportTail( msg, RPT_SEVERITY_INFO );
            }
        }
        else
        {
            if( aReporter )
            {
                msg.Printf( _( "No library symbol found for schematic symbol '%s %s'." ),
                            symbol->GetField( REFERENCE_FIELD )->GetText(),
                            symbol->GetField( VALUE_FIELD )->GetText() );
                aReporter->ReportTail( msg, RPT_SEVERITY_ERROR );
            }
        }

        symbol->SetLibSymbol( libSymbol.release() );
    }

    // Changing the symbol may adjust the bbox of the symbol.  This re-inserts the
    // item with the new bbox
    for( auto symbol : symbols )
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

        LIB_SYMBOL* libSymbol = nullptr;

        if( it != m_libSymbols.end() )
            libSymbol = new LIB_SYMBOL( *it->second );

        symbol->SetLibSymbol( libSymbol );

        m_rtree.insert( symbol );
    }
}


void SCH_SCREEN::SwapSymbolLinks( const SCH_SYMBOL* aOriginalSymbol, const SCH_SYMBOL* aNewSymbol )
{
    wxCHECK( aOriginalSymbol && aNewSymbol /* && m_rtree.contains( aOriginalSymbol, true ) */,
             /* void */ );

    if( aOriginalSymbol->GetSchSymbolLibraryName() == aNewSymbol->GetSchSymbolLibraryName() )
        return;
}


void SCH_SCREEN::Print( const RENDER_SETTINGS* aSettings )
{
    // Ensure links are up to date, even if a library was reloaded for some reason:
    std::vector<SCH_ITEM*> junctions;
    std::vector<SCH_ITEM*> bitmaps;
    std::vector<SCH_ITEM*> other;

    for( SCH_ITEM* item : Items() )
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

    for( SCH_ITEM* item : bitmaps )
        item->Print( aSettings, wxPoint( 0, 0 ) );

    for( SCH_ITEM* item : other )
        item->Print( aSettings, wxPoint( 0, 0 ) );

    for( SCH_ITEM* item : junctions )
        item->Print( aSettings, wxPoint( 0, 0 ) );
}


void SCH_SCREEN::Plot( PLOTTER* aPlotter ) const
{
    // Ensure links are up to date, even if a library was reloaded for some reason:
    std::vector< SCH_ITEM* > junctions;
    std::vector< SCH_ITEM* > bitmaps;
    std::vector< SCH_ITEM* > other;

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

    int defaultPenWidth = aPlotter->RenderSettings()->GetDefaultPenWidth();

    // Bitmaps are drawn first to ensure they are in the background
    // This is particularly important for the wxPostscriptDC (used in *nix printers) as
    // the bitmap PS command clears the screen
    for( const SCH_ITEM* item : bitmaps )
    {
        aPlotter->SetCurrentLineWidth( std::max( item->GetPenWidth(), defaultPenWidth ) );
        item->Plot( aPlotter );
    }

    for( const SCH_ITEM* item : other )
    {
        aPlotter->SetCurrentLineWidth( std::max( item->GetPenWidth(), defaultPenWidth ) );
        item->Plot( aPlotter );
    }

    for( const SCH_ITEM* item : junctions )
    {
        aPlotter->SetCurrentLineWidth( std::max( item->GetPenWidth(), defaultPenWidth ) );
        item->Plot( aPlotter );
    }
}


void SCH_SCREEN::ClearDrawingState()
{
    for( SCH_ITEM* item : Items() )
        item->ClearTempFlags();
}


LIB_PIN* SCH_SCREEN::GetPin( const wxPoint& aPosition, SCH_SYMBOL** aSymbol,
                             bool aEndPointOnly ) const
{
    SCH_SYMBOL*  candidate = NULL;
    LIB_PIN*     pin = NULL;

    for( SCH_ITEM* item : Items().Overlapping( SCH_SYMBOL_T, aPosition ) )
    {
        candidate = static_cast<SCH_SYMBOL*>( item );

        if( aEndPointOnly )
        {
            pin = NULL;

            if( !candidate->GetLibSymbolRef() )
                continue;

            for( pin = candidate->GetLibSymbolRef()->GetNextPin(); pin;
                 pin = candidate->GetLibSymbolRef()->GetNextPin( pin ) )
            {
                // Skip items not used for this part.
                if( candidate->GetUnit() && pin->GetUnit() &&
                    ( pin->GetUnit() != candidate->GetUnit() ) )
                    continue;

                if( candidate->GetConvert() && pin->GetConvert() &&
                    ( pin->GetConvert() != candidate->GetConvert() ) )
                    continue;

                if( candidate->GetPinPhysicalPosition( pin ) == aPosition )
                    break;
            }

            if( pin )
                break;
        }
        else
        {
            pin = (LIB_PIN*) candidate->GetDrawItem( aPosition, LIB_PIN_T );

            if( pin )
                break;
        }
    }

    if( pin && aSymbol )
        *aSymbol = candidate;

    return pin;
}


SCH_SHEET_PIN* SCH_SCREEN::GetSheetPin( const wxPoint& aPosition ) const
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


size_t SCH_SCREEN::CountConnectedItems( const wxPoint& aPos, bool aTestJunctions ) const
{
    size_t count = 0;

    for( const SCH_ITEM* item : Items() )
    {
        if( ( item->Type() != SCH_JUNCTION_T || aTestJunctions ) && item->IsConnected( aPos ) )
            count++;
    }

    return count;
}


void SCH_SCREEN::ClearAnnotation( SCH_SHEET_PATH* aSheetPath )
{

    for( SCH_ITEM* item : Items().OfType( SCH_SYMBOL_T ) )
    {
        SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

        symbol->ClearAnnotation( aSheetPath );
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
    static KICAD_T hierarchicalTypes[] = { SCH_SYMBOL_T, SCH_SHEET_T, SCH_GLOBAL_LABEL_T, EOT };

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
                    return a->GetPosition().y < b->GetPosition().y;
                else
                    return a->GetPosition().x < b->GetPosition().x;
            } );
}


void SCH_SCREEN::TestDanglingEnds( const SCH_SHEET_PATH* aPath,
                                   std::function<void( SCH_ITEM* )>* aChangedHandler ) const
{
    std::vector<DANGLING_END_ITEM> endPoints;

    for( SCH_ITEM* item : Items() )
        item->GetEndPoints( endPoints );

    for( SCH_ITEM* item : Items() )
    {
        if( item->UpdateDanglingState( endPoints, aPath ) )
        {
            if( aChangedHandler )
                (*aChangedHandler)( item );
        }
    }
}


SCH_LINE* SCH_SCREEN::GetLine( const wxPoint& aPosition, int aAccuracy, int aLayer,
                               SCH_LINE_TEST_T aSearchType ) const
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


SCH_TEXT* SCH_SCREEN::GetLabel( const wxPoint& aPosition, int aAccuracy ) const
{
    for( SCH_ITEM* item : Items().Overlapping( aPosition, aAccuracy ) )
    {
        switch( item->Type() )
        {
        case SCH_LABEL_T:
        case SCH_GLOBAL_LABEL_T:
        case SCH_HIER_LABEL_T:
            if( item->HitTest( aPosition, aAccuracy ) )
                return static_cast<SCH_TEXT*>( item );

            break;

        default:
            ;
        }
    }

    return NULL;
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


void SCH_SCREEN::AddBusAlias( std::shared_ptr<BUS_ALIAS> aAlias )
{
    m_aliases.insert( aAlias );
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


SCH_SHEET* SCH_SCREENS::GetSheet( unsigned int aIndex ) const
{
    if( aIndex < m_sheets.size() )
        return m_sheets[ aIndex ];

    return NULL;
}


void SCH_SCREENS::addScreenToList( SCH_SCREEN* aScreen, SCH_SHEET* aSheet )
{
    if( aScreen == NULL )
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
    for( SCH_SHEET_PATH& sheetpath : sch->GetSheets() )
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
    std::vector<SCH_ITEM*> items;
    int count = 0;

    auto timestamp_cmp = []( const EDA_ITEM* a, const EDA_ITEM* b ) -> bool
                         {
                             return a->m_Uuid < b->m_Uuid;
                         };

    std::set<EDA_ITEM*, decltype( timestamp_cmp )> unique_stamps( timestamp_cmp );

    for( SCH_SCREEN* screen : m_screens )
        screen->GetHierarchicalItems( &items );

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


void SCH_SCREENS::DeleteMarkers( enum MARKER_BASE::TYPEMARKER aMarkerType, int aErrorCode,
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


void SCH_SCREENS::DeleteAllMarkers( enum MARKER_BASE::TYPEMARKER aMarkerType,
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

    SCH_SHEET_LIST sheets = sch->GetSheets();

    // All of the library symbols have been replaced with copies so the connection graph
    // pointers are stale.
    if( sch->ConnectionGraph() )
        sch->ConnectionGraph()->Recalculate( sheets, true );
}


bool SCH_SCREENS::HasNoFullyDefinedLibIds()
{
    SCH_SCREEN* screen;
    unsigned cnt = 0;

    for( screen = GetFirst(); screen; screen = GetNext() )
    {
        for( auto item : screen->Items().OfType( SCH_SYMBOL_T ) )
        {
            cnt++;
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );

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
        for( auto item : screen->Items().OfType( SCH_SYMBOL_T ) )
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
        for( auto item : screen->Items().OfType( SCH_SYMBOL_T ) )
        {
            auto symbol = static_cast<SCH_SYMBOL*>( item );

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
    SCH_SCREEN* first = GetFirst();

    if( !first )
        return;

    SCHEMATIC* sch = first->Schematic();

    wxCHECK_RET( sch, "Null schematic in SCH_SCREENS::BuildClientSheetPathList" );

    for( SCH_SCREEN* curr_screen = GetFirst(); curr_screen; curr_screen = GetNext() )
        curr_screen->GetClientSheetPaths().clear();

    for( SCH_SHEET_PATH& sheetpath : sch->GetSheets() )
    {
        SCH_SCREEN* used_screen = sheetpath.LastScreen();

        // SEarch for the used_screen in list and add this unique sheet path:
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
