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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <drill/drill_symbol_assigner.h>

#include <set>

#include <board.h>
#include <board_design_settings.h>
#include <drill/drill_enumerator.h>
#include <plotters/drill_markers.h>


void AssignDrillSymbols( std::vector<DRILL_CHART_GROUP>& aGroups, DRILL_SYMBOL_PROFILE& aProfile )
{
    const int shapeCount = DRILL_MARKERS::CuratedShapeCount();

    std::set<int>      usedShapes;
    std::set<wxString> usedLetters;

    if( aProfile.GetFreezeAssignments() )
    {
        for( DRILL_CHART_GROUP& group : aGroups )
        {
            if( const DRILL_SYMBOL_ASSIGNMENT* existing = aProfile.GetAssignment( group.m_SymbolKey ) )
            {
                group.m_Symbol = *existing;

                if( existing->m_MarkMode == DRILL_MARK_MODE::SHAPE )
                    usedShapes.insert( existing->m_ShapeIndex );
                else if( existing->m_MarkMode == DRILL_MARK_MODE::LETTER )
                    usedLetters.insert( existing->m_Letter );
            }
        }
    }

    auto nextLetter =
            [&]() -> wxString
            {
                for( int i = 0; i < 26 * 27; ++i )
                {
                    wxString candidate;

                    if( i < 26 )
                        candidate = wxString::Format( "%c", 'A' + i );
                    else
                        candidate = wxString::Format( "%c%c", 'A' + ( i / 26 ) - 1, 'A' + ( i % 26 ) );

                    if( !usedLetters.count( candidate ) )
                        return candidate;
                }

                return wxT( "?" );
            };

    for( DRILL_CHART_GROUP& group : aGroups )
    {
        if( aProfile.GetFreezeAssignments() && aProfile.GetAssignment( group.m_SymbolKey ) )
            continue;

        DRILL_SYMBOL_ASSIGNMENT assignment;

        switch( aProfile.GetMarkPolicy() )
        {
        case DRILL_MARK_POLICY::SIZE_TEXT:
            assignment.m_MarkMode = DRILL_MARK_MODE::SIZE_TEXT;
            break;

        case DRILL_MARK_POLICY::LETTERS:
            assignment.m_MarkMode = DRILL_MARK_MODE::LETTER;
            assignment.m_Letter = nextLetter();
            usedLetters.insert( assignment.m_Letter );
            break;

        case DRILL_MARK_POLICY::SHAPES:
        case DRILL_MARK_POLICY::SHAPES_THEN_LETTERS:
        {
            int shape = -1;

            for( int i = 0; i < shapeCount; ++i )
            {
                if( !usedShapes.count( i ) )
                {
                    shape = i;
                    break;
                }
            }

            if( shape >= 0 )
            {
                assignment.m_MarkMode = DRILL_MARK_MODE::SHAPE;
                assignment.m_ShapeIndex = shape;
                usedShapes.insert( shape );
            }
            else if( aProfile.GetMarkPolicy() == DRILL_MARK_POLICY::SHAPES_THEN_LETTERS )
            {
                assignment.m_MarkMode = DRILL_MARK_MODE::LETTER;
                assignment.m_Letter = nextLetter();
                usedLetters.insert( assignment.m_Letter );
            }
            else
            {
                // Out of distinguishable shapes, so reuse from the start rather than draw
                // nothing at all
                assignment.m_MarkMode = DRILL_MARK_MODE::SHAPE;
                assignment.m_ShapeIndex = 0;
            }

            break;
        }
        }

        group.m_Symbol = assignment;
        aProfile.SetAssignment( group.m_SymbolKey, assignment );
    }
}


std::map<std::string, DRILL_SYMBOL_ASSIGNMENT> ResolveDrillSymbols( const BOARD& aBoard )
{
    DRILL_SYMBOL_PROFILE profile = aBoard.GetDesignSettings().GetDrillSymbolProfile();

    DRILL_CHART_MODEL model( profile );
    model.Build( aBoard, EnumerateDrillSpans( aBoard ) );

    std::vector<DRILL_CHART_GROUP> groups = model.Groups();
    AssignDrillSymbols( groups, profile );

    std::map<std::string, DRILL_SYMBOL_ASSIGNMENT> resolved;

    for( const DRILL_CHART_GROUP& group : groups )
        resolved[group.m_SymbolKey] = group.m_Symbol;

    return resolved;
}


std::map<KIID, std::vector<DRILL_SYMBOL_ENTRY>> ResolveDrillSymbolsByItem(
        const BOARD& aBoard, const std::map<std::string, DRILL_SYMBOL_ASSIGNMENT>& aResolved )
{
    const DRILL_SYMBOL_PROFILE& profile = aBoard.GetDesignSettings().GetDrillSymbolProfile();

    std::map<KIID, std::vector<DRILL_SYMBOL_ENTRY>> byItem;

    DRILL_QUERY query;
    query.m_MergePTHNPTH = true;

    for( const DRILL_SPAN& span : EnumerateDrillSpans( aBoard ) )
    {
        query.m_Span = span;

        for( const DRILL_OPERATION& op : EnumerateDrillOperations( aBoard, query ) )
        {
            const auto it = aResolved.find( profile.GroupKeyString( op ) );

            if( it == aResolved.end() )
                continue;

            DRILL_SYMBOL_ENTRY entry;
            entry.m_Symbol = it->second;
            entry.m_Span = span;
            entry.m_Position = op.m_Position;
            entry.m_Kind = op.m_Kind;
            entry.m_Diameter = op.m_Diameter;
            entry.m_SizeXY = op.m_SizeXY;
            entry.m_Orientation = op.m_Orientation;
            entry.m_IsSlot = op.m_IsSlot;

            byItem[op.m_SourceId].push_back( entry );
        }
    }

    return byItem;
}
