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

#include <cmp_tree_model_adapter.h>

#include <eda_pattern_match.h>
#include <wx/tokenzr.h>
#include <symbol_lib_table.h>
#include <wx/progdlg.h>

CMP_TREE_MODEL_ADAPTER_BASE::PTR CMP_TREE_MODEL_ADAPTER::Create( SYMBOL_LIB_TABLE* aLibs )
{
    auto adapter = new CMP_TREE_MODEL_ADAPTER( aLibs );
    auto container = CMP_TREE_MODEL_ADAPTER::PTR( adapter );
    return container;
}


CMP_TREE_MODEL_ADAPTER::CMP_TREE_MODEL_ADAPTER( SYMBOL_LIB_TABLE* aLibs )
    : m_libs( aLibs )
{}


CMP_TREE_MODEL_ADAPTER::~CMP_TREE_MODEL_ADAPTER()
{}


void CMP_TREE_MODEL_ADAPTER::AddLibrary( wxString const& aLibNickname )
{
    bool onlyPowerSymbols = ( GetFilter() == CMP_FILTER_POWER );

    wxArrayString aliases;

    try
    {
        m_libs->EnumerateSymbolLib( aLibNickname, aliases, onlyPowerSymbols );
    }
    catch( const IO_ERROR& ioe )
    {
        wxLogError( wxString::Format( _( "Error occurred loading symbol  library %s."
                                         "\n\n%s" ), aLibNickname, ioe.What() ) );
        return;
    }

    if( aliases.size() > 0 )
    {
        AddAliasList( aLibNickname, aliases );
        m_tree.AssignIntrinsicRanks();
    }
}


void CMP_TREE_MODEL_ADAPTER::AddAliasList(
            wxString const&         aNodeName,
            wxArrayString const&    aAliasNameList )
{
    std::vector<LIB_ALIAS*> alias_list;

    for( const wxString& name: aAliasNameList )
    {
        LIB_ALIAS* a = nullptr;

        try
        {
            a = m_libs->LoadSymbol( aNodeName, name );
        }
        catch( const IO_ERROR& ioe )
        {
            wxLogError( wxString::Format( _( "Error occurred loading symbol %s from library %s."
                                             "\n\n%s" ), name, aNodeName, ioe.What() ) );
            continue;
        }

        if( a )
            alias_list.push_back( a );
    }

    if( alias_list.size() > 0 )
        AddAliasList( aNodeName, alias_list );
}
