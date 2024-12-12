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

#ifndef DRC_RE_RULE_LOADER_H_
#define DRC_RE_RULE_LOADER_H_

#include <memory>
#include <set>
#include <vector>

#include <wx/string.h>

#include <drc/drc_rule.h>

#include "drc_re_loaded_rule.h"
#include "drc_re_panel_matcher.h"


/**
 * Loads DRC rules from .kicad_dru files and converts them to panel entries.
 *
 * This class handles the conversion of text-based DRC rules into the
 * appropriate graphical panel representations. Rules with multiple
 * constraints may be split across multiple panel entries, with the
 * condition and severity duplicated to each.
 */
class DRC_RULE_LOADER
{
public:
    DRC_RULE_LOADER();

    /**
     * Load a single DRC_RULE and convert it to panel entries.
     *
     * If the rule contains more constraints than a single panel can handle,
     * it will be split into multiple entries, preserving order.
     *
     * @param aRule The parsed DRC rule to convert.
     * @param aOriginalText The original text of the rule for round-trip fidelity.
     * @return Vector of panel entries representing the rule.
     */
    std::vector<DRC_RE_LOADED_PANEL_ENTRY> LoadRule( const DRC_RULE& aRule,
                                                     const wxString& aOriginalText );

    /**
     * Load all rules from a .kicad_dru file.
     *
     * @param aPath Path to the rules file.
     * @return Vector of panel entries for all rules in the file.
     */
    std::vector<DRC_RE_LOADED_PANEL_ENTRY> LoadFile( const wxString& aPath );

    /**
     * Load rules from a text string.
     *
     * @param aRulesText The text content of the rules.
     * @return Vector of panel entries for all rules in the text.
     */
    std::vector<DRC_RE_LOADED_PANEL_ENTRY> LoadFromString( const wxString& aRulesText );

private:
    /**
     * Create the appropriate constraint data object for a panel type.
     *
     * @param aPanel The target panel type.
     * @param aRule The source rule.
     * @param aClaimedConstraints The constraints this panel should handle.
     * @return Shared pointer to the populated constraint data.
     */
    std::shared_ptr<DRC_RE_BASE_CONSTRAINT_DATA>
    createConstraintData( DRC_RULE_EDITOR_CONSTRAINT_NAME   aPanel,
                          const DRC_RULE&                   aRule,
                          const std::set<DRC_CONSTRAINT_T>& aClaimedConstraints );

    /**
     * Find a constraint of a specific type in a rule.
     *
     * @param aRule The rule to search.
     * @param aType The constraint type to find.
     * @return Pointer to the constraint, or nullptr if not found.
     */
    const DRC_CONSTRAINT* findConstraint( const DRC_RULE& aRule, DRC_CONSTRAINT_T aType );

    /**
     * Convert internal units (nanometers) to millimeters.
     */
    double toMM( int aValue );

    DRC_PANEL_MATCHER m_matcher;
};


#endif // DRC_RE_RULE_LOADER_H_
