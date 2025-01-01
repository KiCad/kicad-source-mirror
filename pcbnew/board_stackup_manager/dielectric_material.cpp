/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 Jean-Pierre Charras, jp.charras at wanadoo.fr
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


#include "stackup_predefined_prms.h"
#include "dielectric_material.h"
#include <core/arraydim.h>
#include <string_utils.h>               // for UIDouble2Str()


// A list of available substrate material
// These names are used in .gbrjob file, so they are not fully free.
// So do not change name with "used in .gbrjob file" comment.
// These names are in fact usual substrate names.
// However one can add and use other names for material name.
// DO NOT translate them, as they are proper noun
static DIELECTRIC_SUBSTRATE substrateMaterial[]  =
{
    { NotSpecifiedPrm(),   0.0,  0.0    },    // Not specified, not in .gbrjob
    { wxT( "FR4" ),        4.5,  0.02   },    // used in .gbrjob file
    { wxT( "FR408-HR" ),   3.69, 0.0091 },    // used in .gbrjob file
    { wxT( "Polyimide" ),  3.2,  0.004  },    // used in .gbrjob file
    { wxT( "Kapton" ),     3.2,  0.004  },    // used in .gbrjob file
    { wxT( "Polyolefin" ), 1.0,  0.0    },    // used in .gbrjob file
    { wxT( "Al" ),         8.7,  0.001  },    // used in .gbrjob file
    { wxT( "PTFE" ),       2.1,  0.0002 },    // used in .gbrjob file
    { wxT( "Teflon" ),     2.1,  0.0002 },    // used in .gbrjob file
    { wxT( "Ceramic" ),    1.0,  0.0    }     // used in .gbrjob file
                                            // Other names are free
};

static DIELECTRIC_SUBSTRATE solderMaskMaterial[]  =
{
    { NotSpecifiedPrm(),   DEFAULT_EPSILON_R_SOLDERMASK, 0.0 },   // Not specified, not in .gbrjob
    { wxT( "Epoxy" ),      DEFAULT_EPSILON_R_SOLDERMASK, 0.0 },   // Epoxy Liquid material (usual)
    { wxT( "Liquid Ink" ), DEFAULT_EPSILON_R_SOLDERMASK, 0.0 },   // Liquid Ink Photoimageable
    { wxT( "Dry Film" ),   DEFAULT_EPSILON_R_SOLDERMASK, 0.0 }    // Dry Film Photoimageable
};

static DIELECTRIC_SUBSTRATE silkscreenMaterial[]  =
{
    { NotSpecifiedPrm(),        DEFAULT_EPSILON_R_SILKSCREEN, 0.0 },   // Not specified, not in .gbrjob
    { wxT( "Liquid Photo" ),    DEFAULT_EPSILON_R_SILKSCREEN, 0.0 },   // Liquid Ink Photoimageable
    { wxT( "Direct Printing" ), DEFAULT_EPSILON_R_SILKSCREEN, 0.0 }    // Direct Legend Printing
};


wxString DIELECTRIC_SUBSTRATE::FormatEpsilonR()
{
    // return a wxString to print/display Epsilon R
    // note: we do not want scientific notation
    wxString txt = UIDouble2Str( m_EpsilonR );
    return txt;
}


wxString DIELECTRIC_SUBSTRATE::FormatLossTangent()
{
    // return a wxString to print/display Loss Tangent
    // note: we do not want scientific notation
    wxString txt = UIDouble2Str( m_LossTangent );
    return txt;
}


DIELECTRIC_SUBSTRATE_LIST::DIELECTRIC_SUBSTRATE_LIST( DL_MATERIAL_LIST_TYPE aListType )
{
    // Fills the m_substrateList with predefined params:
    switch( aListType )
    {
    case DL_MATERIAL_DIELECTRIC:
        for( unsigned ii = 0; ii < arrayDim( substrateMaterial ); ++ii )
            m_substrateList.push_back( substrateMaterial[ii] );
        break;

    case DL_MATERIAL_SOLDERMASK:
        for( unsigned ii = 0; ii < arrayDim( solderMaskMaterial ); ++ii )
            m_substrateList.push_back( solderMaskMaterial[ii] );
        break;

    case DL_MATERIAL_SILKSCREEN:
        for( unsigned ii = 0; ii < arrayDim( silkscreenMaterial ); ++ii )
            m_substrateList.push_back( silkscreenMaterial[ii] );
        break;
    }
}


DIELECTRIC_SUBSTRATE* DIELECTRIC_SUBSTRATE_LIST::GetSubstrate( int aIdx )
{
    if( aIdx >= 0 && aIdx < GetCount() )
        return &m_substrateList[aIdx];

    return nullptr;
}


DIELECTRIC_SUBSTRATE* DIELECTRIC_SUBSTRATE_LIST::GetSubstrate( const wxString& aName )
{
    for( DIELECTRIC_SUBSTRATE& item : m_substrateList )
    {
        if( item.m_Name.CmpNoCase( aName ) == 0 )
            return &item;
    }

    return nullptr;
}


int DIELECTRIC_SUBSTRATE_LIST::FindSubstrate( DIELECTRIC_SUBSTRATE* aItem )
{
    // Find a item matching aItem. The comparison is for the name case insensitive
    int idx = 0;

    for( DIELECTRIC_SUBSTRATE& item : m_substrateList )
    {

        if( item.m_EpsilonR == aItem->m_EpsilonR &&
            item.m_LossTangent == aItem->m_LossTangent &&
            item.m_Name.CmpNoCase( aItem->m_Name ) == 0 )
        {
            return idx;
        }

        ++idx;
    }

    return -1;
}


int DIELECTRIC_SUBSTRATE_LIST::FindSubstrate( const wxString& aName, double aEpsilonR, double aLossTg )
{
    // Find a item matching parameters
    int idx = 0;

    for( DIELECTRIC_SUBSTRATE& item : m_substrateList )
    {

        if( item.m_EpsilonR == aEpsilonR &&
            item.m_LossTangent == aLossTg &&
            item.m_Name.CmpNoCase( aName ) == 0 )
        {
            return idx;
        }

        ++idx;
    }

    return -1;
}
