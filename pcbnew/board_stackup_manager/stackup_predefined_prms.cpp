/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2009-2019 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file stackup_predefined_prms.cpp
 */

#include "class_board_stackup.h"
#include <convert_to_biu.h>
#include <layers_id_colors_and_visibility.h>
#include <board_design_settings.h>
#include <i18n_utility.h>       // _HKI definition
#include <macros.h>
#include "stackup_predefined_prms.h"

// A list of copper finish standard type names
// They are standard names in .gbdjob files, so avoid changing them or
// ensure they are compatible with .gbrjob file spec.
// These names are in fact usual copper finish names.
static wxString CopperFinishType[] =
{
    _HKI( NOT_SPECIFIED ),      // Not specified, not in .gbrjob file
    _HKI("ENIG"),               // used in .gbrjob file
    _HKI("ENEPIG"),             // used in .gbrjob file
    _HKI("HAL SnPb"),           // used in .gbrjob file
    _HKI("HAL lead-free"),      // used in .gbrjob file
    _HKI("Hard gold"),          // used in .gbrjob file
    _HKI("Immersion tin"),      // used in .gbrjob file
    _HKI("Immersion nickel"),   // used in .gbrjob file
    _HKI("Immersion silver"),   // used in .gbrjob file
    _HKI("Immersion gold"),     // used in .gbrjob file
    _HKI("HT_OSP"),             // used in .gbrjob file
    _HKI("OSP"),                // used in .gbrjob file
    _HKI("None"),               // used in .gbrjob file
    _HKI("User defined")        // keep this option at end
};


// A list of available colors for solder mask and silkscreen
// These names are used in .gbrjob file, so they are not fully free.
// Use only what is allowed in .gbrjob files.
// for other colors (user defined), the defined value is the html color syntax.
static FAB_LAYER_COLOR solderMaskColors[]  =
{
    { _HKI( NOT_SPECIFIED ), wxColor( 80, 80, 80 ) },   // Not specified, not in .gbrjob file
    { _HKI( "Green" ), wxColor( 60, 150, 80 ) },         // used in .gbrjob file
    { _HKI( "Red" ), wxColor( 128, 0, 0 ) },            // used in .gbrjob file
    { _HKI( "Blue" ), wxColor( 0, 0, 128 ) },           // used in .gbrjob file
    { _HKI( "Black" ), wxColor( 20, 20, 20 ) },         // used in .gbrjob file
    { _HKI( "White" ), wxColor( 200, 200, 200 ) },      // used in .gbrjob file
    { _HKI( "Yellow" ), wxColor( 128, 128, 0 ) },       // used in .gbrjob file
    { _HKI( "Purple" ), wxColor( 100, 0, 100 ) },       // used in .gbrjob file
    { _HKI( USER_DEFINED ), wxColor( 128, 128, 128 ) }, //free. the name is a dummy name here
    { "", wxColor( 0, 0, 0 ) }                          // Sentinel
};


// A list of available substrate material
// These names are used in .gbrjob file, so they are not fully free.
// Use only what is allowed in .gbrjob files.
// These names are in fact usual substrate names.
static FAB_SUBSTRATE substrateMaterial[]  =
{
    { _HKI( NOT_SPECIFIED ), 0.0, 0.0 },    // Not specified, not in .gbrjob file
    { _HKI( "FR4" ), 4.5, 0.02 },           // used in .gbrjob file
    { _HKI( "Polyimide" ), 1.0, 0.0 },      // used in .gbrjob file
    { _HKI( "Polyolefin" ), 1.0, 0.0 },     // used in .gbrjob file
    { _HKI( "Al" ), 8.7, 0.001 },           // used in .gbrjob file
    { _HKI( "PTFE" ), 2.1, 0.0002 },        // used in .gbrjob file
    { _HKI( "Teflon" ), 2.1, 0.0002 },      // used in .gbrjob file
    { _HKI( "Ceramic" ), 1.0, 0.0 },        // used in .gbrjob file
    { _HKI( USER_DEFINED ), 1.0, 0.0 },     // Free string
};


wxString FAB_SUBSTRATE::FormatEpsilonR()
{
    // return a wxString to print/display Epsilon R
    wxString txt;
    txt.Printf( "%.1f", m_EpsilonR );
    return txt;
}


wxString FAB_SUBSTRATE::FormatLossTangent()
{
    // return a wxString to print/display Loss Tangent
    wxString txt;
    txt.Printf( "%g", m_LossTangent );
    return txt;
}


FAB_SUBSTRATE_LIST::FAB_SUBSTRATE_LIST()
{
    // Fills the m_substrateList with predefined params:
    for( unsigned ii = 0; ii < arrayDim( substrateMaterial ); ++ii )
        m_substrateList.push_back( substrateMaterial[ii] );
}


FAB_SUBSTRATE* FAB_SUBSTRATE_LIST::GetSubstrate( int aIdx )
{
    if( aIdx >= 0 && aIdx < GetCount() )
        return &m_substrateList[aIdx];

    return nullptr;
}


FAB_SUBSTRATE* FAB_SUBSTRATE_LIST::GetSubstrate( const wxString& aName )
{
    for( FAB_SUBSTRATE& item : m_substrateList )
    {
        if( item.m_Name.CmpNoCase( aName ) == 0 )
            return &item;
    }

    return nullptr;
}


wxArrayString GetCopperFinishStandardList( bool aTranslate )
{
    wxArrayString list;

    for( unsigned ii = 0; ii < arrayDim( CopperFinishType ); ii++ )
        list.Add( aTranslate ? wxGetTranslation( CopperFinishType[ii] ) : CopperFinishType[ii] );

    return list;
}


const FAB_LAYER_COLOR* GetColorStandardList()
{
    return solderMaskColors;
}


int GetColorStandardListCount()
{
    return arrayDim( solderMaskColors ) - 1;
}


int GetColorUserDefinedListIdx()
{
    return GetColorStandardListCount() - 1;
}

