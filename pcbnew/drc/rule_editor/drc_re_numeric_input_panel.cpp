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

#include "drc_re_numeric_input_panel.h"

#include <cmath>
#include <map>

#include <wx/log.h>


const std::map<DRC_RULE_EDITOR_CONSTRAINT_NAME, BITMAPS> NumericConstraintBitMapPairs = {
        { BASIC_CLEARANCE, BITMAPS::constraint_basic_clearance },
        { BOARD_OUTLINE_CLEARANCE, BITMAPS::constraint_outline_clearance },
        { MINIMUM_CLEARANCE, BITMAPS::constraint_minimum_clearance },
        { MINIMUM_ITEM_CLEARANCE, BITMAPS::constraint_minimum_item_clearance },
        { CREEPAGE_DISTANCE, BITMAPS::constraint_creepage_distance},
        { MINIMUM_CONNECTION_WIDTH, BITMAPS::constraint_minimum_connection_width },
        { MINIMUM_TRACK_WIDTH, BITMAPS::constraint_minimum_track_width},
        { COPPER_TO_HOLE_CLEARANCE, BITMAPS::constraint_copper_to_hole_clearance},
        { HOLE_TO_HOLE_CLEARANCE, BITMAPS::constraint_hole_to_hole_clearance},
        { MINIMUM_ANNULAR_WIDTH, BITMAPS::constraint_minimum_annular_width},
        { COPPER_TO_EDGE_CLEARANCE, BITMAPS::constraint_copper_to_edge_clearance},
        { MINIMUM_THROUGH_HOLE, BITMAPS::constraint_minimum_through_hole },
        { HOLE_SIZE, BITMAPS::constraint_hole_size },
        { HOLE_TO_HOLE_DISTANCE, BITMAPS::constraint_hole_to_hole_distance },
        { MINIMUM_UVIA_HOLE, BITMAPS::constraint_minimum_uvia_hole },
        { MINIMUM_UVIA_DIAMETER, BITMAPS::constraint_minimum_uvia_diameter },
        { MINIMUM_VIA_DIAMETER, BITMAPS::constraint_minimum_via_diameter },
        { SILK_TO_SILK_CLEARANCE, BITMAPS::constraint_silk_to_silk_clearance },
        { SILK_TO_SOLDERMASK_CLEARANCE, BITMAPS::constraint_silk_to_soldermask_clearance },
        { MINIMUM_SOLDERMASK_SILVER, BITMAPS::constraint_minimum_soldermask_silver },
        { SOLDERMASK_EXPANSION, BITMAPS::constraint_soldermask_expansion },
        { SOLDERPASTE_EXPANSION, BITMAPS::constraint_solderpaste_expansion },
        { MAXIMUM_ALLOWED_DEVIATION, BITMAPS::constraint_maximum_allowed_deviation },
        { MINIMUM_ANGULAR_RING, BITMAPS::constraint_minimum_angular_ring },
        { MINIMUM_THERMAL_RELIEF_SPOKE_COUNT, BITMAPS::constraint_minimum_thermal_relief_spoke_count },
        { MAXIMUM_VIA_COUNT, BITMAPS::constraint_maximum_via_count },
        { ABSOLUTE_LENGTH, BITMAPS::constraint_absolute_length },
        { MATCHED_LENGTH_DIFF_PAIR, BITMAPS::constraint_matched_length_diff_pair }
    };


DRC_RE_NUMERIC_INPUT_PANEL::DRC_RE_NUMERIC_INPUT_PANEL( wxWindow* aParent,
        const DRC_RE_NUMERIC_INPUT_CONSTRAINT_PANEL_PARAMS& aConstraintPanelParams ) :
        DRC_RE_NUMERIC_INPUT_PANEL_BASE( aParent ),
        m_isCountInput( aConstraintPanelParams.m_isCountInput ),
        m_constraintData( aConstraintPanelParams.m_constraintData )
{
    auto it = NumericConstraintBitMapPairs.find( aConstraintPanelParams.m_constraintType );

    bConstraintImageSizer->Add( GetConstraintImage( this, it->second ), 0,
            wxALL | wxEXPAND, 10 );

    if( !aConstraintPanelParams.m_customLabelText.IsEmpty() )
        m_numericConstraintLabel->SetLabelText( aConstraintPanelParams.m_customLabelText );
    else
        m_numericConstraintLabel->SetLabelText( aConstraintPanelParams.m_constraintTitle );

    if( aConstraintPanelParams.m_isCountInput )
        m_numericConstraintUnit->Hide();
}


DRC_RE_NUMERIC_INPUT_PANEL::~DRC_RE_NUMERIC_INPUT_PANEL()
{
}


bool DRC_RE_NUMERIC_INPUT_PANEL::TransferDataToWindow()
{
    if( m_constraintData )
    {
        if( m_isCountInput )
        {
            m_numericConstraintCtrl->SetValue(
                    wxString::Format( wxS( "%d" ),
                                      static_cast<int>( m_constraintData->GetNumericInputValue() ) ) );
        }
        else
        {
            m_numericConstraintCtrl->SetValue(
                    wxString::Format( _( "%.2f" ), m_constraintData->GetNumericInputValue() ) );
        }
    }

    return true;
}


bool DRC_RE_NUMERIC_INPUT_PANEL::TransferDataFromWindow()
{
    m_constraintData->SetNumericInputValue(
            std::stod( m_numericConstraintCtrl->GetValue().ToStdString() ) );
    return true;
}


bool DRC_RE_NUMERIC_INPUT_PANEL::ValidateInputs( int* aErrorCount, std::string* aValidationMessage )
{
    TransferDataFromWindow();
    VALIDATION_RESULT result = m_constraintData->Validate();

    if( !result.isValid )
    {
        *aErrorCount = result.errors.size();

        for( size_t i = 0; i < result.errors.size(); i++ )
            *aValidationMessage += DRC_RULE_EDITOR_UTILS::FormatErrorMessage( i + 1, result.errors[i] );

        return false;
    }

    return true;
}


wxString DRC_RE_NUMERIC_INPUT_PANEL::GenerateRule( const RULE_GENERATION_CONTEXT& aContext )
{
    if( !m_constraintData )
        return wxEmptyString;

    enum class VALUE_KIND
    {
        MIN,
        MAX,
        OPT
    };

    enum class VALUE_UNIT
    {
        DISTANCE,
        ANGLE,
        COUNT,
        UNITLESS
    };

    static const std::map<wxString, std::pair<VALUE_KIND, VALUE_UNIT>> sDescriptor = {
        { wxS( "annular_width" ), { VALUE_KIND::MIN, VALUE_UNIT::DISTANCE } },
        { wxS( "clearance" ), { VALUE_KIND::MIN, VALUE_UNIT::DISTANCE } },
        { wxS( "connection_width" ), { VALUE_KIND::MIN, VALUE_UNIT::DISTANCE } },
        { wxS( "courtyard_clearance" ), { VALUE_KIND::MIN, VALUE_UNIT::DISTANCE } },
        { wxS( "creepage" ), { VALUE_KIND::MIN, VALUE_UNIT::DISTANCE } },
        { wxS( "daisy_chain_stub" ), { VALUE_KIND::MAX, VALUE_UNIT::DISTANCE } },
        { wxS( "daisy_chain_stub_2" ), { VALUE_KIND::MAX, VALUE_UNIT::DISTANCE } },
        { wxS( "edge_clearance" ), { VALUE_KIND::MIN, VALUE_UNIT::DISTANCE } },
        { wxS( "hole" ), { VALUE_KIND::MIN, VALUE_UNIT::DISTANCE } },
        { wxS( "hole_clearance" ), { VALUE_KIND::MIN, VALUE_UNIT::DISTANCE } },
        { wxS( "hole_to_hole" ), { VALUE_KIND::MIN, VALUE_UNIT::DISTANCE } },
        { wxS( "length" ), { VALUE_KIND::MAX, VALUE_UNIT::DISTANCE } },
        { wxS( "maximum_allowed_deviation" ), { VALUE_KIND::MAX, VALUE_UNIT::DISTANCE } },
        { wxS( "min_resolved_spokes" ), { VALUE_KIND::MIN, VALUE_UNIT::COUNT } },
        { wxS( "net_antenna" ), { VALUE_KIND::MAX, VALUE_UNIT::DISTANCE } },
        { wxS( "physical_clearance" ), { VALUE_KIND::MIN, VALUE_UNIT::DISTANCE } },
        { wxS( "smd_corner" ), { VALUE_KIND::MIN, VALUE_UNIT::DISTANCE } },
        { wxS( "smd_to_plane_plus" ), { VALUE_KIND::MAX, VALUE_UNIT::DISTANCE } },
        { wxS( "silk_clearance" ), { VALUE_KIND::MIN, VALUE_UNIT::DISTANCE } },
        { wxS( "solder_mask_expansion" ), { VALUE_KIND::OPT, VALUE_UNIT::DISTANCE } },
        { wxS( "solder_mask_sliver" ), { VALUE_KIND::MIN, VALUE_UNIT::DISTANCE } },
        { wxS( "solder_paste_abs_margin" ), { VALUE_KIND::OPT, VALUE_UNIT::DISTANCE } },
        { wxS( "thermal_spoke_width" ), { VALUE_KIND::OPT, VALUE_UNIT::DISTANCE } },
        { wxS( "track_angle" ), { VALUE_KIND::MIN, VALUE_UNIT::ANGLE } },
        { wxS( "track_width" ), { VALUE_KIND::MIN, VALUE_UNIT::DISTANCE } },
        { wxS( "via_count" ), { VALUE_KIND::MAX, VALUE_UNIT::COUNT } },
        { wxS( "via_diameter" ), { VALUE_KIND::MIN, VALUE_UNIT::DISTANCE } }
    };

    wxString code = m_constraintData->GetConstraintCode();

    if( code.IsEmpty() )
        code = wxS( "numeric_value" );

    // Adjust mismatched mappings for count-based inputs.
    if( code == wxS( "thermal_spoke_width" ) && m_isCountInput )
        code = wxS( "min_resolved_spokes" );

    auto descriptorIt = sDescriptor.find( code );

    VALUE_KIND valueKind = VALUE_KIND::MIN;
    VALUE_UNIT valueUnit = VALUE_UNIT::DISTANCE;

    if( descriptorIt != sDescriptor.end() )
    {
        valueKind = descriptorIt->second.first;
        valueUnit = descriptorIt->second.second;
    }
    else
    {
        wxLogTrace( wxS( "KI_TRACE_DRC_RULE_EDITOR" ),
                    wxS( "No numeric descriptor for constraint '%s', defaulting to minimum distance." ),
                    code );
    }

    auto formatValue = [&]( double aValue )
    {
        switch( valueUnit )
        {
        case VALUE_UNIT::ANGLE: return formatDouble( aValue ) + wxS( "deg" );
        case VALUE_UNIT::COUNT:
        {
            long long count = static_cast<long long>( std::llround( aValue ) );
            return wxString::Format( wxS( "%lld" ), count );
        }
        case VALUE_UNIT::UNITLESS: return formatDouble( aValue );
        case VALUE_UNIT::DISTANCE:
        default: return formatDouble( aValue ) + wxS( "mm" );
        }
    };

    double rawValue = m_constraintData->GetNumericInputValue();
    wxString formattedValue = formatValue( rawValue );

    wxString clause;

    switch( valueKind )
    {
    case VALUE_KIND::MAX:
        clause = wxString::Format( wxS( "(constraint %s (max %s))" ), code, formattedValue );
        break;
    case VALUE_KIND::OPT:
        clause = wxString::Format( wxS( "(constraint %s (opt %s))" ), code, formattedValue );
        break;
    case VALUE_KIND::MIN:
    default:
        clause = wxString::Format( wxS( "(constraint %s (min %s))" ), code, formattedValue );
        break;
    }

    wxLogTrace( wxS( "KI_TRACE_DRC_RULE_EDITOR" ), wxS( "Numeric constraint clause: %s" ), clause );

    return buildRule( aContext, { clause } );
}
