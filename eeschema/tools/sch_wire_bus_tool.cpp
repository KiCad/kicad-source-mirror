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

#include <connection_graph.h>
#include <sch_wire_bus_tool.h>
#include <ee_selection_tool.h>
#include <ee_actions.h>
#include <sch_edit_frame.h>
#include <sch_view.h>
#include <class_draw_panel_gal.h>
#include <id.h>
#include <eeschema_id.h>
#include <confirm.h>
#include <view/view_group.h>
#include <view/view_controls.h>
#include <view/view.h>
#include <tool/tool_manager.h>
#include <sch_junction.h>
#include <sch_line.h>
#include <sch_bus_entry.h>
#include <sch_text.h>
#include <sch_sheet.h>
#include <advanced_config.h>

TOOL_ACTION EE_ACTIONS::startWire( "eeschema.WireBusDrawing.startWire",
        AS_GLOBAL, 0, "",
        _( "Start Wire" ), _( "Start drawing a wire" ),
        add_line_xpm, AF_ACTIVATE );

TOOL_ACTION EE_ACTIONS::drawWire( "eeschema.WireBusDrawing.drawWires",
        AS_GLOBAL, 
        'W', LEGACY_HK_NAME( "Begin Wire" ),
        _( "Add Wire" ), _( "Add a wire" ),
        add_line_xpm, AF_ACTIVATE );

TOOL_ACTION EE_ACTIONS::startBus( "eeschema.WireBusDrawing.startBus",
        AS_GLOBAL, 0, "",
        _( "Start Bus" ), _( "Start drawing a bus" ),
        add_bus_xpm, AF_ACTIVATE );

TOOL_ACTION EE_ACTIONS::drawBus( "eeschema.WireBusDrawing.drawBusses",
        AS_GLOBAL, 
        'B', LEGACY_HK_NAME( "Begin Bus" ),
        _( "Add Bus" ), _( "Add a bus" ),
        add_bus_xpm, AF_ACTIVATE );

TOOL_ACTION EE_ACTIONS::unfoldBus( "eeschema.WireBusDrawing.unfoldBus",
        AS_GLOBAL, 
        'C', LEGACY_HK_NAME( "Unfold from Bus" ),
        _( "Unfold from Bus" ), _( "Break a wire out of a bus" ),
        nullptr, AF_ACTIVATE );

TOOL_ACTION EE_ACTIONS::startLines( "eeschema.WireBusDrawing.startLines",
        AS_GLOBAL, 0,  "",
        _( "Begin Lines" ), _( "Start drawing connected graphic lines" ),
        add_line_xpm, AF_ACTIVATE );

TOOL_ACTION EE_ACTIONS::drawLines( "eeschema.WireBusDrawing.drawLines",
        AS_GLOBAL, 
        'I', LEGACY_HK_NAME( "Add Graphic PolyLine" ),
        _( "Add Lines" ), _( "Add connected graphic lines" ),
        add_dashed_line_xpm, AF_ACTIVATE );

TOOL_ACTION EE_ACTIONS::finishLineWireOrBus( "eeschema.WireBusDrawing.finishLineWireOrBus",
        AS_GLOBAL, 
        'K', LEGACY_HK_NAME( "End Line Wire Bus" ),
        _( "Finish Wire or Bus" ), _( "Complete drawing at current segment" ),
        checked_ok_xpm, AF_NONE );

TOOL_ACTION EE_ACTIONS::finishWire( "eeschema.WireBusDrawing.finishWire",
        AS_GLOBAL, 0,  "",
        _( "Finish Wire" ), _( "Complete wire with current segment" ),
        checked_ok_xpm, AF_NONE );

TOOL_ACTION EE_ACTIONS::finishBus( "eeschema.WireBusDrawing.finishBus",
        AS_GLOBAL, 0,  "",
        _( "Finish Bus" ), _( "Complete bus with current segment" ),
        checked_ok_xpm, AF_NONE );

TOOL_ACTION EE_ACTIONS::finishLine( "eeschema.WireBusDrawing.finishLine",
        AS_GLOBAL, 0,  "",
        _( "Finish Lines" ), _( "Complete connected lines with current segment" ),
        checked_ok_xpm, AF_NONE );


class BUS_UNFOLD_MENU : public ACTION_MENU
{
public:
    BUS_UNFOLD_MENU() :
        m_showTitle( false )
    {
        SetIcon( add_line2bus_xpm );
        SetTitle( _( "Unfold from Bus" ) );
    }

    void SetShowTitle()
    {
        m_showTitle = true;
    }


protected:
    ACTION_MENU* create() const override
    {
        return new BUS_UNFOLD_MENU();
    }

private:
    void update() override
    {
        SCH_EDIT_FRAME*    frame = (SCH_EDIT_FRAME*) getToolManager()->GetEditFrame();
        EE_SELECTION_TOOL* selTool = getToolManager()->GetTool<EE_SELECTION_TOOL>();
        KICAD_T            busType[] = { SCH_LINE_LOCATE_BUS_T, EOT };
        EE_SELECTION&      selection = selTool->RequestSelection( busType );
        SCH_LINE*          bus = (SCH_LINE*) selection.Front();

        Clear();

        // TODO(JE) remove once real-time is enabled
        if( !ADVANCED_CFG::GetCfg().m_realTimeConnectivity || !CONNECTION_GRAPH::m_allowRealTime )
        {
            frame->RecalculateConnections();

            // Have to pick up the pointer again because it may have been changed by SchematicCleanUp
            selection = selTool->RequestSelection( busType );
            bus = (SCH_LINE*) selection.Front();
        }
        if( !bus )
        {
            Append( ID_POPUP_SCH_UNFOLD_BUS, _( "no bus selected" ), wxEmptyString );
            Enable( ID_POPUP_SCH_UNFOLD_BUS, false );
            return;
        }

        SCH_CONNECTION* connection = bus->Connection( *g_CurrentSheet );

        if( !connection ||  !connection->IsBus() || connection->Members().empty() )
        {
            Append( ID_POPUP_SCH_UNFOLD_BUS, _( "bus has no connections" ), wxEmptyString );
            Enable( ID_POPUP_SCH_UNFOLD_BUS, false );
            return;
        }

        int idx = 0;

        if( m_showTitle )
        {
            Append( ID_POPUP_SCH_UNFOLD_BUS, _( "Unfold from Bus" ), wxEmptyString );
            Enable( ID_POPUP_SCH_UNFOLD_BUS, false );
        }

        for( const auto& member : connection->Members() )
        {
            int id = ID_POPUP_SCH_UNFOLD_BUS + ( idx++ );
            wxString name = member->Name( true );

            if( member->Type() == CONNECTION_BUS )
            {
                ACTION_MENU* submenu = new ACTION_MENU;
                AppendSubMenu( submenu, name );

                for( const auto& sub_member : member->Members() )
                {
                    id = ID_POPUP_SCH_UNFOLD_BUS + ( idx++ );
                    submenu->Append( id, sub_member->Name( true ), wxEmptyString );
                }
            }
            else
            {
                Append( id, name, wxEmptyString );
            }
        }
    }

    bool m_showTitle;
};


SCH_WIRE_BUS_TOOL::SCH_WIRE_BUS_TOOL() :
    EE_TOOL_BASE<SCH_EDIT_FRAME>( "eeschema.WireBusDrawing" )
{
    m_busUnfold = {};
}


SCH_WIRE_BUS_TOOL::~SCH_WIRE_BUS_TOOL()
{
}


using E_C = EE_CONDITIONS;

bool SCH_WIRE_BUS_TOOL::Init()
{
    EE_TOOL_BASE::Init();

    auto wireOrBusTool = [ this ] ( const SELECTION& aSel ) {
        return ( m_frame->GetToolId() == ID_WIRE_TOOL || m_frame->GetToolId() == ID_BUS_TOOL );
    };

    auto lineTool = [ this ] ( const SELECTION& aSel ) {
        return ( m_frame->GetToolId() == ID_SCHEMATIC_LINE_TOOL );
    };

    auto belowRootSheetCondition = [] ( const SELECTION& aSel ) {
        return g_CurrentSheet->Last() != g_RootSheet;
    };

    auto busSelection = E_C::MoreThan( 0 ) && E_C::OnlyType( SCH_LINE_LOCATE_BUS_T );

    auto& ctxMenu = m_menu.GetMenu();

    //
    // Build the tool menu
    //
    ctxMenu.AddItem( EE_ACTIONS::leaveSheet,    belowRootSheetCondition, 2 );

    ctxMenu.AddSeparator( E_C::ShowAlways, 10 );
    ctxMenu.AddItem( EE_ACTIONS::startWire,          wireOrBusTool && E_C::Idle, 10 );
    ctxMenu.AddItem( EE_ACTIONS::startBus,           wireOrBusTool && E_C::Idle, 10 );
    ctxMenu.AddItem( EE_ACTIONS::startLines,         lineTool && E_C::Idle, 10 );
    ctxMenu.AddItem( EE_ACTIONS::finishWire,         IsDrawingWire, 10 );
    ctxMenu.AddItem( EE_ACTIONS::finishBus,          IsDrawingBus, 10 );
    ctxMenu.AddItem( EE_ACTIONS::finishLine,         IsDrawingLine, 10 );

    std::shared_ptr<BUS_UNFOLD_MENU> busUnfoldMenu = std::make_shared<BUS_UNFOLD_MENU>();
    busUnfoldMenu->SetTool( this );
    m_menu.AddSubMenu( busUnfoldMenu );
    ctxMenu.AddMenu( busUnfoldMenu.get(), EE_CONDITIONS::Idle, 10 );

    ctxMenu.AddSeparator( wireOrBusTool && EE_CONDITIONS::Idle, 100 );
    ctxMenu.AddItem( EE_ACTIONS::addJunction,        wireOrBusTool && E_C::Idle, 100 );
    ctxMenu.AddItem( EE_ACTIONS::addLabel,           wireOrBusTool && E_C::Idle, 100 );
    ctxMenu.AddItem( EE_ACTIONS::addGlobalLabel,     wireOrBusTool && E_C::Idle, 100 );
    ctxMenu.AddItem( EE_ACTIONS::addHierLabel,       wireOrBusTool && E_C::Idle, 100 );
    ctxMenu.AddItem( EE_ACTIONS::breakWire,          wireOrBusTool && E_C::Idle, 100 );
    ctxMenu.AddItem( EE_ACTIONS::breakBus,           wireOrBusTool && E_C::Idle, 100 );

    ctxMenu.AddSeparator( wireOrBusTool && EE_CONDITIONS::Idle, 200 );
    ctxMenu.AddItem( EE_ACTIONS::selectNode,         wireOrBusTool && E_C::Idle, 200 );
    ctxMenu.AddItem( EE_ACTIONS::selectConnection,   wireOrBusTool && E_C::Idle, 200 );

    //
    // Add bus unfolding to the selection tool
    //
    CONDITIONAL_MENU& selToolMenu = m_selectionTool->GetToolMenu().GetMenu();

    std::shared_ptr<BUS_UNFOLD_MENU> selBusUnfoldMenu = std::make_shared<BUS_UNFOLD_MENU>();
    selBusUnfoldMenu->SetTool( m_selectionTool );
    m_selectionTool->GetToolMenu().AddSubMenu( selBusUnfoldMenu );
    selToolMenu.AddMenu( selBusUnfoldMenu.get(), busSelection && E_C::Idle, 100 );

    return true;
}


static bool isNewSegment( SCH_ITEM* aItem )
{
    return aItem && aItem->IsNew() && aItem->Type() == SCH_LINE_T;
}


bool SCH_WIRE_BUS_TOOL::IsDrawingLine( const SELECTION& aSelection )
{
    static KICAD_T graphicLineType[] = { SCH_LINE_LOCATE_GRAPHIC_LINE_T, EOT };
    return IsDrawingLineWireOrBus( aSelection ) && aSelection.Front()->IsType( graphicLineType );
}


bool SCH_WIRE_BUS_TOOL::IsDrawingWire( const SELECTION& aSelection )
{
    static KICAD_T wireType[] = { SCH_LINE_LOCATE_WIRE_T, EOT };
    return IsDrawingLineWireOrBus( aSelection ) && aSelection.Front()->IsType( wireType );
}


bool SCH_WIRE_BUS_TOOL::IsDrawingBus( const SELECTION& aSelection )
{
    static KICAD_T busType[] = { SCH_LINE_LOCATE_BUS_T, EOT };
    return IsDrawingLineWireOrBus( aSelection ) && aSelection.Front()->IsType( busType );
}


bool SCH_WIRE_BUS_TOOL::IsDrawingLineWireOrBus( const SELECTION& aSelection )
{
    // NOTE: for immediate hotkeys, it is NOT required that the line, wire or bus tool
    // be selected
    SCH_ITEM* item = (SCH_ITEM*) aSelection.Front();
    return isNewSegment( item );
}


/*
 * Immediate action: start drawing a wire.  Does not select the wire tool.
 */
int SCH_WIRE_BUS_TOOL::StartWire( const TOOL_EVENT& aEvent )
{
    m_toolMgr->RunAction( EE_ACTIONS::clearSelection, true );

    Activate();

    getViewControls()->WarpCursor( m_frame->GetCrossHairPosition(), true );
    SCH_LINE* segment = startSegments( LAYER_WIRE, m_frame->GetCrossHairPosition() );
    return doDrawSegments( LAYER_WIRE, segment );
}


/*
 * Tool action: first call selects the tool; subsequent calls start wires.
 */
int SCH_WIRE_BUS_TOOL::DrawWires( const TOOL_EVENT& aEvent )
{
    if( m_frame->GetToolId() == ID_WIRE_TOOL )
        return StartWire( aEvent );
    else
    {
        m_frame->SetToolID( ID_WIRE_TOOL, wxCURSOR_PENCIL, _( "Add wire" ) );
        m_toolMgr->RunAction( EE_ACTIONS::clearSelection, true );

        return doDrawSegments( LAYER_WIRE, nullptr );
    }
}


/*
 * Immediate action: start drawing a bus.  Does not select the bus tool.
 */
int SCH_WIRE_BUS_TOOL::StartBus( const TOOL_EVENT& aEvent )
{
    m_toolMgr->RunAction( EE_ACTIONS::clearSelection, true );

    Activate();

    getViewControls()->WarpCursor( m_frame->GetCrossHairPosition(), true );
    SCH_LINE* segment = startSegments( LAYER_BUS, m_frame->GetCrossHairPosition() );
    return doDrawSegments( LAYER_BUS, segment );
}


/*
 * Tool action: first call selects the tool; subsequent calls start busses.
 */
int SCH_WIRE_BUS_TOOL::DrawBusses( const TOOL_EVENT& aEvent )
{
    if( m_frame->GetToolId() == ID_BUS_TOOL )
        return StartBus( aEvent );
    else
    {
        m_frame->SetToolID( ID_BUS_TOOL, wxCURSOR_PENCIL, _( "Add bus" ) );
        m_toolMgr->RunAction( EE_ACTIONS::clearSelection, true );

        return doDrawSegments( LAYER_BUS, nullptr );
    }
}


int SCH_WIRE_BUS_TOOL::UnfoldBus( const TOOL_EVENT& aEvent )
{
    wxString* netPtr = aEvent.Parameter<wxString*>();
    wxString  net;
    SCH_LINE* segment = nullptr;

    m_toolMgr->RunAction( EE_ACTIONS::clearSelection, true );

    Activate();

    if( netPtr )
    {
        net = *netPtr;
        delete netPtr;
    }
    else
    {
        BUS_UNFOLD_MENU unfoldMenu;
        unfoldMenu.SetTool( this );
        unfoldMenu.SetShowTitle();

        SetContextMenu( &unfoldMenu, CMENU_NOW );

        while( OPT_TOOL_EVENT evt = Wait() )
        {
            if( evt->Action() == TA_CONTEXT_MENU_CHOICE )
            {
                OPT<int> id = evt->GetCommandId();

                if( id && ( *id > 0 ) )
                    net = *evt->Parameter<wxString*>();

                break;
            }
        }
    }

    if( !net.IsEmpty() )
    {
        // Break a wire for the given net out of the bus
        segment = doUnfoldBus( net );
    }

    // If we have an unfolded wire to draw, then draw it
    if( segment )
        return doDrawSegments( LAYER_WIRE, segment );

    // If we came from one of our tools then re-enter it in the idle state
    if( m_frame->GetToolId() == ID_WIRE_TOOL )
        return doDrawSegments( LAYER_WIRE, nullptr );
    else if( m_frame->GetToolId() == ID_BUS_TOOL )
        return doDrawSegments( LAYER_BUS, nullptr );

    m_frame->SetNoToolSelected();
    return 0;
}


SCH_LINE* SCH_WIRE_BUS_TOOL::doUnfoldBus( const wxString& aNet )
{
    wxPoint  pos = m_frame->GetCrossHairPosition();

    m_busUnfold.entry = new SCH_BUS_WIRE_ENTRY( pos, '\\' );
    m_busUnfold.entry->SetParent( m_frame->GetScreen() );
    m_frame->AddToScreen( m_busUnfold.entry );

    m_busUnfold.label = new SCH_LABEL( m_busUnfold.entry->m_End(), aNet );
    m_busUnfold.label->SetTextSize( wxSize( GetDefaultTextSize(), GetDefaultTextSize() ) );
    m_busUnfold.label->SetLabelSpinStyle( 0 );
    m_busUnfold.label->SetParent( m_frame->GetScreen() );

    m_busUnfold.in_progress = true;
    m_busUnfold.origin = pos;
    m_busUnfold.net_name = aNet;

    m_frame->SetCrossHairPosition( m_busUnfold.entry->m_End() );

    return startSegments( LAYER_WIRE, m_busUnfold.entry->m_End() );
}


/*
 * Immediate action: start drawing a line.  Does not select the line tool.
 */
int SCH_WIRE_BUS_TOOL::StartLine( const TOOL_EVENT& aEvent)
{
    m_toolMgr->RunAction( EE_ACTIONS::clearSelection, true );

    getViewControls()->WarpCursor( m_frame->GetCrossHairPosition(), true );
    SCH_LINE* segment = startSegments( LAYER_NOTES, m_frame->GetCrossHairPosition() );
    return doDrawSegments( LAYER_BUS, segment );
}


/*
 * Tool action: first call selects the tool; subsequent calls start lines.
 */
int SCH_WIRE_BUS_TOOL::DrawLines( const TOOL_EVENT& aEvent)
{
    if( m_frame->GetToolId() == ID_SCHEMATIC_LINE_TOOL )
        return StartLine( aEvent );
    else
    {
        m_frame->SetToolID( ID_SCHEMATIC_LINE_TOOL, wxCURSOR_PENCIL, _( "Add lines" ) );
        m_toolMgr->RunAction( EE_ACTIONS::clearSelection, true );

        return doDrawSegments( LAYER_NOTES, nullptr );
    }
}


// Storage for the line segments while drawing
static DLIST<SCH_LINE> s_wires;


/**
 * A helper function to find any sheet pins at the specified position.
 */
static const SCH_SHEET_PIN* getSheetPin( SCH_SCREEN* aScreen, const wxPoint& aPosition )
{
    for( SCH_ITEM* item = aScreen->GetDrawItems(); item; item = item->Next() )
    {
        if( item->Type() == SCH_SHEET_T )
        {
            SCH_SHEET* sheet = (SCH_SHEET*) item;

            for( const SCH_SHEET_PIN& pin : sheet->GetPins() )
            {
                if( pin.GetPosition() == aPosition )
                    return &pin;
            }
        }
    }

    return nullptr;
}


/**
 * Function ComputeBreakPoint
 * computes the middle coordinate for 2 segments from the start point to \a aPosition
 * with the segments kept in the horizontal or vertical axis only.
 *
 * @param aSegment A pointer to a #SCH_LINE object containing the first line break point
 *                 to compute.
 * @param aPosition A reference to a wxPoint object containing the coordinates of the
 *                  position used to calculate the line break point.
 */
static void computeBreakPoint( SCH_SCREEN* aScreen, SCH_LINE* aSegment, wxPoint& aPosition )
{
    wxCHECK_RET( aSegment != nullptr, wxT( "Cannot compute break point of NULL line segment." ) );

    SCH_LINE* nextSegment = aSegment->Next();

    wxPoint midPoint;
    int iDx = aSegment->GetEndPoint().x - aSegment->GetStartPoint().x;
    int iDy = aSegment->GetEndPoint().y - aSegment->GetStartPoint().y;

    const SCH_SHEET_PIN* connectedPin = getSheetPin( aScreen, aSegment->GetStartPoint() );
    auto force = connectedPin ? connectedPin->GetEdge() : SHEET_UNDEFINED_SIDE;

    if( force == SHEET_LEFT_SIDE || force == SHEET_RIGHT_SIDE )
    {
        if( aPosition.x == connectedPin->GetPosition().x )  // push outside sheet boundary
        {
            int direction = ( force == SHEET_LEFT_SIDE ) ? -1 : 1;
            aPosition.x += int( aScreen->GetGridSize().x * direction );
        }

        midPoint.x = aPosition.x;
        midPoint.y = aSegment->GetStartPoint().y;     // force horizontal
    }
    else if( iDy != 0 )    // keep the first segment orientation (vertical)
    {
        midPoint.x = aSegment->GetStartPoint().x;
        midPoint.y = aPosition.y;
    }
    else if( iDx != 0 )    // keep the first segment orientation (horizontal)
    {
        midPoint.x = aPosition.x;
        midPoint.y = aSegment->GetStartPoint().y;
    }
    else
    {
        if( std::abs( aPosition.x - aSegment->GetStartPoint().x ) <
            std::abs( aPosition.y - aSegment->GetStartPoint().y ) )
        {
            midPoint.x = aSegment->GetStartPoint().x;
            midPoint.y = aPosition.y;
        }
        else
        {
            midPoint.x = aPosition.x;
            midPoint.y = aSegment->GetStartPoint().y;
        }
    }

    aSegment->SetEndPoint( midPoint );
    nextSegment->SetStartPoint( midPoint );
    nextSegment->SetEndPoint( aPosition );
}


int SCH_WIRE_BUS_TOOL::doDrawSegments( int aType, SCH_LINE* aSegment )
{
    bool        forceHV = m_frame->GetForceHVLines();
    SCH_SCREEN* screen = m_frame->GetScreen();
    wxPoint     cursorPos;

    getViewControls()->ShowCursor( true );

    if( aSegment == nullptr )
        Activate();

    // Main loop: keep receiving events
    while( OPT_TOOL_EVENT evt = Wait() )
    {
        cursorPos = (wxPoint)getViewControls()->GetCursorPosition( !evt->Modifier( MD_ALT ) );

        //------------------------------------------------------------------------
        // Handle cancel:
        //
        if( TOOL_EVT_UTILS::IsCancelInteractive( evt.get() ) )
        {
            if( aSegment || m_busUnfold.in_progress )
            {
                m_toolMgr->RunAction( EE_ACTIONS::clearSelection, true );

                aSegment = nullptr;
                s_wires.DeleteAll();

                if( m_busUnfold.entry )
                    m_frame->RemoveFromScreen( m_busUnfold.entry );

                if( m_busUnfold.label && m_busUnfold.label_placed )
                    m_frame->RemoveFromScreen( m_busUnfold.label );

                delete m_busUnfold.entry;
                delete m_busUnfold.label;
                m_busUnfold = {};

                m_view->ClearPreview();
                m_view->ShowPreview( false );

                if( !evt->IsActivate() )
                    continue;
            }

            if( ( evt->IsAction( &EE_ACTIONS::drawWire ) && aType == LAYER_WIRE )
                    || ( evt->IsAction( &EE_ACTIONS::drawBus ) && aType == LAYER_BUS )
                    || ( evt->IsAction( &EE_ACTIONS::unfoldBus ) && aType == LAYER_WIRE ) )
            {
                // Don't reset tool.  If we do the next command will think it needs to
                // re-select the tool rather than start a wire or bus.
            }
            else
            {
                m_frame->SetNoToolSelected();
            }

            break;
        }
        //------------------------------------------------------------------------
        // Handle finish:
        //
        else if( evt->IsAction( &EE_ACTIONS::finishLineWireOrBus )
                     || evt->IsAction( &EE_ACTIONS::finishWire )
                     || evt->IsAction( &EE_ACTIONS::finishBus )
                     || evt->IsAction( &EE_ACTIONS::finishLine ) )
        {
            if( aSegment || m_busUnfold.in_progress )
            {
                finishSegments();
                aSegment = nullptr;
            }

            if( m_frame->GetToolId() == ID_NO_TOOL_SELECTED )
                break;
        }
        //------------------------------------------------------------------------
        // Handle click:
        //
        else if( evt->IsClick( BUT_LEFT ) || ( aSegment && evt->IsDblClick( BUT_LEFT ) ) )
        {
            // First click when unfolding places the label and wire-to-bus entry
            if( m_busUnfold.in_progress && !m_busUnfold.label_placed )
            {
                wxASSERT( aType == LAYER_WIRE );

                m_frame->AddToScreen( m_busUnfold.label );
                m_busUnfold.label_placed = true;
            }

            if( !aSegment )
            {
                aSegment = startSegments( aType, cursorPos );
            }
            // Create a new segment if we're out of previously-created ones
            else if( !aSegment->IsNull() || ( forceHV && !aSegment->Back()->IsNull() ) )
            {
                // Terminate the command if the end point is on a pin, junction, or another
                // wire or bus.
                if( !m_busUnfold.in_progress
                        && screen->IsTerminalPoint( cursorPos, aSegment->GetLayer() ) )
                {
                    finishSegments();
                    aSegment = nullptr;

                    if( m_frame->GetToolId() == ID_NO_TOOL_SELECTED )
                        break;
                }
                else
                {
                    aSegment->SetEndPoint( cursorPos );

                    // Create a new segment, and chain it after the current segment.
                    aSegment = new SCH_LINE( *aSegment );
                    aSegment->SetFlags( IS_NEW | IS_MOVED );
                    aSegment->SetStartPoint( cursorPos );
                    s_wires.PushBack( aSegment );

                    m_selectionTool->AddItemToSel( aSegment, true /*quiet mode*/ );
                }
            }

            if( evt->IsDblClick( BUT_LEFT ) && aSegment )
            {
                if( forceHV )
                    computeBreakPoint( screen, aSegment->Back(), cursorPos );

                finishSegments();
                aSegment = nullptr;

                if( m_frame->GetToolId() == ID_NO_TOOL_SELECTED )
                    break;
            }
        }
        //------------------------------------------------------------------------
        // Handle motion:
        //
        else if( evt->IsMotion() )
        {
            m_view->ClearPreview();

            // Update the bus unfold posture based on the mouse movement
            if( m_busUnfold.in_progress && !m_busUnfold.label_placed )
            {
                wxPoint cursor_delta = cursorPos - m_busUnfold.origin;
                SCH_BUS_WIRE_ENTRY* entry = m_busUnfold.entry;

                bool offset = ( cursor_delta.x < 0 );
                char shape = ( offset ? ( ( cursor_delta.y >= 0 ) ? '/' : '\\' )
                                      : ( ( cursor_delta.y >= 0 ) ? '\\' : '/' ) );

                // Erase and redraw if necessary
                if( shape != entry->GetBusEntryShape() || offset != m_busUnfold.offset )
                {
                    entry->SetBusEntryShape( shape );
                    wxPoint entry_pos = m_busUnfold.origin;

                    if( offset )
                        entry_pos -= entry->GetSize();

                    entry->SetPosition( entry_pos );
                    m_busUnfold.offset = offset;

                    m_frame->RefreshItem( entry );

                    wxPoint wire_start = offset ? entry->GetPosition() : entry->m_End();
                    s_wires.begin()->SetStartPoint( wire_start );
                }

                // Update the label "ghost" position
                m_busUnfold.label->SetPosition( cursorPos );
                m_view->AddToPreview( m_busUnfold.label->Clone() );
            }

            if( aSegment )
            {
                // Coerce the line to vertical or horizontal if necessary
                if( forceHV )
                    computeBreakPoint( screen, aSegment->Back(), cursorPos );
                else
                    aSegment->SetEndPoint( cursorPos );
            }

            for( auto seg = s_wires.begin(); seg; seg = seg->Next() )
            {
                if( !seg->IsNull() )  // Add to preview if segment length != 0
                    m_view->AddToPreview( seg->Clone() );
            }
        }
        //------------------------------------------------------------------------
        // Handle context menu:
        //
        else if( evt->IsClick( BUT_RIGHT ) )
        {
            // Warp after context menu only if dragging...
            if( !aSegment )
                m_toolMgr->VetoContextMenuMouseWarp();

            m_menu.ShowContextMenu( m_selectionTool->GetSelection() );
        }
        else if( evt->Category() == TC_COMMAND && evt->Action() == TA_CONTEXT_MENU_CHOICE )
        {
            if( evt->GetCommandId().get() >= ID_POPUP_SCH_UNFOLD_BUS
                && evt->GetCommandId().get() <= ID_POPUP_SCH_UNFOLD_BUS_END )
            {
                wxASSERT_MSG( !aSegment, "Bus unfold event recieved when already drawing!" );

                aType = LAYER_WIRE;
                wxString net = *evt->Parameter<wxString*>();
                aSegment = doUnfoldBus( net );
            }
        }

        // Enable autopanning and cursor capture only when there is a segment to be placed
        getViewControls()->SetAutoPan( !!aSegment );
        getViewControls()->CaptureCursor( !!aSegment );
    }

    return 0;
}


SCH_LINE* SCH_WIRE_BUS_TOOL::startSegments( int aType, const wxPoint& aPos )
{
    SCH_LINE* segment = nullptr;
    bool      forceHV = m_frame->GetForceHVLines();

    switch( aType )
    {
    default:         segment = new SCH_LINE( aPos, LAYER_NOTES ); break;
    case LAYER_WIRE: segment = new SCH_LINE( aPos, LAYER_WIRE );  break;
    case LAYER_BUS:  segment = new SCH_LINE( aPos, LAYER_BUS );   break;
    }

    segment->SetFlags( IS_NEW | IS_MOVED );
    s_wires.PushBack( segment );

    m_selectionTool->AddItemToSel( segment, true /*quiet mode*/ );

    // We need 2 segments to go from a given start pin to an end point when the
    // horizontal and vertical lines only switch is on.
    if( forceHV )
    {
        segment = new SCH_LINE( *segment );
        segment->SetFlags( IS_NEW | IS_MOVED );
        s_wires.PushBack( segment );

        m_selectionTool->AddItemToSel( segment, true /*quiet mode*/ );
    }

    return segment;
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
 * RemoveBacktracks is called:
 * ------------------->
 */
static void removeBacktracks( DLIST<SCH_LINE>& aWires )
{
    SCH_LINE* next = nullptr;
    std::vector<SCH_LINE*> last_lines;

    for( SCH_LINE* line = aWires.GetFirst(); line; line = next )
    {
        next = line->Next();

        if( line->IsNull() )
        {
            delete s_wires.Remove( line );
            continue;
        }

        if( !last_lines.empty() )
        {
            SCH_LINE* last_line = last_lines[last_lines.size() - 1];
            bool contiguous = ( last_line->GetEndPoint() == line->GetStartPoint() );
            bool backtracks = IsPointOnSegment( last_line->GetStartPoint(),
                                                last_line->GetEndPoint(), line->GetEndPoint() );
            bool total_backtrack = ( last_line->GetStartPoint() == line->GetEndPoint() );

            if( contiguous && backtracks )
            {
                if( total_backtrack )
                {
                    delete s_wires.Remove( last_line );
                    delete s_wires.Remove( line );
                    last_lines.pop_back();
                }
                else
                {
                    last_line->SetEndPoint( line->GetEndPoint() );
                    delete s_wires.Remove( line );
                }
            }
            else
            {
                last_lines.push_back( line );
            }
        }
        else
        {
            last_lines.push_back( line );
        }
    }
}


void SCH_WIRE_BUS_TOOL::finishSegments()
{
    // Clear selection when done so that a new wire can be started.
    // NOTE: this must be done before RemoveBacktracks is called or we might end up with
    // freed selected items.
    m_toolMgr->RunAction( EE_ACTIONS::clearSelection, true );

    PICKED_ITEMS_LIST itemList;

    // Remove segments backtracking over others
    removeBacktracks( s_wires );

    // Collect the possible connection points for the new lines
    std::vector< wxPoint > connections;
    std::vector< wxPoint > new_ends;
    m_frame->GetSchematicConnections( connections );

    // Check each new segment for possible junctions and add/split if needed
    for( SCH_LINE* wire = s_wires.GetFirst(); wire; wire = wire->Next() )
    {
        if( wire->GetFlags() & SKIP_STRUCT )
            continue;

        wire->GetConnectionPoints( new_ends );

        for( auto i : connections )
        {
            if( IsPointOnSegment( wire->GetStartPoint(), wire->GetEndPoint(), i ) )
                new_ends.push_back( i );
        }
        itemList.PushItem( ITEM_PICKER( wire, UR_NEW ) );
    }

    if( m_busUnfold.in_progress && m_busUnfold.label_placed )
    {
        wxASSERT( m_busUnfold.entry && m_busUnfold.label );

        itemList.PushItem( ITEM_PICKER( m_busUnfold.entry, UR_NEW ) );
        itemList.PushItem( ITEM_PICKER( m_busUnfold.label, UR_NEW ) );
    }

    // Get the last non-null wire (this is the last created segment).
    m_frame->SaveCopyForRepeatItem( s_wires.GetLast());

    // Add the new wires
    while( s_wires.GetFirst() )
    {
        s_wires.GetFirst()->ClearFlags( IS_NEW | IS_MOVED );
        m_frame->AddToScreen( s_wires.PopFront() );
    }

    m_view->ClearPreview();
    m_view->ShowPreview( false );

    getViewControls()->CaptureCursor( false );
    getViewControls()->SetAutoPan( false );

    m_frame->SaveCopyInUndoList( itemList, UR_NEW );

    // Correct and remove segments that need to be merged.
    m_frame->SchematicCleanUp();

    for( auto item = m_frame->GetScreen()->GetDrawItems(); item; item = item->Next() )
    {
        if( item->Type() != SCH_COMPONENT_T )
            continue;

        std::vector< wxPoint > pts;
        item->GetConnectionPoints( pts );

        if( pts.size() > 2 )
            continue;

        for( auto i = pts.begin(); i != pts.end(); i++ )
        {
            for( auto j = i + 1; j != pts.end(); j++ )
                m_frame->TrimWire( *i, *j );
        }
    }

    for( auto i : new_ends )
    {
        if( m_frame->GetScreen()->IsJunctionNeeded( i, true ) )
            m_frame->AddJunction( i, true, false );
    }

    if( m_busUnfold.in_progress )
        m_busUnfold = {};

    m_frame->TestDanglingEnds();
    m_toolMgr->PostEvent( EVENTS::SelectedItemsModified );

    m_frame->OnModify();
}


void SCH_WIRE_BUS_TOOL::setTransitions()
{
    Go( &SCH_WIRE_BUS_TOOL::DrawWires,      EE_ACTIONS::drawWire.MakeEvent() );
    Go( &SCH_WIRE_BUS_TOOL::DrawBusses,     EE_ACTIONS::drawBus.MakeEvent() );
    Go( &SCH_WIRE_BUS_TOOL::DrawLines,      EE_ACTIONS::drawLines.MakeEvent() );

    Go( &SCH_WIRE_BUS_TOOL::StartWire,      EE_ACTIONS::startWire.MakeEvent() );
    Go( &SCH_WIRE_BUS_TOOL::StartBus,       EE_ACTIONS::startBus.MakeEvent() );
    Go( &SCH_WIRE_BUS_TOOL::StartLine,      EE_ACTIONS::startLines.MakeEvent() );

    Go( &SCH_WIRE_BUS_TOOL::UnfoldBus,      EE_ACTIONS::unfoldBus.MakeEvent() );
}
