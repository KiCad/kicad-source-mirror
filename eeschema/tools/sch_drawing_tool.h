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

#ifndef SCH_DRAWING_TOOL_H
#define SCH_DRAWING_TOOL_H

#include <tool/tool_interactive.h>
#include <tool/tool_menu.h>
#include <core/optional.h>
#include <sch_base_frame.h>


class SCH_COMPONENT;
class SCH_BUS_WIRE_ENTRY;
class SCH_LABEL;
class SCHLIB_FILTER;
class SCH_EDIT_FRAME;


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
 * Class SCH_DRAWING_TOOL
 *
 * Tool responsible for drawing/placing items (symbols, wires, busses, labels, etc.)
 */

class SCH_DRAWING_TOOL : public TOOL_INTERACTIVE
{
public:
    SCH_DRAWING_TOOL();
    ~SCH_DRAWING_TOOL();

    /// @copydoc TOOL_INTERACTIVE::Init()
    bool Init() override;

    /// @copydoc TOOL_INTERACTIVE::Reset()
    void Reset( RESET_REASON aReason ) override;

    ///> Get the SCH_DRAWING_TOOL top-level context menu
    inline TOOL_MENU& GetToolMenu() { return m_menu; }

    int StartWire( const TOOL_EVENT& aEvent );
    int StartBus( const TOOL_EVENT& aEvent );
    int AddJunction( const TOOL_EVENT& aEvent );
    int AddLabel( const TOOL_EVENT& aEvent );
    int AddGlobalLabel( const TOOL_EVENT& aEvent );

    int PlaceSymbol( const TOOL_EVENT& aEvent );
    int PlacePower( const TOOL_EVENT& aEvent );
    int DrawWire( const TOOL_EVENT& aEvent );
    int DrawBus( const TOOL_EVENT& aEvent );
    int UnfoldBus( const TOOL_EVENT& aEvent );
    int PlaceNoConnect( const TOOL_EVENT& aEvent );
    int PlaceJunction( const TOOL_EVENT& aEvent );
    int PlaceBusWireEntry( const TOOL_EVENT& aEvent );
    int PlaceBusBusEntry( const TOOL_EVENT& aEvent );
    int PlaceLabel( const TOOL_EVENT& aEvent );
    int PlaceGlobalLabel( const TOOL_EVENT& aEvent );
    int PlaceHierarchicalLabel( const TOOL_EVENT& aEvent );
    int DrawSheet( const TOOL_EVENT& aEvent );
    int ResizeSheet( const TOOL_EVENT& aEvent );
    int PlaceSheetPin( const TOOL_EVENT& aEvent );
    int ImportSheetPin( const TOOL_EVENT& aEvent );
    int PlaceSchematicText( const TOOL_EVENT& aEvent );
    int DrawLines( const TOOL_EVENT& aEvent );
    int PlaceImage( const TOOL_EVENT& aEvent );

private:

    int doAddItem( KICAD_T aType );

    int doPlaceComponent( SCH_COMPONENT* aComponent, SCHLIB_FILTER* aFilter,
                          SCH_BASE_FRAME::HISTORY_LIST aHistoryList );

    int doSingleClickPlace( KICAD_T aType );

    int doTwoClickPlace( KICAD_T aType );

    int doDrawSegments( int aType, SCH_LINE* aSegment );
    SCH_LINE* startSegments( int aType, const wxPoint& aPos );
    void finishSegments();

    int doDrawSheet( SCH_SHEET* aSheet );

    ///> Sets up handlers for various events.
    void setTransitions() override;

private:
    KIGFX::SCH_VIEW* m_view;
    KIGFX::VIEW_CONTROLS* m_controls;
    SCH_EDIT_FRAME* m_frame;

    /// Data related to bus unfolding tool.
    BUS_UNFOLDING_T m_busUnfold;

    /// Menu model displayed by the tool.
    TOOL_MENU m_menu;
};

#endif /* SCH_DRAWING_TOOL_H */
