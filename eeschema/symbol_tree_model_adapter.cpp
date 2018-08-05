/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Chris Pavlina <pavlina.chris@gmail.com>
 * Copyright (C) 2014 Henner Zeller <h.zeller@acm.org>
 * Copyright (C) 2014-2017 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <wx/progdlg.h>

#include <eda_pattern_match.h>
#include <symbol_lib_table.h>
#include <class_libentry.h>
#include <generate_alias_info.h>

#include <symbol_tree_model_adapter.h>


bool SYMBOL_TREE_MODEL_ADAPTER::m_show_progress = true;

#define PROGRESS_INTERVAL_MILLIS 66


SYMBOL_TREE_MODEL_ADAPTER::PTR SYMBOL_TREE_MODEL_ADAPTER::Create( LIB_TABLE* aLibs )
{
    return PTR( new SYMBOL_TREE_MODEL_ADAPTER( aLibs ) );
}


SYMBOL_TREE_MODEL_ADAPTER::SYMBOL_TREE_MODEL_ADAPTER( LIB_TABLE* aLibs )
    : m_libs( (SYMBOL_LIB_TABLE*) aLibs )
{}


SYMBOL_TREE_MODEL_ADAPTER::~SYMBOL_TREE_MODEL_ADAPTER()
{}


void SYMBOL_TREE_MODEL_ADAPTER::AddLibraries( const std::vector<wxString>& aNicknames,
                                              wxWindow* aParent )
{
    wxProgressDialog* prg = nullptr;
    wxLongLong        nextUpdate = wxGetUTCTimeMillis() + (PROGRESS_INTERVAL_MILLIS / 2);

    if( m_show_progress )
    {
        prg = new wxProgressDialog( _( "Loading Symbol Libraries" ), wxEmptyString,
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
        prg->Destroy();
        m_show_progress = false;
    }
}


void SYMBOL_TREE_MODEL_ADAPTER::AddLibrary( wxString const& aLibNickname )
{
    bool                        onlyPowerSymbols = ( GetFilter() == CMP_FILTER_POWER );
    std::vector<LIB_ALIAS*>     alias_list;
    std::vector<LIB_TREE_ITEM*> comp_list;

    try
    {
        m_libs->LoadSymbolLib( alias_list, aLibNickname, onlyPowerSymbols );
    }
    catch( const IO_ERROR& ioe )
    {
        wxLogError( wxString::Format( _( "Error loading symbol library %s.\n\n%s" ),
                                      aLibNickname,
                                      ioe.What() ) );
        return;
    }

    if( alias_list.size() > 0 )
    {
        comp_list.assign( alias_list.begin(), alias_list.end() );
        DoAddLibrary( aLibNickname, m_libs->GetDescription( aLibNickname ), comp_list, false );
    }
}


wxString SYMBOL_TREE_MODEL_ADAPTER::GenerateInfo( LIB_ID const& aLibId, int aUnit )
{
    return GenerateAliasInfo( m_libs, aLibId, aUnit );
}
