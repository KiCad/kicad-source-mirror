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

#include <drill/drill_chart_model.h>

#include <algorithm>
#include <map>
#include <set>

#include <board.h>
#include <drill/drill_enumerator.h>
#include <pcb_track.h>


DRILL_CHART_MODEL::DRILL_CHART_MODEL( const DRILL_SYMBOL_PROFILE& aProfile ) :
        m_profile( aProfile )
{
}


namespace
{

/// Whether one operation belongs in a chart carrying this filter
bool matchesFilter( const DRILL_OPERATION& aOp, const DRILL_CHART_FILTER& aFilter )
{
    if( aOp.m_NotPlated && !aFilter.m_NonPlated )
        return false;

    if( !aOp.m_NotPlated && !aFilter.m_Plated )
        return false;

    if( aOp.m_IsSlot && !aFilter.m_Slots )
        return false;

    if( aOp.m_Kind != DRILL_OP_KIND::PRIMARY_DRILL && !aFilter.m_Backdrills )
        return false;

    if( aOp.m_Attribute == HOLE_ATTRIBUTE::HOLE_PAD_CASTELLATED && !aFilter.m_Castellated )
        return false;

    const bool isVia = aOp.m_Attribute == HOLE_ATTRIBUTE::HOLE_VIA_THROUGH
                       || aOp.m_Attribute == HOLE_ATTRIBUTE::HOLE_VIA_BURIED;

    if( isVia && !aFilter.m_Vias )
        return false;

    return true;
}


} // namespace


void DRILL_CHART_MODEL::Build( const BOARD& aBoard, const std::vector<DRILL_SPAN>& aSpans )
{
    Build( aBoard, aSpans, DRILL_CHART_ROW_SPEC() );
}


void DRILL_CHART_MODEL::Build( const BOARD& aBoard, const std::vector<DRILL_SPAN>& aSpans,
                               const DRILL_CHART_ROW_SPEC& aSpec )
{
    m_groups.clear();
    m_totals = DRILL_CHART_TOTALS();

    std::map<std::string, DRILL_CHART_GROUP> byKey;
    std::map<std::string, std::set<std::pair<int, int>>> sitesByKey;

    auto collect =
            [&]( const DRILL_QUERY& aQuery )
            {
                for( const DRILL_OPERATION& op : EnumerateDrillOperations( aBoard, aQuery ) )
                {
                    if( !matchesFilter( op, aSpec.m_Filter ) )
                        continue;

                    const std::string key = m_profile.GroupKeyString( op );
                    auto [it, inserted] = byKey.try_emplace( key );
                    DRILL_CHART_GROUP& group = it->second;

                    if( inserted )
                    {
                        group.m_Key = key;
                        group.m_SymbolKey = key;
                        group.m_Diameter = op.m_Diameter;
                        group.m_SizeXY = op.m_SizeXY;
                        group.m_IsSlot = op.m_IsSlot;
                        group.m_NotPlated = op.m_NotPlated;
                        group.m_TopLayer = op.m_TopLayer;
                        group.m_BottomLayer = op.m_BottomLayer;
                        group.m_Kind = op.m_Kind;
                        group.m_Attribute = op.m_Attribute;
                        group.m_StubLength = op.m_StubLength;
                        group.m_Filled = op.m_Filled;
                        group.m_Capped = op.m_Capped;
                        group.m_TopCovered = op.m_TopCovered;
                        group.m_BottomCovered = op.m_BottomCovered;
                        group.m_TopPlugged = op.m_TopPlugged;
                        group.m_BottomPlugged = op.m_BottomPlugged;
                        group.m_TopTented = op.m_TopTented;
                        group.m_BottomTented = op.m_BottomTented;
                        group.m_FrontPostMachining = op.m_FrontPostMachining;
                        group.m_BackPostMachining = op.m_BackPostMachining;
                    }

                    group.m_OperationCount++;

                    if( op.m_IsSlot )
                        group.m_SlotCount++;

                    group.m_Members.push_back( op.Id() );
                    sitesByKey[key].emplace( op.m_Position.x, op.m_Position.y );
                }
            };

    for( const DRILL_SPAN& span : aSpans )
    {
        DRILL_QUERY plated;
        plated.m_Span = span;
        collect( plated );

        DRILL_QUERY nonPlated;
        nonPlated.m_Span = span;
        nonPlated.m_NonPlatedOnly = true;
        collect( nonPlated );
    }

    std::set<std::pair<int, int>> allSites;

    for( auto& [key, group] : byKey )
    {
        group.m_SiteCount = static_cast<int>( sitesByKey[key].size() );

        for( const auto& [x, y] : sitesByKey[key] )
            group.m_Sites.emplace_back( x, y );

        allSites.insert( sitesByKey[key].begin(), sitesByKey[key].end() );
        m_totals.m_Operations += group.m_OperationCount;
        m_groups.push_back( group );
    }

    m_totals.m_Sites = static_cast<int>( allSites.size() );
    m_totals.m_Groups = static_cast<int>( m_groups.size() );

    std::sort( m_groups.begin(), m_groups.end(),
               []( const DRILL_CHART_GROUP& a, const DRILL_CHART_GROUP& b )
               {
                   if( a.m_NotPlated != b.m_NotPlated )
                       return !a.m_NotPlated;

                   if( a.m_Diameter != b.m_Diameter )
                       return a.m_Diameter < b.m_Diameter;

                   return a.m_Key < b.m_Key;
               } );
}


std::vector<DRILL_SPAN> EnumerateDrillSpans( const BOARD& aBoard )
{
    std::set<DRILL_SPAN> unique;

    for( PCB_TRACK* track : aBoard.Tracks() )
    {
        if( track->Type() != PCB_VIA_T )
            continue;

        PCB_VIA*     via = static_cast<PCB_VIA*>( track );
        PCB_LAYER_ID top;
        PCB_LAYER_ID bottom;

        via->LayerPair( &top, &bottom );

        if( DRILL_LAYER_PAIR( top, bottom ) != DRILL_LAYER_PAIR( F_Cu, B_Cu ) )
            unique.emplace( top, bottom, false, false );

        auto addBackdrill =
                [&]( const PADSTACK::DRILL_PROPS& aDrill )
                {
                    if( aDrill.start == UNDEFINED_LAYER || aDrill.end == UNDEFINED_LAYER )
                        return;

                    if( aDrill.size.x <= 0 && aDrill.size.y <= 0 )
                        return;

                    unique.emplace( aDrill.start, aDrill.end, true, false );
                };

        addBackdrill( via->Padstack().SecondaryDrill() );
        addBackdrill( via->Padstack().TertiaryDrill() );
    }

    std::vector<DRILL_SPAN> spans;
    spans.emplace_back( F_Cu, B_Cu, false, false );

    for( const DRILL_SPAN& span : unique )
    {
        if( span.m_IsBackdrill || span.Pair() != DRILL_LAYER_PAIR( F_Cu, B_Cu ) )
            spans.push_back( span );
    }

    return spans;
}
