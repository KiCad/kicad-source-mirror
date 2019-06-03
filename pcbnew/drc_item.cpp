/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007 Dick Hollenbeck, dick@softplc.com
 * Copyright (C) 2015-2018 KiCad Developers, see change_log.txt for contributors.
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


#include <fctsys.h>
#include <common.h>

#include <pcbnew.h>
#include <tools/drc.h>
#include <drc_item.h>
#include <class_board.h>
#include <base_units.h>


wxString DRC_ITEM::GetErrorText() const
{
    switch( m_ErrorCode )
    {
    case DRCE_UNCONNECTED_ITEMS:
        return wxString( _( "Unconnected items" ) );
    case DRCE_TRACK_NEAR_THROUGH_HOLE:
        return wxString( _( "Track too close to thru-hole" ) );
    case DRCE_TRACK_NEAR_PAD:
        return wxString( _( "Track too close to pad" ) );
    case DRCE_TRACK_NEAR_VIA:
        return wxString( _( "Track too close to via" ) );
    case DRCE_VIA_NEAR_VIA:
        return wxString( _( "Via too close to via" ) );
    case DRCE_VIA_NEAR_TRACK:
        return wxString( _( "Via too close to track" ) );
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
    case DRCE_TRACK_NEAR_ZONE:
        return wxString( _( "Track too close to copper area" ) );
    case DRCE_PAD_NEAR_PAD1:
        return wxString( _( "Pad too close to pad" ) );
    case DRCE_VIA_HOLE_BIGGER:
        return wxString( _( "Via hole > diameter" ) );
    case DRCE_MICRO_VIA_INCORRECT_LAYER_PAIR:
        return wxString( _( "Micro Via: incorrect layer pairs (not adjacent)" ) );
    case DRCE_MICRO_VIA_NOT_ALLOWED:
        return wxString( _( "Micro Via: not allowed" ) );
    case DRCE_BURIED_VIA_NOT_ALLOWED:
        return wxString( _( "Buried Via: not allowed" ) );
    case DRCE_DISABLED_LAYER_ITEM:
        return wxString( _( "Item on a disabled layer" ) );
    case DRCE_ZONES_INTERSECT:
        return wxString( _( "Copper area inside copper area" ) );
    case DRCE_ZONES_TOO_CLOSE:
        return wxString( _( "Copper areas intersect or are too close" ) );

    case DRCE_SUSPICIOUS_NET_FOR_ZONE_OUTLINE:
        return wxString( _( "Copper area belongs to a net which has no pads" ) );

    case DRCE_HOLE_NEAR_PAD:
        return wxString( _( "Hole too close to pad" ) );
    case DRCE_HOLE_NEAR_TRACK:
        return wxString( _( "Hole too close to track" ) );
    case DRCE_TOO_SMALL_TRACK_WIDTH:
        return wxString( _( "Track width too small" ) );
    case DRCE_TOO_SMALL_VIA:
        return wxString( _( "Via size too small" ) );
    case DRCE_TOO_SMALL_MICROVIA:
        return wxString( _( "Micro via size too small" ) );
    case DRCE_TOO_SMALL_VIA_DRILL:
        return wxString( _( "Via drill too small" ) );
    case DRCE_TOO_SMALL_MICROVIA_DRILL:
        return wxString( _( "Micro via drill too small" ) );
    case DRCE_DRILLED_HOLES_TOO_CLOSE:
        return wxString( _( "Drilled holes too close together" ) );
    case DRCE_TRACK_NEAR_EDGE:
        return wxString( _( "Track too close to board edge" ) );
    case DRCE_INVALID_OUTLINE:
        return wxString( _( "Board outline does not form a closed polygon" ) );

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
        return wxString( _( "Via inside keepout area" ) );
    case DRCE_TRACK_INSIDE_KEEPOUT:
        return wxString( _( "Track inside keepout area" ) );
    case DRCE_PAD_INSIDE_KEEPOUT:
        return wxString( _( "Pad inside keepout area" ) );

    case DRCE_VIA_NEAR_COPPER:
        return wxString( _( "Via too close to copper item" ) );
    case DRCE_TRACK_NEAR_COPPER:
        return wxString( _( "Track too close to copper item" ) );
    case DRCE_PAD_NEAR_COPPER:
        return wxString( _( "Pad too close to copper item" ) );

    case DRCE_OVERLAPPING_FOOTPRINTS:
        return wxString( _( "Courtyards overlap" ) );

    case DRCE_MISSING_COURTYARD_IN_FOOTPRINT:
        return wxString( _( "Footprint has no courtyard defined" ) );

    case DRCE_MALFORMED_COURTYARD_IN_FOOTPRINT:
        return wxString( _( "Footprint has incorrect courtyard (not a closed shape)" ) );

    case DRCE_DUPLICATE_FOOTPRINT:
        return wxString( _( "Duplicate footprints" ) );
    case DRCE_MISSING_FOOTPRINT:
        return wxString( _( "Missing footprint" ) );
    case DRCE_EXTRA_FOOTPRINT:
        return wxString( _( "Extra footprint" ) );

    case DRCE_SHORT:
        return wxString( _( "Remove track shorting two nets" ) );
    case DRCE_REDUNDANT_VIA:
        return wxString( _( "Remove redundant via" ) );
    case DRCE_DUPLICATE_TRACK:
        return wxString( _( "Remove duplicate track" ) );
    case DRCE_MERGE_TRACKS:
        return wxString( _( "Merge co-linear tracks" ) );
    case DRCE_DANGLING_TRACK:
        return wxString( _( "Remove dangling track" ) );
    case DRCE_DANGLING_VIA:
        return wxString( _( "Remove dangling via" ) );
    case DRCE_ZERO_LENGTH_TRACK:
        return wxString( _( "Remove zero-length track" ) );
    case DRCE_TRACK_IN_PAD:
        return wxString( _( "Remove track inside pad" ) );

    default:
        return wxString::Format( _( "Unknown DRC error code %d" ), m_ErrorCode );
    }
}


wxString DRC_ITEM::ShowCoord( EDA_UNITS_T aUnits, const wxPoint& aPos )
{
    return wxString::Format( wxT( "@(%s, %s)" ),
                             MessageTextFromValue( aUnits, aPos.x ),
                             MessageTextFromValue( aUnits, aPos.y ) );
}


wxString DRC_ITEM::ShowHtml( EDA_UNITS_T aUnits ) const
{
    wxString mainText = m_MainText;
    // a wxHtmlWindows does not like < and > in the text to display
    // because these chars have a special meaning in html
    mainText.Replace( wxT("<"), wxT("&lt;") );
    mainText.Replace( wxT(">"), wxT("&gt;") );

    wxString errText = GetErrorText();
    errText.Replace( wxT("<"), wxT("&lt;") );
    errText.Replace( wxT(">"), wxT("&gt;") );


    if( m_noCoordinate )
    {
        // omit the coordinate, a NETCLASS has no location
        return wxString::Format( wxT( "<b>%s</b><br>&nbsp;&nbsp; %s" ),
                                 errText,
                                 mainText );
    }
    else if( m_hasSecondItem )
    {
        wxString auxText = m_AuxiliaryText;
        auxText.Replace( wxT("<"), wxT("&lt;") );
        auxText.Replace( wxT(">"), wxT("&gt;") );

        // an html fragment for the entire message in the listbox.  feel free
        // to add color if you want:
        return wxString::Format( wxT( "<b>%s</b><br>&nbsp;&nbsp; %s: %s<br>&nbsp;&nbsp; %s: %s" ),
                                 errText,
                                 ShowCoord( aUnits, m_MainPosition ),
                                 mainText,
                                 ShowCoord( aUnits, m_AuxiliaryPosition ),
                                 auxText );
    }
    else
    {
        return wxString::Format( wxT( "<b>%s</b><br>&nbsp;&nbsp; %s: %s" ),
                                 errText,
                                 ShowCoord( aUnits, m_MainPosition ),
                                 mainText );
    }
}


wxString DRC_ITEM::ShowReport( EDA_UNITS_T aUnits ) const
{
    if( m_hasSecondItem )
    {
        return wxString::Format( wxT( "ErrType(%d): %s\n    %s: %s\n    %s: %s\n" ),
                                 m_ErrorCode,
                                 GetErrorText(),
                                 ShowCoord( aUnits, m_MainPosition ),
                                 m_MainText,
                                 ShowCoord( aUnits, m_AuxiliaryPosition ),
                                 m_AuxiliaryText );
    }
    else
    {
        return wxString::Format( wxT( "ErrType(%d): %s\n    %s: %s\n" ),
                                 m_ErrorCode,
                                 GetErrorText(),
                                 ShowCoord( aUnits, m_MainPosition ),
                                 m_MainText );
    }
}


BOARD_ITEM* DRC_ITEM::GetMainItem( BOARD* aBoard ) const
{
    return aBoard->GetItem( m_mainItemWeakRef );
}


BOARD_ITEM* DRC_ITEM::GetAuxiliaryItem( BOARD* aBoard ) const
{
    return aBoard->GetItem( m_auxItemWeakRef );
}

