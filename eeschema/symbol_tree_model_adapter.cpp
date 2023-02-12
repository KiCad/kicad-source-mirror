/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Chris Pavlina <pavlina.chris@gmail.com>
 * Copyright (C) 2014 Henner Zeller <h.zeller@acm.org>
 * Copyright (C) 2014-2022 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <wx/log.h>
#include <wx/tokenzr.h>
#include <wx/window.h>
#include <core/kicad_algo.h>
#include <pgm_base.h>
#include <project/project_file.h>
#include <widgets/wx_progress_reporters.h>
#include <dialogs/html_message_box.h>
#include <eda_pattern_match.h>
#include <generate_alias_info.h>
#include <sch_base_frame.h>
#include <locale_io.h>
#include <lib_symbol.h>
#include <symbol_async_loader.h>
#include <symbol_lib_table.h>
#include <symbol_tree_model_adapter.h>
#include <string_utils.h>

bool SYMBOL_TREE_MODEL_ADAPTER::m_show_progress = true;

#define PROGRESS_INTERVAL_MILLIS 33      // 30 FPS refresh rate


wxObjectDataPtr<LIB_TREE_MODEL_ADAPTER>
SYMBOL_TREE_MODEL_ADAPTER::Create( EDA_BASE_FRAME* aParent, LIB_TABLE* aLibs )
{
    auto* adapter = new SYMBOL_TREE_MODEL_ADAPTER( aParent, aLibs );
    return wxObjectDataPtr<LIB_TREE_MODEL_ADAPTER>( adapter );
}


SYMBOL_TREE_MODEL_ADAPTER::SYMBOL_TREE_MODEL_ADAPTER( EDA_BASE_FRAME* aParent, LIB_TABLE* aLibs ) :
        LIB_TREE_MODEL_ADAPTER( aParent, "pinned_symbol_libs" ),
        m_libs( (SYMBOL_LIB_TABLE*) aLibs )
{
    // Symbols may have different value from name
    m_availableColumns.emplace_back( wxT( "Value" ) );
}


SYMBOL_TREE_MODEL_ADAPTER::~SYMBOL_TREE_MODEL_ADAPTER()
{}


bool SYMBOL_TREE_MODEL_ADAPTER::AddLibraries( const std::vector<wxString>& aNicknames,
                                              SCH_BASE_FRAME* aFrame )
{
    std::unique_ptr<WX_PROGRESS_REPORTER> progressReporter = nullptr;

    if( m_show_progress )
    {
        progressReporter = std::make_unique<WX_PROGRESS_REPORTER>( aFrame,
                                                                   _( "Loading Symbol Libraries" ),
                                                                   aNicknames.size(), true );
    }

    // Disable KIID generation: not needed for library parts; sometimes very slow
    KIID::CreateNilUuids( true );

    std::unordered_map<wxString, std::vector<LIB_SYMBOL*>> loadedSymbols;

    SYMBOL_ASYNC_LOADER loader( aNicknames, m_libs,
                                GetFilter() == LIB_TREE_MODEL_ADAPTER::SYM_FILTER_POWER,
                                &loadedSymbols, progressReporter.get() );

    LOCALE_IO toggle;

    loader.Start();

    while( !loader.Done() )
    {
        if( progressReporter && !progressReporter->KeepRefreshing() )
            break;

        wxMilliSleep( PROGRESS_INTERVAL_MILLIS );
    }

    loader.Join();

    bool cancelled = false;

    if( progressReporter )
        cancelled = progressReporter->IsCancelled();

    if( !loader.GetErrors().IsEmpty() )
    {
        HTML_MESSAGE_BOX dlg( aFrame, _( "Load Error" ) );

        dlg.MessageSet( _( "Errors loading symbols:" ) );

        wxString msg = loader.GetErrors();
        msg.Replace( "\n", "<BR>" );

        dlg.AddHTML_Text( msg );
        dlg.ShowModal();
    }

    if( loadedSymbols.size() > 0 )
    {
        COMMON_SETTINGS* cfg = Pgm().GetCommonSettings();
        PROJECT_FILE&    project = aFrame->Prj().GetProjectFile();

        auto addFunc =
                [&]( const wxString& aLibName, std::vector<LIB_SYMBOL*> aSymbolList,
                     const wxString& aDescription )
                {
                    std::vector<LIB_TREE_ITEM*> treeItems( aSymbolList.begin(), aSymbolList.end() );
                    bool pinned = alg::contains( cfg->m_Session.pinned_symbol_libs, aLibName )
                                  || alg::contains( project.m_PinnedSymbolLibs, aLibName );

                    DoAddLibrary( aLibName, aDescription, treeItems, pinned, false );
                };

        for( const std::pair<const wxString, std::vector<LIB_SYMBOL*>>& pair : loadedSymbols )
        {
            SYMBOL_LIB_TABLE_ROW* row = m_libs->FindRow( pair.first );

            wxCHECK2( row, continue );

            if( !row->GetIsVisible() )
                continue;

            std::vector<wxString> additionalColumns;
            row->GetAvailableSymbolFields( additionalColumns );

            for( const wxString& column : additionalColumns )
                addColumnIfNecessary( column );

            if( row->SupportsSubLibraries() )
            {
                std::vector<wxString> subLibraries;
                row->GetSubLibraryNames( subLibraries );

                wxString parentDesc = m_libs->GetDescription( pair.first );

                for( const wxString& lib : subLibraries )
                {
                    wxString suffix = lib.IsEmpty() ? wxString( wxT( "" ) )
                                                    : wxString::Format( wxT( " - %s" ), lib );
                    wxString name = wxString::Format( wxT( "%s%s" ), pair.first, suffix );
                    wxString desc;

                    if( !parentDesc.IsEmpty() )
                        desc = wxString::Format( wxT( "%s (%s)" ), parentDesc, lib );

                    std::vector<LIB_SYMBOL*> symbols;

                    std::copy_if( pair.second.begin(), pair.second.end(),
                                  std::back_inserter( symbols ),
                                  [&lib]( LIB_SYMBOL* aSym )
                                  {
                                      return lib.IsSameAs( aSym->GetLibId().GetSubLibraryName() );
                                  } );

                    addFunc( name, symbols, desc );
                }
            }
            else
            {
                addFunc( pair.first, pair.second, m_libs->GetDescription( pair.first ) );
            }
        }
    }

    KIID::CreateNilUuids( false );

    m_tree.AssignIntrinsicRanks();

    if( progressReporter )
    {
        // Force immediate deletion of the APP_PROGRESS_DIALOG.  Do not use Destroy(), or Destroy()
        // followed by wxSafeYield() because on Windows, APP_PROGRESS_DIALOG has some side effects
        // on the event loop manager.
        // One in particular is the call of ShowModal() following SYMBOL_TREE_MODEL_ADAPTER
        // creating a APP_PROGRESS_DIALOG (which has incorrect modal behaviour).
        progressReporter.reset();
        m_show_progress = false;
    }

    return !cancelled;
}


void SYMBOL_TREE_MODEL_ADAPTER::AddLibrary( wxString const& aLibNickname, bool pinned )
{
    bool                        onlyPowerSymbols = ( GetFilter() == SYM_FILTER_POWER );
    std::vector<LIB_SYMBOL*>    symbols;
    std::vector<LIB_TREE_ITEM*> comp_list;

    try
    {
        m_libs->LoadSymbolLib( symbols, aLibNickname, onlyPowerSymbols );
    }
    catch( const IO_ERROR& ioe )
    {
        wxLogError( _( "Error loading symbol library '%s'." ) + wxS( "\n%s" ),
                    aLibNickname,
                    ioe.What() );
        return;
    }

    if( symbols.size() > 0 )
    {
        comp_list.assign( symbols.begin(), symbols.end() );
        DoAddLibrary( aLibNickname, m_libs->GetDescription( aLibNickname ), comp_list, pinned,
                      false );
    }
}


wxString SYMBOL_TREE_MODEL_ADAPTER::GenerateInfo( LIB_ID const& aLibId, int aUnit )
{
    return GenerateAliasInfo( m_libs, aLibId, aUnit );
}


