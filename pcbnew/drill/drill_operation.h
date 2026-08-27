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

#ifndef DRILL_OPERATION_H
#define DRILL_OPERATION_H

#include <optional>

#include <drill/drill_span.h>
#include <geometry/eda_angle.h>
#include <kiid.h>
#include <layer_ids.h>
#include <math/vector2d.h>
#include <padstack.h>

class BOARD_ITEM;


/**
 * Which of the padstack's drill props produced the operation.
 */
enum class DRILL_OP_KIND
{
    PRIMARY_DRILL,
    SECONDARY_DRILL,
    TERTIARY_DRILL
};


/**
 * Globally unique handle for one machining action.
 */
struct DRILL_OPERATION_ID
{
    KIID          m_Owner;
    DRILL_OP_KIND m_Kind = DRILL_OP_KIND::PRIMARY_DRILL;

    bool operator<( const DRILL_OPERATION_ID& aOther ) const
    {
        return m_Owner == aOther.m_Owner ? m_Kind < aOther.m_Kind : m_Owner < aOther.m_Owner;
    }
};


struct DRILL_POST_MACHINING
{
    PAD_DRILL_POST_MACHINING_MODE m_Mode = PAD_DRILL_POST_MACHINING_MODE::UNKNOWN;
    int                           m_Size = 0;
    int                           m_Depth = 0;
    int                           m_Angle = 0;

    bool IsSet() const { return m_Mode != PAD_DRILL_POST_MACHINING_MODE::UNKNOWN; }
};


/**
 * One machining action, which is also one NC hit and one tool assignment.
 *
 * Post-machining is not a kind of its own. It rides on the record that made the hole, the
 * way HOLE_INFO does, so one operation projects onto exactly one HOLE_INFO. A consumer that
 * wants a countersink on a row of its own expands the front and back fields itself.
 */
struct DRILL_OPERATION
{
    DRILL_OP_KIND m_Kind = DRILL_OP_KIND::PRIMARY_DRILL;

    BOARD_ITEM* m_SourceItem = nullptr;   ///< non-owning, valid only for this enumeration
    KIID        m_SourceId;

    VECTOR2I  m_Position;
    /**
     * Source axes, never normalized to (width, length). The Excellon writer picks slot
     * endpoints from whichever of X and Y is longer.
     */
    VECTOR2I  m_SizeXY;
    EDA_ANGLE m_Orientation = ANGLE_0;
    int       m_Diameter = 0;
    bool      m_IsSlot = false;

    PCB_LAYER_ID m_TopLayer = F_Cu;
    PCB_LAYER_ID m_BottomLayer = B_Cu;
    PCB_LAYER_ID m_DrillStart = UNDEFINED_LAYER;
    PCB_LAYER_ID m_DrillEnd = UNDEFINED_LAYER;

    bool           m_NotPlated = false;
    HOLE_ATTRIBUTE m_Attribute = HOLE_ATTRIBUTE::HOLE_UNKNOWN;

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

    std::optional<int> m_StubLength;

    DRILL_OPERATION_ID Id() const { return { m_SourceId, m_Kind }; }

    bool IsBackdrill() const { return m_Kind != DRILL_OP_KIND::PRIMARY_DRILL; }
};


/**
 * Selects which operations EnumerateDrillOperations() returns.
 *
 * The defaults mirror what the drill writers ask for, so a query built with only a span set
 * reproduces the writer's own hole list.
 */
struct DRILL_QUERY
{
    DRILL_SPAN m_Span;

    /**
     * Emit only non-plated holes. Vias are suppressed outright. For pads the flag is
     * ignored when m_MergePTHNPTH is set.
     */
    bool m_NonPlatedOnly = false;

    /**
     * Emit plated and non-plated together, as the merged drill file does.
     */
    bool m_MergePTHNPTH = false;
};

#endif // DRILL_OPERATION_H
