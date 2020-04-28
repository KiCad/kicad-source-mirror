/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 KiCad Developers, see change_log.txt for contributors.
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
#include <erc.h>
#include <erc_item.h>


wxString ERC_ITEM::GetErrorText( int aErrorCode ) const
{
    if( aErrorCode < 0 )
        aErrorCode = m_errorCode;

    switch( aErrorCode )
    {
    case ERCE_UNSPECIFIED:
        return wxString( _("ERC err unspecified") );
    case ERCE_DUPLICATE_SHEET_NAME:
        return wxString( _("Duplicate sheet names within a given sheet") );
    case ERCE_PIN_NOT_CONNECTED:
        return wxString( _("Pin not connected") );
    case ERCE_PIN_NOT_DRIVEN:
        return wxString( _( "Pin connected to other pins, but not driven by any pin" ) );
    case ERCE_PIN_TO_PIN_WARNING:
        KI_FALLTHROUGH;         // Must share text with ERCE_PIN_TO_PIN_ERROR
    case ERCE_PIN_TO_PIN_ERROR:
        return wxString( _("Conflict problem between pins") );
    case ERCE_HIERACHICAL_LABEL:
        return wxString( _("Mismatch between hierarchical labels and pins sheets") );
    case ERCE_NOCONNECT_CONNECTED:
        return wxString( _("A pin with a \"no connection\" flag is connected") );
    case ERCE_NOCONNECT_NOT_CONNECTED:
        return wxString( _("Unconnected \"no connection\" flag") );
    case ERCE_LABEL_NOT_CONNECTED:
        return wxString( _("Label not connected anywhere else in the schematic") );
    case ERCE_SIMILAR_LABELS:
        return wxString( _("Labels are similar (lower/upper case difference only)" ) );
    case ERCE_DIFFERENT_UNIT_FP:
        return wxString( _("Different footprint assigned in another unit of the same component" ) );
    case ERCE_DIFFERENT_UNIT_NET:
        return wxString( _("Different net assigned to a shared pin in another unit of the same component" ) );
    case ERCE_BUS_ALIAS_CONFLICT:
        return wxString( _("Conflict between bus alias definitions across schematic sheets") );
    case ERCE_DRIVER_CONFLICT:
        return wxString( _( "More than one name given to this bus or net" ) );
    case ERCE_BUS_ENTRY_CONFLICT:
        return wxString( _( "Net is graphically connected to a bus but not a bus member" ) );
    case ERCE_BUS_LABEL_ERROR:
        return wxString( _( "Label attached to bus item does not describe a bus" ) );
    case ERCE_BUS_TO_BUS_CONFLICT:
        return wxString( _( "Buses are graphically connected but share no bus members" ) );
    case ERCE_BUS_TO_NET_CONFLICT:
        return wxString( _( "Invalid connection between bus and net items" ) );
    case ERCE_GLOBLABEL:
        return wxString( _( "Global label not connected anywhere else in the schematic" ) );
    case ERCE_UNRESOLVED_VARIABLE:
        return wxString( _( "Unresolved text variable" ) );
    default:
        wxFAIL_MSG( "Missing ERC error description" );
        return wxString( wxT( "Unknown" ) );
    }
}
