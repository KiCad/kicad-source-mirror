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

#ifndef DRC_RE_PANEL_MATCHER_H_
#define DRC_RE_PANEL_MATCHER_H_

#include <set>
#include <vector>

#include <drc/drc_rule.h>
#include "drc_rule_editor_enums.h"


/**
 * Defines which constraint types a panel can claim.
 *
 * The matcher uses these claims to determine which panel should handle
 * a given set of constraints from a DRC rule file.
 */
struct DRC_PANEL_CLAIM
{
    DRC_RULE_EDITOR_CONSTRAINT_NAME panelType;
    std::set<DRC_CONSTRAINT_T>      requiredConstraints;
    std::set<DRC_CONSTRAINT_T>      optionalConstraints;
    int                             priority;

    DRC_PANEL_CLAIM( DRC_RULE_EDITOR_CONSTRAINT_NAME aPanel,
                     std::set<DRC_CONSTRAINT_T>      aRequired,
                     std::set<DRC_CONSTRAINT_T>      aOptional,
                     int                             aPriority ) :
            panelType( aPanel ),
            requiredConstraints( std::move( aRequired ) ),
            optionalConstraints( std::move( aOptional ) ),
            priority( aPriority )
    {
    }
};


/**
 * Result of matching a panel to constraints.
 */
struct DRC_PANEL_MATCH
{
    DRC_RULE_EDITOR_CONSTRAINT_NAME panelType;
    std::set<DRC_CONSTRAINT_T>      claimedConstraints;

    DRC_PANEL_MATCH( DRC_RULE_EDITOR_CONSTRAINT_NAME aPanel,
                     std::set<DRC_CONSTRAINT_T>      aClaimed ) :
            panelType( aPanel ),
            claimedConstraints( std::move( aClaimed ) )
    {
    }
};


/**
 * Maps DRC rule constraints to appropriate editor panels.
 *
 * When loading a .kicad_dru file, this class determines which graphical
 * panels should be used to edit each rule. Multi-constraint panels (like
 * VIA_STYLE which handles both via_diameter and hole_size) have higher
 * priority and are tried first.
 *
 * If a rule contains more constraints than any single panel can handle,
 * it will be split across multiple panels. Constraints that cannot be
 * mapped to any panel are sent to the CUSTOM_RULE fallback panel.
 */
class DRC_PANEL_MATCHER
{
public:
    DRC_PANEL_MATCHER();

    /**
     * Match a DRC rule to one or more panel types.
     *
     * The rule's constraints are analyzed and matched to panels using
     * a priority-based system. Multi-constraint panels are tried first.
     * If a rule has more constraints than one panel can handle, multiple
     * panel matches are returned.
     *
     * @param aRule The DRC rule to analyze.
     * @return Vector of panel matches, each claiming a subset of constraints.
     */
    std::vector<DRC_PANEL_MATCH> MatchRule( const DRC_RULE& aRule );

    /**
     * Check if a specific panel type can load a set of constraints.
     *
     * @param aPanel The panel type to check.
     * @param aConstraints The constraint types to check.
     * @return True if the panel can represent all given constraints.
     */
    bool CanPanelLoad( DRC_RULE_EDITOR_CONSTRAINT_NAME              aPanel,
                       const std::set<DRC_CONSTRAINT_T>& aConstraints );

    /**
     * Get the panel type for a single constraint.
     *
     * This is a convenience method for simple rules with one constraint.
     *
     * @param aConstraintType The DRC constraint type.
     * @return The recommended panel type, or CUSTOM_RULE if no match.
     */
    DRC_RULE_EDITOR_CONSTRAINT_NAME GetPanelForConstraint( DRC_CONSTRAINT_T aConstraintType );

private:
    void initClaims();

    bool matchesClaim( const DRC_PANEL_CLAIM&           aClaim,
                       const std::set<DRC_CONSTRAINT_T>& aConstraints,
                       std::set<DRC_CONSTRAINT_T>*      aClaimedOut );

    std::vector<DRC_PANEL_CLAIM> m_claims;
};


#endif // DRC_RE_PANEL_MATCHER_H_
