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

#include <sch_line_wire_bus_tool.h>

#include <wx/debug.h>
#include <wx/gdicmn.h>
#include <wx/string.h>
#include <wx/translation.h>
#include <algorithm>
#include <cstdlib>
#include <iterator>
#include <memory>
#include <utility>
#include <vector>

#include <layer_ids.h>
#include <math/vector2d.h>
#include <advanced_config.h>
#include <gal/graphics_abstraction_layer.h>
#include <view/view_controls.h>
#include <tool/actions.h>
#include <tool/conditional_menu.h>
#include <tool/selection.h>
#include <tool/selection_conditions.h>
#include <tool/tool_event.h>
#include <trigo.h>
#include <eeschema_id.h>
#include <sch_bus_entry.h>
#include <sch_connection.h>
#include <sch_edit_frame.h>
#include <sch_junction.h>
#include <sch_line.h>
#include <sch_screen.h>
#include <sch_sheet.h>
#include <sch_group.h>
#include <sch_sheet_pin.h>
#include <schematic.h>
#include <sch_commit.h>
#include <sch_actions.h>
#include <junction_helpers.h>
#include <ee_grid_helper.h>
#include <sch_selection.h>
#include <sch_selection_tool.h>


using BUS_GETTER = std::function<SCH_LINE*()>;

class BUS_UNFOLD_MENU : public ACTION_MENU
{
public:
    /**
     * @param aBusGetter Function to get the bus to unfold, which will probably
     *                   be looking for a likely bus in a selection.
    */
    BUS_UNFOLD_MENU( BUS_GETTER aBusGetter ) :
        ACTION_MENU( true ),
        m_showTitle( false ),
        m_busGetter( aBusGetter )
    {
        SetIcon( BITMAPS::add_line2bus );
        SetTitle( _( "Unfold from Bus" ) );
    }

    void SetShowTitle()
    {
        m_showTitle = true;
    }

    bool PassHelpTextToHandler() override { return true; }

protected:
    ACTION_MENU* create() const override
    {
        return new BUS_UNFOLD_MENU( m_busGetter );
    }

private:
    void update() override
    {
        SCH_LINE* bus = m_busGetter();
        Clear();
        // Pick up the pointer again because it may have been changed by SchematicCleanUp
        bus = m_busGetter();

        if( !bus )
        {
            Append( ID_POPUP_SCH_UNFOLD_BUS, _( "No bus selected" ), wxEmptyString );
            Enable( ID_POPUP_SCH_UNFOLD_BUS, false );
            return;
        }

        SCH_CONNECTION* connection = bus->Connection();

        if( !connection || !connection->IsBus() || connection->Members().empty() )
        {
            Append( ID_POPUP_SCH_UNFOLD_BUS, _( "Bus has no members" ), wxEmptyString );
            Enable( ID_POPUP_SCH_UNFOLD_BUS, false );
            return;
        }

        int idx = 0;

        if( m_showTitle )
        {
            Append( ID_POPUP_SCH_UNFOLD_BUS, _( "Unfold from Bus" ), wxEmptyString );
            Enable( ID_POPUP_SCH_UNFOLD_BUS, false );
        }

        for( const std::shared_ptr<SCH_CONNECTION>& member : connection->Members() )
        {
            int id = ID_POPUP_SCH_UNFOLD_BUS + ( idx++ );
            wxString name = member->FullLocalName();

            if( member->Type() == CONNECTION_TYPE::BUS )
            {
                ACTION_MENU* submenu = new ACTION_MENU( true, m_tool );
                AppendSubMenu( submenu, SCH_CONNECTION::PrintBusForUI( name ), name );

                for( const std::shared_ptr<SCH_CONNECTION>& sub_member : member->Members() )
                {
                    id = ID_POPUP_SCH_UNFOLD_BUS + ( idx++ );
                    name = sub_member->FullLocalName();
                    submenu->Append( id, SCH_CONNECTION::PrintBusForUI( name ), name );
                }
            }
            else
            {
                Append( id, name, wxEmptyString );
            }
        }
    }

    bool m_showTitle;
    BUS_GETTER m_busGetter;
};


/**
 * Settings for bus unfolding that are persistent across invocations of the tool
 */
struct BUS_UNFOLD_PERSISTENT_SETTINGS
{
    SPIN_STYLE label_spin_style;
};


static BUS_UNFOLD_PERSISTENT_SETTINGS busUnfoldPersistentSettings = {
    SPIN_STYLE::RIGHT,
};


SCH_LINE_WIRE_BUS_TOOL::SCH_LINE_WIRE_BUS_TOOL() :
        SCH_TOOL_BASE<SCH_EDIT_FRAME>( "eeschema.InteractiveDrawingLineWireBus" ),
        m_inDrawingTool( false )
{
    m_busUnfold = {};
    m_wires.reserve( 16 );
}


SCH_LINE_WIRE_BUS_TOOL::~SCH_LINE_WIRE_BUS_TOOL()
{
}


bool SCH_LINE_WIRE_BUS_TOOL::Init()
{
    SCH_TOOL_BASE::Init();

    const auto busGetter = [this]()
    {
        return getBusForUnfolding();
    };

    std::shared_ptr<BUS_UNFOLD_MENU>
            busUnfoldMenu = std::make_shared<BUS_UNFOLD_MENU>( busGetter );
    busUnfoldMenu->SetTool( this );
    m_menu->RegisterSubMenu( busUnfoldMenu );

    std::shared_ptr<BUS_UNFOLD_MENU> selBusUnfoldMenu = std::make_shared<BUS_UNFOLD_MENU>( busGetter );
    selBusUnfoldMenu->SetTool( m_selectionTool );
    m_selectionTool->GetToolMenu().RegisterSubMenu( selBusUnfoldMenu );

    auto wireOrBusTool =
            [this]( const SELECTION& aSel )
            {
                return ( m_frame->IsCurrentTool( SCH_ACTIONS::drawWire )
                      || m_frame->IsCurrentTool( SCH_ACTIONS::drawBus ) );
            };

    auto lineTool =
            [this]( const SELECTION& aSel )
            {
                return m_frame->IsCurrentTool( SCH_ACTIONS::drawLines );
            };

    auto belowRootSheetCondition =
            [&]( const SELECTION& aSel )
            {
                return m_frame->GetCurrentSheet().Last() != &m_frame->Schematic().Root();
            };

    auto busSelection = SCH_CONDITIONS::MoreThan( 0 )
                        && SCH_CONDITIONS::OnlyTypes( { SCH_ITEM_LOCATE_BUS_T } );

    auto haveHighlight =
            [&]( const SELECTION& sel )
            {
                SCH_EDIT_FRAME* editFrame = dynamic_cast<SCH_EDIT_FRAME*>( m_frame );

                return editFrame && !editFrame->GetHighlightedConnection().IsEmpty();
            };

    auto& ctxMenu = m_menu->GetMenu();

    // Build the tool menu
    //
    ctxMenu.AddItem( SCH_ACTIONS::clearHighlight,       haveHighlight && SCH_CONDITIONS::Idle, 1 );
    ctxMenu.AddSeparator(                               haveHighlight && SCH_CONDITIONS::Idle, 1 );

    ctxMenu.AddSeparator( 10 );
    ctxMenu.AddItem( SCH_ACTIONS::drawWire,             wireOrBusTool && SCH_CONDITIONS::Idle, 10 );
    ctxMenu.AddItem( SCH_ACTIONS::drawBus,              wireOrBusTool && SCH_CONDITIONS::Idle, 10 );
    ctxMenu.AddItem( SCH_ACTIONS::drawLines,            lineTool && SCH_CONDITIONS::Idle, 10 );

    ctxMenu.AddItem( SCH_ACTIONS::undoLastSegment,      SCH_CONDITIONS::ShowAlways, 10 );
    ctxMenu.AddItem( SCH_ACTIONS::switchSegmentPosture, SCH_CONDITIONS::ShowAlways, 10 );
    ctxMenu.AddItem( ACTIONS::finishInteractive,        IsDrawingLineWireOrBus, 10 );

    ctxMenu.AddMenu( busUnfoldMenu.get(),               SCH_CONDITIONS::Idle, 10 );

    ctxMenu.AddSeparator( 100 );
    ctxMenu.AddItem( SCH_ACTIONS::placeJunction,        wireOrBusTool && SCH_CONDITIONS::Idle, 100 );
    ctxMenu.AddItem( SCH_ACTIONS::placeLabel,           wireOrBusTool && SCH_CONDITIONS::Idle, 100 );
    ctxMenu.AddItem( SCH_ACTIONS::placeClassLabel,      wireOrBusTool && SCH_CONDITIONS::Idle, 100 );
    ctxMenu.AddItem( SCH_ACTIONS::placeGlobalLabel,     wireOrBusTool && SCH_CONDITIONS::Idle, 100 );
    ctxMenu.AddItem( SCH_ACTIONS::placeHierLabel,       wireOrBusTool && SCH_CONDITIONS::Idle, 100 );
    ctxMenu.AddItem( SCH_ACTIONS::breakWire,            wireOrBusTool && SCH_CONDITIONS::Idle, 100 );
    ctxMenu.AddItem( SCH_ACTIONS::slice,                ( wireOrBusTool || lineTool )
                                                            && SCH_CONDITIONS::Idle, 100 );
    ctxMenu.AddItem( SCH_ACTIONS::leaveSheet,           belowRootSheetCondition, 150 );

    ctxMenu.AddSeparator( 200 );
    ctxMenu.AddItem( SCH_ACTIONS::selectNode,           wireOrBusTool && SCH_CONDITIONS::Idle, 200 );
    ctxMenu.AddItem( SCH_ACTIONS::selectConnection,     wireOrBusTool && SCH_CONDITIONS::Idle, 200 );

    // Add bus unfolding to the selection tool
    //
    CONDITIONAL_MENU& selToolMenu = m_selectionTool->GetToolMenu().GetMenu();

    selToolMenu.AddMenu( selBusUnfoldMenu.get(),        busSelection && SCH_CONDITIONS::Idle, 100 );

    return true;
}


bool SCH_LINE_WIRE_BUS_TOOL::IsDrawingLineWireOrBus( const SELECTION& aSelection )
{
    // NOTE: for immediate hotkeys, it is NOT required that the line, wire or bus tool
    // be selected
    SCH_ITEM* item = (SCH_ITEM*) aSelection.Front();
    return item && item->IsNew() && item->Type() == SCH_LINE_T;
}


int SCH_LINE_WIRE_BUS_TOOL::DrawSegments( const TOOL_EVENT& aEvent )
{
    if( m_inDrawingTool )
        return 0;

    REENTRANCY_GUARD guard( &m_inDrawingTool );

    const DRAW_SEGMENT_EVENT_PARAMS* params = aEvent.Parameter<const DRAW_SEGMENT_EVENT_PARAMS*>();
    SCH_COMMIT                       commit( m_toolMgr );

    m_frame->PushTool( aEvent );
    m_toolMgr->RunAction( ACTIONS::selectionClear );

    if( aEvent.HasPosition() )
    {
        EE_GRID_HELPER    grid( m_toolMgr );
        GRID_HELPER_GRIDS gridType = ( params->layer == LAYER_NOTES ) ? GRID_GRAPHICS : GRID_WIRES;

        grid.SetSnap( !aEvent.Modifier( MD_SHIFT ) );
        grid.SetUseGrid( getView()->GetGAL()->GetGridSnapping() && !aEvent.DisableGridSnapping() );

        VECTOR2D cursorPos = grid.BestSnapAnchor( aEvent.Position(), gridType, nullptr );
        startSegments( commit, params->layer, cursorPos, params->sourceSegment );
    }

    return doDrawSegments( aEvent, commit, params->layer, params->quitOnDraw );
}


int SCH_LINE_WIRE_BUS_TOOL::UnfoldBus( const TOOL_EVENT& aEvent )
{
    if( m_inDrawingTool )
        return 0;

    REENTRANCY_GUARD guard( &m_inDrawingTool );

    SCH_COMMIT commit( m_toolMgr );
    wxString*  netPtr = aEvent.Parameter<wxString*>();
    wxString   net;
    SCH_LINE*  segment = nullptr;

    m_frame->PushTool( aEvent );
    Activate();

    if( netPtr )
    {
        net = *netPtr;
        delete netPtr;
    }
    else
    {
        const auto busGetter = [this]()
                {
                    return getBusForUnfolding();
                };
        BUS_UNFOLD_MENU unfoldMenu( busGetter );
        unfoldMenu.SetTool( this );
        unfoldMenu.SetShowTitle();

        SetContextMenu( &unfoldMenu, CMENU_NOW );

        while( TOOL_EVENT* evt = Wait() )
        {
            if( evt->Action() == TA_CHOICE_MENU_CHOICE )
            {
                std::optional<int> id = evt->GetCommandId();

                if( id && ( *id > 0 ) )
                    net = *evt->Parameter<wxString*>();

                break;
            }
            else if( evt->Action() == TA_CHOICE_MENU_CLOSED )
            {
                break;
            }
            else
            {
                evt->SetPassEvent();
            }
        }
    }

    // Break a wire for the given net out of the bus
    if( !net.IsEmpty() )
        segment = doUnfoldBus( commit, net );

    // If we have an unfolded wire to draw, then draw it
    if( segment )
    {
        return doDrawSegments( aEvent, commit, LAYER_WIRE, false );
    }
    else
    {
        m_frame->PopTool( aEvent );
        return 0;
    }
}


SCH_LINE* SCH_LINE_WIRE_BUS_TOOL::getBusForUnfolding()
{
    SCH_SELECTION& selection = m_selectionTool->RequestSelection( { SCH_ITEM_LOCATE_BUS_T } );
    return static_cast<SCH_LINE*>( selection.Front() );
}


SCH_LINE* SCH_LINE_WIRE_BUS_TOOL::doUnfoldBus( SCH_COMMIT& aCommit, const wxString& aNet,
                                               const std::optional<VECTOR2I>& aPos )
{
    SCHEMATIC_SETTINGS& cfg = getModel<SCHEMATIC>()->Settings();
    SCH_SCREEN*         screen = m_frame->GetScreen();
    // use the same function as the menu selector, so we choose the same bus segment
    SCH_LINE* const     bus = getBusForUnfolding();

    if ( bus == nullptr )
    {
        wxASSERT_MSG( false, wxString::Format( "Couldn't find the originating bus line (but had a net: %s )",
                                               aNet ) );
        return nullptr;
    }

    VECTOR2I pos = aPos.value_or( static_cast<VECTOR2I>( getViewControls()->GetCursorPosition() ) );

    // It is possible for the position to be near the bus, but not exactly on it, but
    // we need the bus entry to be on the bus exactly to connect.
    // If the bus segment is H or V, this will be on the selection grid, if it's not,
    // it might not be, but it won't be a broken connection (and the user asked for it!)
    pos = bus->GetSeg().NearestPoint( pos );

    m_toolMgr->RunAction( ACTIONS::selectionClear );

    m_busUnfold.entry = new SCH_BUS_WIRE_ENTRY( pos );
    m_busUnfold.entry->SetParent( screen );
    m_frame->AddToScreen( m_busUnfold.entry, m_frame->GetScreen() );

    m_busUnfold.label = new SCH_LABEL( m_busUnfold.entry->GetEnd(), aNet );
    m_busUnfold.label->SetTextSize( VECTOR2I( cfg.m_DefaultTextSize, cfg.m_DefaultTextSize ) );
    m_busUnfold.label->SetSpinStyle( busUnfoldPersistentSettings.label_spin_style );
    m_busUnfold.label->SetParent( m_frame->GetScreen() );
    m_busUnfold.label->SetFlags( IS_NEW | IS_MOVING );

    m_busUnfold.in_progress = true;
    m_busUnfold.origin = pos;
    m_busUnfold.net_name = aNet;

    getViewControls()->SetCrossHairCursorPosition( m_busUnfold.entry->GetEnd(), false );

    std::vector<DANGLING_END_ITEM> endPointsByType;

    for( SCH_ITEM* item : screen->Items().Overlapping( m_busUnfold.entry->GetBoundingBox() ) )
        item->GetEndPoints( endPointsByType );

    std::vector<DANGLING_END_ITEM> endPointsByPos = endPointsByType;
    DANGLING_END_ITEM_HELPER::sort_dangling_end_items( endPointsByType, endPointsByPos );
    m_busUnfold.entry->UpdateDanglingState( endPointsByType, endPointsByPos );
    m_busUnfold.entry->SetEndDangling( false );
    m_busUnfold.label->SetIsDangling( false );

    return startSegments( aCommit, LAYER_WIRE, m_busUnfold.entry->GetEnd() );
}


const SCH_SHEET_PIN* SCH_LINE_WIRE_BUS_TOOL::getSheetPin( const VECTOR2I& aPosition )
{
    SCH_SCREEN* screen = m_frame->GetScreen();

    for( SCH_ITEM* item : screen->Items().Overlapping( SCH_SHEET_T, aPosition ) )
    {
        SCH_SHEET* sheet = static_cast<SCH_SHEET*>( item );

        for( SCH_SHEET_PIN* pin : sheet->GetPins() )
        {
            if( pin->GetPosition() == aPosition )
                return pin;
        }
    }

    return nullptr;
}


void SCH_LINE_WIRE_BUS_TOOL::computeBreakPoint( const std::pair<SCH_LINE*, SCH_LINE*>& aSegments,
                                                VECTOR2I&                              aPosition,
                                                LINE_MODE                              mode,
                                                bool                                   posture )
{
    wxCHECK_RET( aSegments.first && aSegments.second,
                 wxT( "Cannot compute break point of NULL line segment." ) );

    VECTOR2I  midPoint;
    SCH_LINE* segment      = aSegments.first;
    SCH_LINE* nextSegment  = aSegments.second;

    VECTOR2I delta = aPosition - segment->GetStartPoint();
    int      xDir  = delta.x > 0 ? 1 : -1;
    int      yDir  = delta.y > 0 ? 1 : -1;


    bool preferHorizontal;
    bool preferVertical;

    if( ( mode == LINE_MODE_45 ) && posture )
    {
        preferHorizontal = ( nextSegment->GetEndPoint().x - nextSegment->GetStartPoint().x ) != 0;
        preferVertical   = ( nextSegment->GetEndPoint().y - nextSegment->GetStartPoint().y ) != 0;
    }
    else
    {
        preferHorizontal = ( segment->GetEndPoint().x - segment->GetStartPoint().x ) != 0;
        preferVertical   = ( segment->GetEndPoint().y - segment->GetStartPoint().y ) != 0;
    }

    // Check for times we need to force horizontal sheet pin connections
    const SCH_SHEET_PIN* connectedPin = getSheetPin( segment->GetStartPoint() );
    SHEET_SIDE           force = connectedPin ? connectedPin->GetSide() : SHEET_SIDE::UNDEFINED;

    if( force == SHEET_SIDE::LEFT || force == SHEET_SIDE::RIGHT )
    {
        if( aPosition.x == connectedPin->GetPosition().x )  // push outside sheet boundary
        {
            int direction = ( force == SHEET_SIDE::LEFT ) ? -1 : 1;
            aPosition.x += KiROUND( getView()->GetGAL()->GetGridSize().x * direction );
        }

        preferHorizontal = true;
        preferVertical = false;
    }


    auto breakVertical = [&]() mutable
    {
        switch( mode )
        {
        case LINE_MODE_45:
            if( !posture )
            {
                midPoint.x = segment->GetStartPoint().x;
                midPoint.y = aPosition.y - yDir * abs( delta.x );
            }
            else
            {
                midPoint.x = aPosition.x;
                midPoint.y = segment->GetStartPoint().y + yDir * abs( delta.x );
            }
            break;
        default:
            midPoint.x = segment->GetStartPoint().x;
            midPoint.y = aPosition.y;
        }
    };


    auto breakHorizontal = [&]() mutable
    {
        switch( mode )
        {
        case LINE_MODE_45:
            if( !posture )
            {
                midPoint.x = aPosition.x - xDir * abs( delta.y );
                midPoint.y = segment->GetStartPoint().y;
            }
            else
            {
                midPoint.x = segment->GetStartPoint().x + xDir * abs( delta.y );
                midPoint.y = aPosition.y;
            }
            break;
        default:
            midPoint.x = aPosition.x;
            midPoint.y = segment->GetStartPoint().y;
        }
    };


    // Maintain current line shape if we can, e.g. if we were originally moving
    // vertically keep the first segment vertical
    if( preferVertical )
        breakVertical();
    else if( preferHorizontal )
        breakHorizontal();

    // Check if our 45 degree angle is one of these shapes
    //    /
    //   /
    //  /
    // /__________
    VECTOR2I deltaMidpoint = midPoint - segment->GetStartPoint();

    if( mode == LINE_MODE::LINE_MODE_45 && !posture
        && ( ( alg::signbit( deltaMidpoint.x ) != alg::signbit( delta.x ) )
             || ( alg::signbit( deltaMidpoint.y ) != alg::signbit( delta.y ) ) ) )
    {
        preferVertical = false;
        preferHorizontal = false;
    }
    else if( mode == LINE_MODE::LINE_MODE_45 && posture
             && ( ( abs( deltaMidpoint.x ) > abs( delta.x ) )
                  || ( abs( deltaMidpoint.y ) > abs( delta.y ) ) ) )
    {
        preferVertical = false;
        preferHorizontal = false;
    }

    if( !preferHorizontal && !preferVertical )
    {
        if( std::abs( delta.x ) < std::abs( delta.y ) )
            breakVertical();
        else
            breakHorizontal();
    }

    segment->SetEndPoint( midPoint );
    nextSegment->SetStartPoint( midPoint );
    nextSegment->SetEndPoint( aPosition );
}


int SCH_LINE_WIRE_BUS_TOOL::doDrawSegments( const TOOL_EVENT& aTool, SCH_COMMIT& aCommit,
                                            int aType, bool aQuitOnDraw )
{
    SCH_SCREEN*           screen = m_frame->GetScreen();
    SCH_LINE*             segment = nullptr;
    EE_GRID_HELPER        grid( m_toolMgr );
    GRID_HELPER_GRIDS     gridType = ( aType == LAYER_NOTES ) ? GRID_GRAPHICS : GRID_WIRES;
    KIGFX::VIEW_CONTROLS* controls = getViewControls();
    int                   lastMode = m_frame->eeconfig()->m_Drawing.line_mode;
    static bool           posture = false;

    auto setCursor =
            [&]()
            {
                if( aType == LAYER_WIRE )
                    m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::LINE_WIRE );
                else if( aType == LAYER_BUS )
                    m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::LINE_BUS );
                else if( aType == LAYER_NOTES )
                    m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::LINE_GRAPHIC );
                else
                    m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::LINE_WIRE );
            };

    auto cleanup =
            [&] ()
            {
                m_toolMgr->RunAction( ACTIONS::selectionClear );

                for( SCH_LINE* wire : m_wires )
                    delete wire;

                m_wires.clear();
                segment = nullptr;

                if( m_busUnfold.entry )
                    m_frame->RemoveFromScreen( m_busUnfold.entry, screen );

                if( m_busUnfold.label && !m_busUnfold.label_placed )
                    m_selectionTool->RemoveItemFromSel( m_busUnfold.label, true );

                if( m_busUnfold.label && m_busUnfold.label_placed )
                    m_frame->RemoveFromScreen( m_busUnfold.label, screen );

                delete m_busUnfold.entry;
                delete m_busUnfold.label;
                m_busUnfold = {};

                m_view->ClearPreview();
                m_view->ShowPreview( false );
            };

    Activate();
    // Must be done after Activate() so that it gets set into the correct context
    controls->ShowCursor( true );
    // Set initial cursor
    setCursor();

    // Add the new label to the selection so the rotate command operates on it
    if( m_busUnfold.label )
        m_selectionTool->AddItemToSel( m_busUnfold.label, true );

    // Continue the existing wires if we've started (usually by immediate action preference)
    if( !m_wires.empty() )
        segment = m_wires.back();

    VECTOR2I contextMenuPos;

    // Main loop: keep receiving events
    while( TOOL_EVENT* evt = Wait() )
    {
        LINE_MODE currentMode = (LINE_MODE) m_frame->eeconfig()->m_Drawing.line_mode;
        bool      twoSegments = currentMode != LINE_MODE::LINE_MODE_FREE;

        // The tool hotkey is interpreted as a click when drawing
        bool isSyntheticClick = ( segment || m_busUnfold.in_progress ) && evt->IsActivate()
                                && evt->HasPosition() && evt->Matches( aTool );

        setCursor();
        grid.SetMask( GRID_HELPER::ALL );
        grid.SetSnap( !evt->Modifier( MD_SHIFT ) );
        grid.SetUseGrid( getView()->GetGAL()->GetGridSnapping() && !evt->DisableGridSnapping() );

        if( segment )
        {
            if( segment->GetStartPoint().x == segment->GetEndPoint().x )
                grid.ClearMaskFlag( GRID_HELPER::VERTICAL );

            if( segment->GetStartPoint().y == segment->GetEndPoint().y )
                grid.ClearMaskFlag( GRID_HELPER::HORIZONTAL );
        }

        VECTOR2D eventPosition = evt->HasPosition() ? evt->Position()
                                                    : controls->GetMousePosition();

        VECTOR2I cursorPos = grid.BestSnapAnchor( eventPosition, gridType, segment );
        controls->ForceCursorPosition( true, cursorPos );

        // Need to handle change in H/V mode while drawing
        if( currentMode != lastMode )
        {
            // Need to delete extra segment if we have one
            if( segment && currentMode == LINE_MODE::LINE_MODE_FREE && m_wires.size() >= 2 )
            {
                m_wires.pop_back();
                m_selectionTool->RemoveItemFromSel( segment );
                delete segment;

                segment = m_wires.back();
                segment->SetEndPoint( cursorPos );
            }
            // Add a segment so we can move orthogonally/45
            else if( segment && lastMode == LINE_MODE::LINE_MODE_FREE )
            {
                segment->SetEndPoint( cursorPos );

                // Create a new segment, and chain it after the current segment.
                segment = static_cast<SCH_LINE*>( segment->Duplicate( true, &aCommit ) );
                segment->SetFlags( IS_NEW | IS_MOVING );
                segment->SetStartPoint( cursorPos );
                m_wires.push_back( segment );

                m_selectionTool->AddItemToSel( segment, true /*quiet mode*/ );
            }

            lastMode = currentMode;
        }

        //------------------------------------------------------------------------
        // Handle cancel:
        //
        if( evt->IsCancelInteractive() )
        {
            m_frame->GetInfoBar()->Dismiss();

            if( segment || m_busUnfold.in_progress )
            {
                cleanup();

                if( aQuitOnDraw )
                {
                    m_frame->PopTool( aTool );
                    break;
                }
            }
            else
            {
                m_frame->PopTool( aTool );
                break;
            }
        }
        else if( evt->IsActivate() && !isSyntheticClick )
        {
            if( segment || m_busUnfold.in_progress )
            {
                m_frame->ShowInfoBarMsg( _( "Press <ESC> to cancel drawing." ) );
                evt->SetPassEvent( false );
                continue;
            }

            if( evt->IsMoveTool() )
            {
                // leave ourselves on the stack so we come back after the move
                break;
            }
            else
            {
                m_frame->PopTool( aTool );
                break;
            }
        }
        //------------------------------------------------------------------------
        // Handle finish:
        //
        else if( evt->IsAction( &ACTIONS::finishInteractive ) )
        {
            if( segment || m_busUnfold.in_progress )
            {
                finishSegments( aCommit );
                segment = nullptr;

                aCommit.Push( _( "Draw Wires" ) );

                if( aQuitOnDraw )
                {
                    m_frame->PopTool( aTool );
                    break;
                }
            }
        }
        //------------------------------------------------------------------------
        // Handle click:
        //
        else if( evt->IsClick( BUT_LEFT )
                || ( segment && evt->IsDblClick( BUT_LEFT ) )
                || isSyntheticClick )
        {
            // First click when unfolding places the label and wire-to-bus entry
            if( m_busUnfold.in_progress && !m_busUnfold.label_placed )
            {
                wxASSERT( aType == LAYER_WIRE );

                m_frame->AddToScreen( m_busUnfold.label, screen );
                m_selectionTool->RemoveItemFromSel( m_busUnfold.label, true );
                m_busUnfold.label_placed = true;
            }

            if( !segment )
            {
                segment = startSegments( aCommit, aType, VECTOR2D( cursorPos ) );
            }
            // Create a new segment if we're out of previously-created ones
            else if( !segment->IsNull()
                     || ( twoSegments && !m_wires[m_wires.size() - 2]->IsNull() ) )
            {
                // Terminate the command if the end point is on a pin, junction, label, or another
                // wire or bus.
                if( screen->IsTerminalPoint( cursorPos, segment->GetLayer() ) )
                {
                    finishSegments( aCommit );
                    segment = nullptr;

                    aCommit.Push( _( "Draw Wires" ) );

                    if( aQuitOnDraw )
                    {
                        m_frame->PopTool( aTool );
                        break;
                    }
                }
                else
                {
                    int placedSegments = 1;

                    // When placing lines with the forty-five degree end, the user is
                    // targetting the endpoint with the angled portion, so it's more
                    // intuitive to place both segments at the same time.
                    if( currentMode == LINE_MODE::LINE_MODE_45 )
                        placedSegments++;

                    segment->SetEndPoint( cursorPos );

                    for( int i = 0; i < placedSegments; i++ )
                    {
                        // Create a new segment, and chain it after the current segment.
                        segment = static_cast<SCH_LINE*>( segment->Duplicate( true, &aCommit ) );
                        segment->SetFlags( IS_NEW | IS_MOVING );
                        segment->SetStartPoint( cursorPos );
                        m_wires.push_back( segment );

                        m_selectionTool->AddItemToSel( segment, true /*quiet mode*/ );
                    }
                }
            }

            if( evt->IsDblClick( BUT_LEFT ) && segment )
            {
                if( twoSegments && m_wires.size() >= 2 )
                {
                    computeBreakPoint( { m_wires[m_wires.size() - 2], segment }, cursorPos,
                                       currentMode, posture );
                }

                finishSegments( aCommit );
                segment = nullptr;

                aCommit.Push( _( "Draw Wires" ) );

                if( aQuitOnDraw )
                {
                    m_frame->PopTool( aTool );
                    break;
                }
            }
        }
        //------------------------------------------------------------------------
        // Handle motion:
        //
        else if( evt->IsMotion() || evt->IsAction( &ACTIONS::refreshPreview ) )
        {
            m_view->ClearPreview();

            // Update the bus unfold posture based on the mouse movement
            if( m_busUnfold.in_progress && !m_busUnfold.label_placed )
            {
                VECTOR2I cursor_delta = cursorPos - m_busUnfold.origin;
                SCH_BUS_WIRE_ENTRY* entry = m_busUnfold.entry;

                bool flipX = ( cursor_delta.x < 0 );
                bool flipY = ( cursor_delta.y < 0 );

                // Erase and redraw if necessary
                if( flipX != m_busUnfold.flipX || flipY != m_busUnfold.flipY )
                {
                    VECTOR2I size  = entry->GetSize();
                    int    ySign = flipY ? -1 : 1;
                    int    xSign = flipX ? -1 : 1;

                    size.x = std::abs( size.x ) * xSign;
                    size.y = std::abs( size.y ) * ySign;
                    entry->SetSize( size );

                    m_busUnfold.flipY = flipY;
                    m_busUnfold.flipX = flipX;

                    m_frame->UpdateItem( entry, false, true );
                    m_wires.front()->SetStartPoint( entry->GetEnd() );
                }

                // Update the label "ghost" position
                m_busUnfold.label->SetPosition( cursorPos );
                m_view->AddToPreview( m_busUnfold.label->Clone() );

                // Ensure segment is non-null at the start of bus unfold
                if( !segment )
                    segment = m_wires.back();
            }

            if( segment )
            {
                // Coerce the line to vertical/horizontal/45 as necessary
                if( twoSegments && m_wires.size() >= 2 )
                {
                    computeBreakPoint( { m_wires[m_wires.size() - 2], segment }, cursorPos,
                                       currentMode, posture );
                }
                else
                {
                    segment->SetEndPoint( cursorPos );
                }
            }

            for( SCH_LINE* wire : m_wires )
            {
                if( !wire->IsNull() )
                    m_view->AddToPreview( wire->Clone() );
            }

            std::vector<SCH_ITEM*> previewItems;

            for( SCH_LINE* wire : m_wires )
            {
                if( !wire->IsNull() )
                    previewItems.push_back( wire );
            }

            if( m_busUnfold.entry )
                previewItems.push_back( m_busUnfold.entry );

            for( SCH_JUNCTION* jct : JUNCTION_HELPERS::PreviewJunctions( m_frame->GetScreen(),
                                                                          previewItems ) )
            {
                m_view->AddToPreview( jct, true );
            }
        }
        else if( evt->IsAction( &SCH_ACTIONS::undoLastSegment )
                 || evt->IsAction( &ACTIONS::doDelete )
                 || evt->IsAction( &ACTIONS::undo ) )
        {
            if( ( currentMode == LINE_MODE::LINE_MODE_FREE && m_wires.size() > 1 )
                || ( LINE_MODE::LINE_MODE_90 && m_wires.size() > 2 ) )
            {
                m_view->ClearPreview();

                m_wires.pop_back();
                m_selectionTool->RemoveItemFromSel( segment );
                delete segment;

                segment = m_wires.back();
                cursorPos = segment->GetEndPoint();
                getViewControls()->WarpMouseCursor( cursorPos, true );

                // Find new bend point for current mode
                if( twoSegments && m_wires.size() >= 2 )
                {
                    computeBreakPoint( { m_wires[m_wires.size() - 2], segment }, cursorPos,
                                       currentMode, posture );
                }
                else
                {
                    segment->SetEndPoint( cursorPos );
                }

                for( SCH_LINE* wire : m_wires )
                {
                    if( !wire->IsNull() )
                        m_view->AddToPreview( wire->Clone() );
                }
            }
            else if( evt->IsAction( &ACTIONS::undo ) )
            {
                // Dispatch as normal undo event
                evt->SetPassEvent();
            }
            else
            {
                wxBell();
            }
        }
        else if( evt->IsAction( &SCH_ACTIONS::switchSegmentPosture ) && m_wires.size() >= 2 )
        {
            posture = !posture;

            // The 90 degree mode doesn't have a forced posture like
            // the 45 degree mode and computeBreakPoint maintains existing 90s' postures.
            // Instead, just swap the 90 angle here.
            if( currentMode == LINE_MODE::LINE_MODE_90 )
            {
                m_view->ClearPreview();

                SCH_LINE* line2 = m_wires[m_wires.size() - 1];
                SCH_LINE* line1 = m_wires[m_wires.size() - 2];

                VECTOR2I delta2 = line2->GetEndPoint() - line2->GetStartPoint();
                VECTOR2I delta1 = line1->GetEndPoint() - line1->GetStartPoint();

                line2->SetStartPoint(line2->GetEndPoint() - delta1);
                line1->SetEndPoint(line1->GetStartPoint() + delta2);

                for( SCH_LINE* wire : m_wires )
                {
                    if( !wire->IsNull() )
                        m_view->AddToPreview( wire->Clone() );
                }
            }
            else
            {
                computeBreakPoint( { m_wires[m_wires.size() - 2], segment }, cursorPos, currentMode,
                                   posture );

                m_toolMgr->PostAction( ACTIONS::refreshPreview );
            }
        }
        //------------------------------------------------------------------------
        // Handle context menu:
        //
        else if( evt->IsClick( BUT_RIGHT ) )
        {
            // Warp after context menu only if dragging...
            if( !segment )
                m_toolMgr->VetoContextMenuMouseWarp();

            contextMenuPos = cursorPos;
            m_menu->ShowContextMenu( m_selectionTool->GetSelection() );
        }
        else if( evt->Category() == TC_COMMAND && evt->Action() == TA_CHOICE_MENU_CHOICE )
        {
            if( *evt->GetCommandId() >= ID_POPUP_SCH_UNFOLD_BUS
                && *evt->GetCommandId() <= ID_POPUP_SCH_UNFOLD_BUS_END )
            {
                wxASSERT_MSG( !segment, "Bus unfold event received when already drawing!" );

                aType = LAYER_WIRE;
                wxString net = *evt->Parameter<wxString*>();
                segment = doUnfoldBus( aCommit, net, contextMenuPos );
            }
        }
        //------------------------------------------------------------------------
        // Handle TOOL_ACTION special cases
        //
        else if( evt->IsAction( &SCH_ACTIONS::rotateCW ) || evt->IsAction( &SCH_ACTIONS::rotateCCW ) )
        {
            if( m_busUnfold.in_progress )
            {
                m_busUnfold.label->Rotate90( evt->IsAction( &SCH_ACTIONS::rotateCW ) );
                busUnfoldPersistentSettings.label_spin_style = m_busUnfold.label->GetSpinStyle();

                m_toolMgr->PostAction( ACTIONS::refreshPreview );
            }
            else
            {
                wxBell();
            }
        }
        else if( evt->IsAction( &ACTIONS::redo ) )
        {
            wxBell();
        }
        else
        {
            evt->SetPassEvent();
        }

        // Enable autopanning and cursor capture only when there is a segment to be placed
        controls->SetAutoPan( segment != nullptr );
        controls->CaptureCursor( segment != nullptr );
    }

    controls->SetAutoPan( false );
    controls->CaptureCursor( false );
    m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::ARROW );
    controls->ForceCursorPosition( false );
    return 0;
}


SCH_LINE* SCH_LINE_WIRE_BUS_TOOL::startSegments( SCH_COMMIT& aCommit, int aType, const VECTOR2D& aPos,
                                                 SCH_LINE* aSegment )
{
    if( !aSegment )
        aSegment = m_frame->GetScreen()->GetLine( aPos, 0, aType );

    if( !aSegment )
    {
        switch( aType )
        {
        default:         aSegment = new SCH_LINE( aPos, LAYER_NOTES ); break;
        case LAYER_WIRE: aSegment = new SCH_LINE( aPos, LAYER_WIRE );  break;
        case LAYER_BUS:  aSegment = new SCH_LINE( aPos, LAYER_BUS );   break;
        }

        // Give segments a parent so they find the default line/wire/bus widths
        aSegment->SetParent( &m_frame->Schematic() );
    }
    else
    {
        aSegment = static_cast<SCH_LINE*>( aSegment->Duplicate( true, &aCommit ) );
        aSegment->SetStartPoint( aPos );
    }


    aSegment->SetFlags( IS_NEW | IS_MOVING );
    m_wires.push_back( aSegment );

    m_selectionTool->AddItemToSel( aSegment, true /*quiet mode*/ );

    // We need 2 segments to go from a given start pin to an end point when the
    // horizontal and vertical lines only switch is on.
    if( m_frame->eeconfig()->m_Drawing.line_mode )
    {
        aSegment = static_cast<SCH_LINE*>( aSegment->Duplicate( true, &aCommit ) );
        aSegment->SetFlags( IS_NEW | IS_MOVING );
        m_wires.push_back( aSegment );

        m_selectionTool->AddItemToSel( aSegment, true /*quiet mode*/ );
    }

    return aSegment;
}


/**
 * In a contiguous list of wires, remove wires that backtrack over the previous
 * wire. Example:
 *
 * Wire is added:
 * ---------------------------------------->
 *
 * A second wire backtracks over it:
 * -------------------<====================>
 *
 * simplifyWireList is called:
 * ------------------->
 */
void SCH_LINE_WIRE_BUS_TOOL::simplifyWireList()
{
    for( auto it = m_wires.begin(); it != m_wires.end(); )
    {
        SCH_LINE* line = *it;

        if( line->IsNull() )
        {
            delete line;
            it = m_wires.erase( it );
            continue;
        }

        auto next_it = it;
        ++next_it;

        if( next_it == m_wires.end() )
            break;

        SCH_LINE* next_line = *next_it;

        if( SCH_LINE* merged = line->MergeOverlap( m_frame->GetScreen(), next_line, false ) )
        {
            delete line;
            delete next_line;
            it = m_wires.erase( it );
            *it = merged;
        }

        ++it;
    }
}


void SCH_LINE_WIRE_BUS_TOOL::finishSegments( SCH_COMMIT& aCommit )
{
    // Clear selection when done so that a new wire can be started.
    // NOTE: this must be done before simplifyWireList is called or we might end up with
    // freed selected items.
    m_toolMgr->RunAction( ACTIONS::selectionClear );

    SCH_SCREEN* screen = m_frame->GetScreen();

    // Remove segments backtracking over others
    simplifyWireList();

    // Collect the possible connection points for the new lines
    std::vector<VECTOR2I> connections = screen->GetConnections();
    std::vector<VECTOR2I> new_ends;

    // Check each new segment for possible junctions and add/split if needed
    for( SCH_LINE* wire : m_wires )
    {
        if( wire->HasFlag( SKIP_STRUCT ) )
            continue;

        std::vector<VECTOR2I> tmpends = wire->GetConnectionPoints();

        new_ends.insert( new_ends.end(), tmpends.begin(), tmpends.end() );

        for( const VECTOR2I& pt : connections )
        {
            if( IsPointOnSegment( wire->GetStartPoint(), wire->GetEndPoint(), pt ) )
                new_ends.push_back( pt );
        }

        aCommit.Added( wire, screen );
    }

    if( m_busUnfold.in_progress && m_busUnfold.label_placed )
    {
        wxASSERT( m_busUnfold.entry && m_busUnfold.label );

        aCommit.Added( m_busUnfold.entry, screen );
        m_frame->SaveCopyForRepeatItem( m_busUnfold.entry );

        aCommit.Added( m_busUnfold.label, screen );
        m_frame->AddCopyForRepeatItem( m_busUnfold.label );
        m_busUnfold.label->ClearEditFlags();
    }
    else if( !m_wires.empty() )
    {
        m_frame->SaveCopyForRepeatItem( m_wires[0] );
    }

    for( size_t ii = 1; ii < m_wires.size(); ++ii )
        m_frame->AddCopyForRepeatItem( m_wires[ii] );

    // Get the last non-null wire (this is the last created segment).
    if( !m_wires.empty() )
        m_frame->AddCopyForRepeatItem( m_wires.back() );

    // Add the new wires
    for( SCH_LINE* wire : m_wires )
    {
        wire->ClearFlags( IS_NEW | IS_MOVING );
        m_frame->AddToScreen( wire, screen );
    }

    m_wires.clear();
    m_view->ClearPreview();
    m_view->ShowPreview( false );

    getViewControls()->CaptureCursor( false );
    getViewControls()->SetAutoPan( false );

    // Correct and remove segments that need to be merged.
    m_frame->Schematic().CleanUp( &aCommit );

    std::vector<SCH_ITEM*> symbols;

    for( SCH_ITEM* symbol : m_frame->GetScreen()->Items().OfType( SCH_SYMBOL_T ) )
        symbols.push_back( symbol );

    for( SCH_ITEM* symbol : symbols )
    {
        std::vector<VECTOR2I> pts = symbol->GetConnectionPoints();

        if( pts.size() > 2 )
            continue;

        for( auto pt = pts.begin(); pt != pts.end(); pt++ )
        {
            for( auto secondPt = pt + 1; secondPt != pts.end(); secondPt++ )
                m_frame->TrimWire( &aCommit, *pt, *secondPt );
        }
    }

    for( const VECTOR2I& pt : new_ends )
    {
        if( m_frame->GetScreen()->IsExplicitJunctionNeeded( pt ) )
            m_frame->AddJunction( &aCommit, m_frame->GetScreen(), pt );
    }

    if( m_busUnfold.in_progress )
        m_busUnfold = {};

    for( SCH_ITEM* item : m_frame->GetScreen()->Items() )
        item->ClearEditFlags();
}


int SCH_LINE_WIRE_BUS_TOOL::TrimOverLappingWires( SCH_COMMIT* aCommit, SCH_SELECTION* aSelection  )
{
    SCHEMATIC* sch = getModel<SCHEMATIC>();
    SCH_SCREEN* screen = sch->CurrentSheet().LastScreen();

    std::set<SCH_LINE*> lines;
    BOX2I bb = aSelection->GetBoundingBox();

    for( EDA_ITEM* item : screen->Items().Overlapping( SCH_LINE_T, bb ) )
        lines.insert( static_cast<SCH_LINE*>( item ) );

    for( unsigned ii = 0; ii < aSelection->GetSize(); ii++ )
    {
        SCH_ITEM* item = dynamic_cast<SCH_ITEM*>( aSelection->GetItem( ii ) );

        if( !item || !item->IsConnectable() || ( item->Type() == SCH_LINE_T ) )
            continue;

        std::vector<VECTOR2I> pts = item->GetConnectionPoints();

        /// If the line intersects with an item in the selection at only two points,
        /// then we can remove the line between the two points.
        for( SCH_LINE* line : lines )
        {
            std::vector<VECTOR2I> conn_pts;

            for( const VECTOR2I& pt : pts )
            {
                if( IsPointOnSegment( line->GetStartPoint(), line->GetEndPoint(), pt ) )
                    conn_pts.push_back( pt );

                if( conn_pts.size() > 2 )
                    break;
            }

            if( conn_pts.size() == 2 )
                m_frame->TrimWire( aCommit, conn_pts[0], conn_pts[1] );
        }
    }

    return 0;
}


int SCH_LINE_WIRE_BUS_TOOL::AddJunctionsIfNeeded( SCH_COMMIT* aCommit, SCH_SELECTION* aSelection )
{
    SCH_SCREEN*           screen = m_frame->GetScreen();
    std::deque<EDA_ITEM*> allItems;

    for( EDA_ITEM* item : aSelection->Items() )
    {
        allItems.push_back( item );

        if( item->Type() == SCH_GROUP_T )
        {
            static_cast<SCH_GROUP*>( item )->RunOnChildren(
                    [&]( SCH_ITEM* child )
                    {
                        allItems.push_back( child );
                    }, RECURSE_MODE::RECURSE );
        }
    }

    for( const VECTOR2I& point : screen->GetNeededJunctions( allItems ) )
        m_frame->AddJunction( aCommit, m_frame->GetScreen(), point );

    return 0;
}


void SCH_LINE_WIRE_BUS_TOOL::setTransitions()
{
    Go( &SCH_LINE_WIRE_BUS_TOOL::DrawSegments,         SCH_ACTIONS::drawWire.MakeEvent() );
    Go( &SCH_LINE_WIRE_BUS_TOOL::DrawSegments,         SCH_ACTIONS::drawBus.MakeEvent() );
    Go( &SCH_LINE_WIRE_BUS_TOOL::DrawSegments,         SCH_ACTIONS::drawLines.MakeEvent() );

    Go( &SCH_LINE_WIRE_BUS_TOOL::UnfoldBus,            SCH_ACTIONS::unfoldBus.MakeEvent() );
}
