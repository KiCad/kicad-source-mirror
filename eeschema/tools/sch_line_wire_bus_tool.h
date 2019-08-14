/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 CERN
 * Copyright (C) 2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <tools/ee_tool_base.h>
#include <core/optional.h>
#include <sch_base_frame.h>


class SCH_BUS_WIRE_ENTRY;
class SCH_LABEL;
class SCH_EDIT_FRAME;
class EE_SELECTION_TOOL;


/// Collection of data related to the bus unfolding tool
struct BUS_UNFOLDING_T
{
    bool in_progress;   ///< True if bus unfold operation is running
    bool offset;        ///< True if the bus entry should be offset from origin
    bool label_placed;  ///< True if user has placed the net label

    wxPoint origin;     ///< Origin (on the bus) of the unfold
    wxString net_name;  ///< Net label for the unfolding operation

    SCH_BUS_WIRE_ENTRY* entry;
    SCH_LABEL* label;
};


/**
 * Class SCH_LINE_DRAWING_TOOL
 *
 * Tool responsible for drawing/placing items (symbols, wires, busses, labels, etc.)
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
    int doDrawSegments( const std::string& aTool, int aType, SCH_LINE* aSegment );
    SCH_LINE* startSegments( int aType, const VECTOR2D& aPos );
    SCH_LINE* doUnfoldBus( const wxString& aNet );
    void finishSegments();

    ///> Sets up handlers for various events.
    void setTransitions() override;

private:
    /// Data related to bus unfolding tool.
    BUS_UNFOLDING_T       m_busUnfold;
};

#endif /* SCH_LINE_WIRE_BUS_TOOL_H */
