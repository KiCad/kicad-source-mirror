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
#include "wx/html/m_templ.h"
#include "wx/html/styleparams.h"
#include <erc.h>
#include <erc_item.h>
#include <i18n_utility.h>

wxString ERC_ITEM::GetErrorText( int aErrorCode, bool aTranslate ) const
{
    wxString msg;

    if( aErrorCode < 0 )
        aErrorCode = m_errorCode;

    switch( aErrorCode )
    {
    case ERCE_UNSPECIFIED:
        msg = _HKI("ERC err unspecified" );
        break;
    case ERCE_DUPLICATE_SHEET_NAME:
        msg = _HKI("Duplicate sheet names within a given sheet" );
        break;
    case ERCE_PIN_NOT_CONNECTED:
        msg = _HKI("Pin not connected" );
        break;
    case ERCE_PIN_NOT_DRIVEN:
        msg = _HKI( "Pin connected to other pins, but not driven by any pin" );
        break;
    case ERCE_PIN_TO_PIN_WARNING:
    case ERCE_PIN_TO_PIN_ERROR:
        msg = _HKI("Conflict problem between pins" );
        break;
    case ERCE_HIERACHICAL_LABEL:
        msg = _HKI("Mismatch between hierarchical labels and pins sheets" );
        break;
    case ERCE_NOCONNECT_CONNECTED:
        msg = _HKI("A pin with a \"no connection\" flag is connected" );
        break;
    case ERCE_NOCONNECT_NOT_CONNECTED:
        msg = _HKI("Unconnected \"no connection\" flag" );
        break;
    case ERCE_LABEL_NOT_CONNECTED:
        msg = _HKI("Label not connected anywhere else in the schematic" );
        break;
    case ERCE_SIMILAR_LABELS:
        msg = _HKI("Labels are similar (lower/upper case difference only)" );
        break;
    case ERCE_DIFFERENT_UNIT_FP:
        msg = _HKI("Different footprint assigned in another unit of the same component" );
        break;
    case ERCE_DIFFERENT_UNIT_NET:
        msg = _HKI("Different net assigned to a shared pin in another unit of the same component" );
        break;
    case ERCE_BUS_ALIAS_CONFLICT:
        msg = _HKI("Conflict between bus alias definitions across schematic sheets" );
        break;
    case ERCE_DRIVER_CONFLICT:
        msg = _HKI( "More than one name given to this bus or net" );
        break;
    case ERCE_BUS_ENTRY_CONFLICT:
        msg = _HKI( "Net is graphically connected to a bus but not a bus member" );
        break;
    case ERCE_BUS_LABEL_ERROR:
        msg = _HKI( "Label attached to bus item does not describe a bus" );
        break;
    case ERCE_BUS_TO_BUS_CONFLICT:
        msg = _HKI( "Buses are graphically connected but share no bus members" );
        break;
    case ERCE_BUS_TO_NET_CONFLICT:
        msg = _HKI( "Invalid connection between bus and net items" );
        break;
    case ERCE_GLOBLABEL:
        msg = _HKI( "Global label not connected anywhere else in the schematic" );
        break;
    case ERCE_UNRESOLVED_VARIABLE:
        msg = _HKI( "Unresolved text variable" );
        break;
    default:
        wxFAIL_MSG( "Missing ERC error description" );
        msg = _HKI( "Unknown ERC violation" );
        break;
    }

    if( aTranslate )
        return wxGetTranslation( msg );
    else
        return msg;
}
