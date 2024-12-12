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

#ifndef DRC_RE_RULE_SAVER_H_
#define DRC_RE_RULE_SAVER_H_

#include <vector>

#include <wx/string.h>

#include "drc_re_loaded_rule.h"

class BOARD;


/**
 * Saves DRC panel entries back to .kicad_dru files.
 *
 * This class handles the conversion of graphical panel entries back to text-based
 * DRC rules. It supports round-trip preservation, meaning unedited rules will be
 * saved with their original text intact.
 */
class DRC_RULE_SAVER
{
public:
    DRC_RULE_SAVER();

    /**
     * Save all panel entries to a file.
     *
     * @param aPath Path to the output file.
     * @param aEntries Vector of panel entries to save.
     * @param aBoard Optional board for layer name resolution.
     * @return True if the file was saved successfully.
     */
    bool SaveFile( const wxString&                               aPath,
                   const std::vector<DRC_RE_LOADED_PANEL_ENTRY>& aEntries,
                   const BOARD*                                  aBoard = nullptr );

    /**
     * Generate rule text from panel entries.
     *
     * @param aEntries Vector of panel entries to convert.
     * @param aBoard Optional board for layer name resolution.
     * @return String containing all rules in DRC file format.
     */
    wxString GenerateRulesText( const std::vector<DRC_RE_LOADED_PANEL_ENTRY>& aEntries,
                                const BOARD*                                  aBoard = nullptr );

private:
    /**
     * Generate the rule text for a single panel entry.
     *
     * If the entry has not been edited and has original text, the original
     * text is returned for round-trip fidelity. Otherwise, the rule is
     * regenerated from the panel data.
     *
     * @param aEntry The panel entry to convert.
     * @param aBoard Optional board for layer name resolution.
     * @return Rule text in DRC file format.
     */
    wxString generateRuleText( const DRC_RE_LOADED_PANEL_ENTRY& aEntry,
                               const BOARD*                     aBoard );

    /**
     * Generate a layer clause from an LSET.
     *
     * @param aLayers The layer set.
     * @param aBoard The board for layer name resolution.
     * @return Layer clause string like "(layer \"F.Cu\" \"B.Cu\")".
     */
    wxString generateLayerClause( const LSET& aLayers, const BOARD* aBoard );

    /**
     * Generate a severity clause.
     *
     * @param aSeverity The severity level.
     * @return Severity clause string like "(severity warning)".
     */
    wxString generateSeverityClause( SEVERITY aSeverity );
};


#endif // DRC_RE_RULE_SAVER_H_
