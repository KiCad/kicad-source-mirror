/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007 Dick Hollenbeck, dick@softplc.com
 * Copyright (C) 2007 KiCad Developers, see change_log.txt for contributors.
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


/*************************************************/
/* class_drc_item.cpp - DRC_ITEM class functions */
/*************************************************/
#include <fctsys.h>
#include <common.h>

#include <pcbnew.h>
#include <drc_stuff.h>
#include <class_drc_item.h>
#include <base_units.h>


wxString DRC_ITEM::GetErrorText() const
{
    switch( m_ErrorCode )
    {
    case DRCE_UNCONNECTED_PADS:
        return wxString( _( "Unconnected pads" ) );
    case DRCE_TRACK_NEAR_THROUGH_HOLE:
        return wxString( _( "Track near thru-hole" ) );
    case DRCE_TRACK_NEAR_PAD:
        return wxString( _( "Track near pad" ) );
    case DRCE_TRACK_NEAR_VIA:
        return wxString( _( "Track near via" ) );
    case DRCE_VIA_NEAR_VIA:
        return wxString( _( "Via near via" ) );
    case DRCE_VIA_NEAR_TRACK:
        return wxString( _( "Via near track" ) );
    case DRCE_TRACK_ENDS1:
    case DRCE_TRACK_ENDS2:
    case DRCE_TRACK_ENDS3:
    case DRCE_TRACK_ENDS4:
    case DRCE_ENDS_PROBLEM1:
    case DRCE_ENDS_PROBLEM2:
    case DRCE_ENDS_PROBLEM3:
    case DRCE_ENDS_PROBLEM4:
    case DRCE_ENDS_PROBLEM5:
        return wxString( _( "Two track ends too close" ) );
    case DRCE_TRACK_SEGMENTS_TOO_CLOSE:
        return wxString( _( "Two parallel track segments too close" ) );
    case DRCE_TRACKS_CROSSING:
        return wxString( _( "Tracks crossing" ) );
    case DRCE_PAD_NEAR_PAD1:
        return wxString( _( "Pad near pad" ) );
    case DRCE_VIA_HOLE_BIGGER:
        return wxString( _( "Via hole > diameter" ) );
    case DRCE_MICRO_VIA_INCORRECT_LAYER_PAIR:
        return wxString( _( "Micro Via: incorrect layer pairs (not adjacent)" ) );
    case COPPERAREA_INSIDE_COPPERAREA:
        return wxString( _( "Copper area inside copper area" ) );
    case COPPERAREA_CLOSE_TO_COPPERAREA:
        return wxString( _( "Copper areas intersect or are too close" ) );
    case DRCE_NON_EXISTANT_NET_FOR_ZONE_OUTLINE:
        return wxString( _( "Copper area has a nonexistent net name" ) );
    case DRCE_HOLE_NEAR_PAD:
        return wxString( _( "Hole near pad" ) );
    case DRCE_HOLE_NEAR_TRACK:
        return wxString( _( "Hole near track" ) );
    case DRCE_TOO_SMALL_TRACK_WIDTH:
        return wxString( _( "Too small track width" ) );
    case DRCE_TOO_SMALL_VIA:
        return wxString( _( "Too small via size" ) );
    case DRCE_TOO_SMALL_MICROVIA:
        return wxString( _( "Too small micro via size" ) );

    // use &lt; since this is text ultimately embedded in HTML
    case DRCE_NETCLASS_TRACKWIDTH:
        return wxString( _( "NetClass Track Width &lt; global limit" ) );
    case DRCE_NETCLASS_CLEARANCE:
        return wxString( _( "NetClass Clearance &lt; global limit" ) );
    case DRCE_NETCLASS_VIASIZE:
        return wxString( _( "NetClass Via Dia &lt; global limit" ) );
    case DRCE_NETCLASS_VIADRILLSIZE:
        return wxString( _( "NetClass Via Drill &lt; global limit" ) );
    case DRCE_NETCLASS_uVIASIZE:
        return wxString( _( "NetClass uVia Dia &lt; global limit" ) );
    case DRCE_NETCLASS_uVIADRILLSIZE:
        return wxString( _( "NetClass uVia Drill &lt; global limit" ) );

    case DRCE_VIA_INSIDE_KEEPOUT:
        return wxString( _( "Via inside a keepout area" ) );

    case DRCE_TRACK_INSIDE_KEEPOUT:
        return wxString( _( "Track inside a keepout area" ) );

    case DRCE_PAD_INSIDE_KEEPOUT:
        return wxString( _( "Pad inside a keepout area" ) );

    case DRCE_VIA_INSIDE_TEXT:
        return wxString( _( "Via inside a text" ) );

    case DRCE_TRACK_INSIDE_TEXT:
        return wxString( _( "Track inside a text" ) );

    case DRCE_PAD_INSIDE_TEXT:
        return wxString( _( "Pad inside a text" ) );

    default:
        return wxString::Format( wxT( "Unknown DRC error code %d" ), m_ErrorCode );
    }
}


wxString DRC_ITEM::ShowCoord( const wxPoint& aPos )
{
    wxString ret;

    ret << aPos;
    return ret;
}
