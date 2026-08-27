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

#include <drill_legacy_adapter.h>


std::vector<HOLE_INFO> ToLegacyHoleList( const std::vector<DRILL_OPERATION>& aOperations )
{
    std::vector<HOLE_INFO> holes;
    holes.reserve( aOperations.size() );

    for( const DRILL_OPERATION& op : aOperations )
    {
        HOLE_INFO hole;

        hole.m_ItemParent = op.m_SourceItem;
        hole.m_Hole_Diameter = op.m_Diameter;
        hole.m_Tool_Reference = -1;
        hole.m_Hole_Size = op.m_SizeXY;
        hole.m_Hole_Orient = op.m_Orientation;
        hole.m_Hole_Shape = op.m_IsSlot ? 1 : 0;
        hole.m_Hole_Pos = op.m_Position;
        hole.m_Hole_Bottom_Layer = op.m_BottomLayer;
        hole.m_Hole_Top_Layer = op.m_TopLayer;
        hole.m_Hole_NotPlated = op.m_NotPlated;
        hole.m_HoleAttribute = op.m_Attribute;
        hole.m_Hole_Filled = op.m_Filled;
        hole.m_Hole_Capped = op.m_Capped;
        hole.m_Hole_Top_Covered = op.m_TopCovered;
        hole.m_Hole_Bot_Covered = op.m_BottomCovered;
        hole.m_Hole_Top_Plugged = op.m_TopPlugged;
        hole.m_Hole_Bot_Plugged = op.m_BottomPlugged;
        hole.m_Hole_Top_Tented = op.m_TopTented;
        hole.m_Hole_Bot_Tented = op.m_BottomTented;
        hole.m_IsBackdrill = op.IsBackdrill();
        hole.m_FrontPostMachining = op.m_FrontPostMachining.m_Mode;
        hole.m_FrontPostMachiningSize = op.m_FrontPostMachining.m_Size;
        hole.m_FrontPostMachiningDepth = op.m_FrontPostMachining.m_Depth;
        hole.m_FrontPostMachiningAngle = op.m_FrontPostMachining.m_Angle;
        hole.m_BackPostMachining = op.m_BackPostMachining.m_Mode;
        hole.m_BackPostMachiningSize = op.m_BackPostMachining.m_Size;
        hole.m_BackPostMachiningDepth = op.m_BackPostMachining.m_Depth;
        hole.m_BackPostMachiningAngle = op.m_BackPostMachining.m_Angle;
        hole.m_DrillStart = op.m_DrillStart;
        hole.m_DrillEnd = op.m_DrillEnd;
        hole.m_StubLength = op.m_StubLength;

        holes.push_back( hole );
    }

    return holes;
}
