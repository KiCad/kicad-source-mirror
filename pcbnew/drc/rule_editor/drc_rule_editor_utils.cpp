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
#include "drc_re_numeric_constraint_types.h"
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

static const CODE_MAP sCodeMap = { { MINIMUM_CLEARANCE, "clearance" },
                                   { CREEPAGE_DISTANCE, "creepage" },
                                   { MINIMUM_CONNECTION_WIDTH, "connection_width" },
                                   { COPPER_TO_HOLE_CLEARANCE, "hole_clearance" },
                                   { HOLE_TO_HOLE_CLEARANCE, "hole_to_hole" },
                                   { MINIMUM_THERMAL_RELIEF_SPOKE_COUNT, "min_resolved_spokes" },
                                   { MINIMUM_ANNULAR_WIDTH, "annular_width" },
                                   { COPPER_TO_EDGE_CLEARANCE, "edge_clearance" },
                                   { COURTYARD_CLEARANCE, "courtyard_clearance" },
                                   { PHYSICAL_CLEARANCE, "physical_clearance" },
                                   { MINIMUM_THROUGH_HOLE, "hole_size" },
                                   { HOLE_SIZE, "hole_size" },
                                   { HOLE_TO_HOLE_DISTANCE, "hole_to_hole" },
                                   { MINIMUM_UVIA_HOLE, "hole_size" },
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
                                   { VIAS_UNDER_SMD, "disallow via" } };

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
    case SOLDER_MASK_SLIVER_CONSTRAINT:      return "solder_mask_sliver";
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
                    auto data = DRC_RULE_EDITOR_UTILS::CreateNumericConstraintData( type );
                    data->SetRuleName( aRule->m_Name );
                    data->SetNumericInputValue( val );
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
    case COPPER_TO_EDGE_CLEARANCE:
    case COPPER_TO_HOLE_CLEARANCE:
    case COURTYARD_CLEARANCE:
    case PHYSICAL_CLEARANCE:
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
    case MINIMUM_SOLDERMASK_SILVER:
    case MINIMUM_THERMAL_RELIEF_SPOKE_COUNT:
    case MINIMUM_THROUGH_HOLE:
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


bool DRC_RULE_EDITOR_UTILS::ValidateNumericCtrl( wxTextCtrl* aTextCtrl, const wxString& aLabel, bool aCanBeZero,
                                                 int* aErrorCount, wxString* aValidationMessage )
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
                    DRC_RULE_EDITOR_UTILS::FormatErrorMessage( *aErrorCount,
                            wxString::Format( _( "%s should not be empty." ), aLabel ) );
            return false;
        }
        case VALIDATOR_NUMERIC_CTRL::VALIDATION_STATE::NotNumeric:
        {
            ( *aErrorCount )++;
            *aValidationMessage += DRC_RULE_EDITOR_UTILS::FormatErrorMessage(
                    *aErrorCount, wxString::Format( _( "The value of %s must be a valid number." ), aLabel ) );
            return false;
        }
        case VALIDATOR_NUMERIC_CTRL::VALIDATION_STATE::NotGreaterThanZero:
        {
            ( *aErrorCount )++;
            *aValidationMessage += DRC_RULE_EDITOR_UTILS::FormatErrorMessage(
                    *aErrorCount, wxString::Format( _( "The value of %s must be greater than 0." ), aLabel ) );
            return false;
        }
        default: break;
        }
    }

    return true;
}


bool DRC_RULE_EDITOR_UTILS::ValidateIntegerCtrl( wxTextCtrl* aTextCtrl, const wxString& aLabel, bool aCanBeZero,
                                                 int* aErrorCount, wxString* aValidationMessage )
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
                    DRC_RULE_EDITOR_UTILS::FormatErrorMessage( *aErrorCount,
                            wxString::Format( _( "%s should not be empty." ), aLabel ) );
            return false;
        }
        case VALIDATOR_NUMERIC_CTRL::VALIDATION_STATE::NotInteger:
        {
            ( *aErrorCount )++;
            *aValidationMessage += DRC_RULE_EDITOR_UTILS::FormatErrorMessage(
                    *aErrorCount, wxString::Format( _( "The value of %s must be a valid integer." ), aLabel ) );
            return false;
        }
        case VALIDATOR_NUMERIC_CTRL::VALIDATION_STATE::NotGreaterThanZero:
        {
            ( *aErrorCount )++;
            *aValidationMessage += DRC_RULE_EDITOR_UTILS::FormatErrorMessage(
                    *aErrorCount, wxString::Format( _( "The value of %s must be greater than 0." ), aLabel ) );
            return false;
        }
        default: break;
        }
    }

    return true;
}


bool DRC_RULE_EDITOR_UTILS::ValidateComboCtrl( wxComboBox* aComboBox, const wxString& aLabel, int* aErrorCount,
                                               wxString* aValidationMessage )
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
            *aValidationMessage += DRC_RULE_EDITOR_UTILS::FormatErrorMessage( *aErrorCount,
                    wxString::Format( _( "Please choose %s." ), aLabel ) );
            return false;
        }
        default: break;
        }
    }

    return true;
}


bool DRC_RULE_EDITOR_UTILS::ValidateMinMaxCtrl( wxTextCtrl* aMinTextCtrl, wxTextCtrl* aMaxTextCtrl,
                                                const wxString& aMinLabel, const wxString& aMaxLabel,
                                                int* aErrorCount, wxString* aValidationMessage )
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
                    *aErrorCount, wxString::Format( _( "%s value cannot be greater than %s value." ),
                                                    aMinLabel, aMaxLabel ) );
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
                                                         wxTextCtrl* aMaxTextCtrl, const wxString& aMinLabel,
                                                         const wxString& aPreferredLabel, const wxString& aMaxLabel,
                                                         int* aErrorCount, wxString* aValidationMessage )
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
                    *aErrorCount, wxString::Format( _( "%s value cannot be greater than %s value." ),
                                                    aMinLabel, aPreferredLabel ) );
            return false;
        }
        case VALIDATE_MIN_PREFERRED_MAX_CTRL::VALIDATION_STATE::PreferredGreaterThanMax:
        {
            ( *aErrorCount )++;
            *aValidationMessage += DRC_RULE_EDITOR_UTILS::FormatErrorMessage(
                    *aErrorCount, wxString::Format( _( "%s value cannot be greater than %s value." ),
                                                    aPreferredLabel, aMaxLabel ) );
            return false;
        }
        case VALIDATE_MIN_PREFERRED_MAX_CTRL::VALIDATION_STATE::MinGreaterThanMax:
        {
            ( *aErrorCount )++;
            *aValidationMessage += DRC_RULE_EDITOR_UTILS::FormatErrorMessage(
                     *aErrorCount, wxString::Format( _( "%s value cannot be greater than %s value." ),
                                                    aMinLabel, aMaxLabel ) );
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


bool DRC_RULE_EDITOR_UTILS::ValidateCheckBoxCtrls( const std::vector<wxCheckBox*>& aCheckboxes, const wxString& aLabel,
                                                   int* aErrorCount, wxString* aValidationMessage )
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
                    *aErrorCount, wxString::Format( _( "Please select at least one option from %s list." ), aLabel ));
            return false;
        }
        default: break;
        }
    }

    return true;
}


wxString DRC_RULE_EDITOR_UTILS::FormatErrorMessage( int aErrorCount, const wxString& aErrorMessage )
{
    return wxString::Format( wxS( "%d. %s\n" ), aErrorCount, aErrorMessage );
}


// ==================== Pure Validators (No GUI Dependencies) ====================

bool DRC_RULE_EDITOR_UTILS::ValidateNumericValue( double aValue, bool aCanBeZero, const wxString& aLabel,
                                                   VALIDATION_RESULT* aResult )
{
    if( !aCanBeZero && aValue <= 0.0 )
    {
        aResult->AddError( wxString::Format( _( "The value of %s must be greater than 0." ), aLabel ) );
        return false;
    }

    return true;
}


bool DRC_RULE_EDITOR_UTILS::ValidateNumericString( const wxString& aValueStr, bool aCanBeZero,
                                                    bool aIntegerOnly, const wxString& aLabel,
                                                    VALIDATION_RESULT* aResult )
{
    if( aValueStr.IsEmpty() )
    {
        aResult->AddError( wxString::Format( _( "%s should not be empty." ), aLabel ) );
        return false;
    }

    try
    {
        if( aIntegerOnly )
        {
            std::string stdStr = aValueStr.ToStdString();
            size_t pos;
            long   intVal = std::stol( stdStr, &pos );

            if( pos != stdStr.length() )
            {
                aResult->AddError( wxString::Format( _( "The value of %s must be a valid integer." ), aLabel ) );
                return false;
            }

            if( !aCanBeZero && intVal <= 0 )
            {
                aResult->AddError( wxString::Format( _( "The value of %s must be greater than 0." ), aLabel ) );
                return false;
            }
        }
        else
        {
            std::string stdStr = aValueStr.ToStdString();
            size_t pos;
            double floatVal = std::stod( stdStr, &pos );

            if( pos != stdStr.length() )
            {
                aResult->AddError( wxString::Format( _( "The value of %s must be a valid number." ), aLabel ) );
                return false;
            }

            if( !aCanBeZero && floatVal <= 0.0 )
            {
                aResult->AddError( wxString::Format( _( "The value of %s must be greater than 0." ), aLabel ) );
                return false;
            }
        }
    }
    catch( const std::exception& )
    {
        if( aIntegerOnly )
            aResult->AddError( wxString::Format( _( "The value of %s must be a valid integer." ), aLabel ) );
        else
            aResult->AddError( wxString::Format( _( "The value of %s must be a valid number." ), aLabel ) );

        return false;
    }

    return true;
}


bool DRC_RULE_EDITOR_UTILS::ValidateMinMax( double aMin, double aMax, const wxString& aMinLabel,
                                             const wxString& aMaxLabel, VALIDATION_RESULT* aResult )
{
    if( aMin > aMax )
    {
        aResult->AddError( wxString::Format( _( "%s value cannot be greater than %s value." ), aMinLabel, aMaxLabel ));
        return false;
    }

    return true;
}


bool DRC_RULE_EDITOR_UTILS::ValidateMinPreferredMax( double aMin, double aPreferred, double aMax,
                                                      const wxString& aMinLabel,
                                                      const wxString& aPrefLabel,
                                                      const wxString& aMaxLabel,
                                                      VALIDATION_RESULT* aResult )
{
    bool valid = true;

    if( aMin > aPreferred )
    {
        aResult->AddError( wxString::Format( _( "%s value cannot be greater than %s value." ), aMinLabel, aPrefLabel ));
        valid = false;
    }

    if( aPreferred > aMax )
    {
        aResult->AddError( wxString::Format( _( "%s value cannot be greater than %s value." ), aPrefLabel, aMaxLabel ));
        valid = false;
    }

    if( aMin > aMax )
    {
        aResult->AddError( wxString::Format( _( "%s value cannot be greater than %s value." ), aMinLabel, aMaxLabel ));
        valid = false;
    }

    return valid;
}


bool DRC_RULE_EDITOR_UTILS::ValidateAtLeastOneSelected( const std::vector<bool>& aSelected,
                                                         const wxString&          aLabel,
                                                         VALIDATION_RESULT*       aResult )
{
    for( bool selected : aSelected )
    {
        if( selected )
            return true;
    }

    aResult->AddError( wxString::Format( _( "Please select at least one option from %s list." ), aLabel ) );
    return false;
}


bool DRC_RULE_EDITOR_UTILS::ValidateSelection( int aSelectionIndex, const wxString& aLabel,
                                                VALIDATION_RESULT* aResult )
{
    if( aSelectionIndex < 0 )
    {
        aResult->AddError( wxString::Format( _( "Please choose %s." ), aLabel ) );
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


// ==================== Layer Filtering ====================

DRC_LAYER_CATEGORY DRC_RULE_EDITOR_UTILS::GetLayerCategoryForConstraint(
        DRC_RULE_EDITOR_CONSTRAINT_NAME aConstraintType )
{
    switch( aConstraintType )
    {
    // COPPER_ONLY: Constraints that only apply to copper layers
    case MINIMUM_CLEARANCE:
    case CREEPAGE_DISTANCE:
    case COPPER_TO_HOLE_CLEARANCE:
    case COPPER_TO_EDGE_CLEARANCE:
    case VIA_STYLE:
    case MINIMUM_VIA_DIAMETER:
    case MINIMUM_UVIA_DIAMETER:
    case MINIMUM_UVIA_HOLE:
    case MINIMUM_ANNULAR_WIDTH:
    case MINIMUM_CONNECTION_WIDTH:
    case ROUTING_WIDTH:
    case ROUTING_DIFF_PAIR:
    case MINIMUM_THERMAL_RELIEF_SPOKE_COUNT:
    case MATCHED_LENGTH_DIFF_PAIR:
    case ABSOLUTE_LENGTH:
        return DRC_LAYER_CATEGORY::COPPER_ONLY;

    // SILKSCREEN_ONLY: Constraints that only apply to silkscreen layers
    case SILK_TO_SILK_CLEARANCE:
        return DRC_LAYER_CATEGORY::SILKSCREEN_ONLY;

    // SOLDERMASK_ONLY: Constraints that only apply to soldermask layers
    case MINIMUM_SOLDERMASK_SILVER:
    case SOLDERMASK_EXPANSION:
        return DRC_LAYER_CATEGORY::SOLDERMASK_ONLY;

    // SOLDERPASTE_ONLY: Constraints that only apply to solderpaste layers
    case SOLDERPASTE_EXPANSION:
        return DRC_LAYER_CATEGORY::SOLDERPASTE_ONLY;

    // TOP_BOTTOM_ANY: Constraints with simplified top/bottom/any selection
    case COURTYARD_CLEARANCE:
    case SILK_TO_SOLDERMASK_CLEARANCE:
    case VIAS_UNDER_SMD:
    case ALLOWED_ORIENTATION:
        return DRC_LAYER_CATEGORY::TOP_BOTTOM_ANY;

    // GENERAL_ANY_LAYER: Constraints that can apply to any layer type
    case HOLE_TO_HOLE_CLEARANCE:
    case PHYSICAL_CLEARANCE:
    case HOLE_SIZE:
    case HOLE_TO_HOLE_DISTANCE:
    case MINIMUM_THROUGH_HOLE:
    case MINIMUM_TEXT_HEIGHT_AND_THICKNESS:
    case MAXIMUM_ALLOWED_DEVIATION:
    case MINIMUM_ANGULAR_RING:
        return DRC_LAYER_CATEGORY::GENERAL_ANY_LAYER;

    // NO_LAYER_SELECTOR: Constraints where layer selection doesn't apply
    case MAXIMUM_VIA_COUNT:
    case PERMITTED_LAYERS:
    case CUSTOM_RULE:
        return DRC_LAYER_CATEGORY::NO_LAYER_SELECTOR;

    default:
        return DRC_LAYER_CATEGORY::GENERAL_ANY_LAYER;
    }
}


wxString DRC_RULE_EDITOR_UTILS::TranslateTopBottomLayer( DRC_RULE_EDITOR_CONSTRAINT_NAME aConstraintType,
                                                         bool aIsTop )
{
    switch( aConstraintType )
    {
    case COURTYARD_CLEARANCE:
        return wxString::Format( wxS( "(layer \"%s\")" ),
                                 aIsTop ? wxS( "F.CrtYd" ) : wxS( "B.CrtYd" ) );

    case SILK_TO_SOLDERMASK_CLEARANCE:
        // Generates a condition to match silk on one side with mask on the same side
        return wxString::Format( wxS( "(condition \"A.Layer == '%s' && B.Layer == '%s'\")" ),
                                 aIsTop ? wxS( "F.SilkS" ) : wxS( "B.SilkS" ),
                                 aIsTop ? wxS( "F.Mask" ) : wxS( "B.Mask" ) );

    case VIAS_UNDER_SMD:
    case ALLOWED_ORIENTATION:
        return wxString::Format( wxS( "(layer \"%s\")" ), aIsTop ? wxS( "F.Cu" ) : wxS( "B.Cu" ) );

    default:
        return wxEmptyString;
    }
}


std::shared_ptr<DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA>
DRC_RULE_EDITOR_UTILS::CreateNumericConstraintData( DRC_RULE_EDITOR_CONSTRAINT_NAME aType )
{
    switch( aType )
    {
    case COPPER_TO_EDGE_CLEARANCE:           return std::make_shared<DRC_RE_COPPER_TO_EDGE_CLEARANCE_CONSTRAINT_DATA>();
    case COPPER_TO_HOLE_CLEARANCE:           return std::make_shared<DRC_RE_COPPER_TO_HOLE_CLEARANCE_CONSTRAINT_DATA>();
    case COURTYARD_CLEARANCE:                return std::make_shared<DRC_RE_COURTYARD_CLEARANCE_CONSTRAINT_DATA>();
    case PHYSICAL_CLEARANCE:                 return std::make_shared<DRC_RE_PHYSICAL_CLEARANCE_CONSTRAINT_DATA>();
    case CREEPAGE_DISTANCE:                  return std::make_shared<DRC_RE_CREEPAGE_DISTANCE_CONSTRAINT_DATA>();
    case HOLE_SIZE:                          return std::make_shared<DRC_RE_HOLE_SIZE_CONSTRAINT_DATA>();
    case HOLE_TO_HOLE_CLEARANCE:             return std::make_shared<DRC_RE_HOLE_TO_HOLE_CLEARANCE_CONSTRAINT_DATA>();
    case HOLE_TO_HOLE_DISTANCE:              return std::make_shared<DRC_RE_HOLE_TO_HOLE_DISTANCE_CONSTRAINT_DATA>();
    case MAXIMUM_ALLOWED_DEVIATION:          return std::make_shared<DRC_RE_MAXIMUM_ALLOWED_DEVIATION_CONSTRAINT_DATA>();
    case MAXIMUM_VIA_COUNT:                  return std::make_shared<DRC_RE_MAXIMUM_VIA_COUNT_CONSTRAINT_DATA>();
    case MINIMUM_ANGULAR_RING:               return std::make_shared<DRC_RE_MINIMUM_ANGULAR_RING_CONSTRAINT_DATA>();
    case MINIMUM_ANNULAR_WIDTH:              return std::make_shared<DRC_RE_MINIMUM_ANNULAR_WIDTH_CONSTRAINT_DATA>();
    case MINIMUM_CLEARANCE:                  return std::make_shared<DRC_RE_MINIMUM_CLEARANCE_CONSTRAINT_DATA>();
    case MINIMUM_CONNECTION_WIDTH:           return std::make_shared<DRC_RE_MINIMUM_CONNECTION_WIDTH_CONSTRAINT_DATA>();
    case MINIMUM_SOLDERMASK_SILVER:          return std::make_shared<DRC_RE_MINIMUM_SOLDERMASK_SILVER_CONSTRAINT_DATA>();
    case MINIMUM_THERMAL_RELIEF_SPOKE_COUNT: return std::make_shared<DRC_RE_MINIMUM_THERMAL_SPOKE_COUNT_CONSTRAINT_DATA>();
    case MINIMUM_THROUGH_HOLE:               return std::make_shared<DRC_RE_MINIMUM_THROUGH_HOLE_CONSTRAINT_DATA>();
    case MINIMUM_UVIA_DIAMETER:              return std::make_shared<DRC_RE_MINIMUM_UVIA_DIAMETER_CONSTRAINT_DATA>();
    case MINIMUM_UVIA_HOLE:                  return std::make_shared<DRC_RE_MINIMUM_UVIA_HOLE_CONSTRAINT_DATA>();
    case MINIMUM_VIA_DIAMETER:               return std::make_shared<DRC_RE_MINIMUM_VIA_DIAMETER_CONSTRAINT_DATA>();
    case SILK_TO_SILK_CLEARANCE:             return std::make_shared<DRC_RE_SILK_TO_SILK_CLEARANCE_CONSTRAINT_DATA>();
    case SILK_TO_SOLDERMASK_CLEARANCE:       return std::make_shared<DRC_RE_SILK_TO_SOLDERMASK_CLEARANCE_CONSTRAINT_DATA>();
    case SOLDERMASK_EXPANSION:               return std::make_shared<DRC_RE_SOLDERMASK_EXPANSION_CONSTRAINT_DATA>();
    case SOLDERPASTE_EXPANSION:              return std::make_shared<DRC_RE_SOLDERPASTE_EXPANSION_CONSTRAINT_DATA>();
    default:                                 return std::make_shared<DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA>();
    }
}


std::shared_ptr<DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA>
DRC_RULE_EDITOR_UTILS::CreateNumericConstraintData( DRC_RULE_EDITOR_CONSTRAINT_NAME aType,
                                                    const DRC_RE_BASE_CONSTRAINT_DATA& aBase )
{
    switch( aType )
    {
    case COPPER_TO_EDGE_CLEARANCE:           return std::make_shared<DRC_RE_COPPER_TO_EDGE_CLEARANCE_CONSTRAINT_DATA>( aBase );
    case COPPER_TO_HOLE_CLEARANCE:           return std::make_shared<DRC_RE_COPPER_TO_HOLE_CLEARANCE_CONSTRAINT_DATA>( aBase );
    case COURTYARD_CLEARANCE:                return std::make_shared<DRC_RE_COURTYARD_CLEARANCE_CONSTRAINT_DATA>( aBase );
    case PHYSICAL_CLEARANCE:                 return std::make_shared<DRC_RE_PHYSICAL_CLEARANCE_CONSTRAINT_DATA>( aBase );
    case CREEPAGE_DISTANCE:                  return std::make_shared<DRC_RE_CREEPAGE_DISTANCE_CONSTRAINT_DATA>( aBase );
    case HOLE_SIZE:                          return std::make_shared<DRC_RE_HOLE_SIZE_CONSTRAINT_DATA>( aBase );
    case HOLE_TO_HOLE_CLEARANCE:             return std::make_shared<DRC_RE_HOLE_TO_HOLE_CLEARANCE_CONSTRAINT_DATA>( aBase );
    case HOLE_TO_HOLE_DISTANCE:              return std::make_shared<DRC_RE_HOLE_TO_HOLE_DISTANCE_CONSTRAINT_DATA>( aBase );
    case MAXIMUM_ALLOWED_DEVIATION:          return std::make_shared<DRC_RE_MAXIMUM_ALLOWED_DEVIATION_CONSTRAINT_DATA>( aBase );
    case MAXIMUM_VIA_COUNT:                  return std::make_shared<DRC_RE_MAXIMUM_VIA_COUNT_CONSTRAINT_DATA>( aBase );
    case MINIMUM_ANGULAR_RING:               return std::make_shared<DRC_RE_MINIMUM_ANGULAR_RING_CONSTRAINT_DATA>( aBase );
    case MINIMUM_ANNULAR_WIDTH:              return std::make_shared<DRC_RE_MINIMUM_ANNULAR_WIDTH_CONSTRAINT_DATA>( aBase );
    case MINIMUM_CLEARANCE:                  return std::make_shared<DRC_RE_MINIMUM_CLEARANCE_CONSTRAINT_DATA>( aBase );
    case MINIMUM_CONNECTION_WIDTH:           return std::make_shared<DRC_RE_MINIMUM_CONNECTION_WIDTH_CONSTRAINT_DATA>( aBase );
    case MINIMUM_SOLDERMASK_SILVER:          return std::make_shared<DRC_RE_MINIMUM_SOLDERMASK_SILVER_CONSTRAINT_DATA>( aBase );
    case MINIMUM_THERMAL_RELIEF_SPOKE_COUNT: return std::make_shared<DRC_RE_MINIMUM_THERMAL_SPOKE_COUNT_CONSTRAINT_DATA>( aBase );
    case MINIMUM_THROUGH_HOLE:               return std::make_shared<DRC_RE_MINIMUM_THROUGH_HOLE_CONSTRAINT_DATA>( aBase );
    case MINIMUM_UVIA_DIAMETER:              return std::make_shared<DRC_RE_MINIMUM_UVIA_DIAMETER_CONSTRAINT_DATA>( aBase );
    case MINIMUM_UVIA_HOLE:                  return std::make_shared<DRC_RE_MINIMUM_UVIA_HOLE_CONSTRAINT_DATA>( aBase );
    case MINIMUM_VIA_DIAMETER:               return std::make_shared<DRC_RE_MINIMUM_VIA_DIAMETER_CONSTRAINT_DATA>( aBase );
    case SILK_TO_SILK_CLEARANCE:             return std::make_shared<DRC_RE_SILK_TO_SILK_CLEARANCE_CONSTRAINT_DATA>( aBase );
    case SILK_TO_SOLDERMASK_CLEARANCE:       return std::make_shared<DRC_RE_SILK_TO_SOLDERMASK_CLEARANCE_CONSTRAINT_DATA>( aBase );
    case SOLDERMASK_EXPANSION:               return std::make_shared<DRC_RE_SOLDERMASK_EXPANSION_CONSTRAINT_DATA>( aBase );
    case SOLDERPASTE_EXPANSION:              return std::make_shared<DRC_RE_SOLDERPASTE_EXPANSION_CONSTRAINT_DATA>( aBase );
    default:                                 return std::make_shared<DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA>( aBase );
    }
}
