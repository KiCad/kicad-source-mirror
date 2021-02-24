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
#include <widgets/app_progress_dialog.h>

#include <eda_pattern_match.h>
#include <symbol_lib_table.h>
#include <lib_symbol.h>
#include <generate_alias_info.h>
#include <symbol_tree_model_adapter.h>


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
    APP_PROGRESS_DIALOG* prg = nullptr;
    wxLongLong        nextUpdate = wxGetUTCTimeMillis() + (PROGRESS_INTERVAL_MILLIS / 2);

    if( m_show_progress )
    {
        prg = new APP_PROGRESS_DIALOG( _( "Loading Symbol Libraries" ), wxEmptyString,
                                       aNicknames.size(), aParent );
    }

    unsigned int ii = 0;

    for( const auto& nickname : aNicknames )
    {
        if( prg && wxGetUTCTimeMillis() > nextUpdate )
        {
            prg->Update( ii, wxString::Format( _( "Loading library \"%s\"" ), nickname ) );

            nextUpdate = wxGetUTCTimeMillis() + PROGRESS_INTERVAL_MILLIS;
        }

        AddLibrary( nickname );
        ii++;
    }

    m_tree.AssignIntrinsicRanks();

    if( prg )
    {
        // Force immediate deletion of the APP_PROGRESS_DIALOG
        // ( do not use Destroy(), or use Destroy() followed by wxSafeYield() )
        // because on Windows, APP_PROGRESS_DIALOG has some side effects on the event loop
        // manager. A side effect is the call of ShowModal() of a dialog following
        // the use of SYMBOL_TREE_MODEL_ADAPTER creating a APP_PROGRESS_DIALOG
        // has a broken behavior (incorrect modal behavior).
        delete prg;
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
