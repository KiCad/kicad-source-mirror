/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 CERN
 * Copyright (C) 2019-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <wx/gdicmn.h>
#include <wx/string.h>
#include <string>
#include <vector>

#include <math/vector2d.h>
#include <tool/tool_event.h>
#include <sch_edit_frame.h>
#include <sch_line.h>

#include <ee_tool_base.h>


class SCH_BUS_WIRE_ENTRY;
class SCH_LABEL;
class SCH_EDIT_FRAME;
class EE_SELECTION_TOOL;


/// Collection of data related to the bus unfolding tool
struct BUS_UNFOLDING_T
{
    bool in_progress;   ///< True if bus unfold operation is running
    bool flipX;         ///< True if the bus entry should be flipped in the x-axis
    bool flipY;         ///< True if the bus entry should be flipped in the y-axis
    bool label_placed;  ///< True if user has placed the net label

    wxPoint origin;     ///< Origin (on the bus) of the unfold
    wxString net_name;  ///< Net label for the unfolding operation

    SCH_BUS_WIRE_ENTRY* entry;
    SCH_LABEL* label;
};


struct DRAW_SEGMENT_EVENT_PARAMS
{
    SCH_LAYER_ID layer;
    bool         quitOnDraw;
};

/**
 * Tool responsible for drawing/placing items (symbols, wires, buses, labels, etc.)
 */

class SCH_LINE_WIRE_BUS_TOOL : public EE_TOOL_BASE<SCH_EDIT_FRAME>
{
public:
    SCH_LINE_WIRE_BUS_TOOL();
    ~SCH_LINE_WIRE_BUS_TOOL();

    /// @copydoc TOOL_INTERACTIVE::Init()
    bool Init() override;

    int DrawSegments( const TOOL_EVENT& aEvent );
    int UnfoldBus( const TOOL_EVENT& aEvent );

    // SELECTION_CONDITIONs:
    static bool IsDrawingLine( const SELECTION& aSelection );
    static bool IsDrawingWire( const SELECTION& aSelection );
    static bool IsDrawingBus( const SELECTION& aSelection );
    static bool IsDrawingLineWireOrBus( const SELECTION& aSelection );

    /**
     * Handle the addition of junctions to a selection of objects
     */
    int AddJunctionsIfNeeded( const TOOL_EVENT& aEvent );

private:
    int       doDrawSegments( const std::string& aTool, int aType, bool aQuitOnDraw );
    SCH_LINE* startSegments( int aType, const VECTOR2D& aPos );
    SCH_LINE* doUnfoldBus( const wxString& aNet, wxPoint aPos = wxDefaultPosition );
    void finishSegments();

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
    const SCH_SHEET_PIN* getSheetPin( const wxPoint& aPosition );

    /**
     * Compute the middle coordinate for 2 segments from the start point to \a aPosition
     * with the segments kept in the horizontal or vertical axis only.
     *
     * @param aSegments A pair of pointers to a #SCH_LINE objects containing the first line
     *                  break point to compute.
     * @param aPosition A reference to a wxPoint object containing the coordinates of the
     *                  position used to calculate the line break point.
     */
    void computeBreakPoint( const std::pair<SCH_LINE*, SCH_LINE*>& aSegments, wxPoint& aPosition );

private:
    /// Data related to bus unfolding tool.
    BUS_UNFOLDING_T         m_busUnfold;

    /// Storage for the line segments while drawing
    std::vector<SCH_LINE*>   m_wires;
};

#endif /* SCH_LINE_WIRE_BUS_TOOL_H */
