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

#ifndef DRC_RE_LOADED_RULE_H_
#define DRC_RE_LOADED_RULE_H_

#include <memory>
#include <wx/string.h>

#include <layer_ids.h>
#include <widgets/report_severity.h>

#include "drc_rule_editor_enums.h"
#include "drc_re_base_constraint_data.h"


/**
 * Represents a rule loaded from a .kicad_dru file and mapped to a panel.
 *
 * When loading DRC rules, a single rule may be split across multiple panel
 * entries if it contains more constraints than any one panel can handle.
 * This struct captures all the information needed to display the rule in a
 * panel and preserve round-trip fidelity when saving.
 */
struct DRC_RE_LOADED_PANEL_ENTRY
{
    DRC_RULE_EDITOR_CONSTRAINT_NAME              panelType = CUSTOM_RULE;
    std::shared_ptr<DRC_RE_BASE_CONSTRAINT_DATA> constraintData;
    wxString                                     ruleName;
    wxString                                     condition;
    SEVERITY                                     severity = RPT_SEVERITY_UNDEFINED;
    LSET                                         layerCondition;

    wxString originalRuleText;
    bool     wasEdited = false;

    DRC_RE_LOADED_PANEL_ENTRY() = default;

    DRC_RE_LOADED_PANEL_ENTRY( DRC_RULE_EDITOR_CONSTRAINT_NAME              aPanel,
                               std::shared_ptr<DRC_RE_BASE_CONSTRAINT_DATA> aData,
                               const wxString&                              aRuleName,
                               const wxString&                              aCondition,
                               SEVERITY                                     aSeverity,
                               const LSET&                                  aLayers ) :
            panelType( aPanel ),
            constraintData( std::move( aData ) ),
            ruleName( aRuleName ),
            condition( aCondition ),
            severity( aSeverity ),
            layerCondition( aLayers )
    {
    }
};


#endif // DRC_RE_LOADED_RULE_H_
