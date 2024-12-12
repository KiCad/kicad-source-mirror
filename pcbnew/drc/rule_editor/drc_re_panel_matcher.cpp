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

#include "drc_re_panel_matcher.h"

#include <algorithm>


/**
 * Panel Claim Priority System
 *
 * When a DRC rule contains multiple constraints, the matcher must decide which
 * graphical panels should edit each constraint. The priority system ensures
 * deterministic, user-friendly matching.
 *
 * Priority tiers:
 *   100  Multi-constraint panels that combine related constraints
 *        (e.g., ROUTING_DIFF_PAIR handles track_width + diff_pair_gap together)
 *   80-90  Two-constraint panels (VIA_STYLE, TEXT_HEIGHT_THICKNESS)
 *   60-70  Single-constraint panels with min/opt/max semantics
 *   20-30  Generic single-constraint panels
 *   5-15   Boolean and fallback panels
 *   0      CUSTOM_RULE catches all unmapped constraints
 *
 * Algorithm:
 *   1. Extract all constraint types from the rule
 *   2. Iterate through claims in priority order (highest first)
 *   3. For each claim, check if ALL required constraints are present
 *   4. If match found, claim those constraints and remove them from the pool
 *   5. Continue until all constraints are claimed
 *   6. Any remaining constraints go to CUSTOM_RULE fallback
 *
 * Example: A rule with track_width + diff_pair_gap + clearance becomes
 *   - ROUTING_DIFF_PAIR panel (claims track_width + diff_pair_gap at priority 100)
 *   - MINIMUM_CLEARANCE panel (claims clearance at priority 30)
 */

DRC_PANEL_MATCHER::DRC_PANEL_MATCHER()
{
    initClaims();
}


void DRC_PANEL_MATCHER::initClaims()
{
    // Priority 100: ROUTING_DIFF_PAIR claims track_width + diff_pair_gap + diff_pair_uncoupled
    // This must be checked before ROUTING_WIDTH which also wants track_width
    m_claims.emplace_back(
            ROUTING_DIFF_PAIR,
            std::set<DRC_CONSTRAINT_T>{ TRACK_WIDTH_CONSTRAINT, DIFF_PAIR_GAP_CONSTRAINT },
            std::set<DRC_CONSTRAINT_T>{ MAX_UNCOUPLED_CONSTRAINT },
            100 );

    // Priority 90: VIA_STYLE claims via_diameter + hole_size
    m_claims.emplace_back(
            VIA_STYLE,
            std::set<DRC_CONSTRAINT_T>{ VIA_DIAMETER_CONSTRAINT, HOLE_SIZE_CONSTRAINT },
            std::set<DRC_CONSTRAINT_T>{},
            90 );

    // Priority 80: MINIMUM_TEXT_HEIGHT_THICKNESS claims text_height + text_thickness
    m_claims.emplace_back(
            MINIMUM_TEXT_HEIGHT_AND_THICKNESS,
            std::set<DRC_CONSTRAINT_T>{ TEXT_HEIGHT_CONSTRAINT, TEXT_THICKNESS_CONSTRAINT },
            std::set<DRC_CONSTRAINT_T>{},
            80 );

    // Priority 70: ROUTING_WIDTH claims track_width (single constraint, min/opt/max values)
    m_claims.emplace_back(
            ROUTING_WIDTH,
            std::set<DRC_CONSTRAINT_T>{ TRACK_WIDTH_CONSTRAINT },
            std::set<DRC_CONSTRAINT_T>{},
            70 );

    // Priority 60: ABSOLUTE_LENGTH claims length (single constraint, min/opt/max values)
    m_claims.emplace_back(
            ABSOLUTE_LENGTH,
            std::set<DRC_CONSTRAINT_T>{ LENGTH_CONSTRAINT },
            std::set<DRC_CONSTRAINT_T>{},
            60 );

    // Priority 50: PERMITTED_LAYERS claims assertion for layer-based rules
    m_claims.emplace_back(
            PERMITTED_LAYERS,
            std::set<DRC_CONSTRAINT_T>{ ASSERTION_CONSTRAINT },
            std::set<DRC_CONSTRAINT_T>{},
            50 );

    // Priority 40: ALLOWED_ORIENTATION claims assertion for orientation-based rules
    // Note that both PERMITTED_LAYERS and ALLOWED_ORIENTATION claim ASSERTION_CONSTRAINT.
    // The UI must differentiate based on assertion content.
    m_claims.emplace_back(
            ALLOWED_ORIENTATION,
            std::set<DRC_CONSTRAINT_T>{ ASSERTION_CONSTRAINT },
            std::set<DRC_CONSTRAINT_T>{},
            40 );

    // Priority 30: Generic single-constraint panels

    // Clearance constraints
    m_claims.emplace_back(
            MINIMUM_CLEARANCE,
            std::set<DRC_CONSTRAINT_T>{ CLEARANCE_CONSTRAINT },
            std::set<DRC_CONSTRAINT_T>{},
            30 );

    m_claims.emplace_back(
            COPPER_TO_EDGE_CLEARANCE,
            std::set<DRC_CONSTRAINT_T>{ EDGE_CLEARANCE_CONSTRAINT },
            std::set<DRC_CONSTRAINT_T>{},
            30 );

    m_claims.emplace_back(
            COPPER_TO_HOLE_CLEARANCE,
            std::set<DRC_CONSTRAINT_T>{ HOLE_CLEARANCE_CONSTRAINT },
            std::set<DRC_CONSTRAINT_T>{},
            30 );

    m_claims.emplace_back(
            HOLE_TO_HOLE_CLEARANCE,
            std::set<DRC_CONSTRAINT_T>{ HOLE_TO_HOLE_CONSTRAINT },
            std::set<DRC_CONSTRAINT_T>{},
            30 );

    m_claims.emplace_back(
            COURTYARD_CLEARANCE,
            std::set<DRC_CONSTRAINT_T>{ COURTYARD_CLEARANCE_CONSTRAINT },
            std::set<DRC_CONSTRAINT_T>{},
            30 );

    m_claims.emplace_back(
            PHYSICAL_CLEARANCE,
            std::set<DRC_CONSTRAINT_T>{ PHYSICAL_CLEARANCE_CONSTRAINT },
            std::set<DRC_CONSTRAINT_T>{},
            30 );

    m_claims.emplace_back(
            SILK_TO_SILK_CLEARANCE,
            std::set<DRC_CONSTRAINT_T>{ SILK_CLEARANCE_CONSTRAINT },
            std::set<DRC_CONSTRAINT_T>{},
            30 );

    m_claims.emplace_back(
            CREEPAGE_DISTANCE,
            std::set<DRC_CONSTRAINT_T>{ CREEPAGE_CONSTRAINT },
            std::set<DRC_CONSTRAINT_T>{},
            30 );

    // Size constraints
    m_claims.emplace_back(
            HOLE_SIZE,
            std::set<DRC_CONSTRAINT_T>{ HOLE_SIZE_CONSTRAINT },
            std::set<DRC_CONSTRAINT_T>{},
            20 );

    m_claims.emplace_back(
            MINIMUM_VIA_DIAMETER,
            std::set<DRC_CONSTRAINT_T>{ VIA_DIAMETER_CONSTRAINT },
            std::set<DRC_CONSTRAINT_T>{},
            20 );

    m_claims.emplace_back(
            MINIMUM_ANNULAR_WIDTH,
            std::set<DRC_CONSTRAINT_T>{ ANNULAR_WIDTH_CONSTRAINT },
            std::set<DRC_CONSTRAINT_T>{},
            20 );

    m_claims.emplace_back(
            MINIMUM_CONNECTION_WIDTH,
            std::set<DRC_CONSTRAINT_T>{ CONNECTION_WIDTH_CONSTRAINT },
            std::set<DRC_CONSTRAINT_T>{},
            20 );

    // Via count
    m_claims.emplace_back(
            MAXIMUM_VIA_COUNT,
            std::set<DRC_CONSTRAINT_T>{ VIA_COUNT_CONSTRAINT },
            std::set<DRC_CONSTRAINT_T>{},
            20 );

    // Solder mask/paste
    m_claims.emplace_back(
            SOLDERMASK_EXPANSION,
            std::set<DRC_CONSTRAINT_T>{ SOLDER_MASK_EXPANSION_CONSTRAINT },
            std::set<DRC_CONSTRAINT_T>{},
            20 );

    m_claims.emplace_back(
            SOLDERPASTE_EXPANSION,
            std::set<DRC_CONSTRAINT_T>{ SOLDER_PASTE_ABS_MARGIN_CONSTRAINT },
            std::set<DRC_CONSTRAINT_T>{},
            20 );

    // Thermal relief
    m_claims.emplace_back(
            MINIMUM_THERMAL_RELIEF_SPOKE_COUNT,
            std::set<DRC_CONSTRAINT_T>{ MIN_RESOLVED_SPOKES_CONSTRAINT },
            std::set<DRC_CONSTRAINT_T>{},
            20 );

    // Skew constraints
    m_claims.emplace_back(
            MATCHED_LENGTH_DIFF_PAIR,
            std::set<DRC_CONSTRAINT_T>{ SKEW_CONSTRAINT },
            std::set<DRC_CONSTRAINT_T>{},
            20 );

    // Diff pair gap only (when not combined with track_width)
    m_claims.emplace_back(
            ROUTING_DIFF_PAIR,
            std::set<DRC_CONSTRAINT_T>{ DIFF_PAIR_GAP_CONSTRAINT },
            std::set<DRC_CONSTRAINT_T>{ MAX_UNCOUPLED_CONSTRAINT },
            15 );

    // Priority 5: Bool constraints
    m_claims.emplace_back(
            VIAS_UNDER_SMD,
            std::set<DRC_CONSTRAINT_T>{ DISALLOW_CONSTRAINT },
            std::set<DRC_CONSTRAINT_T>{},
            5 );

    // Sort claims by priority descending
    std::sort( m_claims.begin(), m_claims.end(),
               []( const DRC_PANEL_CLAIM& a, const DRC_PANEL_CLAIM& b )
               {
                   return a.priority > b.priority;
               } );
}


bool DRC_PANEL_MATCHER::matchesClaim( const DRC_PANEL_CLAIM&            aClaim,
                                       const std::set<DRC_CONSTRAINT_T>& aConstraints,
                                       std::set<DRC_CONSTRAINT_T>*       aClaimedOut )
{
    // Check that all required constraints are present
    for( DRC_CONSTRAINT_T required : aClaim.requiredConstraints )
    {
        if( aConstraints.find( required ) == aConstraints.end() )
            return false;
    }

    // Build the set of claimed constraints (required + any optional that are present)
    std::set<DRC_CONSTRAINT_T> claimed = aClaim.requiredConstraints;

    for( DRC_CONSTRAINT_T optional : aClaim.optionalConstraints )
    {
        if( aConstraints.find( optional ) != aConstraints.end() )
            claimed.insert( optional );
    }

    if( aClaimedOut )
        *aClaimedOut = claimed;

    return true;
}


std::vector<DRC_PANEL_MATCH> DRC_PANEL_MATCHER::MatchRule( const DRC_RULE& aRule )
{
    std::vector<DRC_PANEL_MATCH> matches;

    // Extract all constraint types from the rule
    std::set<DRC_CONSTRAINT_T> remaining;

    for( const DRC_CONSTRAINT& constraint : aRule.m_Constraints )
    {
        if( constraint.m_Type != NULL_CONSTRAINT )
            remaining.insert( constraint.m_Type );
    }

    if( remaining.empty() )
        return matches;

    // Iterate through claims by priority (already sorted in initClaims)
    for( const DRC_PANEL_CLAIM& claim : m_claims )
    {
        if( remaining.empty() )
            break;

        std::set<DRC_CONSTRAINT_T> claimed;

        if( matchesClaim( claim, remaining, &claimed ) )
        {
            matches.emplace_back( claim.panelType, claimed );

            // Remove claimed constraints from remaining set
            for( DRC_CONSTRAINT_T type : claimed )
                remaining.erase( type );
        }
    }

    // Any remaining constraints go to CUSTOM_RULE panel
    if( !remaining.empty() )
        matches.emplace_back( CUSTOM_RULE, remaining );

    return matches;
}


bool DRC_PANEL_MATCHER::CanPanelLoad( DRC_RULE_EDITOR_CONSTRAINT_NAME   aPanel,
                                       const std::set<DRC_CONSTRAINT_T>& aConstraints )
{
    for( const DRC_PANEL_CLAIM& claim : m_claims )
    {
        if( claim.panelType == aPanel )
        {
            // Check if all constraints can be handled by this panel
            std::set<DRC_CONSTRAINT_T> allHandled = claim.requiredConstraints;
            allHandled.insert( claim.optionalConstraints.begin(), claim.optionalConstraints.end() );

            for( DRC_CONSTRAINT_T type : aConstraints )
            {
                if( allHandled.find( type ) == allHandled.end() )
                    return false;
            }

            return true;
        }
    }

    // CUSTOM_RULE can load anything
    return aPanel == CUSTOM_RULE;
}


DRC_RULE_EDITOR_CONSTRAINT_NAME DRC_PANEL_MATCHER::GetPanelForConstraint( DRC_CONSTRAINT_T aConstraintType )
{
    std::set<DRC_CONSTRAINT_T> singleConstraint{ aConstraintType };

    for( const DRC_PANEL_CLAIM& claim : m_claims )
    {
        // Only match claims with exactly one required constraint and no optional
        if( claim.requiredConstraints.size() == 1 &&
            claim.optionalConstraints.empty() &&
            claim.requiredConstraints.count( aConstraintType ) > 0 )
        {
            return claim.panelType;
        }
    }

    return CUSTOM_RULE;
}
