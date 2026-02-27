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
#include <string_utils.h>
#include <wx/ffile.h>

#include "drc_re_base_constraint_data.h"
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

    // Group entries by (ruleName, condition) for merging same-name same-condition rules
    // Use a vector to preserve insertion order
    std::vector<std::pair<std::pair<wxString, wxString>, std::vector<const DRC_RE_LOADED_PANEL_ENTRY*>>>
            groupedEntries;
    std::map<std::pair<wxString, wxString>, size_t> groupIndex;

    for( const DRC_RE_LOADED_PANEL_ENTRY& entry : aEntries )
    {
        auto key = std::make_pair( entry.ruleName, entry.condition );
        auto it = groupIndex.find( key );

        if( it == groupIndex.end() )
        {
            groupIndex[key] = groupedEntries.size();
            groupedEntries.push_back( { key, { &entry } } );
        }
        else
        {
            groupedEntries[it->second].second.push_back( &entry );
        }
    }

    // Generate rule text for each group
    for( const auto& [key, entries] : groupedEntries )
    {
        wxString ruleText;

        if( entries.size() == 1 )
        {
            // Single entry, no merge needed
            ruleText = generateRuleText( *entries[0], aBoard );
        }
        else
        {
            // Multiple entries with same name/condition need merging
            ruleText = generateMergedRuleText( entries, aBoard );
        }

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

    wxString ruleText = aEntry.constraintData->GetGeneratedRule();

    if( ruleText.IsEmpty() )
    {
        RULE_GENERATION_CONTEXT ctx;
        ctx.ruleName = aEntry.ruleName;
        ctx.conditionExpression = aEntry.condition;
        ctx.constraintCode = aEntry.constraintData->GetConstraintCode();
        ctx.comment = aEntry.constraintData->GetComment();

        // Generate layer clause if layers are specified
        if( aEntry.layerCondition.any() )
        {
            if( aEntry.panelType == SILK_TO_SOLDERMASK_CLEARANCE )
            {
                bool isFront = aEntry.layerCondition.test( F_SilkS );
                wxString silkCond = wxString::Format(
                        wxS( "A.Layer == '%s' && B.Layer == '%s'" ),
                        isFront ? wxS( "F.SilkS" ) : wxS( "B.SilkS" ),
                        isFront ? wxS( "F.Mask" ) : wxS( "B.Mask" ) );

                if( !ctx.conditionExpression.IsEmpty() )
                    ctx.conditionExpression = silkCond + wxS( " && " ) + ctx.conditionExpression;
                else
                    ctx.conditionExpression = silkCond;
            }
            else if( aBoard )
            {
                ctx.layerClause = generateLayerClause( aEntry.layerCondition, aBoard );
            }
        }

        ruleText = aEntry.constraintData->GenerateRule( ctx );
    }

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


wxString DRC_RULE_SAVER::generateMergedRuleText(
        const std::vector<const DRC_RE_LOADED_PANEL_ENTRY*>& aEntries,
        const BOARD*                                         aBoard )
{
    if( aEntries.empty() )
        return wxEmptyString;

    // Check if all entries are unedited and the first one has original text
    // If so, we can use round-trip preservation
    bool allUnedited = true;

    for( const auto* entry : aEntries )
    {
        if( entry->wasEdited )
        {
            allUnedited = false;
            break;
        }
    }

    if( allUnedited && !aEntries[0]->originalRuleText.IsEmpty() )
        return aEntries[0]->originalRuleText;

    // Otherwise, merge constraint clauses from all entries
    const DRC_RE_LOADED_PANEL_ENTRY* firstEntry = aEntries[0];

    RULE_GENERATION_CONTEXT ctx;
    ctx.ruleName = firstEntry->ruleName;
    ctx.conditionExpression = firstEntry->condition;

    // Generate layer clause if layers are specified on any entry
    for( const auto* entry : aEntries )
    {
        if( entry->layerCondition.any() && aBoard )
        {
            ctx.layerClause = generateLayerClause( entry->layerCondition, aBoard );
            break;
        }
    }

    // Collect all constraint clauses from all entries
    std::vector<wxString> allClauses;

    for( const auto* entry : aEntries )
    {
        if( entry->constraintData )
        {
            RULE_GENERATION_CONTEXT entryCtx;
            entryCtx.ruleName = entry->ruleName;
            entryCtx.conditionExpression = entry->condition;
            entryCtx.constraintCode = entry->constraintData->GetConstraintCode();

            auto clauses = entry->constraintData->GetConstraintClauses( entryCtx );

            for( const wxString& clause : clauses )
            {
                if( !clause.IsEmpty() )
                    allClauses.push_back( clause );
            }
        }
    }

    // Build the merged rule
    wxString rule;
    rule << wxS( "(rule " ) << DRC_RE_BASE_CONSTRAINT_DATA::sanitizeRuleName( ctx.ruleName )
         << wxS( "\n" );

    if( !ctx.layerClause.IsEmpty() )
        rule << wxS( "\t" ) << ctx.layerClause << wxS( "\n" );

    for( const wxString& clause : allClauses )
        rule << wxS( "\t" ) << clause << wxS( "\n" );

    if( !ctx.conditionExpression.IsEmpty() )
    {
        rule << wxS( "\t(condition \"" )
             << EscapeString( ctx.conditionExpression, CTX_QUOTED_STR ) << wxS( "\")\n" );
    }

    // Add severity if any entry has non-default severity
    for( const auto* entry : aEntries )
    {
        if( entry->severity != RPT_SEVERITY_UNDEFINED && entry->severity != RPT_SEVERITY_ERROR )
        {
            rule << wxS( "\t" ) << generateSeverityClause( entry->severity ) << wxS( "\n" );
            break;
        }
    }

    rule << wxS( ")" );

    return rule;
}
