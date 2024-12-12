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

#include "drc_re_rule_saver.h"

#include <board.h>
#include <lset.h>
#include <wx/ffile.h>

#include "drc_rule_editor_enums.h"


DRC_RULE_SAVER::DRC_RULE_SAVER()
{
}


bool DRC_RULE_SAVER::SaveFile( const wxString&                               aPath,
                                const std::vector<DRC_RE_LOADED_PANEL_ENTRY>& aEntries,
                                const BOARD*                                  aBoard )
{
    wxFFile file( aPath, "w" );

    if( !file.IsOpened() )
        return false;

    wxString content = GenerateRulesText( aEntries, aBoard );
    file.Write( content );
    file.Close();

    return true;
}


wxString DRC_RULE_SAVER::GenerateRulesText( const std::vector<DRC_RE_LOADED_PANEL_ENTRY>& aEntries,
                                             const BOARD*                                  aBoard )
{
    wxString result = "(version 1)\n";

    for( const DRC_RE_LOADED_PANEL_ENTRY& entry : aEntries )
    {
        wxString ruleText = generateRuleText( entry, aBoard );

        if( !ruleText.IsEmpty() )
            result += ruleText + "\n";
    }

    return result;
}


wxString DRC_RULE_SAVER::generateRuleText( const DRC_RE_LOADED_PANEL_ENTRY& aEntry,
                                            const BOARD*                     aBoard )
{
    // Round-trip preservation: return original text if not edited
    if( !aEntry.wasEdited && !aEntry.originalRuleText.IsEmpty() )
        return aEntry.originalRuleText;

    // Otherwise, regenerate from panel data
    if( !aEntry.constraintData )
        return wxEmptyString;

    RULE_GENERATION_CONTEXT ctx;
    ctx.ruleName = aEntry.ruleName;
    ctx.conditionExpression = aEntry.condition;
    ctx.constraintCode = aEntry.constraintData->GetConstraintCode();
    ctx.comment = aEntry.constraintData->GetComment();

    // Generate layer clause if layers are specified
    if( aEntry.layerCondition.any() && aBoard )
        ctx.layerClause = generateLayerClause( aEntry.layerCondition, aBoard );

    // Generate the rule text from the constraint data
    wxString ruleText = aEntry.constraintData->GenerateRule( ctx );

    // If severity is specified and not default, we need to inject it
    // The GenerateRule method should handle this, but we verify here
    if( aEntry.severity != RPT_SEVERITY_UNDEFINED && aEntry.severity != RPT_SEVERITY_ERROR )
    {
        wxString severityClause = generateSeverityClause( aEntry.severity );

        if( !severityClause.IsEmpty() && !ruleText.Contains( "(severity" ) )
        {
            // Insert severity clause before the closing paren
            size_t lastParen = ruleText.rfind( ')' );

            if( lastParen != wxString::npos )
            {
                ruleText = ruleText.Left( lastParen ) + "\n\t" + severityClause + ")";
            }
        }
    }

    return ruleText;
}


wxString DRC_RULE_SAVER::generateLayerClause( const LSET& aLayers, const BOARD* aBoard )
{
    if( !aBoard || !aLayers.any() )
        return wxEmptyString;

    wxString layerStr = "(layer";

    for( PCB_LAYER_ID layer : aLayers.Seq() )
        layerStr += " \"" + aBoard->GetLayerName( layer ) + "\"";

    layerStr += ")";

    return layerStr;
}


wxString DRC_RULE_SAVER::generateSeverityClause( SEVERITY aSeverity )
{
    switch( aSeverity )
    {
    case RPT_SEVERITY_IGNORE:    return "(severity ignore)";
    case RPT_SEVERITY_WARNING:   return "(severity warning)";
    case RPT_SEVERITY_ERROR:     return "(severity error)";
    case RPT_SEVERITY_EXCLUSION: return "(severity exclusion)";
    default:                     return wxEmptyString;
    }
}
