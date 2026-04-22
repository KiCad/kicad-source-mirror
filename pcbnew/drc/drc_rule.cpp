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


#include <board.h>
#include <board_item.h>
#include <api/api_enums.h>
#include <api/board/board.pb.h>
#include <base_units.h>
#include <drc/drc_rule.h>
#include <drc/drc_rule_condition.h>


DRC_RULE::DRC_RULE() :
        m_Unary( false ),
        m_ImplicitItemId( 0 ),
        m_ImplicitItem( nullptr ),
        m_LayerCondition( LSET::AllLayersMask() ),
        m_Condition( nullptr ),
        m_Severity( RPT_SEVERITY_UNDEFINED ),
        m_implicitSource( DRC_IMPLICIT_SOURCE::NONE )
{
}


DRC_RULE::DRC_RULE( const wxString& aName ) :
        m_Unary( false ),
        m_ImplicitItemId( 0 ),
        m_ImplicitItem( nullptr ),
        m_Name( aName ),
        m_LayerCondition( LSET::AllLayersMask() ),
        m_Condition( nullptr ),
        m_Severity( RPT_SEVERITY_UNDEFINED ),
        m_implicitSource( DRC_IMPLICIT_SOURCE::NONE )
{
}


DRC_RULE::~DRC_RULE()
{
    delete m_Condition;
}


void DRC_RULE::AddConstraint( DRC_CONSTRAINT& aConstraint )
{
    aConstraint.SetParentRule( this );
    m_Constraints.push_back( aConstraint );
}


std::optional<DRC_CONSTRAINT> DRC_RULE::FindConstraint( DRC_CONSTRAINT_T aType )
{
    for( DRC_CONSTRAINT& c : m_Constraints )
    {
        if( c.m_Type == aType )
            return c;
    }

    return std::optional<DRC_CONSTRAINT>();
}


wxString DRC_RULE::FormatRuleFromProto( const kiapi::board::CustomRule& aRule, wxString* aErrorText )
{
    if( aErrorText )
        *aErrorText = wxEmptyString;

    auto fail =
            [&]( const wxString& aMessage ) -> wxString
            {
                if( aErrorText )
                    *aErrorText = aMessage;

                return wxEmptyString;
            };

    auto escapeQuotedRuleText =
            []( const wxString& aText ) -> wxString
            {
                wxString escaped = aText;
                escaped.Replace( "\\", "\\\\" );
                escaped.Replace( "\"", "\\\"" );
                return escaped;
            };

    auto hasConstraintOption =
            []( const kiapi::board::CustomRuleConstraint& aConstraint,
                const kiapi::board::CustomRuleConstraintOption aOption ) -> bool
            {
                for( int optionValue : aConstraint.options() )
                {
                    if( static_cast<kiapi::board::CustomRuleConstraintOption>( optionValue ) == aOption )
                        return true;
                }

                return false;
            };

    auto getConstraintToken =
            []( const kiapi::board::CustomRuleConstraintType aType,
                bool* aAddWithinDiffPairs ) -> std::optional<wxString>
            {
                if( aAddWithinDiffPairs )
                    *aAddWithinDiffPairs = false;

                switch( aType )
                {
                case kiapi::board::CRCT_CLEARANCE:               return wxS( "clearance" );
                case kiapi::board::CRCT_CREEPAGE:                return wxS( "creepage" );
                case kiapi::board::CRCT_HOLE_CLEARANCE:          return wxS( "hole_clearance" );
                case kiapi::board::CRCT_HOLE_TO_HOLE:            return wxS( "hole_to_hole" );
                case kiapi::board::CRCT_EDGE_CLEARANCE:          return wxS( "edge_clearance" );
                case kiapi::board::CRCT_HOLE_SIZE:               return wxS( "hole_size" );
                case kiapi::board::CRCT_COURTYARD_CLEARANCE:     return wxS( "courtyard_clearance" );
                case kiapi::board::CRCT_SILK_CLEARANCE:          return wxS( "silk_clearance" );
                case kiapi::board::CRCT_TEXT_HEIGHT:             return wxS( "text_height" );
                case kiapi::board::CRCT_TEXT_THICKNESS:          return wxS( "text_thickness" );
                case kiapi::board::CRCT_TRACK_WIDTH:             return wxS( "track_width" );
                case kiapi::board::CRCT_TRACK_SEGMENT_LENGTH:    return wxS( "track_segment_length" );
                case kiapi::board::CRCT_ANNULAR_WIDTH:           return wxS( "annular_width" );
                case kiapi::board::CRCT_ZONE_CONNECTION:         return wxS( "zone_connection" );
                case kiapi::board::CRCT_THERMAL_RELIEF_GAP:      return wxS( "thermal_relief_gap" );
                case kiapi::board::CRCT_THERMAL_SPOKE_WIDTH:     return wxS( "thermal_spoke_width" );
                case kiapi::board::CRCT_MIN_RESOLVED_SPOKES:     return wxS( "min_resolved_spokes" );
                case kiapi::board::CRCT_SOLDER_MASK_EXPANSION:   return wxS( "solder_mask_expansion" );
                case kiapi::board::CRCT_SOLDER_PASTE_ABS_MARGIN: return wxS( "solder_paste_abs_margin" );
                case kiapi::board::CRCT_SOLDER_PASTE_REL_MARGIN: return wxS( "solder_paste_rel_margin" );
                case kiapi::board::CRCT_DISALLOW:                return wxS( "disallow" );
                case kiapi::board::CRCT_VIA_DIAMETER:            return wxS( "via_diameter" );
                case kiapi::board::CRCT_LENGTH:                  return wxS( "length" );
                case kiapi::board::CRCT_SKEW:                    return wxS( "skew" );
                case kiapi::board::CRCT_DIFF_PAIR_GAP:           return wxS( "diff_pair_gap" );
                case kiapi::board::CRCT_MAX_UNCOUPLED:           return wxS( "diff_pair_uncoupled" );
                case kiapi::board::CRCT_DIFF_PAIR_INTRA_SKEW:
                {
                    if( aAddWithinDiffPairs )
                        *aAddWithinDiffPairs = true;

                    return wxS( "skew" );
                }
                case kiapi::board::CRCT_VIA_COUNT:               return wxS( "via_count" );
                case kiapi::board::CRCT_PHYSICAL_CLEARANCE:      return wxS( "physical_clearance" );
                case kiapi::board::CRCT_PHYSICAL_HOLE_CLEARANCE: return wxS( "physical_hole_clearance" );
                case kiapi::board::CRCT_ASSERTION:               return wxS( "assertion" );
                case kiapi::board::CRCT_CONNECTION_WIDTH:        return wxS( "connection_width" );
                case kiapi::board::CRCT_TRACK_ANGLE:             return wxS( "track_angle" );
                case kiapi::board::CRCT_VIA_DANGLING:            return wxS( "via_dangling" );
                case kiapi::board::CRCT_BRIDGED_MASK:            return wxS( "bridged_mask" );
                case kiapi::board::CRCT_SOLDER_MASK_SLIVER:      return wxS( "solder_mask_sliver" );

                case kiapi::board::CRCT_UNKNOWN:
                default:
                    return std::nullopt;
                }
            };

    auto formatNumericValueForConstraint =
            [&]( const kiapi::board::CustomRuleConstraint& aConstraint,
                 const int aValue ) -> wxString
            {
                if( aConstraint.type() == kiapi::board::CRCT_TRACK_ANGLE )
                    return wxString::Format( "%ddeg", aValue );

                switch( aConstraint.type() )
                {
                case kiapi::board::CRCT_VIA_COUNT:
                case kiapi::board::CRCT_MIN_RESOLVED_SPOKES:
                case kiapi::board::CRCT_VIA_DANGLING:
                case kiapi::board::CRCT_BRIDGED_MASK:
                    return wxString::Format( "%d", aValue );

                default:
                    break;
                }

                if( hasConstraintOption( aConstraint, kiapi::board::CRCO_TIME_DOMAIN ) )
                    return wxString::Format( "%dps", aValue );

                wxString formatted = wxString::FromUTF8( EDA_UNIT_UTILS::FormatInternalUnits( pcbIUScale, aValue ) );
                return formatted + "mm";
            };

    auto severityToken =
            []( const kiapi::common::types::RuleSeverity aSeverity ) -> wxString
            {
                switch( aSeverity )
                {
                case kiapi::common::types::RuleSeverity::RS_WARNING:   return wxS( "warning" );
                case kiapi::common::types::RuleSeverity::RS_ERROR:     return wxS( "error" );
                case kiapi::common::types::RuleSeverity::RS_EXCLUSION: return wxS( "exclusion" );
                case kiapi::common::types::RuleSeverity::RS_IGNORE:    return wxS( "ignore" );

                default:
                    return wxEmptyString;
                }
            };

    if( aRule.name().empty() )
        return fail( wxS( "Rules must have a name" ) );

    wxString ruleText;
    ruleText << "(rule \"" << escapeQuotedRuleText( wxString::FromUTF8( aRule.name() ) ) << "\"\n";

    if( aRule.has_comments() && !aRule.comments().empty() )
    {
        wxArrayString commentLines = wxSplit( wxString::FromUTF8( aRule.comments() ), '\n', '\0' );

        for( const wxString& line : commentLines )
            ruleText << "\t# " << line << "\n";
    }

    if( !aRule.condition().empty() )
    {
        ruleText << "\t(condition \""
                 << escapeQuotedRuleText( wxString::FromUTF8( aRule.condition() ) )
                 << "\")\n";
    }

    if( aRule.layer_condition_case() == kiapi::board::CustomRule::LayerConditionCase::kLayerMode )
    {
        switch( aRule.layer_mode() )
        {
        case kiapi::board::CRLM_OUTER:
            ruleText << "\t(layer outer)\n";
            break;

        case kiapi::board::CRLM_INNER:
            ruleText << "\t(layer inner)\n";
            break;

        case kiapi::board::CRLM_UNKNOWN:
        default:
            break;
        }
    }
    else if( aRule.layer_condition_case() == kiapi::board::CustomRule::LayerConditionCase::kSingleLayer )
    {
        PCB_LAYER_ID layer =
                FromProtoEnum<PCB_LAYER_ID, kiapi::board::types::BoardLayer>( aRule.single_layer() );

        if( layer == UNDEFINED_LAYER || layer == UNSELECTED_LAYER )
            return fail( wxS( "Invalid single_layer value in custom rule" ) );

        ruleText << "\t(layer \"" << BOARD::GetStandardLayerName( layer ) << "\")\n";
    }

    for( const kiapi::board::CustomRuleConstraint& constraint : aRule.constraints() )
    {
        bool addWithinDiffPairs = false;
        std::optional<wxString> token = getConstraintToken( constraint.type(), &addWithinDiffPairs );

        if( !token )
            return fail( wxS( "Unsupported custom rule constraint type" ) );

        wxString text = wxS( "\t(constraint " ) + *token;

        if( constraint.has_name() && !constraint.name().empty() )
        {
            text += wxS( " (name \"" )
                    + escapeQuotedRuleText( wxString::FromUTF8( constraint.name() ) )
                    + wxS( "\")" );
        }

        if( constraint.value_case() == kiapi::board::CustomRuleConstraint::ValueCase::kDisallow )
        {
            for( int disallowTypeValue : constraint.disallow().types() )
            {
                kiapi::board::CustomRuleDisallowType disallowType =
                        static_cast<kiapi::board::CustomRuleDisallowType>( disallowTypeValue );

                switch( disallowType )
                {
                case kiapi::board::CRDT_THROUGH_VIAS: text += wxS( " through_via" ); break;
                case kiapi::board::CRDT_MICRO_VIAS:   text += wxS( " micro_via" );   break;
                case kiapi::board::CRDT_BLIND_VIAS:   text += wxS( " blind_via" );   break;
                case kiapi::board::CRDT_BURIED_VIAS:  text += wxS( " buried_via" );  break;
                case kiapi::board::CRDT_TRACKS:       text += wxS( " track" );       break;
                case kiapi::board::CRDT_PADS:         text += wxS( " pad" );         break;
                case kiapi::board::CRDT_ZONES:        text += wxS( " zone" );        break;
                case kiapi::board::CRDT_TEXTS:        text += wxS( " text" );        break;
                case kiapi::board::CRDT_GRAPHICS:     text += wxS( " graphic" );     break;
                case kiapi::board::CRDT_HOLES:        text += wxS( " hole" );        break;
                case kiapi::board::CRDT_FOOTPRINTS:   text += wxS( " footprint" );   break;

                case kiapi::board::CRDT_UNKNOWN:
                default:
                    break;
                }
            }
        }
        else if( constraint.value_case() == kiapi::board::CustomRuleConstraint::ValueCase::kZoneConnection )
        {
            switch( constraint.zone_connection() )
            {
            case kiapi::board::types::ZCS_FULL:
                text += wxS( " solid" );
                break;

            case kiapi::board::types::ZCS_THERMAL:
                text += wxS( " thermal_reliefs" );
                break;

            case kiapi::board::types::ZCS_NONE:
                text += wxS( " none" );
                break;

            default:
                return fail( wxS( "Unsupported zone connection style" ) );
            }
        }
        else if( constraint.value_case() == kiapi::board::CustomRuleConstraint::ValueCase::kAssertionExpression )
        {
            text += wxS( " \"" )
                    + escapeQuotedRuleText( wxString::FromUTF8( constraint.assertion_expression() ) )
                    + wxS( "\"" );
        }
        else if( constraint.value_case() == kiapi::board::CustomRuleConstraint::ValueCase::kNumeric )
        {
            const kiapi::common::types::MinOptMax& numeric = constraint.numeric();

            if( numeric.has_min() )
                text += wxS( " (min " )
                        + formatNumericValueForConstraint( constraint, numeric.min() )
                        + wxS( ")" );

            if( numeric.has_opt() )
                text += wxS( " (opt " )
                        + formatNumericValueForConstraint( constraint, numeric.opt() )
                        + wxS( ")" );

            if( numeric.has_max() )
                text += wxS( " (max " )
                        + formatNumericValueForConstraint( constraint, numeric.max() )
                        + wxS( ")" );
        }

        if( addWithinDiffPairs
            || hasConstraintOption( constraint, kiapi::board::CRCO_SKEW_WITHIN_DIFF_PAIRS ) )
        {
            text += wxS( " (within_diff_pairs)" );
        }

        text += wxS( ")" );
        ruleText << text << "\n";
    }

    wxString severity = severityToken( aRule.severity() );

    if( !severity.IsEmpty() )
        ruleText << "\t(severity " << severity << ")\n";

    ruleText << ")\n";
    return ruleText;
}


void DRC_CONSTRAINT::ToProto( kiapi::board::CustomRuleConstraint& aProto ) const
{
    aProto.set_type( ToProtoEnum<DRC_CONSTRAINT_T, kiapi::board::CustomRuleConstraintType>( m_Type ) );

    if( !m_name.IsEmpty() )
        aProto.set_name( m_name.ToUTF8() );

    for( size_t ii = 0; ii < static_cast<size_t>( OPTIONS::NUM_OPTIONS ); ++ii )
    {
        OPTIONS option = static_cast<OPTIONS>( ii );

        if( GetOption( option ) )
            aProto.add_options( ToProtoEnum<OPTIONS, kiapi::board::CustomRuleConstraintOption>( option ) );
    }

    if( m_Type == DISALLOW_CONSTRAINT )
    {
        kiapi::board::CustomRuleDisallowSettings* disallow = aProto.mutable_disallow();

        for( int flag = DRC_DISALLOW_THROUGH_VIAS; flag <= DRC_DISALLOW_FOOTPRINTS; flag <<= 1 )
        {
            if( m_DisallowFlags & flag )
            {
                disallow->add_types( ToProtoEnum<DRC_DISALLOW_T, kiapi::board::CustomRuleDisallowType>(
                        static_cast<DRC_DISALLOW_T>( flag ) ) );
            }
        }
    }
    else if( m_Type == ZONE_CONNECTION_CONSTRAINT )
    {
        aProto.set_zone_connection(
                ToProtoEnum<ZONE_CONNECTION, kiapi::board::types::ZoneConnectionStyle>( m_ZoneConnection ) );
    }
    else if( m_Type == ASSERTION_CONSTRAINT )
    {
        if( m_Test )
            aProto.set_assertion_expression( m_Test->GetExpression().ToUTF8() );
    }
    else
    {
        kiapi::common::types::MinOptMax* numeric = aProto.mutable_numeric();

        if( m_Value.HasMin() )
            numeric->set_min( m_Value.Min() );

        if( m_Value.HasOpt() )
            numeric->set_opt( m_Value.Opt() );

        if( m_Value.HasMax() )
            numeric->set_max( m_Value.Max() );
    }
}
