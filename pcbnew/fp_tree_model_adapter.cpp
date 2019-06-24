/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018-2019 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <fp_lib_table.h>
#include <footprint_info.h>
#include <footprint_info_impl.h>
#include <generate_footprint_info.h>

#include "fp_tree_model_adapter.h"

FP_TREE_MODEL_ADAPTER::PTR FP_TREE_MODEL_ADAPTER::Create( LIB_TABLE* aLibs )
{
    return PTR( new FP_TREE_MODEL_ADAPTER( aLibs ) );
}


FP_TREE_MODEL_ADAPTER::FP_TREE_MODEL_ADAPTER( LIB_TABLE* aLibs )
    : m_libs( (FP_LIB_TABLE*) aLibs )
{}


FP_TREE_MODEL_ADAPTER::~FP_TREE_MODEL_ADAPTER()
{}


void FP_TREE_MODEL_ADAPTER::AddLibraries()
{
    for( const auto& libName : m_libs->GetLogicalLibs() )
    {
        const FP_LIB_TABLE_ROW* library = m_libs->FindRow( libName );

        DoAddLibrary( libName, library->GetDescr(), getFootprints( libName ), true );
    }

    m_tree.AssignIntrinsicRanks();
}


std::vector<LIB_TREE_ITEM*> FP_TREE_MODEL_ADAPTER::getFootprints( const wxString& aLibName )
{
    std::vector<LIB_TREE_ITEM*> libList;

    auto fullListStart = GFootprintList.GetList().begin();
    auto fullListEnd = GFootprintList.GetList().end();
    std::unique_ptr<FOOTPRINT_INFO> dummy( new FOOTPRINT_INFO_IMPL( aLibName, wxEmptyString ) );

    // List is sorted, so use a binary search to find the range of footnotes for our library
    auto libBounds = std::equal_range( fullListStart, fullListEnd, dummy,
        []( const std::unique_ptr<FOOTPRINT_INFO>& a, const std::unique_ptr<FOOTPRINT_INFO>& b )
        {
            return StrNumCmp( a->GetLibNickname(), b->GetLibNickname(), false ) < 0;
        } );

    for( auto i = libBounds.first; i != libBounds.second; ++i )
        libList.push_back( i->get() );

    return libList;
}


wxString FP_TREE_MODEL_ADAPTER::GenerateInfo( LIB_ID const& aLibId, int aUnit )
{
    return GenerateFootprintInfo( m_libs, aLibId );
}
