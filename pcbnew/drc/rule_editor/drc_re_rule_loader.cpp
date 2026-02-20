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

#include "drc_re_rule_loader.h"

#include <reporter.h>
#include <component_classes/component_class_assignment_rule.h>
#include <drc/drc_rule_parser.h>
#include <drc/drc_rule_condition.h>
#include <wx/ffile.h>

#include "drc_re_via_style_constraint_data.h"
#include "drc_re_rtg_diff_pair_constraint_data.h"
#include "drc_re_min_txt_ht_th_constraint_data.h"
#include "drc_re_routing_width_constraint_data.h"
#include "drc_re_abs_length_two_constraint_data.h"
#include "drc_re_numeric_input_constraint_data.h"
#include "drc_re_custom_rule_constraint_data.h"
#include "drc_re_bool_input_constraint_data.h"
#include "drc_re_allowed_orientation_constraint_data.h"
#include "drc_re_permitted_layers_constraint_data.h"
#include "drc_re_numeric_constraint_types.h"
#include "drc_rule_editor_utils.h"


DRC_RULE_LOADER::DRC_RULE_LOADER()
{
}


double DRC_RULE_LOADER::toMM( int aValue )
{
    return aValue / 1000000.0;
}


const DRC_CONSTRAINT* DRC_RULE_LOADER::findConstraint( const DRC_RULE& aRule, DRC_CONSTRAINT_T aType )
{
    for( const DRC_CONSTRAINT& constraint : aRule.m_Constraints )
    {
        if( constraint.m_Type == aType )
            return &constraint;
    }

    return nullptr;
}


std::shared_ptr<DRC_RE_BASE_CONSTRAINT_DATA>
DRC_RULE_LOADER::createConstraintData( DRC_RULE_EDITOR_CONSTRAINT_NAME   aPanel,
                                        const DRC_RULE&                   aRule,
                                        const std::set<DRC_CONSTRAINT_T>& aClaimedConstraints )
{
    switch( aPanel )
    {
    case VIA_STYLE:
    {
        auto data = std::make_shared<DRC_RE_VIA_STYLE_CONSTRAINT_DATA>();
        data->SetRuleName( aRule.m_Name );
        data->SetConstraintCode( "via_style" );

        const DRC_CONSTRAINT* viaDia = findConstraint( aRule, VIA_DIAMETER_CONSTRAINT );
        const DRC_CONSTRAINT* holeSize = findConstraint( aRule, HOLE_SIZE_CONSTRAINT );

        if( viaDia )
        {
            data->SetMinViaDiameter( toMM( viaDia->GetValue().Min() ) );
            data->SetPreferredViaDiameter( toMM( viaDia->GetValue().Opt() ) );
            data->SetMaxViaDiameter( toMM( viaDia->GetValue().Max() ) );
        }

        if( holeSize )
        {
            data->SetMinViaHoleSize( toMM( holeSize->GetValue().Min() ) );
            data->SetPreferredViaHoleSize( toMM( holeSize->GetValue().Opt() ) );
            data->SetMaxViaHoleSize( toMM( holeSize->GetValue().Max() ) );
        }

        return data;
    }

    case ROUTING_DIFF_PAIR:
    {
        auto data = std::make_shared<DRC_RE_ROUTING_DIFF_PAIR_CONSTRAINT_DATA>();
        data->SetRuleName( aRule.m_Name );
        data->SetConstraintCode( "diff_pair_gap" );

        const DRC_CONSTRAINT* trackWidth = findConstraint( aRule, TRACK_WIDTH_CONSTRAINT );
        const DRC_CONSTRAINT* diffGap = findConstraint( aRule, DIFF_PAIR_GAP_CONSTRAINT );
        const DRC_CONSTRAINT* uncoupled = findConstraint( aRule, MAX_UNCOUPLED_CONSTRAINT );

        if( trackWidth )
        {
            data->SetMinWidth( toMM( trackWidth->GetValue().Min() ) );
            data->SetPreferredWidth( toMM( trackWidth->GetValue().Opt() ) );
            data->SetMaxWidth( toMM( trackWidth->GetValue().Max() ) );
        }

        if( diffGap )
        {
            data->SetMinGap( toMM( diffGap->GetValue().Min() ) );
            data->SetPreferredGap( toMM( diffGap->GetValue().Opt() ) );
            data->SetMaxGap( toMM( diffGap->GetValue().Max() ) );
        }

        if( uncoupled )
        {
            data->SetMaxUncoupledLength( toMM( uncoupled->GetValue().Max() ) );
        }

        const DRC_CONSTRAINT* skew = findConstraint( aRule, SKEW_CONSTRAINT );

        if( skew )
        {
            data->SetMaxSkew( toMM( skew->GetValue().Max() ) );
        }

        return data;
    }

    case MINIMUM_TEXT_HEIGHT_AND_THICKNESS:
    {
        auto data = std::make_shared<DRC_RE_MINIMUM_TEXT_HEIGHT_THICKNESS_CONSTRAINT_DATA>();
        data->SetRuleName( aRule.m_Name );
        data->SetConstraintCode( "text_height" );

        const DRC_CONSTRAINT* textHeight = findConstraint( aRule, TEXT_HEIGHT_CONSTRAINT );
        const DRC_CONSTRAINT* textThickness = findConstraint( aRule, TEXT_THICKNESS_CONSTRAINT );

        if( textHeight )
            data->SetMinTextHeight( toMM( textHeight->GetValue().Min() ) );

        if( textThickness )
            data->SetMinTextThickness( toMM( textThickness->GetValue().Min() ) );

        return data;
    }

    case MINIMUM_TRACK_WIDTH:
    {
        auto data = std::make_shared<DRC_RE_MINIMUM_TRACK_WIDTH_CONSTRAINT_DATA>();
        data->SetRuleName( aRule.m_Name );
        data->SetConstraintCode( "track_width" );

        const DRC_CONSTRAINT* constraint = findConstraint( aRule, TRACK_WIDTH_CONSTRAINT );

        if( constraint )
            data->SetNumericInputValue( toMM( constraint->GetValue().Min() ) );

        return data;
    }

    case ROUTING_WIDTH:
    {
        auto data = std::make_shared<DRC_RE_ROUTING_WIDTH_CONSTRAINT_DATA>();
        data->SetRuleName( aRule.m_Name );
        data->SetConstraintCode( "track_width" );

        const DRC_CONSTRAINT* trackWidth = findConstraint( aRule, TRACK_WIDTH_CONSTRAINT );

        if( trackWidth )
        {
            data->SetMinRoutingWidth( toMM( trackWidth->GetValue().Min() ) );
            data->SetPreferredRoutingWidth( toMM( trackWidth->GetValue().Opt() ) );
            data->SetMaxRoutingWidth( toMM( trackWidth->GetValue().Max() ) );
        }

        return data;
    }

    case ABSOLUTE_LENGTH:
    {
        auto data = std::make_shared<DRC_RE_ABSOLUTE_LENGTH_TWO_CONSTRAINT_DATA>();
        data->SetRuleName( aRule.m_Name );
        data->SetConstraintCode( "length" );

        const DRC_CONSTRAINT* length = findConstraint( aRule, LENGTH_CONSTRAINT );

        if( length )
        {
            data->SetMinimumLength( toMM( length->GetValue().Min() ) );
            data->SetOptimumLength( toMM( length->GetValue().Opt() ) );
            data->SetMaximumLength( toMM( length->GetValue().Max() ) );
        }

        return data;
    }

    case MATCHED_LENGTH_DIFF_PAIR:
    {
        auto data = std::make_shared<DRC_RE_MATCHED_LENGTH_DIFF_PAIR_CONSTRAINT_DATA>();
        data->SetRuleName( aRule.m_Name );
        data->SetConstraintCode( "length" );

        const DRC_CONSTRAINT* length = findConstraint( aRule, LENGTH_CONSTRAINT );

        if( length )
        {
            data->SetMinimumLength( toMM( length->GetValue().Min() ) );
            data->SetOptimumLength( toMM( length->GetValue().Opt() ) );
            data->SetMaximumLength( toMM( length->GetValue().Max() ) );
        }

        const DRC_CONSTRAINT* skew = findConstraint( aRule, SKEW_CONSTRAINT );

        if( skew )
            data->SetMaxSkew( toMM( skew->GetValue().Max() ) );

        return data;
    }

    case PERMITTED_LAYERS:
    {
        auto data = std::make_shared<DRC_RE_PERMITTED_LAYERS_CONSTRAINT_DATA>();
        data->SetRuleName( aRule.m_Name );

        const DRC_CONSTRAINT* constraint = findConstraint( aRule, ASSERTION_CONSTRAINT );

        if( constraint && constraint->m_Test )
        {
            wxString expr = constraint->m_Test->GetExpression();

            data->SetTopLayerEnabled( expr.Contains( wxS( "F.Cu" ) ) );
            data->SetBottomLayerEnabled( expr.Contains( wxS( "B.Cu" ) ) );
        }

        return data;
    }

    case ALLOWED_ORIENTATION:
    {
        auto data = std::make_shared<DRC_RE_ALLOWED_ORIENTATION_CONSTRAINT_DATA>();
        data->SetRuleName( aRule.m_Name );
        data->SetConstraintCode( wxS( "allowed_orientation" ) );

        const DRC_CONSTRAINT* constraint = findConstraint( aRule, ASSERTION_CONSTRAINT );

        if( constraint && constraint->m_Test )
        {
            wxString expr = constraint->m_Test->GetExpression();

            data->SetIsZeroDegreesAllowed( expr.Contains( wxS( "== 0 deg" ) ) );
            data->SetIsNinetyDegreesAllowed( expr.Contains( wxS( "== 90 deg" ) ) );
            data->SetIsOneEightyDegreesAllowed( expr.Contains( wxS( "== 180 deg" ) ) );
            data->SetIsTwoSeventyDegreesAllowed( expr.Contains( wxS( "== 270 deg" ) ) );

            if( !data->GetIsZeroDegreesAllowed() && !data->GetIsNinetyDegreesAllowed()
                && !data->GetIsOneEightyDegreesAllowed() && !data->GetIsTwoSeventyDegreesAllowed() )
            {
                data->SetIsAllDegreesAllowed( true );
            }
        }
        else
        {
            data->SetIsAllDegreesAllowed( true );
        }

        return data;
    }

    case VIAS_UNDER_SMD:
    {
        auto data = std::make_shared<DRC_RE_BOOL_INPUT_CONSTRAINT_DATA>();
        data->SetRuleName( aRule.m_Name );
        data->SetConstraintCode( wxS( "disallow via" ) );

        const DRC_CONSTRAINT* constraint = findConstraint( aRule, DISALLOW_CONSTRAINT );

        if( constraint )
        {
            int viaFlags = DRC_DISALLOW_THROUGH_VIAS | DRC_DISALLOW_BLIND_VIAS | DRC_DISALLOW_BURIED_VIAS
                           | DRC_DISALLOW_MICRO_VIAS;
            data->SetBoolInputValue( ( constraint->m_DisallowFlags & viaFlags ) != 0 );
        }

        return data;
    }

    case CUSTOM_RULE:
    {
        auto data = std::make_shared<DRC_RE_CUSTOM_RULE_CONSTRAINT_DATA>();
        data->SetRuleName( aRule.m_Name );
        return data;
    }

    default:
    {
        // For numeric input types, create a generic numeric constraint data
        if( DRC_RULE_EDITOR_UTILS::IsNumericInputType( aPanel ) )
        {
            auto data = DRC_RULE_EDITOR_UTILS::CreateNumericConstraintData( aPanel );
            data->SetRuleName( aRule.m_Name );

            wxString code = DRC_RULE_EDITOR_UTILS::GetConstraintCode( aPanel );
            data->SetConstraintCode( code );

            // Find the first matching constraint from the claimed set
            for( DRC_CONSTRAINT_T type : aClaimedConstraints )
            {
                const DRC_CONSTRAINT* constraint = findConstraint( aRule, type );

                if( constraint )
                {
                    if( type == VIA_COUNT_CONSTRAINT )
                        data->SetNumericInputValue( toMM( constraint->GetValue().Max() ) );
                    else if( type == MIN_RESOLVED_SPOKES_CONSTRAINT )
                        data->SetNumericInputValue( constraint->GetValue().Min() );
                    else
                        data->SetNumericInputValue( toMM( constraint->GetValue().Min() ) );

                    break;
                }
            }

            return data;
        }

        // Fallback to custom rule
        auto data = std::make_shared<DRC_RE_CUSTOM_RULE_CONSTRAINT_DATA>();
        data->SetRuleName( aRule.m_Name );
        return data;
    }
    }
}


std::vector<DRC_RE_LOADED_PANEL_ENTRY> DRC_RULE_LOADER::LoadRule( const DRC_RULE& aRule,
                                                                   const wxString& aOriginalText )
{
    std::vector<DRC_RE_LOADED_PANEL_ENTRY> entries;

    // Get condition expression if present
    wxString condition;

    if( aRule.m_Condition )
        condition = aRule.m_Condition->GetExpression();

    // Match the rule to panels
    std::vector<DRC_PANEL_MATCH> matches = m_matcher.MatchRule( aRule );

    for( DRC_PANEL_MATCH& match : matches )
    {
        if( match.panelType == PERMITTED_LAYERS && match.claimedConstraints.count( ASSERTION_CONSTRAINT ) )
        {
            const DRC_CONSTRAINT* assertion = findConstraint( aRule, ASSERTION_CONSTRAINT );

            if( assertion && assertion->m_Test )
            {
                wxString expr = assertion->m_Test->GetExpression();

                if( expr.Contains( wxS( "Orientation" ) ) && !expr.Contains( wxS( "Layer" ) ) )
                    match.panelType = ALLOWED_ORIENTATION;
            }
        }

        if( match.panelType == ROUTING_WIDTH && match.claimedConstraints.count( TRACK_WIDTH_CONSTRAINT ) )
        {
            const DRC_CONSTRAINT* tw = findConstraint( aRule, TRACK_WIDTH_CONSTRAINT );

            if( tw && !tw->GetValue().HasOpt() && !tw->GetValue().HasMax() )
                match.panelType = MINIMUM_TRACK_WIDTH;
        }

        auto constraintData = createConstraintData( match.panelType, aRule, match.claimedConstraints );

        if( constraintData )
        {
            constraintData->SetRuleCondition( condition );

            DRC_RE_LOADED_PANEL_ENTRY entry( match.panelType, constraintData, aRule.m_Name,
                                             condition, aRule.m_Severity, aRule.m_LayerCondition );

            // Preserve original layer source text for round-trip fidelity
            entry.layerSource = aRule.m_LayerSource;

            // Store original text only for the first entry to avoid duplication issues
            if( entries.empty() )
                entry.originalRuleText = aOriginalText;

            entries.push_back( std::move( entry ) );
        }
    }

    // If no matches, create a custom rule entry
    if( entries.empty() )
    {
        auto customData = std::make_shared<DRC_RE_CUSTOM_RULE_CONSTRAINT_DATA>();
        customData->SetRuleName( aRule.m_Name );
        customData->SetRuleCondition( condition );
        customData->SetRuleText( aOriginalText );

        DRC_RE_LOADED_PANEL_ENTRY entry( CUSTOM_RULE, customData, aRule.m_Name, condition,
                                         aRule.m_Severity, aRule.m_LayerCondition );
        entry.layerSource = aRule.m_LayerSource;
        entry.originalRuleText = aOriginalText;
        entries.push_back( std::move( entry ) );
    }

    return entries;
}


std::vector<DRC_RE_LOADED_PANEL_ENTRY> DRC_RULE_LOADER::LoadFromString( const wxString& aRulesText )
{
    std::vector<DRC_RE_LOADED_PANEL_ENTRY> allEntries;
    std::vector<std::shared_ptr<DRC_RULE>> parsedRules;

    wxString rulesText = aRulesText;

    if( !rulesText.Contains( "(version" ) )
        rulesText.Prepend( "(version 1)\n" );

    try
    {
        DRC_RULES_PARSER parser( rulesText, "Rule Loader" );
        parser.Parse( parsedRules, nullptr );
    }
    catch( const IO_ERROR& )
    {
        return allEntries;
    }

    for( const auto& rule : parsedRules )
    {
        // Extract the actual original text from the file content
        wxString originalText = extractRuleText( aRulesText, rule->m_Name );

        std::vector<DRC_RE_LOADED_PANEL_ENTRY> ruleEntries = LoadRule( *rule, originalText );

        for( auto& entry : ruleEntries )
            allEntries.push_back( std::move( entry ) );
    }

    return allEntries;
}


wxString DRC_RULE_LOADER::extractRuleText( const wxString& aContent, const wxString& aRuleName )
{
    // Search for the rule by name, handling both quoted and unquoted names.
    // The quoted form includes the closing quote as a boundary so partial
    // matches like "Clearance" vs "Clearance for BGA" are not possible.
    // The unquoted form needs an explicit boundary check.
    wxString quotedSearch = wxString::Format( wxS( "(rule \"%s\"" ), aRuleName );
    wxString unquotedSearch = wxString::Format( wxS( "(rule %s" ), aRuleName );

    size_t startPos = aContent.find( quotedSearch );

    if( startPos == wxString::npos )
    {
        size_t pos = 0;

        while( ( pos = aContent.find( unquotedSearch, pos ) ) != wxString::npos )
        {
            size_t afterMatch = pos + unquotedSearch.length();

            if( afterMatch >= aContent.length()
                || aContent[afterMatch] == ')'
                || aContent[afterMatch] == ' '
                || aContent[afterMatch] == '\n'
                || aContent[afterMatch] == '\r'
                || aContent[afterMatch] == '\t' )
            {
                startPos = pos;
                break;
            }

            pos = afterMatch;
        }
    }

    if( startPos == wxString::npos )
        return wxEmptyString;

    // Find the matching closing parenthesis by counting balanced parens
    int parenCount = 0;
    size_t endPos = startPos;
    bool inString = false;
    bool escaped = false;

    for( size_t i = startPos; i < aContent.length(); ++i )
    {
        wxUniChar c = aContent[i];

        if( escaped )
        {
            escaped = false;
            continue;
        }

        if( c == '\\' )
        {
            escaped = true;
            continue;
        }

        if( c == '"' )
        {
            inString = !inString;
            continue;
        }

        if( inString )
            continue;

        if( c == '(' )
        {
            parenCount++;
        }
        else if( c == ')' )
        {
            parenCount--;

            if( parenCount == 0 )
            {
                endPos = i;
                break;
            }
        }
    }

    if( parenCount != 0 )
        return wxEmptyString;

    return aContent.Mid( startPos, endPos - startPos + 1 );
}


std::vector<DRC_RE_LOADED_PANEL_ENTRY> DRC_RULE_LOADER::LoadFile( const wxString& aPath )
{
    std::vector<DRC_RE_LOADED_PANEL_ENTRY> allEntries;

    wxFFile file( aPath, "r" );

    if( !file.IsOpened() )
        return allEntries;

    wxString content;
    file.ReadAll( &content );
    file.Close();

    return LoadFromString( content );
}
