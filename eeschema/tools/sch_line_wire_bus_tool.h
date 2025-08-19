/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 CERN
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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef SCH_LINE_WIRE_BUS_TOOL_H
#define SCH_LINE_WIRE_BUS_TOOL_H

#include <wx/string.h>
#include <string>
#include <vector>

#include <math/vector2d.h>
#include <sch_edit_frame.h>
#include <sch_line.h>

#include <sch_tool_base.h>

class TOOL_EVENT;

class SCH_BUS_WIRE_ENTRY;
class SCH_LABEL;
class SCH_EDIT_FRAME;
class SCH_SELECTION_TOOL;


/// Collection of data related to the bus unfolding tool
struct BUS_UNFOLDING_T
{
    bool in_progress;   ///< True if bus unfold operation is running
    bool flipX;         ///< True if the bus entry should be flipped in the x-axis
    bool flipY;         ///< True if the bus entry should be flipped in the y-axis
    bool label_placed;  ///< True if user has placed the net label

    VECTOR2I origin;   ///< Origin (on the bus) of the unfold
    wxString net_name;  ///< Net label for the unfolding operation

    SCH_BUS_WIRE_ENTRY* entry;
    SCH_LABEL* label;
};


struct DRAW_SEGMENT_EVENT_PARAMS
{
    SCH_LAYER_ID layer;
    bool         quitOnDraw;
    SCH_LINE*    sourceSegment;
};

/**
 * Tool responsible for drawing/placing items (symbols, wires, buses, labels, etc.)
 */

class SCH_LINE_WIRE_BUS_TOOL : public SCH_TOOL_BASE<SCH_EDIT_FRAME>
{
public:
    SCH_LINE_WIRE_BUS_TOOL();
    ~SCH_LINE_WIRE_BUS_TOOL();

    /// @copydoc TOOL_INTERACTIVE::Init()
    bool Init() override;

    /**
     * Break a single segment into two at the specified point.
     *
     * @param aCommit Transaction container used to record changes for undo/redo
     * @param aSegment Line segment to break
     * @param aPoint Point at which to break the segment
     * @param aNewSegment Pointer to the newly created segment (if created)
     * @param aScreen is the screen to examine
     */
    void BreakSegment( SCH_COMMIT* aCommit, SCH_LINE* aSegment, const VECTOR2I& aPoint, SCH_LINE** aNewSegment,
                       SCH_SCREEN* aScreen );

    /**
     * Check every wire and bus for a intersection at \a aPoint and break into two segments
     * at \a aPoint if an intersection is found.
     *
     * @param aCommit Transaction container used to record changes for undo/redo
     * @param aPoint Test this point for an intersection.
     * @param aScreen is the screen to examine.
     * @return True if any wires or buses were broken.
     */
    bool BreakSegments( SCH_COMMIT* aCommit, const VECTOR2I& aPoint, SCH_SCREEN* aScreen );

    /**
     * Test all junctions and bus entries in the schematic for intersections with wires and
     * buses and breaks any intersections into multiple segments.
     *
     * @param aCommit Transaction container used to record changes for undo/redo
     * @param aScreen is the screen to examine.
     * @return True if any wires or buses were broken.
     */
    bool BreakSegmentsOnJunctions( SCH_COMMIT* aCommit, SCH_SCREEN* aScreen );

    int DrawSegments( const TOOL_EVENT& aEvent );
    int UnfoldBus( const TOOL_EVENT& aEvent );

    // SELECTION_CONDITIONs:
    static bool IsDrawingLineWireOrBus( const SELECTION& aSelection );

    /**
     * Handle the addition of junctions to a selection of objects
     */
    int AddJunctionsIfNeeded( SCH_COMMIT* aCommit, SCH_SELECTION* aSelection );

    SCH_JUNCTION* AddJunction( SCH_COMMIT* aCommit, SCH_SCREEN* aScreen, const VECTOR2I& aPos );

    /**
     * Logic to remove wires when overlapping correct items
     */
    int TrimOverLappingWires( SCH_COMMIT* aCommit, SCH_SELECTION* aSelection );

private:
    int doDrawSegments( const TOOL_EVENT& aTool, SCH_COMMIT& aCommit, int aType,
                        bool aQuitOnDraw );

    SCH_LINE* startSegments( SCH_COMMIT& aCommit, int aType, const VECTOR2D& aPos,
                             SCH_LINE* aSegment = nullptr );

    /**
     * Choose a bus to unfold based on the current tool selection.
    */
    SCH_LINE* getBusForUnfolding();

    /**
     * Unfold the given bus from the given position.
     *
     * @param aNet The name of the net to unfold
     * @param aPos The position to unfold the bus from, which will be the cursor if
     *            not provided, and will then be snapped to the selected bus segment.
    */
    SCH_LINE* doUnfoldBus( SCH_COMMIT& aCommit, const wxString& aNet,
                           const std::optional<VECTOR2I>& aPos = std::nullopt );

    void finishSegments( SCH_COMMIT& aCommit );

    /**
     * Iterate over the wire list and removes the null segments and
     * overlapping segments to create a simplified wire list
     */
    void simplifyWireList();

    ///< Set up handlers for various events.
    void setTransitions() override;

    /**
     * Search for a sheet pin at a location.
     *
     * @param aPosition grid point to search for existing sheet pin
     * @return Pointer to sheet pin or nullptr on failure
     */
    const SCH_SHEET_PIN* getSheetPin( const VECTOR2I& aPosition );

    /**
     * Compute the middle coordinate for 2 segments from the start point to \a aPosition
     * with the segments kept in the horizontal or vertical axis only.
     *
     * @param aSegments A pair of pointers to a #SCH_LINE objects containing the first line
     *                  break point to compute.
     * @param aPosition A reference to a wxPoint object containing the coordinates of the
     *                  position used to calculate the line break point.
     * @param mode      LINE_MODE specifying the way to break the line
     * @param posture   Toggles the posture of the line
     */
    void computeBreakPoint( const std::pair<SCH_LINE*, SCH_LINE*>& aSegments, VECTOR2I& aPosition,
                            LINE_MODE mode, bool posture );

private:
    bool                    m_inDrawingTool;   // Reentrancy guard

    BUS_UNFOLDING_T         m_busUnfold;
    std::vector<SCH_LINE*>  m_wires;           // Lines being drawn
};

#endif /* SCH_LINE_WIRE_BUS_TOOL_H */
