/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Chris Pavlina <pavlina.chris@gmail.com>
 * Copyright (C) 2014 Henner Zeller <h.zeller@acm.org>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "symbol_tree_model_adapter.h"

#include <wx/log.h>
#include <wx/tokenzr.h>
#include <wx/window.h>
#include <core/kicad_algo.h>
#include <pgm_base.h>
#include <project/project_file.h>
#include <widgets/wx_progress_reporters.h>
#include <dialogs/html_message_box.h>
#include <generate_alias_info.h>
#include <sch_base_frame.h>
#include <locale_io.h>
#include <string_utils.h>
#include <trace_helpers.h>
#include <libraries/symbol_library_adapter.h>

bool SYMBOL_TREE_MODEL_ADAPTER::m_show_progress = true;

#define PROGRESS_INTERVAL_MILLIS 33      // 30 FPS refresh rate


wxObjectDataPtr<LIB_TREE_MODEL_ADAPTER>
SYMBOL_TREE_MODEL_ADAPTER::Create( SCH_BASE_FRAME* aParent,
                                   SYMBOL_LIBRARY_ADAPTER* aManager )
{
    auto* adapter = new SYMBOL_TREE_MODEL_ADAPTER( aParent, aManager );
    return wxObjectDataPtr<LIB_TREE_MODEL_ADAPTER>( adapter );
}


SYMBOL_TREE_MODEL_ADAPTER::SYMBOL_TREE_MODEL_ADAPTER( SCH_BASE_FRAME* aParent,
                                                      SYMBOL_LIBRARY_ADAPTER* aLibs ) :
        LIB_TREE_MODEL_ADAPTER( aParent, "pinned_symbol_libs",
                                aParent->GetViewerSettingsBase()->m_LibTree ),
        m_adapter( aLibs ),
        m_check_pending_libraries_timer( nullptr )
{
    m_availableColumns.emplace_back( GetDefaultFieldName( FIELD_T::VALUE, false ) );
    m_availableColumns.emplace_back( GetDefaultFieldName( FIELD_T::FOOTPRINT, false ) );

    // Description is always shown
    //m_availableColumns.emplace_back( GetDefaultFieldName( FIELD_T::DESCRIPTION, false ) );

    // Datasheet probably isn't useful, but better to leave that decision to the user:
    m_availableColumns.emplace_back( GetDefaultFieldName( FIELD_T::DATASHEET, false ) );
}


SYMBOL_TREE_MODEL_ADAPTER::~SYMBOL_TREE_MODEL_ADAPTER()
{}


void SYMBOL_TREE_MODEL_ADAPTER::AddLibraries( SCH_BASE_FRAME* aFrame )
{
    COMMON_SETTINGS* cfg = Pgm().GetCommonSettings();
    PROJECT_FILE&    project = aFrame->Prj().GetProjectFile();

    auto addFunc =
            [&]( const wxString& aLibName, const std::vector<LIB_SYMBOL*>& aSymbolList,
                 const wxString& aDescription )
            {
                std::vector<LIB_TREE_ITEM*> treeItems( aSymbolList.begin(), aSymbolList.end() );
                bool pinned = alg::contains( cfg->m_Session.pinned_symbol_libs, aLibName )
                              || alg::contains( project.m_PinnedSymbolLibs, aLibName );

                DoAddLibrary( aLibName, aDescription, treeItems, pinned, false );
            };

    LIBRARY_MANAGER& manager = Pgm().GetLibraryManager();

    std::vector<wxString> toLoad;

    std::ranges::copy( m_pending_load_libraries, std::back_inserter( toLoad ) );

    bool isLazyLoad = !m_pending_load_libraries.empty();
    m_pending_load_libraries.clear();

    if( toLoad.empty() )
    {
        for( const LIBRARY_TABLE_ROW* row : manager.Rows( LIBRARY_TABLE_TYPE::SYMBOL ) )
        {
            if( row->Hidden() )
                continue;

            toLoad.emplace_back( row->Nickname() );
        }
    }

    for( const wxString& lib : toLoad )
    {
        if( !m_adapter->GetLibraryStatus( lib ) )
        {
            m_pending_load_libraries.insert( lib );
            continue;
        }

        std::optional<const LIBRARY_TABLE_ROW*> rowResult =
                manager.GetRow( LIBRARY_TABLE_TYPE::SYMBOL, lib );

        wxCHECK2( rowResult, continue );

        wxString libDescription = ( *rowResult )->Description();

        std::vector<LIB_SYMBOL*> libSymbols = m_adapter->GetSymbols( lib );

        for( const wxString& column : m_adapter->GetAvailableExtraFields( lib ) )
            addColumnIfNecessary( column );

        if( m_adapter->SupportsSubLibraries( lib ) )
        {
            for( const auto& [nickname, description] : m_adapter->GetSubLibraries( lib ) )
            {
                wxString suffix = lib.IsEmpty() ? wxString( wxT( "" ) )
                                                : wxString::Format( wxT( " - %s" ), nickname );
                wxString name = wxString::Format( wxT( "%s%s" ), lib, suffix );
                wxString desc = description;

                if( !libDescription.IsEmpty() )
                {
                    desc = wxString::Format( wxT( "%s (%s)" ),
                                             libDescription,
                                             desc.IsEmpty() ? lib : desc );
                }

                UTF8 utf8Lib( nickname );

                std::vector<LIB_SYMBOL*> symbols;

                std::copy_if( libSymbols.begin(), libSymbols.end(),
                              std::back_inserter( symbols ),
                              [&utf8Lib]( LIB_SYMBOL* aSym )
                              {
                                  return utf8Lib == aSym->GetLibId().GetSubLibraryName();
                              } );

                addFunc( name, symbols, desc );
            }
        }
        else
        {
            addFunc( lib, libSymbols, libDescription );
        }
    }

    if( !m_pending_load_libraries.empty() && !m_check_pending_libraries_timer )
    {
        m_check_pending_libraries_timer = std::make_unique<wxTimer>( aFrame );

        wxLogTrace( traceLibraries, "%zu pending libraries, starting lazy load...",
                    m_pending_load_libraries.size() );

        aFrame->Bind( wxEVT_TIMER,
                [&, aFrame]( wxTimerEvent& )
                {
                    AddLibraries( aFrame );

                    if( m_pending_load_libraries.empty() )
                    {
                        m_check_pending_libraries_timer->Stop();
                        m_check_pending_libraries_timer.reset();
                        wxLogTrace( traceLibraries, "Done lazy-loading libraries" );
                    }
                },
                m_check_pending_libraries_timer->GetId() );

        m_check_pending_libraries_timer->Start( 1000 );
    }

    m_tree.AssignIntrinsicRanks( m_shownColumns );

    if( isLazyLoad && m_lazyLoadHandler )
    {
        createMissingColumns();
        m_lazyLoadHandler();
    }
}


void SYMBOL_TREE_MODEL_ADAPTER::AddLibrary( wxString const& aLibNickname, bool pinned )
{
    SYMBOL_LIBRARY_ADAPTER::SYMBOL_TYPE type =
            ( GetFilter() != nullptr ) ? SYMBOL_LIBRARY_ADAPTER::SYMBOL_TYPE::POWER_ONLY
                                       : SYMBOL_LIBRARY_ADAPTER::SYMBOL_TYPE::ALL_SYMBOLS;
    std::vector<LIB_SYMBOL*>    symbols = m_adapter->GetSymbols( aLibNickname, type );
    std::vector<LIB_TREE_ITEM*> comp_list;

    std::optional<const LIBRARY_TABLE_ROW*> row =
            Pgm().GetLibraryManager().GetRow( LIBRARY_TABLE_TYPE::SYMBOL, aLibNickname );

    if( row && symbols.size() > 0 )
    {
        comp_list.assign( symbols.begin(), symbols.end() );
        DoAddLibrary( aLibNickname, ( *row )->Description(), comp_list, pinned, false );
    }
}


wxString SYMBOL_TREE_MODEL_ADAPTER::GenerateInfo( LIB_ID const& aLibId, int aUnit )
{
    return GenerateAliasInfo( m_adapter, aLibId, aUnit );
}
