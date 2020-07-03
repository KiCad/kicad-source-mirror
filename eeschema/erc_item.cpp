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


// These, being statically-defined, require specialized I18N handling.  We continue to
// use the _() macro so that string harvesting by the I18N framework doesn't have to be
// specialized, but we don't translate on initialization and instead do it in the getters.

#undef _
#define _(s) s

ERC_ITEM ERC_ITEM::duplicateSheetName( ERCE_DUPLICATE_SHEET_NAME,
        _( "Duplicate sheet names within a given sheet" ),
        wxT( "duplicate_sheet_names" ) );

ERC_ITEM ERC_ITEM::pinNotConnected( ERCE_PIN_NOT_CONNECTED,
        _( "Pin not connected" ),
        wxT( "pin_not_connected" ) );

ERC_ITEM ERC_ITEM::pinNotDriven( ERCE_PIN_NOT_DRIVEN,
        _( "Pin connected to other pins, but not driven by any pin" ),
        wxT( "pin_not_driven" ) );

ERC_ITEM ERC_ITEM::pinTableWarning( ERCE_PIN_TO_PIN_WARNING,
        _( "Conflict problem between pins" ),
        wxT( "pin_to_pin" ) );

ERC_ITEM ERC_ITEM::pinTableError( ERCE_PIN_TO_PIN_ERROR,
        _( "Conflict problem between pins" ),
        wxT( "pin_to_pin" ) );

ERC_ITEM ERC_ITEM::hierLabelMismatch( ERCE_HIERACHICAL_LABEL,
        _( "Mismatch between hierarchical labels and pins sheets" ),
        wxT( "hier_label_mismatch" ) );

ERC_ITEM ERC_ITEM::noConnectConnected( ERCE_NOCONNECT_CONNECTED,
        _( "A pin with a \"no connection\" flag is connected" ),
        wxT( "no_connect_connected" ) );

ERC_ITEM ERC_ITEM::noConnectDangling( ERCE_NOCONNECT_NOT_CONNECTED,
        _( "Unconnected \"no connection\" flag" ),
        wxT( "no_connect_dangling" ) );

ERC_ITEM ERC_ITEM::labelDangling( ERCE_LABEL_NOT_CONNECTED,
        _( "Label not connected anywhere else in the schematic" ),
        wxT( "label_dangling" ) );

ERC_ITEM ERC_ITEM::globalLabelDangling( ERCE_GLOBLABEL,
        _( "Global label not connected anywhere else in the schematic" ),
        wxT( "global_label_dangling" ) );

ERC_ITEM ERC_ITEM::similarLabels( ERCE_SIMILAR_LABELS,
        _( "Labels are similar (lower/upper case difference only) "),
        wxT( "similar_labels" ) );

ERC_ITEM ERC_ITEM::differentUnitFootprint( ERCE_DIFFERENT_UNIT_FP,
        _( "Different footprint assigned in another unit of the same component" ),
        wxT( "different_unit_footprint" ) );

ERC_ITEM ERC_ITEM::differentUnitNet( ERCE_DIFFERENT_UNIT_NET,
        _( "Different net assigned to a shared pin in another unit of the same component" ),
        wxT( "different_unit_net" ) );

ERC_ITEM ERC_ITEM::busDefinitionConflict( ERCE_BUS_ALIAS_CONFLICT,
        _( "Conflict between bus alias definitions across schematic sheets" ),
        wxT( "bus_definition_conflict" ) );

ERC_ITEM ERC_ITEM::multipleNetNames( ERCE_DRIVER_CONFLICT,
        _( "More than one name given to this bus or net" ),
        wxT( "multiple_net_names" ) );

ERC_ITEM ERC_ITEM::netNotBusMember( ERCE_BUS_ENTRY_CONFLICT,
        _( "Net is graphically connected to a bus but not a bus member" ),
        wxT( "net_not_bus_member" ) );

ERC_ITEM ERC_ITEM::busLabelSyntax( ERCE_BUS_LABEL_ERROR,
        _( "Label attached to bus item does not describe a bus" ),
        wxT( "bus_label_syntax" ) );

ERC_ITEM ERC_ITEM::busToBusConflict( ERCE_BUS_TO_BUS_CONFLICT,
        _( "Buses are graphically connected but share no bus members" ),
        wxT( "bus_to_bus_conflict" ) );

ERC_ITEM ERC_ITEM::busToNetConflict( ERCE_BUS_TO_NET_CONFLICT,
        _( "Invalid connection between bus and net items" ),
        wxT( "bus_to_net_conflict" ) );

ERC_ITEM ERC_ITEM::unresolvedVariable( ERCE_UNRESOLVED_VARIABLE,
        _( "Unresolved text variable" ),
        wxT( "unresolved_variable" ) );

std::vector<std::reference_wrapper<RC_ITEM>> ERC_ITEM::allItemTypes( {
                 ERC_ITEM::duplicateSheetName,
                 ERC_ITEM::pinNotConnected,
                 ERC_ITEM::pinNotDriven,
                 ERC_ITEM::pinTableWarning,
                 ERC_ITEM::hierLabelMismatch,
                 ERC_ITEM::noConnectConnected,
                 ERC_ITEM::noConnectDangling,
                 ERC_ITEM::labelDangling,
                 ERC_ITEM::globalLabelDangling,
                 ERC_ITEM::similarLabels,
                 ERC_ITEM::differentUnitFootprint,
                 ERC_ITEM::differentUnitNet,
                 ERC_ITEM::busDefinitionConflict,
                 ERC_ITEM::multipleNetNames,
                 ERC_ITEM::netNotBusMember,
                 ERC_ITEM::busLabelSyntax,
                 ERC_ITEM::busToBusConflict,
                 ERC_ITEM::busToNetConflict,
                 ERC_ITEM::unresolvedVariable
         } );



ERC_ITEM* ERC_ITEM::Create( int aErrorCode )
{
    switch( aErrorCode )
    {
    case ERCE_DUPLICATE_SHEET_NAME:
        return new ERC_ITEM( duplicateSheetName );

    case ERCE_PIN_NOT_CONNECTED:
        return new ERC_ITEM( pinNotConnected );

    case ERCE_PIN_NOT_DRIVEN:
        return new ERC_ITEM( pinNotDriven );

    case ERCE_PIN_TO_PIN_WARNING:
        return new ERC_ITEM( pinTableWarning );

    case ERCE_PIN_TO_PIN_ERROR:
        return new ERC_ITEM( pinTableError );

    case ERCE_HIERACHICAL_LABEL:
        return new ERC_ITEM( hierLabelMismatch );

    case ERCE_NOCONNECT_CONNECTED:
        return new ERC_ITEM( noConnectConnected );

    case ERCE_NOCONNECT_NOT_CONNECTED:
        return new ERC_ITEM( noConnectDangling );

    case ERCE_LABEL_NOT_CONNECTED:
        return new ERC_ITEM( labelDangling );

    case ERCE_SIMILAR_LABELS:
        return new ERC_ITEM( similarLabels );

    case ERCE_DIFFERENT_UNIT_FP:
        return new ERC_ITEM( differentUnitFootprint );

    case ERCE_DIFFERENT_UNIT_NET:
        return new ERC_ITEM( differentUnitNet );

    case ERCE_BUS_ALIAS_CONFLICT:
        return new ERC_ITEM( busDefinitionConflict );

    case ERCE_DRIVER_CONFLICT:
        return new ERC_ITEM( multipleNetNames );

    case ERCE_BUS_ENTRY_CONFLICT:
        return new ERC_ITEM( netNotBusMember );

    case ERCE_BUS_LABEL_ERROR:
        return new ERC_ITEM( busLabelSyntax );

    case ERCE_BUS_TO_BUS_CONFLICT:
        return new ERC_ITEM( busToBusConflict );

    case ERCE_BUS_TO_NET_CONFLICT:
        return new ERC_ITEM( busToNetConflict );

    case ERCE_GLOBLABEL:
        return new ERC_ITEM( globalLabelDangling );

    case ERCE_UNRESOLVED_VARIABLE:
        return new ERC_ITEM( unresolvedVariable );

    case ERCE_UNSPECIFIED:
    default:
        wxFAIL_MSG( "Unknown ERC error code" );
        return nullptr;
    }
}
