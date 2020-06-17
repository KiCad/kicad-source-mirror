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


DRC_ITEM::DRC_ITEM( int aErrorCode )
{
    m_errorCode = aErrorCode;
}


DRC_ITEM::DRC_ITEM( const wxString& aErrorText )
{
    for( int errorCode = DRCE_FIRST; errorCode <= DRCE_LAST; ++errorCode )
    {
        if( aErrorText == GetErrorText( errorCode, false ) )
        {
            m_errorCode = errorCode;
            break;
        }
    }
}


wxString DRC_ITEM::GetErrorText( int aCode, bool aTranslate ) const
{
    wxString msg;

    if( aCode < 0 )
        aCode = m_errorCode;

    switch( aCode )
    {
    case DRCE_UNCONNECTED_ITEMS:        msg = _HKI( "Unconnected items" );                  break;
    case DRCE_TRACK_NEAR_HOLE:          msg = _HKI( "Track too close to hole" );            break;
    case DRCE_TRACK_NEAR_PAD:           msg = _HKI( "Track too close to pad" );             break;
    case DRCE_TRACK_NEAR_VIA:           msg = _HKI( "Track too close to via" );             break;
    case DRCE_VIA_NEAR_VIA:             msg = _HKI( "Vias too close" );                     break;
    case DRCE_VIA_NEAR_TRACK:           msg = _HKI( "Via too close to track" );             break;
    case DRCE_TRACK_ENDS:               msg = _HKI( "Track ends too close" );               break;
    case DRCE_TRACK_SEGMENTS_TOO_CLOSE: msg = _HKI( "Parallel tracks too close" );          break;
    case DRCE_TRACKS_CROSSING:          msg = _HKI( "Tracks crossing" );                    break;
    case DRCE_TRACK_NEAR_ZONE:          msg = _HKI( "Track too close to copper area" );     break;
    case DRCE_PAD_NEAR_PAD:             msg = _HKI( "Pads too close" );                     break;
    case DRCE_VIA_HOLE_BIGGER:          msg = _HKI( "Via hole larger than diameter" );      break;
    case DRCE_MICROVIA_TOO_MANY_LAYERS: msg = _HKI( "Micro via through too many layers" );  break;
    case DRCE_MICROVIA_NOT_ALLOWED:     msg = _HKI( "Micro via not allowed" );              break;
    case DRCE_BURIED_VIA_NOT_ALLOWED:   msg = _HKI( "Buried via not allowed" );             break;
    case DRCE_DISABLED_LAYER_ITEM:      msg = _HKI( "Item on a disabled layer" );           break;
    case DRCE_ZONES_INTERSECT:          msg = _HKI( "Copper areas intersect" );             break;
    case DRCE_ZONES_TOO_CLOSE:          msg = _HKI( "Copper areas too close" );             break;
    case DRCE_ZONE_HAS_EMPTY_NET:       msg = _HKI( "Copper zone net has no pads" );        break;
    case DRCE_DANGLING_VIA:             msg = _HKI( "Via is not connected" );               break;
    case DRCE_DANGLING_TRACK:           msg = _HKI( "Track has unconnected end" );          break;
    case DRCE_HOLE_NEAR_PAD:            msg = _HKI( "Hole too close to pad" );              break;
    case DRCE_HOLE_NEAR_TRACK:          msg = _HKI( "Hole too close to track" );            break;
    case DRCE_TOO_SMALL_TRACK_WIDTH:    msg = _HKI( "Track width too small" );              break;
    case DRCE_TOO_LARGE_TRACK_WIDTH:    msg = _HKI( "Track width too large" );              break;
    case DRCE_TOO_SMALL_VIA:            msg = _HKI( "Via size too small" );                 break;
    case DRCE_TOO_SMALL_VIA_ANNULUS:    msg = _HKI( "Via annulus too small" );              break;
    case DRCE_TOO_SMALL_MICROVIA:       msg = _HKI( "Micro via size too small" );           break;
    case DRCE_TOO_SMALL_VIA_DRILL:      msg = _HKI( "Via drill too small" );                break;
    case DRCE_TOO_SMALL_PAD_DRILL:      msg = _HKI( "Pad drill too small" );                break;
    case DRCE_TOO_SMALL_MICROVIA_DRILL: msg = _HKI( "Micro via drill too small" );          break;
    case DRCE_DRILLED_HOLES_TOO_CLOSE:  msg = _HKI( "Drilled holes too close together" );   break;
    case DRCE_TRACK_NEAR_EDGE:          msg = _HKI( "Track too close to board edge" );      break;
    case DRCE_VIA_NEAR_EDGE:            msg = _HKI( "Via too close to board edge" );        break;
    case DRCE_PAD_NEAR_EDGE:            msg = _HKI( "Pad too close to board edge" );        break;
    case DRCE_INVALID_OUTLINE:          msg = _HKI( "Board has malformed outline" );        break;

    case DRCE_NETCLASS_TRACKWIDTH:      msg = _HKI( "NetClass Track Width too small" );     break;
    case DRCE_NETCLASS_CLEARANCE:       msg = _HKI( "NetClass Clearance too small" );       break;
    case DRCE_NETCLASS_VIAANNULUS:      msg = _HKI( "NetClass via annulus too small" );     break;
    case DRCE_NETCLASS_VIASIZE:         msg = _HKI( "NetClass Via Dia too small" );         break;
    case DRCE_NETCLASS_VIADRILLSIZE:    msg = _HKI( "NetClass Via Drill too small" );       break;
    case DRCE_NETCLASS_uVIASIZE:        msg = _HKI( "NetClass uVia Dia too small" );        break;
    case DRCE_NETCLASS_uVIADRILLSIZE:   msg = _HKI( "NetClass uVia Drill too small" );      break;

    case DRCE_VIA_INSIDE_KEEPOUT:       msg = _HKI( "Via inside keepout area" );            break;
    case DRCE_MICROVIA_INSIDE_KEEPOUT:  msg = _HKI( "Micro via inside keepout area" );      break;
    case DRCE_BBVIA_INSIDE_KEEPOUT:     msg = _HKI( "Buried via inside keepout area" );     break;
    case DRCE_TRACK_INSIDE_KEEPOUT:     msg = _HKI( "Track inside keepout area" );          break;
    case DRCE_PAD_INSIDE_KEEPOUT:       msg = _HKI( "Pad inside keepout area" );            break;
    case DRCE_FOOTPRINT_INSIDE_KEEPOUT: msg = _HKI( "Footprint inside keepout area" );      break;
    case DRCE_HOLE_INSIDE_KEEPOUT:      msg = _HKI( "Hole inside keepout area" );           break;
    case DRCE_TEXT_INSIDE_KEEPOUT:      msg = _HKI( "Text inside keepout area" );           break;
    case DRCE_GRAPHICS_INSIDE_KEEPOUT:  msg = _HKI( "Graphic inside keepout area" );        break;

    case DRCE_VIA_NEAR_COPPER:          msg = _HKI( "Via too close to copper item" );       break;
    case DRCE_TRACK_NEAR_COPPER:        msg = _HKI( "Track too close to copper item" );     break;
    case DRCE_PAD_NEAR_COPPER:          msg = _HKI( "Pad too close to copper item" );       break;

    case DRCE_OVERLAPPING_FOOTPRINTS:   msg = _HKI( "Courtyards overlap" );                 break;
    case DRCE_MISSING_COURTYARD:        msg = _HKI( "Footprint has no courtyard defined" ); break;
    case DRCE_MALFORMED_COURTYARD:      msg = _HKI( "Footprint has malformed courtyard" );  break;
    case DRCE_PTH_IN_COURTYARD:         msg = _HKI( "PTH inside courtyard" );               break;
    case DRCE_NPTH_IN_COURTYARD:        msg = _HKI( "NPTH inside courtyard" );              break;

    case DRCE_DUPLICATE_FOOTPRINT:      msg = _HKI( "Duplicate footprints" );               break;
    case DRCE_MISSING_FOOTPRINT:        msg = _HKI( "Missing footprint" );                  break;
    case DRCE_EXTRA_FOOTPRINT:          msg = _HKI( "Extra footprint" );                    break;

    case DRCE_UNRESOLVED_VARIABLE:      msg = _HKI( "Unresolved text variable" );           break;

    default:
        wxFAIL_MSG( "Missing DRC error description" );
        msg = _HKI( "Unknown DRC violation" );
        break;
    }

    if( aTranslate )
        return wxGetTranslation( msg );
    else
        return msg;
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


