/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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


#include "wx/html/m_templ.h"
#include "wx/html/styleparams.h"
#include <erc/erc.h>
#include <eda_draw_frame.h>
#include <marker_base.h>
#include <sch_edit_frame.h>
#include <i18n_utility.h>


// These, being statically-defined, require specialized I18N handling.
// We don't translate on initialization and instead do it in the getters.

// NOTE: Avoid changing the settings key for an ERC item after it has been created

ERC_ITEM ERC_ITEM::heading_connections( 0, _HKI( "Connections" ), "" );
ERC_ITEM ERC_ITEM::heading_conflicts( 0, _HKI( "Conflicts" ), "" );
ERC_ITEM ERC_ITEM::heading_misc( 0, _HKI( "Miscellaneous" ), "" );
ERC_ITEM ERC_ITEM::heading_internal( 0, "", "" );

ERC_ITEM ERC_ITEM::duplicateSheetName( ERCE_DUPLICATE_SHEET_NAME,
        _HKI( "Duplicate sheet names within a given sheet" ),
        wxT( "duplicate_sheet_names" ) );

ERC_ITEM ERC_ITEM::endpointOffGrid( ERCE_ENDPOINT_OFF_GRID,
        _HKI( "Symbol pin or wire end off connection grid" ),
        wxT( "endpoint_off_grid" ) );

ERC_ITEM ERC_ITEM::pinNotConnected( ERCE_PIN_NOT_CONNECTED,
        _HKI( "Pin not connected" ),
        wxT( "pin_not_connected" ) );

ERC_ITEM ERC_ITEM::pinNotDriven( ERCE_PIN_NOT_DRIVEN,
        _HKI( "Input pin not driven by any Output pins" ),
        wxT( "pin_not_driven" ) );

ERC_ITEM ERC_ITEM::powerpinNotDriven( ERCE_POWERPIN_NOT_DRIVEN,
        _HKI( "Input Power pin not driven by any Output Power pins" ),
        wxT( "power_pin_not_driven" ) );

ERC_ITEM ERC_ITEM::duplicatePinError( ERCE_DUPLICATE_PIN_ERROR,
        _HKI( "Multiple pins with the same pin number" ),
        wxT( "duplicate_pins" ) );

ERC_ITEM ERC_ITEM::pinTableWarning( ERCE_PIN_TO_PIN_WARNING,
        _HKI( "Conflict problem between pins" ),
        wxT( "pin_to_pin" ) );

ERC_ITEM ERC_ITEM::pinTableError( ERCE_PIN_TO_PIN_ERROR,
        _HKI( "Conflict problem between pins" ),
        wxT( "pin_to_pin" ) );

ERC_ITEM ERC_ITEM::genericWarning( ERCE_GENERIC_WARNING,
        _HKI( "Warning" ),
        wxT( "generic-warning" ) );

ERC_ITEM ERC_ITEM::genericError( ERCE_GENERIC_ERROR,
        _HKI( "Error" ),
        wxT( "generic-error" ) );

ERC_ITEM ERC_ITEM::hierLabelMismatch( ERCE_HIERACHICAL_LABEL,
        _HKI( "Mismatch between hierarchical labels and sheet pins" ),
        wxT( "hier_label_mismatch" ) );

ERC_ITEM ERC_ITEM::fourWayJunction( ERCE_FOUR_WAY_JUNCTION,
        _HKI( "Four connection points are joined together" ),
        wxT( "four_way_junction" ) );

ERC_ITEM ERC_ITEM::labelMultipleWires( ERCE_LABEL_MULTIPLE_WIRES,
        _HKI( "Label connects more than one wire" ),
        wxT( "label_multiple_wires" ) );

ERC_ITEM ERC_ITEM::noConnectConnected( ERCE_NOCONNECT_CONNECTED,
        _HKI( "A pin with a \"no connection\" flag is connected" ),
        wxT( "no_connect_connected" ) );

ERC_ITEM ERC_ITEM::noConnectDangling( ERCE_NOCONNECT_NOT_CONNECTED,
        _HKI( "Unconnected \"no connection\" flag" ),
        wxT( "no_connect_dangling" ) );

ERC_ITEM ERC_ITEM::labelDangling( ERCE_LABEL_NOT_CONNECTED,
        _HKI( "Label not connected" ),
        wxT( "label_dangling" ) );

ERC_ITEM ERC_ITEM::isolatedPinLabel( ERCE_LABEL_SINGLE_PIN,
        _HKI( "Label connected to only one pin" ),
        wxT( "isolated_pin_label" ) );

ERC_ITEM ERC_ITEM::similarLabels( ERCE_SIMILAR_LABELS,
        _HKI( "Labels are similar (lower/upper case difference only)"),
        wxT( "similar_labels" ) );

ERC_ITEM ERC_ITEM::similarPower( ERCE_SIMILAR_POWER,
        _HKI( "Power pins are similar (lower/upper case difference only)"),
        wxT( "similar_power" ) );

ERC_ITEM ERC_ITEM::similarLabelAndPower( ERCE_SIMILAR_LABEL_AND_POWER,
        _HKI( "Power pin and label are similar (lower/upper case difference only)"),
        wxT( "similar_label_and_power" ) );

ERC_ITEM ERC_ITEM::singleGlobalLabel( ERCE_SINGLE_GLOBAL_LABEL,
        _HKI( "Global label only appears once in the schematic"),
        wxT( "single_global_label" ) );

ERC_ITEM ERC_ITEM::sameLocalGlobalLabel( ERCE_SAME_LOCAL_GLOBAL_LABEL,
        _HKI( "Local and global labels have same name" ),
        wxT( "same_local_global_label" ) );

ERC_ITEM ERC_ITEM::differentUnitFootprint( ERCE_DIFFERENT_UNIT_FP,
        _HKI( "Different footprint assigned in another unit of the symbol" ),
        wxT( "different_unit_footprint" ) );

ERC_ITEM ERC_ITEM::differentUnitNet( ERCE_DIFFERENT_UNIT_NET,
        _HKI( "Different net assigned to a shared pin in another unit of the symbol" ),
        wxT( "different_unit_net" ) );

ERC_ITEM ERC_ITEM::busDefinitionConflict( ERCE_BUS_ALIAS_CONFLICT,
        _HKI( "Conflict between bus alias definitions across schematic sheets" ),
        wxT( "bus_definition_conflict" ) );

ERC_ITEM ERC_ITEM::multipleNetNames( ERCE_DRIVER_CONFLICT,
        _HKI( "More than one name given to this bus or net" ),
        wxT( "multiple_net_names" ) );

ERC_ITEM ERC_ITEM::netNotBusMember( ERCE_BUS_ENTRY_CONFLICT,
        _HKI( "Net is graphically connected to a bus but not a bus member" ),
        wxT( "net_not_bus_member" ) );

ERC_ITEM ERC_ITEM::busToBusConflict( ERCE_BUS_TO_BUS_CONFLICT,
        _HKI( "Buses are graphically connected but share no bus members" ),
        wxT( "bus_to_bus_conflict" ) );

ERC_ITEM ERC_ITEM::busToNetConflict( ERCE_BUS_TO_NET_CONFLICT,
        _HKI( "Invalid connection between bus and net items" ),
        wxT( "bus_to_net_conflict" ) );

ERC_ITEM ERC_ITEM::groundPinNotGround( ERCE_GROUND_PIN_NOT_GROUND,
        _HKI( "Ground pin not connected to ground net" ),
        wxT( "ground_pin_not_ground" ) );

ERC_ITEM ERC_ITEM::stackedPinName( ERCE_STACKED_PIN_SYNTAX,
        _HKI( "Pin name resembles stacked pin" ),
        wxT( "stacked_pin_name" ) );

ERC_ITEM ERC_ITEM::unresolvedVariable( ERCE_UNRESOLVED_VARIABLE,
        _HKI( "Unresolved text variable" ),
        wxT( "unresolved_variable" ) );

ERC_ITEM ERC_ITEM::undefinedNetclass( ERCE_UNDEFINED_NETCLASS,
        _HKI( "Undefined netclass" ),
        wxT( "undefined_netclass" ) );

ERC_ITEM ERC_ITEM::simulationModelIssues( ERCE_SIMULATION_MODEL,
        _HKI( "SPICE model issue" ),
        wxT( "simulation_model_issue" ) );

ERC_ITEM ERC_ITEM::wireDangling( ERCE_WIRE_DANGLING,
        _HKI( "Wires not connected to anything" ),
        wxT( "wire_dangling" ) );

ERC_ITEM ERC_ITEM::libSymbolIssues( ERCE_LIB_SYMBOL_ISSUES,
        _HKI( "Library symbol issue" ),
        wxT( "lib_symbol_issues" ) );

ERC_ITEM ERC_ITEM::libSymbolMismatch( ERCE_LIB_SYMBOL_MISMATCH,
        _HKI( "Symbol doesn't match copy in library" ),
        wxT( "lib_symbol_mismatch" ) );

ERC_ITEM ERC_ITEM::footprintLinkIssues( ERCE_FOOTPRINT_LINK_ISSUES,
        _HKI( "Footprint link issue" ),
        wxT( "footprint_link_issues" ) );

ERC_ITEM ERC_ITEM::footprintFilters( ERCE_FOOTPRINT_FILTERS,
        _HKI( "Assigned footprint doesn't match footprint filters" ),
        wxT( "footprint_filter" ) );

ERC_ITEM ERC_ITEM::unannotated( ERCE_UNANNOTATED,
        _HKI( "Symbol is not annotated" ),
        wxT( "unannotated" ) );

ERC_ITEM ERC_ITEM::extraUnits( ERCE_EXTRA_UNITS,
        _HKI( "Symbol has more units than are defined" ),
        wxT( "extra_units" ) );

ERC_ITEM ERC_ITEM::missingUnits( ERCE_MISSING_UNIT,
        _HKI( "Symbol has units that are not placed" ),
        wxT( "missing_unit" ) );

ERC_ITEM ERC_ITEM::missingInputPin( ERCE_MISSING_INPUT_PIN,
        _HKI( "Symbol has input pins that are not placed" ),
        wxT( "missing_input_pin" ) );

ERC_ITEM ERC_ITEM::missingBidiPin( ERCE_MISSING_BIDI_PIN,
        _HKI( "Symbol has bidirectional pins that are not placed" ),
        wxT( "missing_bidi_pin" ) );

ERC_ITEM ERC_ITEM::missingPowerInputPin( ERCE_MISSING_POWER_INPUT_PIN,
        _HKI( "Symbol has power input pins that are not placed" ),
        wxT( "missing_power_pin" ) );

ERC_ITEM ERC_ITEM::differentUnitValue( ERCE_DIFFERENT_UNIT_VALUE,
        _HKI( "Units of same symbol have different values" ),
        wxT( "unit_value_mismatch" ) );

ERC_ITEM ERC_ITEM::duplicateReference( ERCE_DUPLICATE_REFERENCE,
        _HKI( "Duplicate reference designators" ),
        wxT( "duplicate_reference" ) );

ERC_ITEM ERC_ITEM::busEntryNeeded( ERCE_BUS_ENTRY_NEEDED,
        _HKI( "Bus Entry needed" ),
        wxT( "bus_entry_needed" ) );

ERC_ITEM ERC_ITEM::unconnectedWireEndpoint( ERCE_UNCONNECTED_WIRE_ENDPOINT,
        _HKI( "Unconnected wire endpoint" ),
        wxT( "unconnected_wire_endpoint" ) );

std::vector<std::reference_wrapper<RC_ITEM>> ERC_ITEM::allItemTypes(
        {
            ERC_ITEM::heading_connections,
            ERC_ITEM::pinNotConnected,
            ERC_ITEM::pinNotDriven,
            ERC_ITEM::powerpinNotDriven,
            ERC_ITEM::noConnectConnected,
            ERC_ITEM::noConnectDangling,
            ERC_ITEM::labelDangling,
            ERC_ITEM::isolatedPinLabel,
            ERC_ITEM::singleGlobalLabel,
            ERC_ITEM::sameLocalGlobalLabel,
            ERC_ITEM::wireDangling,
            ERC_ITEM::busEntryNeeded,
            ERC_ITEM::endpointOffGrid,
            ERC_ITEM::fourWayJunction,
            ERC_ITEM::labelMultipleWires,
            ERC_ITEM::unconnectedWireEndpoint,

            ERC_ITEM::heading_conflicts,
            ERC_ITEM::duplicateReference,
            ERC_ITEM::pinTableWarning,
            ERC_ITEM::differentUnitValue,
            ERC_ITEM::differentUnitFootprint,
            ERC_ITEM::differentUnitNet,
            ERC_ITEM::duplicateSheetName,
            ERC_ITEM::hierLabelMismatch,
            ERC_ITEM::multipleNetNames,
            ERC_ITEM::busDefinitionConflict,
            ERC_ITEM::busToBusConflict,
            ERC_ITEM::busToNetConflict,
            ERC_ITEM::netNotBusMember,
            ERC_ITEM::groundPinNotGround,

            ERC_ITEM::heading_misc,
            ERC_ITEM::stackedPinName,
            ERC_ITEM::unannotated,
            ERC_ITEM::unresolvedVariable,
            ERC_ITEM::undefinedNetclass,
            ERC_ITEM::simulationModelIssues,
            ERC_ITEM::similarLabels,
            ERC_ITEM::similarPower,
            ERC_ITEM::similarLabelAndPower,
            // Commented out until the logic for this element is coded
            // TODO: Add bus label syntax checking
            //                 ERC_ITEM::busLabelSyntax,
            ERC_ITEM::libSymbolIssues,
            ERC_ITEM::libSymbolMismatch,
            ERC_ITEM::footprintLinkIssues,
            ERC_ITEM::footprintFilters,
            ERC_ITEM::extraUnits,
            ERC_ITEM::missingUnits,
            ERC_ITEM::missingInputPin,
            ERC_ITEM::missingBidiPin,
            ERC_ITEM::missingPowerInputPin,

            // ERC_ITEM types with no user-editable severities
            // NOTE: this MUST be the last grouping in the list!
            ERC_ITEM::heading_internal,
            ERC_ITEM::duplicatePinError,
            ERC_ITEM::pinTableWarning,
            ERC_ITEM::pinTableError,
            ERC_ITEM::genericWarning,
            ERC_ITEM::genericError
        } );


std::shared_ptr<ERC_ITEM> ERC_ITEM::Create( int aErrorCode )
{
    switch( aErrorCode )
    {
    case ERCE_DUPLICATE_SHEET_NAME:    return std::make_shared<ERC_ITEM>( duplicateSheetName );
    case ERCE_ENDPOINT_OFF_GRID:       return std::make_shared<ERC_ITEM>( endpointOffGrid );
    case ERCE_PIN_NOT_CONNECTED:       return std::make_shared<ERC_ITEM>( pinNotConnected );
    case ERCE_PIN_NOT_DRIVEN:          return std::make_shared<ERC_ITEM>( pinNotDriven );
    case ERCE_POWERPIN_NOT_DRIVEN:     return std::make_shared<ERC_ITEM>( powerpinNotDriven );
    case ERCE_DUPLICATE_PIN_ERROR:     return std::make_shared<ERC_ITEM>( duplicatePinError );
    case ERCE_PIN_TO_PIN_WARNING:      return std::make_shared<ERC_ITEM>( pinTableWarning );
    case ERCE_PIN_TO_PIN_ERROR:        return std::make_shared<ERC_ITEM>( pinTableError );
    case ERCE_GENERIC_WARNING:         return std::make_shared<ERC_ITEM>( genericWarning );
    case ERCE_GENERIC_ERROR:           return std::make_shared<ERC_ITEM>( genericError );
    case ERCE_HIERACHICAL_LABEL:       return std::make_shared<ERC_ITEM>( hierLabelMismatch );
    case ERCE_NOCONNECT_CONNECTED:     return std::make_shared<ERC_ITEM>( noConnectConnected );
    case ERCE_NOCONNECT_NOT_CONNECTED: return std::make_shared<ERC_ITEM>( noConnectDangling );
    case ERCE_FOUR_WAY_JUNCTION:       return std::make_shared<ERC_ITEM>( fourWayJunction );
    case ERCE_LABEL_MULTIPLE_WIRES:    return std::make_shared<ERC_ITEM>( labelMultipleWires );
    case ERCE_LABEL_NOT_CONNECTED:     return std::make_shared<ERC_ITEM>( labelDangling );
    case ERCE_SIMILAR_LABELS:          return std::make_shared<ERC_ITEM>( similarLabels );
    case ERCE_SIMILAR_POWER:           return std::make_shared<ERC_ITEM>( similarPower );
    case ERCE_SIMILAR_LABEL_AND_POWER: return std::make_shared<ERC_ITEM>( similarLabelAndPower );
    case ERCE_SINGLE_GLOBAL_LABEL:     return std::make_shared<ERC_ITEM>( singleGlobalLabel );
    case ERCE_SAME_LOCAL_GLOBAL_LABEL: return std::make_shared<ERC_ITEM>( sameLocalGlobalLabel );
    case ERCE_DIFFERENT_UNIT_FP:       return std::make_shared<ERC_ITEM>( differentUnitFootprint );
    case ERCE_DIFFERENT_UNIT_NET:      return std::make_shared<ERC_ITEM>( differentUnitNet );
    case ERCE_BUS_ALIAS_CONFLICT:      return std::make_shared<ERC_ITEM>( busDefinitionConflict );
    case ERCE_DRIVER_CONFLICT:         return std::make_shared<ERC_ITEM>( multipleNetNames );
    case ERCE_BUS_ENTRY_CONFLICT:      return std::make_shared<ERC_ITEM>( netNotBusMember );
    case ERCE_BUS_TO_BUS_CONFLICT:     return std::make_shared<ERC_ITEM>( busToBusConflict );
    case ERCE_BUS_TO_NET_CONFLICT:     return std::make_shared<ERC_ITEM>( busToNetConflict );
    case ERCE_GROUND_PIN_NOT_GROUND:   return std::make_shared<ERC_ITEM>( groundPinNotGround );
    case ERCE_LABEL_SINGLE_PIN:        return std::make_shared<ERC_ITEM>( isolatedPinLabel );
    case ERCE_UNRESOLVED_VARIABLE:     return std::make_shared<ERC_ITEM>( unresolvedVariable );
    case ERCE_UNDEFINED_NETCLASS:      return std::make_shared<ERC_ITEM>( undefinedNetclass );
    case ERCE_SIMULATION_MODEL:        return std::make_shared<ERC_ITEM>( simulationModelIssues );
    case ERCE_WIRE_DANGLING:           return std::make_shared<ERC_ITEM>( wireDangling );
    case ERCE_LIB_SYMBOL_ISSUES:       return std::make_shared<ERC_ITEM>( libSymbolIssues );
    case ERCE_LIB_SYMBOL_MISMATCH:     return std::make_shared<ERC_ITEM>( libSymbolMismatch );
    case ERCE_FOOTPRINT_LINK_ISSUES:   return std::make_shared<ERC_ITEM>( footprintLinkIssues );
    case ERCE_FOOTPRINT_FILTERS:       return std::make_shared<ERC_ITEM>( footprintFilters );
    case ERCE_UNANNOTATED:             return std::make_shared<ERC_ITEM>( unannotated );
    case ERCE_EXTRA_UNITS:             return std::make_shared<ERC_ITEM>( extraUnits );
    case ERCE_DIFFERENT_UNIT_VALUE:    return std::make_shared<ERC_ITEM>( differentUnitValue );
    case ERCE_DUPLICATE_REFERENCE:     return std::make_shared<ERC_ITEM>( duplicateReference );
    case ERCE_BUS_ENTRY_NEEDED:        return std::make_shared<ERC_ITEM>( busEntryNeeded );
    case ERCE_MISSING_UNIT:            return std::make_shared<ERC_ITEM>( missingUnits );
    case ERCE_MISSING_INPUT_PIN:       return std::make_shared<ERC_ITEM>( missingInputPin );
    case ERCE_MISSING_POWER_INPUT_PIN: return std::make_shared<ERC_ITEM>( missingPowerInputPin );
    case ERCE_MISSING_BIDI_PIN:        return std::make_shared<ERC_ITEM>( missingBidiPin );
    case ERCE_UNCONNECTED_WIRE_ENDPOINT: return std::make_shared<ERC_ITEM>( unconnectedWireEndpoint );
    case ERCE_STACKED_PIN_SYNTAX:      return std::make_shared<ERC_ITEM>( stackedPinName );
    case ERCE_UNSPECIFIED:
    default:
        wxFAIL_MSG( wxS( "Unknown ERC error code" ) );
        break;
    }

    return nullptr;
}

/**
 * Override of RC_TREE_MODEL::GetValue which returns item descriptions in a specific
 * SCH_SHEET_PATH context, if a context is available on the given SCH_MARKER or ERC_ITEM
 * targets.
 */
void ERC_TREE_MODEL::GetValue( wxVariant& aVariant, wxDataViewItem const& aItem,
                               unsigned int aCol ) const
{
    SCH_EDIT_FRAME*           schEditFrame = static_cast<SCH_EDIT_FRAME*>( m_editFrame );
    const RC_TREE_NODE*       node = ToNode( aItem );
    std::shared_ptr<ERC_ITEM> ercItem = std::static_pointer_cast<ERC_ITEM>( node->m_RcItem );
    MARKER_BASE*              marker = ercItem->GetParent();
    wxString                  msg;

    auto getItemDesc =
            [&]( EDA_ITEM* aCurrItem, SCH_SHEET_PATH& aSheet )
            {
                SCH_SHEET_PATH curSheet = schEditFrame->GetCurrentSheet();
                wxString       desc;

                if( aSheet != curSheet )
                {
                    schEditFrame->SetCurrentSheet( aSheet );
                    aSheet.UpdateAllScreenReferences();
                    {
                        desc = aCurrItem->GetItemDescription( m_editFrame, true );
                    }
                    schEditFrame->SetCurrentSheet( curSheet );
                    curSheet.UpdateAllScreenReferences();
                }
                else
                {
                    desc = aCurrItem->GetItemDescription( m_editFrame, true );
                }

                return desc;
            };

    switch( node->m_Type )
    {
    case RC_TREE_NODE::MARKER:
        if( marker )
        {
            SEVERITY severity = ercItem->GetParent()->GetSeverity();

            if( severity == RPT_SEVERITY_EXCLUSION )
            {
                if( schEditFrame->GetSeverity( ercItem->GetErrorCode() ) == RPT_SEVERITY_WARNING )
                    msg = _( "Excluded warning: " );
                else
                    msg = _( "Excluded error: " );
            }
            else if( severity == RPT_SEVERITY_WARNING )
            {
                msg = _( "Warning: " );
            }
            else
            {
                msg = _( "Error: " );
            }
        }

        msg += ercItem->GetErrorMessage( true );
        break;

    case RC_TREE_NODE::MAIN_ITEM:
        if( marker && marker->GetMarkerType() == MARKER_BASE::MARKER_DRAWING_SHEET )
        {
            msg = _( "Drawing Sheet" );
        }
        else
        {
            msg = getItemDesc( schEditFrame->ResolveItem( ercItem->GetMainItemID() ),
                               ercItem->MainItemHasSheetPath() ? ercItem->GetMainItemSheetPath()
                                                               : schEditFrame->GetCurrentSheet() );
        }

        break;

    case RC_TREE_NODE::AUX_ITEM:
        msg = getItemDesc( schEditFrame->ResolveItem( ercItem->GetAuxItemID() ),
                           ercItem->AuxItemHasSheetPath() ? ercItem->GetAuxItemSheetPath()
                                                          : schEditFrame->GetCurrentSheet() );
        break;

    case RC_TREE_NODE::AUX_ITEM2:
        msg = getItemDesc( schEditFrame->ResolveItem( ercItem->GetAuxItem2ID() ),
                           schEditFrame->GetCurrentSheet() );
        break;

    case RC_TREE_NODE::AUX_ITEM3:
        msg = getItemDesc( schEditFrame->ResolveItem( ercItem->GetAuxItem3ID() ),
                           schEditFrame->GetCurrentSheet() );
        break;

    case RC_TREE_NODE::COMMENT:
        if( marker )
            msg = marker->GetComment();

        break;
    }

    msg.Replace( wxS( "\n" ), wxS( " " ) );
    aVariant = msg;
}
