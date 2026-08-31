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

#ifndef DRILL_CHART_MODEL_H
#define DRILL_CHART_MODEL_H

#include <string>
#include <vector>

#include <drill/drill_operation.h>
#include <drill/drill_chart_template.h>
#include <drill/drill_symbol_profile.h>

class BOARD;


/**
 * One chart row.
 *
 * Operations and sites are counted separately because they differ on any board with
 * backdrills, where several machining actions share one XY location.
 */
struct DRILL_CHART_GROUP
{
    /**
     * Identity of this row. May be finer than the symbol key when the chart splits further.
     */
    std::string m_Key;

    /**
     * Identity of the symbol, which always belongs to the profile's grouping. Two rows can
     * legitimately share one mark when a chart splits a profile group.
     */
    std::string m_SymbolKey;

    int      m_Diameter = 0;
    VECTOR2I m_SizeXY;
    bool     m_IsSlot = false;
    bool     m_NotPlated = false;

    PCB_LAYER_ID   m_TopLayer = F_Cu;
    PCB_LAYER_ID   m_BottomLayer = B_Cu;
    DRILL_OP_KIND  m_Kind = DRILL_OP_KIND::PRIMARY_DRILL;
    HOLE_ATTRIBUTE m_Attribute = HOLE_ATTRIBUTE::HOLE_UNKNOWN;

    std::optional<int>      m_StubLength;

    bool m_Filled = false;
    bool m_Capped = false;
    bool m_TopCovered = false;
    bool m_BottomCovered = false;
    bool m_TopPlugged = false;
    bool m_BottomPlugged = false;
    bool m_TopTented = false;
    bool m_BottomTented = false;

    DRILL_POST_MACHINING m_FrontPostMachining;
    DRILL_POST_MACHINING m_BackPostMachining;

    int m_OperationCount = 0;
    int m_SiteCount = 0;
    int m_SlotCount = 0;

    DRILL_SYMBOL_ASSIGNMENT m_Symbol;

    std::vector<DRILL_OPERATION_ID> m_Members;

    /**
     * Distinct hole locations, kept so totals can union them rather than sum site counts.
     */
    std::vector<VECTOR2I> m_Sites;
};


struct DRILL_CHART_TOTALS
{
    int m_Operations = 0;
    int m_Sites = 0;
    int m_Groups = 0;
};


/**
 * Turns the board's drill operations into chart rows for one symbol profile.
 *
 * Rows come back sorted by diameter then by the group key, so a chart is stable across
 * rebuilds even when the board changes underneath it.
 */
/**
 * What a particular chart wants out of the model.
 *
 * Filtering has to happen per operation rather than per finished group. With a grouping key
 * disabled, one group can hold both plated and non-plated operations, and rejecting the group
 * afterwards either drops holes that were asked for or keeps holes that were not.
 */
struct DRILL_CHART_ROW_SPEC
{
    DRILL_CHART_FILTER m_Filter;
};


class DRILL_CHART_MODEL
{
public:
    DRILL_CHART_MODEL( const DRILL_SYMBOL_PROFILE& aProfile );

    void Build( const BOARD& aBoard, const std::vector<DRILL_SPAN>& aSpans );

    void Build( const BOARD& aBoard, const std::vector<DRILL_SPAN>& aSpans,
                const DRILL_CHART_ROW_SPEC& aSpec );

    const std::vector<DRILL_CHART_GROUP>& Groups() const { return m_groups; }
    const DRILL_CHART_TOTALS& Totals() const { return m_totals; }

private:
    const DRILL_SYMBOL_PROFILE& m_profile;

    std::vector<DRILL_CHART_GROUP> m_groups;
    DRILL_CHART_TOTALS             m_totals;
};


/**
 * Every drill span present on the board, through-holes first.
 *
 * Shared with the drill writers so a per-span chart and a per-span drill file cannot
 * disagree about which spans exist.
 */
std::vector<DRILL_SPAN> EnumerateDrillSpans( const BOARD& aBoard );

#endif // DRILL_CHART_MODEL_H
