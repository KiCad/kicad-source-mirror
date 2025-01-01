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


#include <wx/string.h>
#include <core/arraydim.h>
#include <board_design_settings.h>
#include <i18n_utility.h>       // _HKI definition
#include "stackup_predefined_prms.h"

// A list of copper finish standard type names.
// They are standard names in .gbdjob files, so avoid changing them or ensure they are
// compatible with .gbrjob file spec.
static wxString copperFinishType[] =
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


// A list of available colors for solder mask and silkscreen.
// These names are used in .gbrjob file, so they are not fully free.  Use only what is allowed in
// .gbrjob files.
// For other colors (user defined), the defined value is the html color syntax in .kicad_pcb files
// and R<integer>G<integer>B<integer> in .gbrjob file.
static std::vector<FAB_LAYER_COLOR> gbrjobColors  =
{
    { NotSpecifiedPrm(),      wxColor(  80,  80,  80 ) },  // Not specified, not in .gbrjob file
    { _HKI( "Green" ),        wxColor(  60, 150,  80 ) },  // used in .gbrjob file
    { _HKI( "Red" ),          wxColor( 128,   0,   0 ) },  // used in .gbrjob file
    { _HKI( "Blue" ),         wxColor(   0,   0, 128 ) },  // used in .gbrjob file
    { _HKI( "Purple" ),       wxColor(  80,   0,  80 ) },  // used in .gbrjob file
    { _HKI( "Black" ),        wxColor(  20,  20,  20 ) },  // used in .gbrjob file
    { _HKI( "White" ),        wxColor( 200, 200, 200 ) },  // used in .gbrjob file
    { _HKI( "Yellow" ),       wxColor( 128, 128,   0 ) },  // used in .gbrjob file
    { _HKI( "User defined" ), wxColor( 128, 128, 128 ) }   // Free; the name is a dummy name here
};


// These are used primarily as a source for the 3D renderer.  They are written
// as R<integer>G<integer>B<integer>  to the .gbrjob file.
static std::vector<FAB_LAYER_COLOR> dielectricColors =
{
    { NotSpecifiedPrm(),          wxColor(  80,  80,  80, 255 ) },
    { _HKI( "FR4 natural" ),      wxColor( 109, 116,  75, 212 ) },
    { _HKI( "PTFE natural" ),     wxColor( 252, 252, 250, 230 ) },
    { _HKI( "Polyimide" ),        wxColor( 205, 130,   0, 170 ) },
    { _HKI( "Phenolic natural" ), wxColor(  92,  17,   6, 230 ) },
    { _HKI( "Aluminum" ),         wxColor( 213, 213, 213, 255 ) },
    { _HKI( "User defined" ),     wxColor( 128, 128, 128, 212 ) }
};


wxArrayString GetStandardCopperFinishes( bool aTranslate )
{
    wxArrayString list;

    for( unsigned ii = 0; ii < arrayDim( copperFinishType ); ii++ )
        list.Add( aTranslate ? wxGetTranslation( copperFinishType[ii] ) : copperFinishType[ii] );

    return list;
}

std::vector<FAB_LAYER_COLOR> dummy;
const std::vector<FAB_LAYER_COLOR>& GetStandardColors( BOARD_STACKUP_ITEM_TYPE aType )
{
    switch( aType )
    {
    case BS_ITEM_TYPE_SILKSCREEN: return gbrjobColors;
    case BS_ITEM_TYPE_SOLDERMASK: return gbrjobColors;
    case BS_ITEM_TYPE_DIELECTRIC: return dielectricColors;
    default:                      return dummy;
    }
}


int GetColorUserDefinedListIdx( BOARD_STACKUP_ITEM_TYPE aType )
{
    // this is the last item in list
    return GetStandardColors( aType ).size() - 1;
}


bool IsColorNameNormalized( const wxString& aName )
{
    static std::vector<wxString> list =
    {
        wxT( "Green" ), wxT( "Red" ), wxT( "Blue" ),
        wxT( "Black" ), wxT( "White" ), wxT( "Yellow" )
    };

   for( wxString& candidate : list )
   {
       if( candidate.CmpNoCase( aName ) == 0 )
           return true;
   }

   return false;
}


const wxString FAB_LAYER_COLOR::GetColorAsString() const
{
    if( IsColorNameNormalized( m_colorName ) )
        return m_colorName;

    return wxString::Format( wxT( "R%dG%dB%d" ),
                             int( m_color.r*255 ), int( m_color.g*255 ), int( m_color.b*255 ) );
}
