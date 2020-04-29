/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007 Dick Hollenbeck, dick@softplc.com
 * Copyright (C) 2015-2020 KiCad Developers, see change_log.txt for contributors.
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
#include "wx/html/m_templ.h"
#include "wx/html/styleparams.h"
#include <drc/drc.h>
#include <drc/drc_item.h>
#include <class_board.h>


wxString DRC_ITEM::GetErrorText( int aCode ) const
{
    if( aCode < 0 )
        aCode = m_errorCode;

    switch( aCode )
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
    case DRCE_TRACK_ENDS:
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
        return wxString( _( "Micro Via: incorrect layer pairs" ) );
    case DRCE_MICRO_VIA_NOT_ALLOWED:
        return wxString( _( "Micro Via: not allowed (board design rule constraints)" ) );
    case DRCE_BURIED_VIA_NOT_ALLOWED:
        return wxString( _( "Buried Via: not allowed (board design rule constraints)" ) );
    case DRCE_DISABLED_LAYER_ITEM:
        return wxString( _( "Item on a disabled layer" ) );
    case DRCE_ZONES_INTERSECT:
        return wxString( _( "Copper areas intersect" ) );
    case DRCE_ZONES_TOO_CLOSE:
        return wxString( _( "Copper areas too close" ) );

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
        return wxString( _( "NetClass Track Width too small" ) );
    case DRCE_NETCLASS_CLEARANCE:
        return wxString( _( "NetClass Clearance too small" ) );
    case DRCE_NETCLASS_VIASIZE:
        return wxString( _( "NetClass Via Dia too small" ) );
    case DRCE_NETCLASS_VIADRILLSIZE:
        return wxString( _( "NetClass Via Drill too small" ) );
    case DRCE_NETCLASS_uVIASIZE:
        return wxString( _( "NetClass uVia Dia too small" ) );
    case DRCE_NETCLASS_uVIADRILLSIZE:
        return wxString( _( "NetClass uVia Drill too small" ) );

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

    case DRCE_UNRESOLVED_VARIABLE:
        return wxString( _( "Unresolved text variable" ) );

    default:
        return wxEmptyString;
    }
}


wxString escapeHtml( wxString aString )
{
    aString.Replace( wxT("<"), wxT("&lt;") );
    aString.Replace( wxT(">"), wxT("&gt;") );
    return aString;
}


wxString DRC_ITEM::ShowHtml( PCB_BASE_FRAME* aFrame ) const
{
    BOARD_ITEM* mainItem = nullptr;
    BOARD_ITEM* auxItem = nullptr;
    wxString    msg = m_errorMessage.IsEmpty() ? GetErrorText( m_errorCode ) : m_errorMessage;
    wxString    mainText;
    wxString    auxText;

    if( m_mainItemUuid != niluuid )
        mainItem = aFrame->GetBoard()->GetItem( m_mainItemUuid );

    if( m_auxItemUuid != niluuid )
        auxItem = aFrame->GetBoard()->GetItem( m_auxItemUuid );

    if( mainItem )
        mainText = mainItem->GetSelectMenuText( aFrame->GetUserUnits() );

    if( auxItem )
        auxText = auxItem->GetSelectMenuText( aFrame->GetUserUnits() );

    if( mainItem && auxItem )
    {
        // an html fragment for the entire message in the listbox.  feel free
        // to add color if you want:
        return wxString::Format( wxT( "<b>%s</b><br>&nbsp;&nbsp; %s<br>&nbsp;&nbsp; %s" ),
                                 escapeHtml( msg ),
                                 escapeHtml( mainText ),
                                 escapeHtml( auxText ) );
    }
    else if( mainItem )
    {
        return wxString::Format( wxT( "<b>%s</b><br>&nbsp;&nbsp; %s" ),
                                 escapeHtml( msg ),
                                 escapeHtml( mainText ) );
    }
    else
    {
        return wxString::Format( wxT( "<b>%s</b>" ),
                                 escapeHtml( msg ) );
    }
}


