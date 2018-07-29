/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 KiCad Developers, see AUTHORS.txt for contributors.
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


void FP_TREE_MODEL_ADAPTER::AddLibraries( FOOTPRINT_LIST* aFootprintInfoList )
{
    // Note: FOOTPRINT_INFO list must be sorted!

    wxString currentLib;
    std::vector<LIB_TREE_ITEM*> footprints;

    for( auto& footprint : aFootprintInfoList->GetList() )
    {
        if( footprint->GetNickname() != currentLib )
        {
            if( footprints.size() )
                DoAddLibrary( currentLib, m_libs->GetDescription( currentLib ), footprints );

            footprints.clear();
            currentLib = footprint->GetNickname();
        }

        footprints.push_back( footprint.get() );
    }

    if( footprints.size() )
        DoAddLibrary( currentLib, m_libs->GetDescription( currentLib ), footprints );
}


wxString FP_TREE_MODEL_ADAPTER::GenerateInfo( LIB_ID const& aLibId, int aUnit )
{
    return GenerateFootprintInfo( m_libs, aLibId );
}
