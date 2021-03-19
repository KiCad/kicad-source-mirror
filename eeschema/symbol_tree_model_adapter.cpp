/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Chris Pavlina <pavlina.chris@gmail.com>
 * Copyright (C) 2014 Henner Zeller <h.zeller@acm.org>
 * Copyright (C) 2014-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <wx/tokenzr.h>
#include <wx/window.h>
#include <widgets/progress_reporter.h>

#include <eda_pattern_match.h>
#include <symbol_lib_table.h>
#include <lib_symbol.h>
#include <locale_io.h>
#include <generate_alias_info.h>
#include <symbol_tree_model_adapter.h>
#include <symbol_async_loader.h>


bool SYMBOL_TREE_MODEL_ADAPTER::m_show_progress = true;

#define PROGRESS_INTERVAL_MILLIS 66


wxObjectDataPtr<LIB_TREE_MODEL_ADAPTER>
SYMBOL_TREE_MODEL_ADAPTER::Create( EDA_BASE_FRAME* aParent, LIB_TABLE* aLibs )
{
    auto* adapter = new SYMBOL_TREE_MODEL_ADAPTER( aParent, aLibs );
    return wxObjectDataPtr<LIB_TREE_MODEL_ADAPTER>( adapter );
}


SYMBOL_TREE_MODEL_ADAPTER::SYMBOL_TREE_MODEL_ADAPTER( EDA_BASE_FRAME* aParent, LIB_TABLE* aLibs ) :
        LIB_TREE_MODEL_ADAPTER( aParent, "pinned_symbol_libs" ),
        m_libs( (SYMBOL_LIB_TABLE*) aLibs )
{}


SYMBOL_TREE_MODEL_ADAPTER::~SYMBOL_TREE_MODEL_ADAPTER()
{}


void SYMBOL_TREE_MODEL_ADAPTER::AddLibraries( const std::vector<wxString>& aNicknames,
                                              wxWindow* aParent )
{
    std::unique_ptr<WX_PROGRESS_REPORTER> prg = nullptr;

    if( m_show_progress )
    {
        prg = std::make_unique<WX_PROGRESS_REPORTER>( aParent, _( "Loading Symbol Libraries" ),
                                                      aNicknames.size(), true );
    }

    // Disable KIID generation: not needed for library parts; sometimes very slow
    KIID::CreateNilUuids( true );

    std::unordered_map<wxString, std::vector<LIB_PART*>> loadedSymbols;

    SYMBOL_ASYNC_LOADER loader( aNicknames, this, loadedSymbols, prg.get() );

    LOCALE_IO toggle;

    loader.Start();

    while( !loader.Done() )
    {
        if( prg )
            prg->KeepRefreshing();

        wxMilliSleep( PROGRESS_INTERVAL_MILLIS );
    }

    if( prg && prg->IsCancelled() )
    {
        loader.Abort();
    }
    else
    {
        loader.Join();
    }

    if( !loader.GetErrors().IsEmpty() )
    {
        wxLogError( loader.GetErrors() );
    }

    if( loadedSymbols.size() > 0 )
    {
        for( const std::pair<const wxString, std::vector<LIB_PART*>>& pair : loadedSymbols )
        {
            std::vector<LIB_TREE_ITEM*> treeItems( pair.second.begin(), pair.second.end() );
            DoAddLibrary( pair.first, m_libs->GetDescription( pair.first ), treeItems, false );
        }
    }

    KIID::CreateNilUuids( false );

    m_tree.AssignIntrinsicRanks();

    if( prg )
    {
        // Force immediate deletion of the APP_PROGRESS_DIALOG
        // ( do not use Destroy(), or use Destroy() followed by wxSafeYield() )
        // because on Windows, APP_PROGRESS_DIALOG has some side effects on the event loop
        // manager. A side effect is the call of ShowModal() of a dialog following
        // the use of SYMBOL_TREE_MODEL_ADAPTER creating a APP_PROGRESS_DIALOG
        // has a broken behavior (incorrect modal behavior).
        prg.reset();
        m_show_progress = false;
    }
}


void SYMBOL_TREE_MODEL_ADAPTER::AddLibrary( wxString const& aLibNickname )
{
    bool                        onlyPowerSymbols = ( GetFilter() == CMP_FILTER_POWER );
    std::vector<LIB_PART*>      symbols;
    std::vector<LIB_TREE_ITEM*> comp_list;

    try
    {
        m_libs->LoadSymbolLib( symbols, aLibNickname, onlyPowerSymbols );
    }
    catch( const IO_ERROR& ioe )
    {
        wxLogError( wxString::Format( _( "Error loading symbol library %s.\n\n%s" ),
                                      aLibNickname,
                                      ioe.What() ) );
        return;
    }

    if( symbols.size() > 0 )
    {
        comp_list.assign( symbols.begin(), symbols.end() );
        DoAddLibrary( aLibNickname, m_libs->GetDescription( aLibNickname ), comp_list, false );
    }
}


wxString SYMBOL_TREE_MODEL_ADAPTER::GenerateInfo( LIB_ID const& aLibId, int aUnit )
{
    return GenerateAliasInfo( m_libs, aLibId, aUnit );
}
