/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2009-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "board_stackup.h"
#include <convert_to_biu.h>
#include <core/arraydim.h>
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
    NotSpecifiedPrm(),            // Not specified, not in .gbrjob file
    _HKI( "ENIG" ),               // used in .gbrjob file
    _HKI( "ENEPIG" ),             // used in .gbrjob file
    _HKI( "HAL SnPb" ),           // used in .gbrjob file
    _HKI( "HAL lead-free" ),      // used in .gbrjob file
    _HKI( "Hard gold" ),          // used in .gbrjob file
    _HKI( "Immersion tin" ),      // used in .gbrjob file
    _HKI( "Immersion nickel" ),   // used in .gbrjob file
    _HKI( "Immersion silver" ),   // used in .gbrjob file
    _HKI( "Immersion gold" ),     // used in .gbrjob file
    _HKI( "HT_OSP" ),             // used in .gbrjob file
    _HKI( "OSP" ),                // used in .gbrjob file
    _HKI( "None" ),               // used in .gbrjob file
    _HKI( "User defined" )        // keep this option at end
};


// A list of available colors for solder mask and silkscreen
// These names are used in .gbrjob file, so they are not fully free.
// Use only what is allowed in .gbrjob files.
// for other colors (user defined), the defined value is the
// html color syntax in .kicad_pcb files
// and R<integer>G<integer>B<integer> In gbrjob file.
static FAB_LAYER_COLOR solderMaskColors[]  =
{
    { NotSpecifiedPrm(), wxColor( 80, 80, 80 ) },       // Not specified, not in .gbrjob file
    { _HKI( "Green" ), wxColor( 60, 150, 80 ) },        // used in .gbrjob file
    { _HKI( "Red" ), wxColor( 128, 0, 0 ) },            // used in .gbrjob file
    { _HKI( "Blue" ), wxColor( 0, 0, 128 ) },           // used in .gbrjob file
    { _HKI( "Black" ), wxColor( 20, 20, 20 ) },         // used in .gbrjob file
    { _HKI( "White" ), wxColor( 200, 200, 200 ) },      // used in .gbrjob file
    { _HKI( "Yellow" ), wxColor( 128, 128, 0 ) },       // used in .gbrjob file
    { _HKI( "User defined" ), wxColor( 128, 128, 128 ) }//free. the name is a dummy name here
};


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
    return arrayDim( solderMaskColors );
}


int GetColorUserDefinedListIdx()
{
    // this is the last item in list
    return GetColorStandardListCount() - 1;
}
