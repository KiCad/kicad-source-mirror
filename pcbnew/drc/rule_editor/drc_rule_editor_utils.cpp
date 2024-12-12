/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "drc_rule_editor_utils.h"
#include <reporter.h>
#include <component_classes/component_class_assignment_rule.h>
#include <drc/drc_rule_parser.h>
#include <drc/drc_rule.h>
#include <drc/drc_rule_condition.h>
#include "drc_re_validator_numeric_ctrl.h"
#include "drc_re_validator_min_max_ctrl.h"
#include "drc_re_validator_min_preferred_max_ctrl.h"
#include "drc_re_validator_checkbox_list.h"
#include "drc_re_validator_combo_ctrl.h"
#include "drc_re_custom_rule_constraint_data.h"
#include "drc_re_via_style_constraint_data.h"
#include "drc_re_routing_width_constraint_data.h"
#include "drc_re_numeric_input_constraint_data.h"
#include "drc_re_bool_input_constraint_data.h"
#include <board.h>
#include <wx/ffile.h>
#include <wx/regex.h>
#include <unordered_map>

using CODE_MAP = std::unordered_map<DRC_RULE_EDITOR_CONSTRAINT_NAME, const char*>;
using REVERSE_CODE_MAP = std::unordered_map<wxString, DRC_RULE_EDITOR_CONSTRAINT_NAME, wxStringHash, wxStringEqual>;

static const CODE_MAP sCodeMap = { { BASIC_CLEARANCE, "clearance" },
                                   { BOARD_OUTLINE_CLEARANCE, "edge_clearance" },
                                   { MINIMUM_CLEARANCE, "clearance" },
                                   { MINIMUM_ITEM_CLEARANCE, "clearance" },
                                   { CREEPAGE_DISTANCE, "creepage" },
                                   { MINIMUM_CONNECTION_WIDTH, "connection_width" },
                                   { MINIMUM_TRACK_WIDTH, "track_width" },
                                   { COPPER_TO_HOLE_CLEARANCE, "hole_clearance" },
                                   { HOLE_TO_HOLE_CLEARANCE, "hole_to_hole" },
                                   { MINIMUM_THERMAL_RELIEF_SPOKE_COUNT, "thermal_spoke_width" },
                                   { MINIMUM_ANNULAR_WIDTH, "annular_width" },
                                   { COPPER_TO_EDGE_CLEARANCE, "edge_clearance" },
                                   { COURTYARD_CLEARANCE, "courtyard_clearance" },
                                   { PHYSICAL_CLEARANCE, "physical_clearance" },
                                   { MINIMUM_THROUGH_HOLE, "hole" },
                                   { HOLE_SIZE, "hole" },
                                   { HOLE_TO_HOLE_DISTANCE, "hole_to_hole" },
                                   { MINIMUM_UVIA_HOLE, "hole" },
                                   { MINIMUM_UVIA_DIAMETER, "via_diameter" },
                                   { MINIMUM_VIA_DIAMETER, "via_diameter" },
                                   { VIA_STYLE, "via_style" },
                                   { MINIMUM_TEXT_HEIGHT_AND_THICKNESS, "text_height" },
                                   { SILK_TO_SILK_CLEARANCE, "silk_clearance" },
                                   { SILK_TO_SOLDERMASK_CLEARANCE, "silk_clearance" },
                                   { MINIMUM_SOLDERMASK_SILVER, "solder_mask_sliver" },
                                   { SOLDERMASK_EXPANSION, "solder_mask_expansion" },
                                   { SOLDERPASTE_EXPANSION, "solder_paste_abs_margin" },
                                   { MAXIMUM_ALLOWED_DEVIATION, "maximum_allowed_deviation" },
                                   { MINIMUM_ANGULAR_RING, "annular_width" },
                                   { MATCHED_LENGTH_DIFF_PAIR, "length" },
                                   { ROUTING_DIFF_PAIR, "diff_pair_gap" },
                                   { ROUTING_WIDTH, "track_width" },
                                   { MAXIMUM_VIA_COUNT, "via_count" },
                                   { ABSOLUTE_LENGTH, "length" },
                                   { PERMITTED_LAYERS, "permitted_layers" },
                                   { ALLOWED_ORIENTATION, "allowed_orientation" },
                                   { VIAS_UNDER_SMD, "vias_under_smd" } };

static const REVERSE_CODE_MAP sCodeReverse = []
{
    REVERSE_CODE_MAP map;
    for( const auto& [type, code] : sCodeMap )
        map.emplace( wxString::FromUTF8( code ), type );
    return map;
}();

static std::vector<DRC_RULE_EDITOR_UTILS::RuleConverter>& GetRuleConverters()
{
    static std::vector<DRC_RULE_EDITOR_UTILS::RuleConverter> converters;
    return converters;
}

void DRC_RULE_EDITOR_UTILS::RegisterRuleConverter( RuleConverter aConverter )
{
    GetRuleConverters().insert( GetRuleConverters().begin(), aConverter );
}

static wxString GetConstraintCodeFromType( DRC_CONSTRAINT_T aType )
{
    switch( aType )
    {
    case CLEARANCE_CONSTRAINT:               return "clearance";
    case EDGE_CLEARANCE_CONSTRAINT:          return "edge_clearance";
    case HOLE_CLEARANCE_CONSTRAINT:          return "hole_clearance";
    case HOLE_TO_HOLE_CONSTRAINT:            return "hole_to_hole";
    case HOLE_SIZE_CONSTRAINT:               return "hole_size";
    case TRACK_WIDTH_CONSTRAINT:             return "track_width";
    case ANNULAR_WIDTH_CONSTRAINT:           return "annular_width";
    case VIA_DIAMETER_CONSTRAINT:            return "via_diameter";
    case DISALLOW_CONSTRAINT:                return "disallow";
    case COURTYARD_CLEARANCE_CONSTRAINT:     return "courtyard_clearance";
    case SILK_CLEARANCE_CONSTRAINT:          return "silk_clearance";
    case TEXT_HEIGHT_CONSTRAINT:             return "text_height";
    case TEXT_THICKNESS_CONSTRAINT:          return "text_thickness";
    case TRACK_ANGLE_CONSTRAINT:             return "track_angle";
    case TRACK_SEGMENT_LENGTH_CONSTRAINT:    return "track_segment_length";
    case CONNECTION_WIDTH_CONSTRAINT:        return "connection_width";
    case VIA_DANGLING_CONSTRAINT:            return "via_dangling";
    case ZONE_CONNECTION_CONSTRAINT:         return "zone_connection";
    case THERMAL_RELIEF_GAP_CONSTRAINT:      return "thermal_relief_gap";
    case THERMAL_SPOKE_WIDTH_CONSTRAINT:     return "thermal_spoke_width";
    case MIN_RESOLVED_SPOKES_CONSTRAINT:     return "min_resolved_spokes";
    case SOLDER_MASK_EXPANSION_CONSTRAINT:   return "solder_mask_expansion";
    case SOLDER_PASTE_ABS_MARGIN_CONSTRAINT: return "solder_paste_abs_margin";
    case SOLDER_PASTE_REL_MARGIN_CONSTRAINT: return "solder_paste_rel_margin";
    case LENGTH_CONSTRAINT:                  return "length";
    case SKEW_CONSTRAINT:                    return "skew";
    case VIA_COUNT_CONSTRAINT:               return "via_count";
    case DIFF_PAIR_GAP_CONSTRAINT:           return "diff_pair_gap";
    case MAX_UNCOUPLED_CONSTRAINT:           return "diff_pair_uncoupled";
    case PHYSICAL_CLEARANCE_CONSTRAINT:      return "physical_clearance";
    case PHYSICAL_HOLE_CLEARANCE_CONSTRAINT: return "physical_hole_clearance";
    case BRIDGED_MASK_CONSTRAINT:            return "bridged_mask";
    case ASSERTION_CONSTRAINT:               return "assertion";
    default:                                 return "";
    }
}

static void RegisterDefaultConverters()
{
    static bool initialized = false;
    if( initialized )
        return;
    initialized = true;

    // Generic Converter
    DRC_RULE_EDITOR_UTILS::RegisterRuleConverter(
        []( const std::shared_ptr<DRC_RULE>& aRule ) -> std::shared_ptr<DRC_RE_BASE_CONSTRAINT_DATA>
        {
            if( aRule->m_Constraints.empty() ) return nullptr;

            const auto& constraint = aRule->m_Constraints[0];
            wxString code = GetConstraintCodeFromType( constraint.m_Type );

            if( code.IsEmpty() ) return nullptr;

            if( sCodeReverse.count( code ) )
            {
                auto type = sCodeReverse.at( code );

                if( DRC_RULE_EDITOR_UTILS::IsNumericInputType( type ) )
                {
                    double val = constraint.GetValue().Min() / 1000000.0;
                    auto data = std::make_shared<DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA>( 0, 0, val, aRule->m_Name );
                    data->SetConstraintCode( code );
                    return data;
                }
                else if( DRC_RULE_EDITOR_UTILS::IsBoolInputType( type ) )
                {
                    auto data = std::make_shared<DRC_RE_BOOL_INPUT_CONSTRAINT_DATA>( 0, 0, true, aRule->m_Name );
                    data->SetConstraintCode( code );
                    return data;
                }
                else if( code == "track_width" )
                {
                     double minW = constraint.GetValue().Min() / 1000000.0;
                     double optW = constraint.GetValue().Opt() / 1000000.0;
                     double maxW = constraint.GetValue().Max() / 1000000.0;

                     auto data = std::make_shared<DRC_RE_ROUTING_WIDTH_CONSTRAINT_DATA>( 0, 0, aRule->m_Name, minW, optW, maxW );
                     data->SetConstraintCode( code );
                     return data;
                }
            }
            return nullptr;
        } );

    // Via Style Converter
    DRC_RULE_EDITOR_UTILS::RegisterRuleConverter(
        []( const std::shared_ptr<DRC_RULE>& aRule ) -> std::shared_ptr<DRC_RE_BASE_CONSTRAINT_DATA>
        {
            auto diaConstraint = aRule->FindConstraint( VIA_DIAMETER_CONSTRAINT );
            auto holeConstraint = aRule->FindConstraint( HOLE_SIZE_CONSTRAINT );

            if( diaConstraint && holeConstraint )
            {
                double minDia = diaConstraint->GetValue().Min() / 1000000.0;
                double optDia = diaConstraint->GetValue().Opt() / 1000000.0;
                double maxDia = diaConstraint->GetValue().Max() / 1000000.0;

                double minDrill = holeConstraint->GetValue().Min() / 1000000.0;
                double optDrill = holeConstraint->GetValue().Opt() / 1000000.0;
                double maxDrill = holeConstraint->GetValue().Max() / 1000000.0;

                auto data = std::make_shared<DRC_RE_VIA_STYLE_CONSTRAINT_DATA>( 0, 0, aRule->m_Name,
                    minDia, maxDia, optDia, minDrill, maxDrill, optDrill );
                data->SetConstraintCode( "via_style" );
                return data;
            }
            return nullptr;
        } );
}


wxString DRC_RULE_EDITOR_UTILS::GetConstraintCode( DRC_RULE_EDITOR_CONSTRAINT_NAME aConstraintType )
{
    auto it = sCodeMap.find( aConstraintType );
    if( it != sCodeMap.end() )
        return wxString::FromUTF8( it->second );

    return wxString();
}


std::optional<DRC_RULE_EDITOR_CONSTRAINT_NAME> DRC_RULE_EDITOR_UTILS::GetConstraintTypeFromCode( const wxString& aCode )
{
    auto it = sCodeReverse.find( aCode );
    if( it != sCodeReverse.end() )
        return it->second;

    return std::nullopt;
}


wxString DRC_RULE_EDITOR_UTILS::ConstraintToKicadDrc( DRC_RULE_EDITOR_CONSTRAINT_NAME aType )
{
    return GetConstraintCode( aType );
}


bool DRC_RULE_EDITOR_UTILS::ConstraintFromKicadDrc( const wxString& aCode, DRC_RE_BASE_CONSTRAINT_DATA* aData )
{
    if( !aData )
        return false;

    auto type = GetConstraintTypeFromCode( aCode );
    if( type )
    {
        aData->SetConstraintCode( GetConstraintCode( *type ) );
        return true;
    }

    aData->SetConstraintCode( aCode );
    return false;
}


bool DRC_RULE_EDITOR_UTILS::IsBoolInputType( const DRC_RULE_EDITOR_CONSTRAINT_NAME& aConstraintType )
{
    switch( aConstraintType )
    {
    case VIAS_UNDER_SMD: return true;
    default: return false;
    }
}


bool DRC_RULE_EDITOR_UTILS::IsNumericInputType( const DRC_RULE_EDITOR_CONSTRAINT_NAME& aConstraintType )
{
    switch( aConstraintType )
    {
    case BASIC_CLEARANCE:
    case BOARD_OUTLINE_CLEARANCE:
    case COPPER_TO_EDGE_CLEARANCE:
    case COPPER_TO_HOLE_CLEARANCE:
    case COURTYARD_CLEARANCE:
    case CREEPAGE_DISTANCE:
    case HOLE_SIZE:
    case HOLE_TO_HOLE_CLEARANCE:
    case HOLE_TO_HOLE_DISTANCE:
    case MATCHED_LENGTH_DIFF_PAIR:
    case MAXIMUM_ALLOWED_DEVIATION:
    case MAXIMUM_VIA_COUNT:
    case MINIMUM_ANGULAR_RING:
    case MINIMUM_ANNULAR_WIDTH:
    case MINIMUM_CLEARANCE:
    case MINIMUM_CONNECTION_WIDTH:
    case MINIMUM_ITEM_CLEARANCE:
    case MINIMUM_SOLDERMASK_SILVER:
    case MINIMUM_THERMAL_RELIEF_SPOKE_COUNT:
    case MINIMUM_THROUGH_HOLE:
    case MINIMUM_TRACK_WIDTH:
    case MINIMUM_UVIA_DIAMETER:
    case MINIMUM_UVIA_HOLE:
    case MINIMUM_VIA_DIAMETER:
    case SILK_TO_SILK_CLEARANCE:
    case SILK_TO_SOLDERMASK_CLEARANCE:
    case SOLDERMASK_EXPANSION:
    case SOLDERPASTE_EXPANSION:
        return true;
    default:
        return false;
    }
}


bool DRC_RULE_EDITOR_UTILS::ValidateNumericCtrl( wxTextCtrl* aTextCtrl, std::string aLabel, bool aCanBeZero,
                                                 int* aErrorCount, std::string* aValidationMessage )
{
    VALIDATOR_NUMERIC_CTRL validator( aCanBeZero );
    aTextCtrl->SetValidator( validator );

    if( !aTextCtrl->Validate() )
    {
        VALIDATOR_NUMERIC_CTRL* v = static_cast<VALIDATOR_NUMERIC_CTRL*>( aTextCtrl->GetValidator() );

        switch( v->GetValidationState() )
        {
        case VALIDATOR_NUMERIC_CTRL::VALIDATION_STATE::Empty:
        {
            ( *aErrorCount )++;
            *aValidationMessage +=
                    DRC_RULE_EDITOR_UTILS::FormatErrorMessage( *aErrorCount, aLabel + " should not be empty !!" );
            return false;
        }
        case VALIDATOR_NUMERIC_CTRL::VALIDATION_STATE::NotNumeric:
        {
            ( *aErrorCount )++;
            *aValidationMessage += DRC_RULE_EDITOR_UTILS::FormatErrorMessage(
                    *aErrorCount, "The value of " + aLabel + " should be valid numeric value !!" );
            return false;
        }
        case VALIDATOR_NUMERIC_CTRL::VALIDATION_STATE::NotGreaterThanZero:
        {
            ( *aErrorCount )++;
            *aValidationMessage += DRC_RULE_EDITOR_UTILS::FormatErrorMessage(
                    *aErrorCount, "The value of " + aLabel + " must be greater than 0 !!" );
            return false;
        }
        default: break;
        }
    }

    return true;
}


bool DRC_RULE_EDITOR_UTILS::ValidateIntegerCtrl( wxTextCtrl* aTextCtrl, std::string aLabel, bool aCanBeZero,
                                                 int* aErrorCount, std::string* aValidationMessage )
{
    VALIDATOR_NUMERIC_CTRL validator( aCanBeZero, true );
    aTextCtrl->SetValidator( validator );

    if( !aTextCtrl->Validate() )
    {
        VALIDATOR_NUMERIC_CTRL* v = static_cast<VALIDATOR_NUMERIC_CTRL*>( aTextCtrl->GetValidator() );

        switch( v->GetValidationState() )
        {
        case VALIDATOR_NUMERIC_CTRL::VALIDATION_STATE::Empty:
        {
            ( *aErrorCount )++;
            *aValidationMessage +=
                    DRC_RULE_EDITOR_UTILS::FormatErrorMessage( *aErrorCount, aLabel + " should not be empty !!" );
            return false;
        }
        case VALIDATOR_NUMERIC_CTRL::VALIDATION_STATE::NotInteger:
        {
            ( *aErrorCount )++;
            *aValidationMessage += DRC_RULE_EDITOR_UTILS::FormatErrorMessage(
                    *aErrorCount, "The value of " + aLabel + " should be valid integer value !!" );
            return false;
        }
        case VALIDATOR_NUMERIC_CTRL::VALIDATION_STATE::NotGreaterThanZero:
        {
            ( *aErrorCount )++;
            *aValidationMessage += DRC_RULE_EDITOR_UTILS::FormatErrorMessage(
                    *aErrorCount, "The value of " + aLabel + " must be greater than 0 !!" );
            return false;
        }
        default: break;
        }
    }

    return true;
}


bool DRC_RULE_EDITOR_UTILS::ValidateComboCtrl( wxComboBox* aComboBox, std::string aLabel, int* aErrorCount,
                                               std::string* aValidationMessage )
{
    VALIDATOR_COMBO_CTRL cmbCtrlValidator;
    aComboBox->SetValidator( cmbCtrlValidator );

    if( !aComboBox->Validate() )
    {
        VALIDATOR_COMBO_CTRL* v = static_cast<VALIDATOR_COMBO_CTRL*>( aComboBox->GetValidator() );

        switch( v->GetValidationState() )
        {
        case VALIDATOR_COMBO_CTRL::VALIDATION_STATE::NothingSelected:
        {
            ( *aErrorCount )++;
            *aValidationMessage += DRC_RULE_EDITOR_UTILS::FormatErrorMessage( *aErrorCount, "Please choose " + aLabel );
            return false;
        }
        default: break;
        }
    }

    return true;
}


bool DRC_RULE_EDITOR_UTILS::ValidateMinMaxCtrl( wxTextCtrl* aMinTextCtrl, wxTextCtrl* aMaxTextCtrl,
                                                std::string aMinLabel, std::string aMaxLabel, int* aErrorCount,
                                                std::string* aValidationMessage )
{
    aMinTextCtrl->SetName( "min" );
    aMaxTextCtrl->SetName( "max" );

    aMinTextCtrl->SetValidator( VALIDATE_MIN_MAX_CTRL( aMinTextCtrl, aMaxTextCtrl ) );

    if( !aMinTextCtrl->Validate() )
    {
        VALIDATE_MIN_MAX_CTRL* v = static_cast<VALIDATE_MIN_MAX_CTRL*>( aMinTextCtrl->GetValidator() );

        switch( v->GetValidationState() )
        {
        case VALIDATE_MIN_MAX_CTRL::VALIDATION_STATE::MinGreaterThanMax:
        {
            ( *aErrorCount )++;
            *aValidationMessage += DRC_RULE_EDITOR_UTILS::FormatErrorMessage(
                    *aErrorCount, aMinLabel + " value cannot be greater than " + aMaxLabel + " value" );
            return false;
        }
        default: break;
        }
    }

    aMinTextCtrl->SetName( "text" );
    aMaxTextCtrl->SetName( "text" );

    return true;
}


bool DRC_RULE_EDITOR_UTILS::ValidateMinPreferredMaxCtrl( wxTextCtrl* aMinTextCtrl, wxTextCtrl* aPreferredTextCtrl,
                                                         wxTextCtrl* aMaxTextCtrl, std::string aMinLabel,
                                                         std::string aPreferredLabel, std::string aMaxLabel,
                                                         int* aErrorCount, std::string* aValidationMessage )
{
    aMinTextCtrl->SetName( "min" );
    aPreferredTextCtrl->SetName( "preferred" );
    aMaxTextCtrl->SetName( "max" );

    aMinTextCtrl->SetValidator( VALIDATE_MIN_PREFERRED_MAX_CTRL( aMinTextCtrl, aPreferredTextCtrl, aMaxTextCtrl ) );

    if( !aMinTextCtrl->Validate() )
    {
        VALIDATE_MIN_PREFERRED_MAX_CTRL* v =
                static_cast<VALIDATE_MIN_PREFERRED_MAX_CTRL*>( aMinTextCtrl->GetValidator() );

        switch( v->GetValidationState() )
        {
        case VALIDATE_MIN_PREFERRED_MAX_CTRL::VALIDATION_STATE::MinGreaterThanPreferred:
        {
            ( *aErrorCount )++;
            *aValidationMessage += DRC_RULE_EDITOR_UTILS::FormatErrorMessage(
                    *aErrorCount, aMinLabel + " value cannot be greater than " + aPreferredLabel + " value" );
            return false;
        }
        case VALIDATE_MIN_PREFERRED_MAX_CTRL::VALIDATION_STATE::PreferredGreaterThanMax:
        {
            ( *aErrorCount )++;
            *aValidationMessage += DRC_RULE_EDITOR_UTILS::FormatErrorMessage(
                    *aErrorCount, aPreferredLabel + " value cannot be greater than " + aMaxLabel + " value" );
            return false;
        }
        case VALIDATE_MIN_PREFERRED_MAX_CTRL::VALIDATION_STATE::MinGreaterThanMax:
        {
            ( *aErrorCount )++;
            *aValidationMessage += DRC_RULE_EDITOR_UTILS::FormatErrorMessage(
                    *aErrorCount, aMinLabel + " value cannot be greater than " + aMaxLabel + " value" );
            return false;
        }
        default: break;
        }
    }

    aMinTextCtrl->SetName( "text" );
    aPreferredTextCtrl->SetName( "text" );
    aMaxTextCtrl->SetName( "text" );

    return true;
}


bool DRC_RULE_EDITOR_UTILS::ValidateCheckBoxCtrls( const std::vector<wxCheckBox*>& aCheckboxes, std::string aLabel,
                                                   int* aErrorCount, std::string* aValidationMessage )
{
    VALIDATE_CHECKBOX_LIST validator( aCheckboxes );

    aCheckboxes[0]->SetValidator( validator );

    if( !aCheckboxes[0]->Validate() )
    {
        VALIDATE_CHECKBOX_LIST* v = static_cast<VALIDATE_CHECKBOX_LIST*>( aCheckboxes[0]->GetValidator() );

        switch( v->GetValidationState() )
        {
        case VALIDATE_CHECKBOX_LIST::VALIDATION_STATE::NotSelected:
        {
            ( *aErrorCount )++;
            *aValidationMessage += DRC_RULE_EDITOR_UTILS::FormatErrorMessage(
                    *aErrorCount, "Please select at least one option from " + aLabel + " list" );
            return false;
        }
        default: break;
        }
    }

    return true;
}


std::string DRC_RULE_EDITOR_UTILS::FormatErrorMessage( const int& aErrorCount, const std::string aErrorMessage )
{
    return std::to_string( aErrorCount ) + ". " + aErrorMessage + "\n";
}


// ==================== Pure Validators (No GUI Dependencies) ====================

bool DRC_RULE_EDITOR_UTILS::ValidateNumericValue( double aValue, bool aCanBeZero, const std::string& aLabel,
                                                   VALIDATION_RESULT* aResult )
{
    if( !aCanBeZero && aValue <= 0.0 )
    {
        aResult->AddError( "The value of " + aLabel + " must be greater than 0" );
        return false;
    }

    return true;
}


bool DRC_RULE_EDITOR_UTILS::ValidateNumericString( const std::string& aValueStr, bool aCanBeZero,
                                                    bool aIntegerOnly, const std::string& aLabel,
                                                    VALIDATION_RESULT* aResult )
{
    if( aValueStr.empty() )
    {
        aResult->AddError( aLabel + " should not be empty" );
        return false;
    }

    try
    {
        if( aIntegerOnly )
        {
            size_t pos;
            long   intVal = std::stol( aValueStr, &pos );

            if( pos != aValueStr.length() )
            {
                aResult->AddError( "The value of " + aLabel + " should be a valid integer value" );
                return false;
            }

            if( !aCanBeZero && intVal <= 0 )
            {
                aResult->AddError( "The value of " + aLabel + " must be greater than 0" );
                return false;
            }
        }
        else
        {
            size_t pos;
            double floatVal = std::stod( aValueStr, &pos );

            if( pos != aValueStr.length() )
            {
                aResult->AddError( "The value of " + aLabel + " should be a valid numeric value" );
                return false;
            }

            if( !aCanBeZero && floatVal <= 0.0 )
            {
                aResult->AddError( "The value of " + aLabel + " must be greater than 0" );
                return false;
            }
        }
    }
    catch( const std::exception& )
    {
        if( aIntegerOnly )
            aResult->AddError( "The value of " + aLabel + " should be a valid integer value" );
        else
            aResult->AddError( "The value of " + aLabel + " should be a valid numeric value" );

        return false;
    }

    return true;
}


bool DRC_RULE_EDITOR_UTILS::ValidateMinMax( double aMin, double aMax, const std::string& aMinLabel,
                                             const std::string& aMaxLabel, VALIDATION_RESULT* aResult )
{
    if( aMin > aMax )
    {
        aResult->AddError( aMinLabel + " value cannot be greater than " + aMaxLabel + " value" );
        return false;
    }

    return true;
}


bool DRC_RULE_EDITOR_UTILS::ValidateMinPreferredMax( double aMin, double aPreferred, double aMax,
                                                      const std::string& aMinLabel,
                                                      const std::string& aPrefLabel,
                                                      const std::string& aMaxLabel,
                                                      VALIDATION_RESULT* aResult )
{
    bool valid = true;

    if( aMin > aPreferred )
    {
        aResult->AddError( aMinLabel + " value cannot be greater than " + aPrefLabel + " value" );
        valid = false;
    }

    if( aPreferred > aMax )
    {
        aResult->AddError( aPrefLabel + " value cannot be greater than " + aMaxLabel + " value" );
        valid = false;
    }

    if( aMin > aMax )
    {
        aResult->AddError( aMinLabel + " value cannot be greater than " + aMaxLabel + " value" );
        valid = false;
    }

    return valid;
}


bool DRC_RULE_EDITOR_UTILS::ValidateAtLeastOneSelected( const std::vector<bool>& aSelected,
                                                         const std::string&       aLabel,
                                                         VALIDATION_RESULT*       aResult )
{
    for( bool selected : aSelected )
    {
        if( selected )
            return true;
    }

    aResult->AddError( "Please select at least one option from " + aLabel + " list" );
    return false;
}


bool DRC_RULE_EDITOR_UTILS::ValidateSelection( int aSelectionIndex, const std::string& aLabel,
                                                VALIDATION_RESULT* aResult )
{
    if( aSelectionIndex < 0 )
    {
        aResult->AddError( "Please choose " + aLabel );
        return false;
    }

    return true;
}


std::vector<std::shared_ptr<DRC_RE_BASE_CONSTRAINT_DATA>> DRC_RULE_EDITOR_UTILS::ParseRules( const wxString& aRules )
{
    RegisterDefaultConverters();

    std::vector<std::shared_ptr<DRC_RE_BASE_CONSTRAINT_DATA>> rules;
    std::vector<std::shared_ptr<DRC_RULE>> parsedRules;

    wxString rulesText = aRules;
    if( !rulesText.Contains( "(version" ) )
        rulesText.Prepend( "(version 1)\n" );

    try
    {
        DRC_RULES_PARSER parser( rulesText, "Rule Editor Source" );
        parser.Parse( parsedRules, nullptr );
    }
    catch( const IO_ERROR& )
    {
    }

    for( const auto& rule : parsedRules )
    {
        std::shared_ptr<DRC_RE_BASE_CONSTRAINT_DATA> data;

        for( const auto& converter : GetRuleConverters() )
        {
            data = converter( rule );
            if( data )
                break;
        }

        if( !data )
        {
            auto customData = std::make_shared<DRC_RE_CUSTOM_RULE_CONSTRAINT_DATA>( 0, 0, rule->m_Name );
            customData->SetRuleText( wxString::Format( "(rule \"%s\" ...)", rule->m_Name ) );
            data = customData;
        }

        if( rule->m_Condition )
            data->SetRuleCondition( rule->m_Condition->GetExpression() );

        rules.push_back( data );
    }

    return rules;
}

bool DRC_RULE_EDITOR_UTILS::SaveRules( const wxString& aFilename,
                                       const std::vector<std::shared_ptr<DRC_RE_BASE_CONSTRAINT_DATA>>& aRules,
                                       const BOARD* aBoard )
{
    wxFFile file( aFilename, "w" );

    if( !file.IsOpened() )
        return false;

    file.Write( "(version 1)\n" );

    for( const auto& data : aRules )
    {
        if( !data )
            continue;

        RULE_GENERATION_CONTEXT ctx;
        ctx.ruleName = data->GetRuleName();
        ctx.comment = data->GetComment();
        ctx.conditionExpression = data->GetRuleCondition();
        ctx.constraintCode = data->GetConstraintCode();

        std::vector<PCB_LAYER_ID> layers = data->GetLayers();

        if( !layers.empty() && aBoard )
        {
            wxString layerStr = "(layer";

            for( PCB_LAYER_ID layer : layers )
                layerStr += " \"" + aBoard->GetLayerName( layer ) + "\"";

            layerStr += ")";
            ctx.layerClause = layerStr;
        }

        wxString ruleText = data->GenerateRule( ctx );
        file.Write( ruleText + "\n" );
    }

    file.Close();
    return true;
}
