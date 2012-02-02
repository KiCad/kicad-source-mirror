/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2008 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2008 KiCad Developers, see change_log.txt for contributors.
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

/*
 * This file contains the global constants and variables used in the PCB
 * applications Pcbnew, CvPcb, and GervView.  The goal of this was to
 * unobfuscate the original header file design that made it very difficult
 * to figure out where these variables lived.  Ideally, they should be pushed
 * back into the application layer.
 */

#include <fctsys.h>
#include <pcbcommon.h>
#include <plot_common.h>

#include <class_pad.h>
#include <class_zone_setting.h>
#include <class_board_design_settings.h>


class MODULE;


/* return a one bit layer mask from a layer number
 * aLayerNumber = the layer number to convert (0 .. LAYER_COUNT-1)
 */
int GetLayerMask( int aLayerNumber )
{
    wxASSERT( aLayerNumber < LAYER_COUNT && aLayerNumber >= 0 );

#if 0
    // Look up Table for conversion one layer number -> one bit layer mask:
    static int tabOneLayerMask[LAYER_COUNT] =
    {
        0x00000001, 0x00000002, 0x00000004, 0x00000008,
        0x00000010, 0x00000020, 0x00000040, 0x00000080,
        0x00000100, 0x00000200, 0x00000400, 0x00000800,
        0x00001000, 0x00002000, 0x00004000, 0x00008000,
        0x00010000, 0x00020000, 0x00040000, 0x00080000,
        0x00100000, 0x00200000, 0x00400000, 0x00800000,
        0x01000000, 0x02000000, 0x04000000, 0x08000000,
        0x10000000, 0x20000000, 0x40000000, 0x80000000
    };

    return( tabOneLayerMask[aLayerNumber] );
#else
    return 1 << aLayerNumber;
#endif
}

/* Look up Table for conversion copper layer count -> general copper layer
 * mask: */
int g_TabAllCopperLayerMask[NB_COPPER_LAYERS] = {
    0x0001, 0x8001, 0x8003, 0x8007,
    0x800F, 0x801F, 0x803F, 0x807F,
    0x80FF, 0x81FF, 0x83FF, 0x87FF,
    0x8FFF, 0x9FFF, 0xCFFF, 0xFFFF
};


DISPLAY_OPTIONS DisplayOpt;      /* Display options for board items */

/* PCB file name extension definitions. */
wxString NetExtBuffer( wxT( "net" ) );
wxString NetCmpExtBuffer( wxT( "cmp" ) );
wxString g_Shapes3DExtBuffer( wxT( "wrl" ) );
const wxString ModuleFileExtension( wxT( "mod" ) );

/* PCB file name wild card definitions. */
const wxString ModuleFileWildcard( _( "KiCad footprint library files (*.mod)|*.mod" ) );

int    g_RotationAngle;

int    g_AnchorColor        = BLUE;
int    g_ModuleTextCMPColor = LIGHTGRAY;
int    g_ModuleTextCUColor  = MAGENTA;
int    g_ModuleTextNOVColor = DARKGRAY;
int    g_PadCUColor         = GREEN;
int    g_PadCMPColor        = RED;


// Current design settings:
class BOARD_DESIGN_SETTINGS g_DesignSettings;

/**
 * Used in track creation, a list of track segments currently being created,
 * with the newest track at the end of the list, sorted by new-ness.  e.g. use
 * TRACK->Back() to get the next older track, TRACK->Next() to get the next
 * newer track.
 */
DLIST<TRACK> g_CurrentTrackList;

bool g_Zone_45_Only = false;

// Default setting used when creating a new zone
ZONE_SETTING g_Zone_Default_Setting;

D_PAD g_Pad_Master( (MODULE*) NULL );
