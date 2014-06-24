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

#include <class_board.h>
#include <class_pad.h>
#include <class_zone_settings.h>
#include <class_board_design_settings.h>


class MODULE;


DISPLAY_OPTIONS DisplayOpt;      // Display options for board items

int    g_AnchorColor        = BLUE;
int    g_ModuleTextCMPColor = LIGHTGRAY;
int    g_ModuleTextCUColor  = MAGENTA;
int    g_ModuleTextNOVColor = DARKGRAY;
int    g_PadCUColor         = GREEN;
int    g_PadCMPColor        = RED;


/**
 * Used in track creation, a list of track segments currently being created,
 * with the newest track at the end of the list, sorted by new-ness.  e.g. use
 * TRACK->Back() to get the next older track, TRACK->Next() to get the next
 * newer track.
 */
DLIST<TRACK> g_CurrentTrackList;

void AccumulateDescription( wxString &aDesc, const wxString &aItem )
{
    if( !aDesc.IsEmpty() )
        aDesc << wxT(", ");
    aDesc << aItem;
}


wxString LayerMaskDescribe( const BOARD *aBoard, LSET aMask )
{
    // Try the single or no- layer case (easy)
    LAYER_ID layer = aMask.ExtractLayer();

    switch( (int) layer )
    {
    case UNSELECTED_LAYER:
        return _( "No layers" );

    case UNDEFINED_LAYER:
        break;

    default:
        return aBoard->GetLayerName( layer );
    }

    // Try to be smart and useful, starting with outer copper
    // (which are more important than internal ones)
    wxString layerInfo;

    if( aMask[F_Cu] )
        AccumulateDescription( layerInfo, aBoard->GetLayerName( F_Cu ) );

    if( aMask[B_Cu] )
        AccumulateDescription( layerInfo, aBoard->GetLayerName( B_Cu ) );

    if( ( aMask & LSET::InternalCuMask() ).any() )
        AccumulateDescription( layerInfo, _("Internal" ) );

    if( ( aMask & LSET::AllNonCuMask() ).any() )
        AccumulateDescription( layerInfo, _("Non-copper" ) );

    return layerInfo;
}

