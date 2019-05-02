/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#ifndef SCH_LINE_DRAWING_TOOL_H
#define SCH_LINE_DRAWING_TOOL_H

#include <tool/tool_interactive.h>
#include <tool/tool_menu.h>
#include <core/optional.h>
#include <sch_base_frame.h>


class SCH_COMPONENT;
class SCH_BUS_WIRE_ENTRY;
class SCH_LABEL;
class SCHLIB_FILTER;
class SCH_EDIT_FRAME;
class SCH_SELECTION_TOOL;


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

class SCH_LINE_DRAWING_TOOL : public TOOL_INTERACTIVE
{
public:
    SCH_LINE_DRAWING_TOOL();
    ~SCH_LINE_DRAWING_TOOL();

    /// @copydoc TOOL_INTERACTIVE::Init()
    bool Init() override;

    /// @copydoc TOOL_INTERACTIVE::Reset()
    void Reset( RESET_REASON aReason ) override;

    ///> Get the SCH_LINE_DRAWING_TOOL top-level context menu
    inline TOOL_MENU& GetToolMenu() { return m_menu; }

    int StartWire( const TOOL_EVENT& aEvent );
    int StartBus( const TOOL_EVENT& aEvent );
    int StartLines( const TOOL_EVENT& aEvent );

    int DrawWire( const TOOL_EVENT& aEvent );
    int DrawBus( const TOOL_EVENT& aEvent );
    int DrawLines( const TOOL_EVENT& aEvent );

    // SELECTION_CONDITIONs:
    static bool IsDrawingLine( const SELECTION& aSelection );
    static bool IsDrawingWire( const SELECTION& aSelection );
    static bool IsDrawingBus( const SELECTION& aSelection );
    static bool IsDrawingLineWireOrBus( const SELECTION& aSelection );

private:

    int doDrawSegments( int aType, SCH_LINE* aSegment );
    SCH_LINE* startSegments( int aType, const wxPoint& aPos );
    SCH_LINE* unfoldBus( const wxString& aNet );
    void finishSegments();

    ///> Sets up handlers for various events.
    void setTransitions() override;

private:
    SCH_SELECTION_TOOL*   m_selectionTool;
    KIGFX::SCH_VIEW*      m_view;
    KIGFX::VIEW_CONTROLS* m_controls;
    SCH_EDIT_FRAME*       m_frame;

    /// Data related to bus unfolding tool.
    BUS_UNFOLDING_T       m_busUnfold;

    TOOL_MENU             m_menu;
};

#endif /* SCH_LINE_DRAWING_TOOL_H */
