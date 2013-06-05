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


/* Look up Table for conversion copper layer count -> general copper layer
 * mask: */
LAYER_MSK g_TabAllCopperLayerMask[NB_COPPER_LAYERS] = {
    0x0001, 0x8001, 0x8003, 0x8007,
    0x800F, 0x801F, 0x803F, 0x807F,
    0x80FF, 0x81FF, 0x83FF, 0x87FF,
    0x8FFF, 0x9FFF, 0xCFFF, 0xFFFF
};


DISPLAY_OPTIONS DisplayOpt;      // Display options for board items

// This will be always be 450 or 900 (by UI design) at the moment
int    g_RotationAngle;

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

LAYER_NUM FlipLayer( LAYER_NUM oldlayer )
{
    switch( oldlayer )
    {
    case LAYER_N_BACK:
        return LAYER_N_FRONT;

    case LAYER_N_FRONT:
        return LAYER_N_BACK;

    case SILKSCREEN_N_BACK:
        return SILKSCREEN_N_FRONT;

    case SILKSCREEN_N_FRONT:
        return SILKSCREEN_N_BACK;

    case ADHESIVE_N_BACK:
        return ADHESIVE_N_FRONT;

    case ADHESIVE_N_FRONT:
        return ADHESIVE_N_BACK;

    case SOLDERMASK_N_BACK:
        return SOLDERMASK_N_FRONT;

    case SOLDERMASK_N_FRONT:
        return SOLDERMASK_N_BACK;

    case SOLDERPASTE_N_BACK:
        return SOLDERPASTE_N_FRONT;

    case SOLDERPASTE_N_FRONT:
        return SOLDERPASTE_N_BACK;

    // No change for the other layers
    default:
        return oldlayer;
    }
}


LAYER_MSK FlipLayerMask( LAYER_MSK aMask )
{
    LAYER_MSK newMask;

    newMask = aMask & ~(LAYER_BACK | LAYER_FRONT |
                        SILKSCREEN_LAYER_BACK | SILKSCREEN_LAYER_FRONT |
                        ADHESIVE_LAYER_BACK | ADHESIVE_LAYER_FRONT |
                        SOLDERMASK_LAYER_BACK | SOLDERMASK_LAYER_FRONT |
                        SOLDERPASTE_LAYER_BACK | SOLDERPASTE_LAYER_FRONT |
                        ADHESIVE_LAYER_BACK | ADHESIVE_LAYER_FRONT);

    if( aMask & LAYER_BACK )
        newMask |= LAYER_FRONT;

    if( aMask & LAYER_FRONT )
        newMask |= LAYER_BACK;

    if( aMask & SILKSCREEN_LAYER_BACK )
        newMask |= SILKSCREEN_LAYER_FRONT;

    if( aMask & SILKSCREEN_LAYER_FRONT )
        newMask |= SILKSCREEN_LAYER_BACK;

    if( aMask & ADHESIVE_LAYER_BACK )
        newMask |= ADHESIVE_LAYER_FRONT;

    if( aMask & ADHESIVE_LAYER_FRONT )
        newMask |= ADHESIVE_LAYER_BACK;

    if( aMask & SOLDERMASK_LAYER_BACK )
        newMask |= SOLDERMASK_LAYER_FRONT;

    if( aMask & SOLDERMASK_LAYER_FRONT )
        newMask |= SOLDERMASK_LAYER_BACK;

    if( aMask & SOLDERPASTE_LAYER_BACK )
        newMask |= SOLDERPASTE_LAYER_FRONT;

    if( aMask & SOLDERPASTE_LAYER_FRONT )
        newMask |= SOLDERPASTE_LAYER_BACK;

    if( aMask & ADHESIVE_LAYER_BACK )
        newMask |= ADHESIVE_LAYER_FRONT;

    if( aMask & ADHESIVE_LAYER_FRONT )
        newMask |= ADHESIVE_LAYER_BACK;

    return newMask;
}

LAYER_NUM ExtractLayer( LAYER_MSK aMask )
{
    if( aMask == NO_LAYERS )
        return UNSELECTED_LAYER;

    LAYER_NUM candidate = UNDEFINED_LAYER;

    // Scan all the layers and take note of the first set; if other are
    // then found return UNDEFINED_LAYER
    for( LAYER_NUM i = FIRST_LAYER; i < NB_LAYERS; ++i )
    {
        if( aMask & GetLayerMask( i ) )
        {
            if( candidate == UNDEFINED_LAYER )
                candidate = i;
            else 
                return UNDEFINED_LAYER;
        }
    }
    return candidate;
}

wxString LayerMaskDescribe( const BOARD *aBoard, LAYER_MSK aMask )
{
    // Try the single or no- layer case (easy)
    LAYER_NUM layer = ExtractLayer( aMask ); 
    switch( layer )
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
    if( aMask & LAYER_FRONT )
        AccumulateDescription( layerInfo, aBoard->GetLayerName( LAYER_N_FRONT ) );

    if( aMask & LAYER_BACK )
        AccumulateDescription( layerInfo, aBoard->GetLayerName( LAYER_N_BACK ) );
    
    if( aMask & INTERNAL_CU_LAYERS )
        AccumulateDescription( layerInfo, _("Internal" ) );

    if( aMask & ALL_NO_CU_LAYERS )
        AccumulateDescription( layerInfo, _("Non-copper" ) );

    return layerInfo;
}

void AccumulateDescription( wxString &aDesc, const wxString &aItem )
{
    if( !aDesc.IsEmpty() )
        aDesc << wxT(", ");
    aDesc << aItem;
}
