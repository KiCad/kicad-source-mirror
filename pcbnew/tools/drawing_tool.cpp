/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014-2017 CERN
 * Copyright (C) 2018-2021 KiCad Developers, see AUTHORS.txt for contributors.
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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

#include "drawing_tool.h"
#include "pcb_actions.h"
#include <pcb_edit_frame.h>
#include <confirm.h>
#include <import_gfx/dialog_import_gfx.h>
#include <view/view.h>
#include <tool/tool_manager.h>
#include <tools/pcb_grid_helper.h>
#include <tools/pcb_selection_tool.h>
#include <tools/tool_event_utils.h>
#include <tools/zone_create_helper.h>
#include <widgets/appearance_controls.h>
#include <router/router_tool.h>
#include <geometry/geometry_utils.h>
#include <geometry/shape_segment.h>
#include <board_commit.h>
#include <scoped_set_reset.h>
#include <bitmaps.h>
#include <painter.h>
#include <status_popup.h>
#include <dialogs/dialog_text_properties.h>
#include <preview_items/arc_assistant.h>
#include <board.h>
#include <fp_shape.h>
#include <pcb_group.h>
#include <pcb_text.h>
#include <pcb_dimension.h>
#include <zone.h>
#include <footprint.h>
#include <preview_items/two_point_assistant.h>
#include <preview_items/two_point_geom_manager.h>
#include <ratsnest/ratsnest_data.h>
#include <pcbnew_id.h>
#include <dialogs/dialog_track_via_size.h>
#include <kicad_string.h>
#include <macros.h>
#include <widgets/infobar.h>
#include <board_design_settings.h>

using SCOPED_DRAW_MODE = SCOPED_SET_RESET<DRAWING_TOOL::MODE>;


class VIA_SIZE_MENU : public ACTION_MENU
{
public:
    VIA_SIZE_MENU() :
        ACTION_MENU( true )
    {
        SetIcon( BITMAPS::width_track_via );
        SetTitle( _( "Select Via Size" ) );
    }

protected:
    ACTION_MENU* create() const override
    {
        return new VIA_SIZE_MENU();
    }

    void update() override
    {
        PCB_EDIT_FRAME*        frame = (PCB_EDIT_FRAME*) getToolManager()->GetToolHolder();
        EDA_UNITS              units = frame->GetUserUnits();
        BOARD_DESIGN_SETTINGS& bds = frame->GetBoard()->GetDesignSettings();
        bool                   useIndex = !bds.m_UseConnectedTrackWidth
                                                && !bds.UseCustomTrackViaSize();
        wxString               msg;

        Clear();

        Append( ID_POPUP_PCB_SELECT_CUSTOM_WIDTH, _( "Use Custom Values..." ),
                _( "Specify custom track and via sizes" ), wxITEM_CHECK );
        Check( ID_POPUP_PCB_SELECT_CUSTOM_WIDTH, bds.UseCustomTrackViaSize() );

        AppendSeparator();

        for( unsigned i = 1; i < bds.m_ViasDimensionsList.size(); i++ )
        {
            VIA_DIMENSION via = bds.m_ViasDimensionsList[i];

            if( via.m_Drill > 0 )
            {
                msg.Printf( _("Via %s, drill %s" ),
                            MessageTextFromValue( units, via.m_Diameter ),
                            MessageTextFromValue( units, via.m_Drill ) );
            }
            else
            {
                msg.Printf( _( "Via %s" ), MessageTextFromValue( units, via.m_Diameter ) );
            }

            int menuIdx = ID_POPUP_PCB_SELECT_VIASIZE1 + i;
            Append( menuIdx, msg, wxEmptyString, wxITEM_CHECK );
            Check( menuIdx, useIndex && bds.GetViaSizeIndex() == i );
        }
    }

    OPT_TOOL_EVENT eventHandler( const wxMenuEvent& aEvent ) override
    {
        PCB_EDIT_FRAME*        frame = (PCB_EDIT_FRAME*) getToolManager()->GetToolHolder();
        BOARD_DESIGN_SETTINGS& bds = frame->GetBoard()->GetDesignSettings();
        int                    id = aEvent.GetId();

        // On Windows, this handler can be called with an event ID not existing in any
        // menuitem, so only set flags when we have an ID match.

        if( id == ID_POPUP_PCB_SELECT_CUSTOM_WIDTH )
        {
            DIALOG_TRACK_VIA_SIZE sizeDlg( frame, bds );

            if( sizeDlg.ShowModal() == wxID_OK )
            {
                bds.UseCustomTrackViaSize( true );
                bds.m_UseConnectedTrackWidth = false;
            }
        }
        else if( id >= ID_POPUP_PCB_SELECT_VIASIZE1 && id <= ID_POPUP_PCB_SELECT_VIASIZE16 )
        {
            bds.UseCustomTrackViaSize( false );
            bds.m_UseConnectedTrackWidth = false;
            bds.SetViaSizeIndex( id - ID_POPUP_PCB_SELECT_VIASIZE1 );
        }

        return OPT_TOOL_EVENT( PCB_ACTIONS::trackViaSizeChanged.MakeEvent() );
    }
};


DRAWING_TOOL::DRAWING_TOOL() :
    PCB_TOOL_BASE( "pcbnew.InteractiveDrawing" ),
    m_view( nullptr ),
    m_controls( nullptr ),
    m_board( nullptr ),
    m_frame( nullptr ),
    m_mode( MODE::NONE ),
    m_lineWidth( 1 )
{
}


DRAWING_TOOL::~DRAWING_TOOL()
{
}


bool DRAWING_TOOL::Init()
{
    auto activeToolFunctor = [this]( const SELECTION& aSel )
                             {
                                 return m_mode != MODE::NONE;
                             };

    // some interactive drawing tools can undo the last point
    auto canUndoPoint = [this]( const SELECTION& aSel )
                        {
                            return (   m_mode == MODE::ARC
                                    || m_mode == MODE::ZONE
                                    || m_mode == MODE::KEEPOUT
                                    || m_mode == MODE::GRAPHIC_POLYGON );
                        };

    // functor for tools that can automatically close the outline
    auto canCloseOutline = [this]( const SELECTION& aSel )
                           {
                                return (   m_mode == MODE::ZONE
                                        || m_mode == MODE::KEEPOUT
                                        || m_mode == MODE::GRAPHIC_POLYGON );
                           };

    auto viaToolActive = [this]( const SELECTION& aSel )
                         {
                             return m_mode == MODE::VIA;
                         };

    auto& ctxMenu = m_menu.GetMenu();

    // cancel current tool goes in main context menu at the top if present
    ctxMenu.AddItem( ACTIONS::cancelInteractive, activeToolFunctor, 1 );
    ctxMenu.AddSeparator( 1 );

    // tool-specific actions
    ctxMenu.AddItem( PCB_ACTIONS::closeOutline,    canCloseOutline, 200 );
    ctxMenu.AddItem( PCB_ACTIONS::deleteLastPoint, canUndoPoint, 200 );

    ctxMenu.AddSeparator( 500 );

    std::shared_ptr<VIA_SIZE_MENU> viaSizeMenu = std::make_shared<VIA_SIZE_MENU>();
    viaSizeMenu->SetTool( this );
    m_menu.AddSubMenu( viaSizeMenu );
    ctxMenu.AddMenu( viaSizeMenu.get(),            viaToolActive, 500 );

    ctxMenu.AddSeparator( 500 );

    // Type-specific sub-menus will be added for us by other tools
    // For example, zone fill/unfill is provided by the PCB control tool

    // Finally, add the standard zoom/grid items
    getEditFrame<PCB_BASE_FRAME>()->AddStandardSubMenus( m_menu );

    return true;
}


void DRAWING_TOOL::Reset( RESET_REASON aReason )
{
    // Init variables used by every drawing tool
    m_view = getView();
    m_controls = getViewControls();
    m_board = getModel<BOARD>();
    m_frame = getEditFrame<PCB_BASE_EDIT_FRAME>();
}


DRAWING_TOOL::MODE DRAWING_TOOL::GetDrawingMode() const
{
    return m_mode;
}


int DRAWING_TOOL::DrawLine( const TOOL_EVENT& aEvent )
{
    if( m_isFootprintEditor && !m_frame->GetModel() )
        return 0;

    FOOTPRINT*       parentFootprint = dynamic_cast<FOOTPRINT*>( m_frame->GetModel() );
    PCB_SHAPE*       line = m_isFootprintEditor ? new FP_SHAPE( parentFootprint ) : new PCB_SHAPE;
    BOARD_COMMIT     commit( m_frame );
    SCOPED_DRAW_MODE scopedDrawMode( m_mode, MODE::LINE );
    OPT<VECTOR2D>    startingPoint = boost::make_optional<VECTOR2D>( false, VECTOR2D( 0, 0 ) );

    line->SetShape( PCB_SHAPE_TYPE::SEGMENT );
    line->SetFlags( IS_NEW );

    if( aEvent.HasPosition() )
        startingPoint = getViewControls()->GetCursorPosition( !aEvent.DisableGridSnapping() );

    std::string tool = aEvent.GetCommandStr().get();
    m_frame->PushTool( tool );
    Activate();

    while( drawSegment( tool, &line, startingPoint ) )
    {
        if( line )
        {
            if( m_isFootprintEditor )
                static_cast<FP_SHAPE*>( line )->SetLocalCoord();

            commit.Add( line );
            commit.Push( _( "Draw a line segment" ) );
            startingPoint = VECTOR2D( line->GetEnd() );
        }
        else
        {
            startingPoint = NULLOPT;
        }

        line = m_isFootprintEditor ? new FP_SHAPE( parentFootprint ) : new PCB_SHAPE;
        line->SetShape( PCB_SHAPE_TYPE::SEGMENT );
        line->SetFlags( IS_NEW );
    }

    return 0;
}


int DRAWING_TOOL::DrawRectangle( const TOOL_EVENT& aEvent )
{
    if( m_isFootprintEditor && !m_frame->GetModel() )
        return 0;

    FOOTPRINT*       parentFootprint = dynamic_cast<FOOTPRINT*>( m_frame->GetModel() );
    PCB_SHAPE*       rect = m_isFootprintEditor ? new FP_SHAPE( parentFootprint ) : new PCB_SHAPE;
    BOARD_COMMIT     commit( m_frame );
    SCOPED_DRAW_MODE scopedDrawMode( m_mode, MODE::RECTANGLE );
    OPT<VECTOR2D>    startingPoint = boost::make_optional<VECTOR2D>( false, VECTOR2D( 0, 0 ) );

    rect->SetShape( PCB_SHAPE_TYPE::RECT );
    rect->SetFilled( false );
    rect->SetFlags(IS_NEW );

    if( aEvent.HasPosition() )
        startingPoint = getViewControls()->GetCursorPosition( !aEvent.DisableGridSnapping() );

    std::string tool = aEvent.GetCommandStr().get();
    m_frame->PushTool( tool );
    Activate();

    while( drawSegment( tool, &rect, startingPoint ) )
    {
        if( rect )
        {
            if( m_isFootprintEditor )
                static_cast<FP_SHAPE*>( rect )->SetLocalCoord();

            commit.Add( rect );
            commit.Push( _( "Draw a rectangle" ) );

            m_toolMgr->RunAction( PCB_ACTIONS::selectItem, true, rect );
        }

        rect = m_isFootprintEditor ? new FP_SHAPE( parentFootprint ) : new PCB_SHAPE;
        rect->SetShape( PCB_SHAPE_TYPE::RECT );
        rect->SetFilled( false );
        rect->SetFlags(IS_NEW );
        startingPoint = NULLOPT;
    }

    return 0;
}


int DRAWING_TOOL::DrawCircle( const TOOL_EVENT& aEvent )
{
    if( m_isFootprintEditor && !m_frame->GetModel() )
        return 0;

    FOOTPRINT*       parentFootprint = dynamic_cast<FOOTPRINT*>( m_frame->GetModel() );
    PCB_SHAPE*       circle = m_isFootprintEditor ? new FP_SHAPE( parentFootprint ) : new PCB_SHAPE;
    BOARD_COMMIT     commit( m_frame );
    SCOPED_DRAW_MODE scopedDrawMode( m_mode, MODE::CIRCLE );
    OPT<VECTOR2D>    startingPoint = boost::make_optional<VECTOR2D>( false, VECTOR2D( 0, 0 ) );

    circle->SetShape( PCB_SHAPE_TYPE::CIRCLE );
    circle->SetFilled( false );
    circle->SetFlags( IS_NEW );

    if( aEvent.HasPosition() )
        startingPoint = getViewControls()->GetCursorPosition( !aEvent.DisableGridSnapping() );

    std::string tool = aEvent.GetCommandStr().get();
    m_frame->PushTool( tool );
    Activate();

    while( drawSegment( tool, &circle, startingPoint ) )
    {
        if( circle )
        {
            if( m_isFootprintEditor )
                static_cast<FP_SHAPE*>( circle )->SetLocalCoord();

            commit.Add( circle );
            commit.Push( _( "Draw a circle" ) );

            m_toolMgr->RunAction( PCB_ACTIONS::selectItem, true, circle );
        }

        circle = m_isFootprintEditor ? new FP_SHAPE( parentFootprint ) : new PCB_SHAPE;
        circle->SetShape( PCB_SHAPE_TYPE::CIRCLE );
        circle->SetFilled( false );
        circle->SetFlags( IS_NEW );
        startingPoint = NULLOPT;
    }

    return 0;
}


int DRAWING_TOOL::DrawArc( const TOOL_EVENT& aEvent )
{
    if( m_isFootprintEditor && !m_frame->GetModel() )
        return 0;

    FOOTPRINT*       parentFootprint = dynamic_cast<FOOTPRINT*>( m_frame->GetModel() );
    PCB_SHAPE*       arc = m_isFootprintEditor ? new FP_SHAPE( parentFootprint ) : new PCB_SHAPE;
    BOARD_COMMIT     commit( m_frame );
    SCOPED_DRAW_MODE scopedDrawMode( m_mode, MODE::ARC );
    bool             immediateMode = aEvent.HasPosition();

    arc->SetShape( PCB_SHAPE_TYPE::ARC );
    arc->SetFlags( IS_NEW );

    std::string tool = aEvent.GetCommandStr().get();
    m_frame->PushTool( tool );
    Activate();

    while( drawArc( tool, &arc, immediateMode ) )
    {
        if( arc )
        {
            if( m_isFootprintEditor )
                static_cast<FP_SHAPE*>( arc )->SetLocalCoord();

            commit.Add( arc );
            commit.Push( _( "Draw an arc" ) );

            m_toolMgr->RunAction( PCB_ACTIONS::selectItem, true, arc );
        }

        arc = m_isFootprintEditor ? new FP_SHAPE( parentFootprint ) : new PCB_SHAPE;
        arc->SetShape( PCB_SHAPE_TYPE::ARC );
        arc->SetFlags( IS_NEW );
        immediateMode = false;
    }

    return 0;
}


int DRAWING_TOOL::PlaceText( const TOOL_EVENT& aEvent )
{
    if( m_isFootprintEditor && !m_frame->GetModel() )
        return 0;

    BOARD_ITEM*                  text = NULL;
    const BOARD_DESIGN_SETTINGS& dsnSettings = m_frame->GetDesignSettings();
    BOARD_COMMIT                 commit( m_frame );
    SCOPED_DRAW_MODE             scopedDrawMode( m_mode, MODE::TEXT );

    auto cleanup =
            [&]()
            {
                m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );
                m_controls->ForceCursorPosition( false );
                m_controls->ShowCursor( true );
                m_controls->SetAutoPan( false );
                m_controls->CaptureCursor( false );
                delete text;
                text = NULL;
            };

    auto setCursor =
            [&]()
            {
                if( text )
                    m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::MOVING );
                else
                    m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::TEXT );
            };

    m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );
    m_controls->ShowCursor( true );
    // do not capture or auto-pan until we start placing some text

    std::string tool = aEvent.GetCommandStr().get();
    m_frame->PushTool( tool );
    Activate();

    // Prime the pump
    if( !aEvent.IsReactivate() )
        m_toolMgr->RunAction( ACTIONS::cursorClick );

    // Set initial cursor
    setCursor();

    // Main loop: keep receiving events
    while( TOOL_EVENT* evt = Wait() )
    {
        setCursor();
        VECTOR2I cursorPos = m_controls->GetCursorPosition();

        if( evt->IsCancelInteractive() )
        {
            if( text )
            {
                cleanup();
            }
            else
            {
                m_frame->PopTool( tool );
                break;
            }
        }
        else if( evt->IsActivate() )
        {
            if( text )
                cleanup();

            if( evt->IsMoveTool() )
            {
                // leave ourselves on the stack so we come back after the move
                break;
            }
            else
            {
                m_frame->PopTool( tool );
                break;
            }
        }
        else if( evt->IsClick( BUT_RIGHT ) )
        {
            m_menu.ShowContextMenu( selection() );
        }
        else if( evt->IsClick( BUT_LEFT ) )
        {
            bool placing = text != nullptr;

            if( !text )
            {
                m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );

                m_controls->ForceCursorPosition( true, m_controls->GetCursorPosition() );
                PCB_LAYER_ID layer = m_frame->GetActiveLayer();

                // Init the new item attributes
                if( m_isFootprintEditor )
                {
                    FP_TEXT* fpText = new FP_TEXT( (FOOTPRINT*) m_frame->GetModel() );

                    fpText->SetLayer( layer );
                    fpText->SetTextSize( dsnSettings.GetTextSize( layer ) );
                    fpText->SetTextThickness( dsnSettings.GetTextThickness( layer ) );
                    fpText->SetItalic( dsnSettings.GetTextItalic( layer ) );
                    fpText->SetKeepUpright( dsnSettings.GetTextUpright( layer ) );
                    fpText->SetTextPos( (wxPoint) cursorPos );

                    text = fpText;

                    DIALOG_TEXT_PROPERTIES textDialog( m_frame, fpText );
                    bool cancelled;

                    RunMainStack( [&]()
                                  {
                                      cancelled = !textDialog.ShowModal();
                                  } );

                    if( cancelled || NoPrintableChars( fpText->GetText() ) )
                    {
                        delete text;
                        text = nullptr;
                    }
                    else if( fpText->GetTextPos() != (wxPoint) cursorPos )
                    {
                        // If the user modified the location then go ahead and place it there.
                        // Otherwise we'll drag.
                        placing = true;
                    }
                }
                else
                {
                    PCB_TEXT* pcbText = new PCB_TEXT( m_frame->GetModel() );
                    // TODO we have to set IS_NEW, otherwise InstallTextPCB.. creates an undo entry :| LEGACY_CLEANUP
                    pcbText->SetFlags( IS_NEW );

                    pcbText->SetLayer( layer );

                    // Set the mirrored option for layers on the BACK side of the board
                    if( IsBackLayer( layer ) )
                        pcbText->SetMirrored( true );

                    pcbText->SetTextSize( dsnSettings.GetTextSize( layer ) );
                    pcbText->SetTextThickness( dsnSettings.GetTextThickness( layer ) );
                    pcbText->SetItalic( dsnSettings.GetTextItalic( layer ) );
                    pcbText->SetTextPos( (wxPoint) cursorPos );

                    RunMainStack( [&]()
                                  {
                                      m_frame->ShowTextPropertiesDialog( pcbText );
                                  } );

                    if( NoPrintableChars( pcbText->GetText() ) )
                        delete pcbText;
                    else
                        text = pcbText;
                }

                if( text )
                {
                    if( !m_view->IsLayerVisible( text->GetLayer() ) )
                    {
                        m_frame->GetAppearancePanel()->SetLayerVisible( text->GetLayer(), true );
                        m_frame->GetCanvas()->Refresh();
                    }

                    m_controls->WarpCursor( text->GetPosition(), true );
                    m_toolMgr->RunAction( PCB_ACTIONS::selectItem, true, text );
                    m_view->Update( &selection() );

                    // update the cursor so it looks correct before another event
                    setCursor();
                }
            }

            if( placing )
            {
                text->ClearFlags();
                m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );

                commit.Add( text );
                commit.Push( _( "Place a text" ) );

                m_toolMgr->RunAction( PCB_ACTIONS::selectItem, true, text );

                text = nullptr;
            }

            m_controls->ForceCursorPosition( false );
            m_controls->ShowCursor( true );
            m_controls->CaptureCursor( text != nullptr );
            m_controls->SetAutoPan( text != nullptr );
        }
        else if( text && evt->IsMotion() )
        {
            text->SetPosition( (wxPoint) cursorPos );
            selection().SetReferencePoint( cursorPos );
            m_view->Update( &selection() );
        }
        else if( evt->IsAction( &PCB_ACTIONS::properties ) )
        {
            if( text )
            {
                frame()->OnEditItemRequest( text );
                m_view->Update( &selection() );
                frame()->SetMsgPanel( text );
            }
            else
            {
                evt->SetPassEvent();
            }
        }
        else
        {
            evt->SetPassEvent();
        }
    }

    m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::ARROW );
    m_frame->SetMsgPanel( board() );
    return 0;
}


void DRAWING_TOOL::constrainDimension( PCB_DIMENSION_BASE* aDim )
{
    const VECTOR2I lineVector{ aDim->GetEnd() - aDim->GetStart() };

    aDim->SetEnd( wxPoint( VECTOR2I( aDim->GetStart() ) + GetVectorSnapped45( lineVector ) ) );
    aDim->Update();
}


int DRAWING_TOOL::DrawDimension( const TOOL_EVENT& aEvent )
{
    if( m_isFootprintEditor && !m_frame->GetModel() )
        return 0;

    enum DIMENSION_STEPS
    {
        SET_ORIGIN = 0,
        SET_END,
        SET_HEIGHT,
        FINISHED
    };

    TOOL_EVENT             originalEvent = aEvent;
    PCB_DIMENSION_BASE*    dimension     = nullptr;
    BOARD_COMMIT           commit( m_frame );
    PCB_GRID_HELPER        grid( m_toolMgr, m_frame->GetMagneticItemsSettings() );
    BOARD_DESIGN_SETTINGS& boardSettings = m_board->GetDesignSettings();
    PCB_SELECTION          preview;   // A VIEW_GROUP that serves as a preview for the new item(s)
    SCOPED_DRAW_MODE       scopedDrawMode( m_mode, MODE::DIMENSION );
    int                    step = SET_ORIGIN;

    m_view->Add( &preview );

    auto cleanup =
            [&]()
            {
                m_controls->SetAutoPan( false );
                m_controls->CaptureCursor( false );

                preview.Clear();
                m_view->Update( &preview );

                delete dimension;
                dimension = nullptr;
                step = SET_ORIGIN;
            };

    auto setCursor =
            [&]()
            {
                m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::MEASURE );
            };

    m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );
    m_controls->ShowCursor( true );

    std::string tool = aEvent.GetCommandStr().get();
    m_frame->PushTool( tool );
    Activate();

    // Prime the pump
    m_toolMgr->RunAction( ACTIONS::refreshPreview );

    if( aEvent.HasPosition() )
        m_toolMgr->PrimeTool( aEvent.Position() );

    // Set initial cursor
    setCursor();

    // Main loop: keep receiving events
    while( TOOL_EVENT* evt = Wait() )
    {
        if( step > SET_ORIGIN )
            frame()->SetMsgPanel( dimension );

        setCursor();

        grid.SetSnap( !evt->Modifier( MD_SHIFT ) );
        grid.SetUseGrid( getView()->GetGAL()->GetGridSnapping() && !evt->DisableGridSnapping() );

        VECTOR2I cursorPos = evt->HasPosition() ? evt->Position() : m_controls->GetMousePosition();

        cursorPos = grid.BestSnapAnchor( cursorPos, nullptr );
        m_controls->ForceCursorPosition( true, cursorPos );

        if( evt->IsCancelInteractive() )
        {
            m_controls->SetAutoPan( false );

            if( step != SET_ORIGIN )    // start from the beginning
            {
                cleanup();
            }
            else
            {
                m_frame->PopTool( tool );
                break;
            }
        }
        else if( evt->IsActivate() )
        {
            if( step != SET_ORIGIN )
                cleanup();

            if( evt->IsPointEditor() )
            {
                // don't exit (the point editor runs in the background)
            }
            else if( evt->IsMoveTool() )
            {
                // leave ourselves on the stack so we come back after the move
                break;
            }
            else
            {
                m_frame->PopTool( tool );
                break;
            }
        }
        else if( evt->IsAction( &PCB_ACTIONS::incWidth ) && step != SET_ORIGIN )
        {
            m_lineWidth += WIDTH_STEP;
            dimension->SetLineThickness( m_lineWidth );
            m_view->Update( &preview );
            frame()->SetMsgPanel( dimension );
        }
        else if( evt->IsAction( &PCB_ACTIONS::decWidth ) && step != SET_ORIGIN )
        {
            if( m_lineWidth > WIDTH_STEP )
            {
                m_lineWidth -= WIDTH_STEP;
                dimension->SetLineThickness( m_lineWidth );
                m_view->Update( &preview );
                frame()->SetMsgPanel( dimension );
            }
        }
        else if( evt->IsClick( BUT_RIGHT ) )
        {
            m_menu.ShowContextMenu( selection() );
        }
        else if( evt->IsClick( BUT_LEFT ) )
        {
            switch( step )
            {
            case SET_ORIGIN:
            {
                m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );

                PCB_LAYER_ID layer = m_frame->GetActiveLayer();

                // Init the new item attributes
                auto setMeasurementAttributes =
                        [&]( PCB_DIMENSION_BASE* aDim )
                        {
                            aDim->SetUnitsMode( boardSettings.m_DimensionUnitsMode );
                            aDim->SetUnitsFormat( boardSettings.m_DimensionUnitsFormat );
                            aDim->SetPrecision( boardSettings.m_DimensionPrecision );
                            aDim->SetSuppressZeroes( boardSettings.m_DimensionSuppressZeroes );
                            aDim->SetTextPositionMode( boardSettings.m_DimensionTextPosition );
                            aDim->SetKeepTextAligned( boardSettings.m_DimensionKeepTextAligned );

                            if( boardSettings.m_DimensionUnitsMode == DIM_UNITS_MODE::AUTOMATIC )
                                aDim->SetUnits( m_frame->GetUserUnits() );
                        };

                if( originalEvent.IsAction( &PCB_ACTIONS::drawAlignedDimension ) )
                {
                    dimension = new PCB_DIM_ALIGNED( m_board );
                    setMeasurementAttributes( dimension );
                }
                else if( originalEvent.IsAction( &PCB_ACTIONS::drawOrthogonalDimension ) )
                {
                    dimension = new PCB_DIM_ORTHOGONAL( m_board );
                    setMeasurementAttributes( dimension );
                }
                else if( originalEvent.IsAction( &PCB_ACTIONS::drawCenterDimension ) )
                {
                    dimension = new PCB_DIM_CENTER( m_board );
                }
                else if( originalEvent.IsAction( &PCB_ACTIONS::drawLeader ) )
                {
                    dimension = new PCB_DIM_LEADER( m_board );
                    dimension->Text().SetPosition( wxPoint( cursorPos ) );
                }
                else
                {
                    wxFAIL_MSG( "Unhandled action in DRAWING_TOOL::DrawDimension" );
                }

                dimension->SetLayer( layer );
                dimension->Text().SetTextSize( boardSettings.GetTextSize( layer ) );
                dimension->Text().SetTextThickness( boardSettings.GetTextThickness( layer ) );
                dimension->Text().SetItalic( boardSettings.GetTextItalic( layer ) );
                dimension->SetLineThickness( boardSettings.GetLineThickness( layer ) );
                dimension->SetArrowLength( boardSettings.m_DimensionArrowLength );
                dimension->SetExtensionOffset( boardSettings.m_DimensionExtensionOffset );
                dimension->SetStart( (wxPoint) cursorPos );
                dimension->SetEnd( (wxPoint) cursorPos );
                dimension->Update();

                if( !m_view->IsLayerVisible( layer ) )
                {
                    m_frame->GetAppearancePanel()->SetLayerVisible( layer, true );
                    m_frame->GetCanvas()->Refresh();
                }

                preview.Add( dimension );
                frame()->SetMsgPanel( dimension );

                m_controls->SetAutoPan( true );
                m_controls->CaptureCursor( true );
            }
                break;

            case SET_END:
            {
                dimension->SetEnd( (wxPoint) cursorPos );
                dimension->Update();

                if( !!evt->Modifier( MD_SHIFT ) || dimension->Type() == PCB_DIM_CENTER_T )
                    constrainDimension( dimension );

                // Dimensions that have origin and end in the same spot are not valid
                if( dimension->GetStart() == dimension->GetEnd() )
                    --step;
                else if( dimension->Type() == PCB_DIM_LEADER_T )
                    dimension->SetText( wxT( "?" ) );

                if( dimension->Type() == PCB_DIM_CENTER_T )
                {
                    // No separate height/text step
                    ++step;
                    KI_FALLTHROUGH;
                }
                else
                {
                    break;
                }
            }

            case SET_HEIGHT:
                if( dimension->Type() == PCB_DIM_LEADER_T )
                {
                    assert( dimension->GetStart() != dimension->GetEnd() );
                    assert( dimension->GetLineThickness() > 0 );

                    preview.Remove( dimension );

                    commit.Add( dimension );
                    commit.Push( _( "Draw a leader" ) );

                    // Run the edit immediately to set the leader text
                    m_toolMgr->RunAction( PCB_ACTIONS::properties, true, dimension );
                }
                else if( (wxPoint) cursorPos != dimension->GetPosition() )
                {
                    assert( dimension->GetStart() != dimension->GetEnd() );
                    assert( dimension->GetLineThickness() > 0 );

                    preview.Remove( dimension );

                    commit.Add( dimension );
                    commit.Push( _( "Draw a dimension" ) );

                    m_toolMgr->RunAction( PCB_ACTIONS::selectItem, true, dimension );
                }

                break;
            }

            if( ++step == FINISHED )
            {
                step = SET_ORIGIN;
                m_controls->SetAutoPan( false );
                m_controls->CaptureCursor( false );
            }
        }
        else if( evt->IsMotion() )
        {
            switch( step )
            {
            case SET_END:
                dimension->SetEnd( (wxPoint) cursorPos );

                if( dimension->Type() == PCB_DIM_ORTHOGONAL_T )
                {
                    PCB_DIM_ORTHOGONAL* ortho = static_cast<PCB_DIM_ORTHOGONAL*>( dimension );

                    BOX2I bounds( dimension->GetStart(),
                                  dimension->GetEnd() - dimension->GetStart() );

                    // Create a nice preview by measuring the longer dimension
                    bool vert = bounds.GetWidth() < bounds.GetHeight();

                    ortho->SetOrientation( vert ? PCB_DIM_ORTHOGONAL::DIR::VERTICAL
                                                : PCB_DIM_ORTHOGONAL::DIR::HORIZONTAL );
                }

                dimension->Update();

                if( !!evt->Modifier( MD_SHIFT ) || dimension->Type() == PCB_DIM_CENTER_T )
                    constrainDimension( dimension );

                break;

            case SET_HEIGHT:
            {
                if( dimension->Type() == PCB_DIM_ALIGNED_T )
                {
                    PCB_DIM_ALIGNED* aligned = static_cast<PCB_DIM_ALIGNED*>( dimension );

                    // Calculating the direction of travel perpendicular to the selected axis
                    double angle = aligned->GetAngle() + ( M_PI / 2 );

                    wxPoint delta( (wxPoint) cursorPos - dimension->GetEnd() );
                    double  height = ( delta.x * cos( angle ) ) + ( delta.y * sin( angle ) );
                    aligned->SetHeight( height );
                    aligned->Update();
                }
                else if( dimension->Type() == PCB_DIM_ORTHOGONAL_T )
                {
                    PCB_DIM_ORTHOGONAL* ortho = static_cast<PCB_DIM_ORTHOGONAL*>( dimension );

                    BOX2I    bounds( dimension->GetStart(),
                                  dimension->GetEnd() - dimension->GetStart() );
                    VECTOR2I direction( cursorPos - bounds.Centre() );
                    bool     vert;

                    // Only change the orientation when we move outside the bounds
                    if( !bounds.Contains( cursorPos ) )
                    {
                        // If the dimension is horizontal or vertical, set correct orientation
                        // otherwise, test if we're left/right of the bounding box or above/below it
                        if( bounds.GetWidth() == 0 )
                        {
                            vert = true;
                        }
                        else if( bounds.GetHeight() == 0 )
                        {
                            vert = false;
                        }
                        else if( cursorPos.x > bounds.GetLeft() && cursorPos.x < bounds.GetRight() )
                        {
                            vert = false;
                        }
                        else if( cursorPos.y > bounds.GetTop() && cursorPos.y < bounds.GetBottom() )
                        {
                            vert = true;
                        }
                        else
                        {
                            vert = std::abs( direction.y ) < std::abs( direction.x );
                        }
                        ortho->SetOrientation( vert ? PCB_DIM_ORTHOGONAL::DIR::VERTICAL
                                                    : PCB_DIM_ORTHOGONAL::DIR::HORIZONTAL );
                    }
                    else
                    {
                        vert = ortho->GetOrientation() == PCB_DIM_ORTHOGONAL::DIR::VERTICAL;
                    }

                    VECTOR2I heightVector( cursorPos - dimension->GetStart() );
                    ortho->SetHeight( vert ? heightVector.x : heightVector.y );
                    ortho->Update();
                }
                else if( dimension->Type() == PCB_DIM_LEADER_T )
                {
                    // Leader: SET_HEIGHT actually sets the text position directly
                    VECTOR2I lineVector( cursorPos - dimension->GetEnd() );
                    dimension->Text().SetPosition( wxPoint( VECTOR2I( dimension->GetEnd() ) +
                                                            GetVectorSnapped45( lineVector ) ) );
                    dimension->Update();
                }
            }
            break;
            }

            // Show a preview of the item
            m_view->Update( &preview );
        }
        else if( evt->IsAction( &PCB_ACTIONS::layerChanged ) )
        {
            if( dimension )
            {
                PCB_LAYER_ID layer = m_frame->GetActiveLayer();

                if( !m_view->IsLayerVisible( layer ) )
                {
                    m_frame->GetAppearancePanel()->SetLayerVisible( layer, true );
                    m_frame->GetCanvas()->Refresh();
                }

                dimension->SetLayer( layer );
                dimension->Text().SetTextSize( boardSettings.GetTextSize( layer ) );
                dimension->Text().SetTextThickness( boardSettings.GetTextThickness( layer ) );
                dimension->Text().SetItalic( boardSettings.GetTextItalic( layer ) );
                dimension->SetLineThickness( boardSettings.GetLineThickness( layer ) );
                dimension->Update();

                m_view->Update( &preview );
                frame()->SetMsgPanel( dimension );
            }
            else
            {
                evt->SetPassEvent();
            }
        }
        else if( evt->IsAction( &PCB_ACTIONS::properties ) )
        {
            if( step == SET_END || step == SET_HEIGHT )
            {
                frame()->OnEditItemRequest( dimension );
                dimension->Update();
                frame()->SetMsgPanel( dimension );
                break;
            }
            else
            {
                evt->SetPassEvent();
            }
        }
        else
        {
            evt->SetPassEvent();
        }
    }

    if( step != SET_ORIGIN )
        delete dimension;

    m_controls->SetAutoPan( false );
    m_controls->ForceCursorPosition( false );
    m_controls->CaptureCursor( false );
    m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::ARROW );

    m_view->Remove( &preview );
    m_frame->SetMsgPanel( board() );
    return 0;
}


int DRAWING_TOOL::PlaceImportedGraphics( const TOOL_EVENT& aEvent )
{
    if( !m_frame->GetModel() )
        return 0;

    // Note: PlaceImportedGraphics() will convert PCB_SHAPE_T and PCB_TEXT_T to footprint
    // items if needed
    DIALOG_IMPORT_GFX dlg( m_frame, m_isFootprintEditor );
    int dlgResult = dlg.ShowModal();

    std::list<std::unique_ptr<EDA_ITEM>>& list = dlg.GetImportedItems();

    if( dlgResult != wxID_OK )
        return 0;

    // Ensure the list is not empty:
    if( list.empty() )
    {
        wxMessageBox( _( "No graphic items found in file to import") );
        return 0;
    }

    m_toolMgr->RunAction( ACTIONS::cancelInteractive, true );

    std::vector<BOARD_ITEM*> newItems;          // all new items, including group
    std::vector<BOARD_ITEM*> selectedItems;     // the group, or newItems if no group
    PCB_SELECTION            preview;
    BOARD_COMMIT             commit( m_frame );
    PCB_GROUP*               group = nullptr;

    if( dlg.ShouldGroupItems() )
    {
        if( m_isFootprintEditor )
            group = new PCB_GROUP( m_frame->GetBoard()->GetFirstFootprint() );
        else
            group = new PCB_GROUP( m_frame->GetBoard() );

        newItems.push_back( group );
        selectedItems.push_back( group );
        preview.Add( group );
    }

    for( std::unique_ptr<EDA_ITEM>& ptr : list )
    {
        BOARD_ITEM* item = static_cast<BOARD_ITEM*>( ptr.get() );

        if( m_isFootprintEditor )
            wxASSERT( item->Type() == PCB_FP_SHAPE_T || item->Type() == PCB_FP_TEXT_T );
        else
            wxASSERT( item->Type() == PCB_SHAPE_T || item->Type() == PCB_TEXT_T );

        newItems.push_back( item );

        if( group )
            group->AddItem( item );
        else
            selectedItems.push_back( item );

        preview.Add( item );

        ptr.release();
    }

    if( !dlg.IsPlacementInteractive() )
    {
        for( BOARD_ITEM* item : newItems )
            commit.Add( item );

        commit.Push( _( "Place a DXF_SVG drawing" ) );
        return 0;
    }

    m_view->Add( &preview );

    // Clear the current selection then select the drawings so that edit tools work on them
    m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );
    m_toolMgr->RunAction( PCB_ACTIONS::selectItems, true, &selectedItems );

    m_controls->ShowCursor( true );
    m_controls->ForceCursorPosition( false );

    SCOPED_DRAW_MODE scopedDrawMode( m_mode, MODE::DXF );

    // Now move the new items to the current cursor position:
    VECTOR2I cursorPos = m_controls->GetCursorPosition();
    VECTOR2I delta = cursorPos - static_cast<BOARD_ITEM*>( preview.Front() )->GetPosition();

    for( BOARD_ITEM* item : selectedItems )
        item->Move( (wxPoint) delta );

    m_view->Update( &preview );

    std::string tool = aEvent.GetCommandStr().get();
    m_frame->PushTool( tool );
    Activate();

    auto setCursor =
            [&]()
            {
                m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::MOVING );
            };

    // Set initial cursor
    setCursor();

    // Main loop: keep receiving events
    while( TOOL_EVENT* evt = Wait() )
    {
        setCursor();
        cursorPos = m_controls->GetCursorPosition();

        if( evt->IsCancelInteractive() || evt->IsActivate() )
        {
            m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );

            for( BOARD_ITEM* item : newItems )
                delete item;

            break;
        }
        else if( evt->IsMotion() )
        {
            delta = cursorPos - static_cast<BOARD_ITEM*>( preview.Front() )->GetPosition();

            for( BOARD_ITEM* item : selectedItems )
                item->Move( (wxPoint) delta );

            m_view->Update( &preview );
        }
        else if( evt->IsClick( BUT_RIGHT ) )
        {
            m_menu.ShowContextMenu( selection() );
        }
        else if( evt->IsClick( BUT_LEFT ) || evt->IsDblClick( BUT_LEFT ) )
        {
            // Place the imported drawings
            for( BOARD_ITEM* item : newItems )
                commit.Add( item );

            commit.Push( _( "Place a DXF_SVG drawing" ) );
            break;   // This is a one-shot command, not a tool
        }
        else
        {
            evt->SetPassEvent();
        }
    }

    preview.Clear();
    m_view->Remove( &preview );

    m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::ARROW );
    m_frame->PopTool( tool );
    return 0;
}


int DRAWING_TOOL::SetAnchor( const TOOL_EVENT& aEvent )
{
    wxASSERT( m_isFootprintEditor );

    if( !m_frame->GetModel() )
        return 0;

    SCOPED_DRAW_MODE scopedDrawMode( m_mode, MODE::ANCHOR );
    PCB_GRID_HELPER  grid( m_toolMgr, m_frame->GetMagneticItemsSettings() );

    std::string tool = aEvent.GetCommandStr().get();
    m_frame->PushTool( tool );
    Activate();

    m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );
    m_controls->ShowCursor( true );
    m_controls->SetAutoPan( true );
    m_controls->CaptureCursor( false );

    auto setCursor =
            [&]()
            {
                m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::BULLSEYE );
            };

    // Set initial cursor
    setCursor();

    while( TOOL_EVENT* evt = Wait() )
    {
        setCursor();

        grid.SetSnap( !evt->Modifier( MD_SHIFT ) );
        grid.SetUseGrid( getView()->GetGAL()->GetGridSnapping() && !evt->DisableGridSnapping() );
        VECTOR2I cursorPos = grid.BestSnapAnchor( m_controls->GetMousePosition(),
                                                  LSET::AllLayersMask() );
        m_controls->ForceCursorPosition( true, cursorPos );

        if( evt->IsClick( BUT_LEFT ) || evt->IsDblClick( BUT_LEFT ) )
        {
            FOOTPRINT*   footprint = (FOOTPRINT*) m_frame->GetModel();
            BOARD_COMMIT commit( m_frame );
            commit.Modify( footprint );

            // set the new relative internal local coordinates of footprint items
            wxPoint     moveVector = footprint->GetPosition() - (wxPoint) cursorPos;
            footprint->MoveAnchorPosition( moveVector );

            commit.Push( _( "Move the footprint reference anchor" ) );

            // Usually, we do not need to change twice the anchor position,
            // so deselect the active tool
            m_frame->PopTool( tool );
            break;
        }
        else if( evt->IsClick( BUT_RIGHT ) )
        {
            m_menu.ShowContextMenu( selection() );
        }
        else if( evt->IsCancelInteractive() || evt->IsActivate() )
        {
            m_frame->PopTool( tool );
            break;
        }
        else
        {
            evt->SetPassEvent();
        }
    }

    m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::ARROW );
    return 0;
}


int DRAWING_TOOL::ToggleLine45degMode( const TOOL_EVENT& toolEvent )
{
    m_frame->Settings().m_Use45DegreeGraphicSegments = !m_frame->Settings().m_Use45DegreeGraphicSegments;

    return 0;
}


/**
 * Update an PCB_SHAPE from the current state of a TWO_POINT_GEOMETRY_MANAGER
 */
static void updateSegmentFromGeometryMgr( const KIGFX::PREVIEW::TWO_POINT_GEOMETRY_MANAGER& aMgr,
                                          PCB_SHAPE* aGraphic )
{
    if( !aMgr.IsReset() )
    {
        aGraphic->SetStart( (wxPoint) aMgr.GetOrigin() );
        aGraphic->SetEnd( (wxPoint) aMgr.GetEnd() );
    }
}


bool DRAWING_TOOL::drawSegment( const std::string& aTool, PCB_SHAPE** aGraphic,
                                OPT<VECTOR2D> aStartingPoint )
{
    PCB_SHAPE_TYPE shape = ( *aGraphic )->GetShape();
    // Only three shapes are currently supported
    wxASSERT( shape == PCB_SHAPE_TYPE::SEGMENT || shape == PCB_SHAPE_TYPE::CIRCLE
              || shape == PCB_SHAPE_TYPE::RECT );

    EDA_UNITS        userUnits = m_frame->GetUserUnits();
    PCB_GRID_HELPER  grid( m_toolMgr, m_frame->GetMagneticItemsSettings() );
    PCB_SHAPE*&      graphic = *aGraphic;
    PCB_LAYER_ID     drawingLayer = m_frame->GetActiveLayer();

    m_lineWidth = getSegmentWidth( drawingLayer );

    // geometric construction manager
    KIGFX::PREVIEW::TWO_POINT_GEOMETRY_MANAGER twoPointManager;

    // drawing assistant overlay
    // TODO: workaround because PCB_SHAPE_TYPE_T is not visible from commons.
    KIGFX::PREVIEW::GEOM_SHAPE geomShape( static_cast<KIGFX::PREVIEW::GEOM_SHAPE>( shape ) );
    KIGFX::PREVIEW::TWO_POINT_ASSISTANT twoPointAsst( twoPointManager, userUnits, geomShape );

    // Add a VIEW_GROUP that serves as a preview for the new item
    PCB_SELECTION preview;
    m_view->Add( &preview );
    m_view->Add( &twoPointAsst );

    m_controls->ShowCursor( true );

    bool     started = false;
    bool     cancelled = false;
    bool     isLocalOriginSet = ( m_frame->GetScreen()->m_LocalOrigin != VECTOR2D( 0, 0 ) );
    VECTOR2I cursorPos = m_controls->GetMousePosition();

    auto setCursor =
            [&]()
            {
                m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::PENCIL );
            };

    auto cleanup =
            [&]()
            {
                preview.Clear();
                m_view->Update( &preview );
                delete graphic;
                graphic = nullptr;

                if( !isLocalOriginSet )
                    m_frame->GetScreen()->m_LocalOrigin = VECTOR2D( 0, 0 );
            };

    // Prime the pump
    m_toolMgr->RunAction( ACTIONS::refreshPreview );

    if( aStartingPoint )
        m_toolMgr->RunAction( ACTIONS::cursorClick );

    // Set initial cursor
    setCursor();

    // Main loop: keep receiving events
    while( TOOL_EVENT* evt = Wait() )
    {
        setCursor();

        if( started )
            m_frame->SetMsgPanel( graphic );

        grid.SetSnap( !evt->Modifier( MD_SHIFT ) );
        grid.SetUseGrid( getView()->GetGAL()->GetGridSnapping() && !evt->DisableGridSnapping() );
        cursorPos = grid.BestSnapAnchor( m_controls->GetMousePosition(), drawingLayer );
        m_controls->ForceCursorPosition( true, cursorPos );

        // 45 degree angle constraint enabled with an option and toggled with Ctrl
        bool limit45 = frame()->Settings().m_Use45DegreeGraphicSegments;

        if( evt->Modifier( MD_SHIFT ) )
            limit45 = !limit45;

        if( evt->IsCancelInteractive() )
        {
            cleanup();

            if( !started )
            {
                // We've handled the cancel event.  Don't cancel other tools
                evt->SetPassEvent( false );
                m_frame->PopTool( aTool );
                cancelled = true;
            }

            break;
        }
        else if( evt->IsActivate() )
        {
            if( evt->IsPointEditor() )
            {
                // don't exit (the point editor runs in the background)
            }
            else if( evt->IsMoveTool() )
            {
                cleanup();
                // leave ourselves on the stack so we come back after the move
                cancelled = true;
                break;
            }
            else
            {
                cleanup();
                m_frame->PopTool( aTool );
                cancelled = true;
                break;
            }
        }
        else if( evt->IsAction( &PCB_ACTIONS::layerChanged ) )
        {
            drawingLayer = m_frame->GetActiveLayer();
            m_lineWidth = getSegmentWidth( drawingLayer );

            if( graphic )
            {
                if( !m_view->IsLayerVisible( drawingLayer ) )
                {
                    m_frame->GetAppearancePanel()->SetLayerVisible( drawingLayer, true );
                    m_frame->GetCanvas()->Refresh();
                }

                graphic->SetLayer( drawingLayer );
                graphic->SetWidth( m_lineWidth );
                m_view->Update( &preview );
                frame()->SetMsgPanel( graphic );
            }
            else
            {
                evt->SetPassEvent();
            }
        }
        else if( evt->IsAction( &PCB_ACTIONS::properties ) )
        {
            if( started )
            {
                frame()->OnEditItemRequest( graphic );
                m_view->Update( &preview );
                frame()->SetMsgPanel( graphic );
                break;
            }
            else
            {
                evt->SetPassEvent();
            }
        }
        else if( evt->IsClick( BUT_RIGHT ) )
        {
            m_menu.ShowContextMenu( selection() );
        }
        else if( evt->IsClick( BUT_LEFT ) || evt->IsDblClick( BUT_LEFT ) )
        {
            if( !started )
            {
                m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );

                if( aStartingPoint )
                {
                    cursorPos = aStartingPoint.get();
                    aStartingPoint = NULLOPT;
                }

                m_lineWidth = getSegmentWidth( drawingLayer );

                // Init the new item attributes
                graphic->SetShape( static_cast<PCB_SHAPE_TYPE>( shape ) );
                graphic->SetFilled( false );
                graphic->SetWidth( m_lineWidth );
                graphic->SetLayer( drawingLayer );
                grid.SetSkipPoint( cursorPos );

                twoPointManager.SetOrigin( (wxPoint) cursorPos );
                twoPointManager.SetEnd( (wxPoint) cursorPos );

                if( !isLocalOriginSet )
                    m_frame->GetScreen()->m_LocalOrigin = cursorPos;

                preview.Add( graphic );
                frame()->SetMsgPanel( graphic );
                m_controls->SetAutoPan( true );
                m_controls->CaptureCursor( true );

                if( !m_view->IsLayerVisible( drawingLayer ) )
                {
                    m_frame->GetAppearancePanel()->SetLayerVisible( drawingLayer, true );
                    m_frame->GetCanvas()->Refresh();
                }

                updateSegmentFromGeometryMgr( twoPointManager, graphic );

                started = true;
            }
            else if( shape == PCB_SHAPE_TYPE::CIRCLE )
            {
                // No clever logic if drawing a circle
                preview.Clear();
                twoPointManager.Reset();
                break;
            }
            else
            {
                PCB_SHAPE* snapItem = dyn_cast<PCB_SHAPE*>( grid.GetSnapped() );

                if( twoPointManager.GetOrigin() == twoPointManager.GetEnd()
                    || ( evt->IsDblClick( BUT_LEFT ) && shape == PCB_SHAPE_TYPE::SEGMENT )
                    || snapItem )
                // User has clicked twice in the same spot
                //  or clicked on the end of an existing segment (closing a path)
                {
                    BOARD_COMMIT commit( m_frame );

                    // If the user clicks on an existing snap point from a drawsegment
                    //  we finish the segment as they are likely closing a path
                    if( snapItem
                        && ( shape == PCB_SHAPE_TYPE::RECT || graphic->GetLength() > 0.0 ) )
                    {
                        commit.Add( graphic );
                        commit.Push( _( "Draw a line segment" ) );
                        m_toolMgr->RunAction( PCB_ACTIONS::selectItem, true, graphic );
                    }
                    else
                    {
                        delete graphic;
                    }

                    graphic = nullptr;
                }

                preview.Clear();
                twoPointManager.Reset();
                break;
            }

            twoPointManager.SetEnd( cursorPos );
        }
        else if( evt->IsMotion() )
        {
            // 45 degree lines
            if( started
                && ( ( limit45 && shape == PCB_SHAPE_TYPE::SEGMENT )
                     || ( evt->Modifier( MD_SHIFT ) && shape == PCB_SHAPE_TYPE::RECT ) ) )
            {
                const VECTOR2I lineVector( cursorPos - VECTOR2I( twoPointManager.GetOrigin() ) );

                // get a restricted 45/H/V line from the last fixed point to the cursor
                auto newEnd = GetVectorSnapped45( lineVector, ( shape == PCB_SHAPE_TYPE::RECT ) );
                m_controls->ForceCursorPosition( true, VECTOR2I( twoPointManager.GetEnd() ) );
                twoPointManager.SetEnd( twoPointManager.GetOrigin() + (wxPoint) newEnd );
                twoPointManager.SetAngleSnap( true );
            }
            else
            {
                twoPointManager.SetEnd( (wxPoint) cursorPos );
                twoPointManager.SetAngleSnap( false );
            }

            updateSegmentFromGeometryMgr( twoPointManager, graphic );
            m_view->Update( &preview );
            m_view->Update( &twoPointAsst );
        }
        else if( evt->IsAction( &PCB_ACTIONS::incWidth ) )
        {
            m_lineWidth += WIDTH_STEP;
            graphic->SetWidth( m_lineWidth );
            m_view->Update( &preview );
            frame()->SetMsgPanel( graphic );
        }
        else if( evt->IsAction( &PCB_ACTIONS::decWidth ) && ( m_lineWidth > WIDTH_STEP ) )
        {
            m_lineWidth -= WIDTH_STEP;
            graphic->SetWidth( m_lineWidth );
            m_view->Update( &preview );
            frame()->SetMsgPanel( graphic );
        }
        else if( evt->IsAction( &ACTIONS::resetLocalCoords ) )
        {
            isLocalOriginSet = true;
            evt->SetPassEvent();
        }
        else if( evt->IsAction( &ACTIONS::updateUnits ) )
        {
            if( frame()->GetUserUnits() != userUnits )
            {
                userUnits = frame()->GetUserUnits();
                twoPointAsst.SetUnits( userUnits );
                m_view->Update( &twoPointAsst );
            }
            evt->SetPassEvent();
        }
        else
        {
            evt->SetPassEvent();
        }
    }

    if( !isLocalOriginSet ) // reset the relative coordinate if it was not set before
        m_frame->GetScreen()->m_LocalOrigin = VECTOR2D( 0, 0 );

    m_view->Remove( &twoPointAsst );
    m_view->Remove( &preview );
    m_frame->SetMsgPanel( board() );

    m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::ARROW );
    m_controls->SetAutoPan( false );
    m_controls->CaptureCursor( false );
    m_controls->ForceCursorPosition( false );

    return !cancelled;
}


/**
 * Update an arc PCB_SHAPE from the current state
 * of an Arc Geometry Manager
 */
static void updateArcFromConstructionMgr( const KIGFX::PREVIEW::ARC_GEOM_MANAGER& aMgr,
                                          PCB_SHAPE& aArc )
{
    auto vec = aMgr.GetOrigin();

    aArc.SetCenter( { vec.x, vec.y } );

    vec = aMgr.GetStartRadiusEnd();
    aArc.SetArcStart( { vec.x, vec.y } );

    aArc.SetAngle( RAD2DECIDEG( -aMgr.GetSubtended() ) );

    vec = aMgr.GetEndRadiusEnd();
    aArc.SetArcEnd( { vec.x, vec.y } );
}


bool DRAWING_TOOL::drawArc( const std::string& aTool, PCB_SHAPE** aGraphic, bool aImmediateMode )
{
    PCB_SHAPE*&  graphic = *aGraphic;
    PCB_LAYER_ID drawingLayer = m_frame->GetActiveLayer();

    m_lineWidth = getSegmentWidth( drawingLayer );

    // Arc geometric construction manager
    KIGFX::PREVIEW::ARC_GEOM_MANAGER arcManager;

    // Arc drawing assistant overlay
    KIGFX::PREVIEW::ARC_ASSISTANT arcAsst( arcManager, m_frame->GetUserUnits() );

    // Add a VIEW_GROUP that serves as a preview for the new item
    PCB_SELECTION preview;
    m_view->Add( &preview );
    m_view->Add( &arcAsst );
    PCB_GRID_HELPER grid( m_toolMgr, m_frame->GetMagneticItemsSettings() );

    m_controls->ShowCursor( true );

    bool firstPoint = false;
    bool cancelled = false;

    // Set initial cursor
    auto setCursor =
            [&]()
            {
                m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::PENCIL );
            };

    auto cleanup =
            [&] ()
            {
                preview.Clear();
                delete *aGraphic;
                *aGraphic = nullptr;
            };

    // Prime the pump
    m_toolMgr->RunAction( ACTIONS::refreshPreview );

    if( aImmediateMode )
        m_toolMgr->RunAction( ACTIONS::cursorClick );

    setCursor();

    // Main loop: keep receiving events
    while( TOOL_EVENT* evt = Wait() )
    {
        if( firstPoint )
            m_frame->SetMsgPanel( graphic );

        setCursor();

        graphic->SetLayer( drawingLayer );

        grid.SetSnap( !evt->Modifier( MD_SHIFT ) );
        grid.SetUseGrid( getView()->GetGAL()->GetGridSnapping() && !evt->DisableGridSnapping() );
        VECTOR2I cursorPos = grid.BestSnapAnchor( m_controls->GetMousePosition(), graphic );
        m_controls->ForceCursorPosition( true, cursorPos );

        if( evt->IsCancelInteractive() )
        {
            cleanup();

            if( !firstPoint )
            {
                // We've handled the cancel event.  Don't cancel other tools
                evt->SetPassEvent( false );
                m_frame->PopTool( aTool );
                cancelled = true;
            }

            break;
        }
        else if( evt->IsActivate() )
        {
            if( evt->IsPointEditor() )
            {
                // don't exit (the point editor runs in the background)
            }
            else if( evt->IsMoveTool() )
            {
                cleanup();
                // leave ourselves on the stack so we come back after the move
                cancelled = true;
                break;
            }
            else
            {
                cleanup();
                m_frame->PopTool( aTool );
                cancelled = true;
                break;
            }
        }
        else if( evt->IsClick( BUT_LEFT ) )
        {
            if( !firstPoint )
            {
                m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );

                m_controls->SetAutoPan( true );
                m_controls->CaptureCursor( true );

                drawingLayer = m_frame->GetActiveLayer();
                m_lineWidth = getSegmentWidth( drawingLayer );

                // Init the new item attributes
                // (non-geometric, those are handled by the manager)
                graphic->SetShape( PCB_SHAPE_TYPE::ARC );
                graphic->SetWidth( m_lineWidth );

                if( !m_view->IsLayerVisible( drawingLayer ) )
                {
                    m_frame->GetAppearancePanel()->SetLayerVisible( drawingLayer, true );
                    m_frame->GetCanvas()->Refresh();
                }

                preview.Add( graphic );
                frame()->SetMsgPanel( graphic );
                firstPoint = true;
            }

            arcManager.AddPoint( cursorPos, true );
        }
        else if( evt->IsAction( &PCB_ACTIONS::deleteLastPoint ) )
        {
            arcManager.RemoveLastPoint();
        }
        else if( evt->IsMotion() )
        {
            // set angle snap
            arcManager.SetAngleSnap( evt->Modifier( MD_SHIFT ) );

            // update, but don't step the manager state
            arcManager.AddPoint( cursorPos, false );
        }
        else if( evt->IsAction( &PCB_ACTIONS::layerChanged ) )
        {
            drawingLayer = m_frame->GetActiveLayer();
            m_lineWidth = getSegmentWidth( drawingLayer );

            if( graphic )
            {
                if( !m_view->IsLayerVisible( drawingLayer ) )
                {
                    m_frame->GetAppearancePanel()->SetLayerVisible( drawingLayer, true );
                    m_frame->GetCanvas()->Refresh();
                }

                graphic->SetLayer( drawingLayer );
                graphic->SetWidth( m_lineWidth );
                m_view->Update( &preview );
                frame()->SetMsgPanel( graphic );
            }
            else
            {
                evt->SetPassEvent();
            }
        }
        else if( evt->IsAction( &PCB_ACTIONS::properties ) )
        {
            if( arcManager.GetStep() == KIGFX::PREVIEW::ARC_GEOM_MANAGER::SET_START )
            {
                graphic->SetAngle( 900, true );
                frame()->OnEditItemRequest( graphic );
                m_view->Update( &preview );
                frame()->SetMsgPanel( graphic );
                break;
            }
            else if( arcManager.GetStep() == KIGFX::PREVIEW::ARC_GEOM_MANAGER::SET_ANGLE )
            {
                frame()->OnEditItemRequest( graphic );
                m_view->Update( &preview );
                frame()->SetMsgPanel( graphic );
                break;
            }
            else
            {
                evt->SetPassEvent();
            }
        }
        else if( evt->IsClick( BUT_RIGHT ) )
        {
            m_menu.ShowContextMenu( selection() );
        }
        else if( evt->IsAction( &PCB_ACTIONS::incWidth ) )
        {
            m_lineWidth += WIDTH_STEP;
            graphic->SetWidth( m_lineWidth );
            m_view->Update( &preview );
            frame()->SetMsgPanel( graphic );
        }
        else if( evt->IsAction( &PCB_ACTIONS::decWidth ) && m_lineWidth > WIDTH_STEP )
        {
            m_lineWidth -= WIDTH_STEP;
            graphic->SetWidth( m_lineWidth );
            m_view->Update( &preview );
            frame()->SetMsgPanel( graphic );
        }
        else if( evt->IsAction( &PCB_ACTIONS::arcPosture ) )
        {
            arcManager.ToggleClockwise();
        }
        else if( evt->IsAction( &ACTIONS::updateUnits ) )
        {
            arcAsst.SetUnits( frame()->GetUserUnits() );
            m_view->Update( &arcAsst );
            evt->SetPassEvent();
        }
        else
        {
            evt->SetPassEvent();
        }

        if( arcManager.IsComplete() )
        {
            break;
        }
        else if( arcManager.HasGeometryChanged() )
        {
            updateArcFromConstructionMgr( arcManager, *graphic );
            m_view->Update( &preview );
            m_view->Update( &arcAsst );

            if( firstPoint )
                frame()->SetMsgPanel( graphic );
            else
                frame()->SetMsgPanel( board() );
        }
    }

    preview.Remove( graphic );
    m_view->Remove( &arcAsst );
    m_view->Remove( &preview );
    m_frame->SetMsgPanel( board() );

    m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::ARROW );
    m_controls->SetAutoPan( false );
    m_controls->CaptureCursor( false );
    m_controls->ForceCursorPosition( false );

    return !cancelled;
}


bool DRAWING_TOOL::getSourceZoneForAction( ZONE_MODE aMode, ZONE** aZone )
{
    bool clearSelection = false;
    *aZone = nullptr;

    // not an action that needs a source zone
    if( aMode == ZONE_MODE::ADD || aMode == ZONE_MODE::GRAPHIC_POLYGON )
        return true;

    PCB_SELECTION_TOOL*  selTool = m_toolMgr->GetTool<PCB_SELECTION_TOOL>();
    const PCB_SELECTION& selection = selTool->GetSelection();

    if( selection.Empty() )
    {
        clearSelection = true;
        m_toolMgr->RunAction( PCB_ACTIONS::selectionCursor, true );
    }

    // we want a single zone
    if( selection.Size() == 1 )
        *aZone = dyn_cast<ZONE*>( selection[0] );

    // expected a zone, but didn't get one
    if( !*aZone )
    {
        if( clearSelection )
            m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );

        return false;
    }

    return true;
}

int DRAWING_TOOL::DrawZone( const TOOL_EVENT& aEvent )
{
    if( m_isFootprintEditor && !m_frame->GetModel() )
        return 0;

    ZONE_MODE zoneMode = aEvent.Parameter<ZONE_MODE>();
    MODE      drawMode = MODE::ZONE;

    if( aEvent.IsAction( &PCB_ACTIONS::drawRuleArea ) )
        drawMode = MODE::KEEPOUT;

    if( aEvent.IsAction( &PCB_ACTIONS::drawPolygon ) )
        drawMode = MODE::GRAPHIC_POLYGON;

    SCOPED_DRAW_MODE scopedDrawMode( m_mode, drawMode );

    // get a source zone, if we need one. We need it for:
    // ZONE_MODE::CUTOUT (adding a hole to the source zone)
    // ZONE_MODE::SIMILAR (creating a new zone using settings of source zone
    ZONE* sourceZone = nullptr;

    if( !getSourceZoneForAction( zoneMode, &sourceZone ) )
        return 0;

    // Turn zones on if they are off, so that the created object will be visible after completion
    m_frame->SetObjectVisible( LAYER_ZONES );

    ZONE_CREATE_HELPER::PARAMS params;

    params.m_keepout = drawMode == MODE::KEEPOUT;
    params.m_mode = zoneMode;
    params.m_sourceZone = sourceZone;

    if( zoneMode == ZONE_MODE::SIMILAR )
        params.m_layer = sourceZone->GetLayer();
    else
        params.m_layer = m_frame->GetActiveLayer();

    ZONE_CREATE_HELPER zoneTool( *this, params );

    // the geometry manager which handles the zone geometry, and
    // hands the calculated points over to the zone creator tool
    POLYGON_GEOM_MANAGER polyGeomMgr( zoneTool );
    bool constrainAngle = false;

    std::string tool = aEvent.GetCommandStr().get();
    m_frame->PushTool( tool );
    Activate();    // register for events

    m_controls->ShowCursor( true );

    bool    started     = false;
    PCB_GRID_HELPER grid( m_toolMgr, m_frame->GetMagneticItemsSettings() );
    STATUS_TEXT_POPUP status( m_frame );
    status.SetTextColor( wxColour( 255, 0, 0 ) );
    status.SetText( _( "Self-intersecting polygons are not allowed" ) );

    auto setCursor =
            [&]()
            {
                m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::PENCIL );
            };

    auto cleanup =
            [&] ()
            {
                polyGeomMgr.Reset();
                started = false;
                grid.ClearSkipPoint();
                m_controls->SetAutoPan( false );
                m_controls->CaptureCursor( false );
            };

    // Prime the pump
    if( aEvent.HasPosition() )
        m_toolMgr->PrimeTool( aEvent.Position() );

    // Set initial cursor
    setCursor();

    // Main loop: keep receiving events
    while( TOOL_EVENT* evt = Wait() )
    {
        setCursor();

        LSET layers( m_frame->GetActiveLayer() );
        grid.SetSnap( !evt->Modifier( MD_SHIFT ) );
        grid.SetUseGrid( getView()->GetGAL()->GetGridSnapping() && !evt->DisableGridSnapping() );

        VECTOR2I cursorPos = evt->HasPosition() ? evt->Position() : m_controls->GetMousePosition();
        cursorPos          = grid.BestSnapAnchor( cursorPos, layers );

        m_controls->ForceCursorPosition( true, cursorPos );

        if( ( sourceZone && sourceZone->GetHV45() ) || constrainAngle || evt->Modifier( MD_SHIFT ) )
            polyGeomMgr.SetLeaderMode( POLYGON_GEOM_MANAGER::LEADER_MODE::DEG45 );
        else
            polyGeomMgr.SetLeaderMode( POLYGON_GEOM_MANAGER::LEADER_MODE::DIRECT );

        if( evt->IsCancelInteractive() )
        {
            if( polyGeomMgr.IsPolygonInProgress() )
            {
                cleanup();
            }
            else
            {
                // We've handled the cancel event.  Don't cancel other tools
                evt->SetPassEvent( false );
                m_frame->PopTool( tool );
                break;
            }
        }
        else if( evt->IsActivate() )
        {
            if( polyGeomMgr.IsPolygonInProgress() )
                cleanup();

            if( evt->IsPointEditor() )
            {
                // don't exit (the point editor runs in the background)
            }
            else if( evt->IsMoveTool() )
            {
                // leave ourselves on the stack so we come back after the move
                break;
            }
            else
            {
                m_frame->PopTool( tool );
                break;
            }
        }
        else if( evt->IsAction( &PCB_ACTIONS::layerChanged ) )
        {
            if( zoneMode != ZONE_MODE::SIMILAR )
                params.m_layer = frame()->GetActiveLayer();

            if( !m_view->IsLayerVisible( params.m_layer ) )
            {
                m_frame->GetAppearancePanel()->SetLayerVisible( params.m_layer, true );
                m_frame->GetCanvas()->Refresh();
            }
        }
        else if( evt->IsClick( BUT_RIGHT ) )
        {
            m_menu.ShowContextMenu( selection() );
        }
        // events that lock in nodes
        else if( evt->IsClick( BUT_LEFT )
                 || evt->IsDblClick( BUT_LEFT )
                 || evt->IsAction( &PCB_ACTIONS::closeOutline ) )
        {
            // Check if it is double click / closing line (so we have to finish the zone)
            const bool endPolygon = evt->IsDblClick( BUT_LEFT )
                                    || evt->IsAction( &PCB_ACTIONS::closeOutline )
                                    || polyGeomMgr.NewPointClosesOutline( cursorPos );

            if( endPolygon )
            {
                polyGeomMgr.SetFinished();
                polyGeomMgr.Reset();

                started = false;
                m_controls->SetAutoPan( false );
                m_controls->CaptureCursor( false );
            }
            // adding a corner
            else if( polyGeomMgr.AddPoint( cursorPos ) )
            {
                if( !started )
                {
                    started = true;

                    POLYGON_GEOM_MANAGER::LEADER_MODE leaderMode = polyGeomMgr.GetLeaderMode();
                    constrainAngle = ( leaderMode == POLYGON_GEOM_MANAGER::LEADER_MODE::DEG45 );

                    m_controls->SetAutoPan( true );
                    m_controls->CaptureCursor( true );

                    if( !m_view->IsLayerVisible( params.m_layer ) )
                    {
                        m_frame->GetAppearancePanel()->SetLayerVisible( params.m_layer, true );
                        m_frame->GetCanvas()->Refresh();
                    }
                }
            }
        }
        else if( evt->IsAction( &PCB_ACTIONS::deleteLastPoint ) )
        {
            polyGeomMgr.DeleteLastCorner();

            if( !polyGeomMgr.IsPolygonInProgress() )
            {
                // report finished as an empty shape
                polyGeomMgr.SetFinished();

                // start again
                started = false;
                m_controls->SetAutoPan( false );
                m_controls->CaptureCursor( false );
            }
        }
        else if( polyGeomMgr.IsPolygonInProgress()
                 && ( evt->IsMotion() || evt->IsDrag( BUT_LEFT ) ) )
        {
            polyGeomMgr.SetCursorPosition( cursorPos );

            if( polyGeomMgr.IsSelfIntersecting( true ) )
            {
                wxPoint p = wxGetMousePosition() + wxPoint( 20, 20 );
                status.Move( p );
                status.PopupFor( 1500 );
            }
            else
            {
                status.Hide();
            }
        }
        else if( evt->IsAction( &PCB_ACTIONS::properties ) )
        {
            if( started )
            {
                frame()->OnEditItemRequest( zoneTool.GetZone() );
                zoneTool.OnGeometryChange( polyGeomMgr );
                frame()->SetMsgPanel( zoneTool.GetZone() );
            }
            else
            {
                evt->SetPassEvent();
            }
        }
        /*else if( evt->IsAction( &ACTIONS::updateUnits ) )
        {
            // If we ever have an assistant here that reports dimensions, we'll want to
            // update its units here....
            // zoneAsst.SetUnits( frame()->GetUserUnits() );
            // m_view->Update( &zoneAsst );
            evt->SetPassEvent();
        }*/
        else
        {
            evt->SetPassEvent();
        }

    }    // end while

    m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::ARROW );
    m_controls->ForceCursorPosition( false );
    controls()->SetAutoPan( false );
    m_controls->CaptureCursor( false );
    return 0;
}


int DRAWING_TOOL::DrawVia( const TOOL_EVENT& aEvent )
{
    struct VIA_PLACER : public INTERACTIVE_PLACER_BASE
    {
        PCB_BASE_EDIT_FRAME*        m_frame;
        PCB_GRID_HELPER             m_gridHelper;
        std::shared_ptr<DRC_ENGINE> m_drcEngine;
        int                         m_drcEpsilon;
        int                         m_worstClearance;
        bool                        m_allowDRCViolations;
        bool                        m_flaggedDRC;

        VIA_PLACER( PCB_BASE_EDIT_FRAME* aFrame ) :
            m_frame( aFrame ),
            m_gridHelper( aFrame->GetToolManager(), aFrame->GetMagneticItemsSettings() ),
            m_drcEngine( aFrame->GetBoard()->GetDesignSettings().m_DRCEngine ),
            m_drcEpsilon( aFrame->GetBoard()->GetDesignSettings().GetDRCEpsilon() ),
            m_worstClearance( 0 ),
            m_flaggedDRC( false )
        {
            ROUTER_TOOL* router = m_frame->GetToolManager()->GetTool<ROUTER_TOOL>();

            m_allowDRCViolations = router->Router()->Settings().AllowDRCViolations();

            try
            {
                m_drcEngine->InitEngine( aFrame->GetDesignRulesPath() );

                DRC_CONSTRAINT constraint;

                if( m_drcEngine->QueryWorstConstraint( CLEARANCE_CONSTRAINT, constraint ) )
                    m_worstClearance = constraint.GetValue().Min();

                if( m_drcEngine->QueryWorstConstraint( HOLE_CLEARANCE_CONSTRAINT, constraint ) )
                    m_worstClearance = std::max( m_worstClearance, constraint.GetValue().Min() );

                for( FOOTPRINT* footprint : aFrame->GetBoard()->Footprints() )
                {
                    for( PAD* pad : footprint->Pads() )
                        m_worstClearance = std::max( m_worstClearance, pad->GetLocalClearance() );
                }
            }
            catch( PARSE_ERROR& )
            {
            }
        }

        virtual ~VIA_PLACER()
        {
        }

        TRACK* findTrack( VIA* aVia )
        {
            const LSET lset = aVia->GetLayerSet();
            wxPoint position = aVia->GetPosition();
            BOX2I bbox = aVia->GetBoundingBox();

            std::vector<KIGFX::VIEW::LAYER_ITEM_PAIR> items;
            auto view = m_frame->GetCanvas()->GetView();
            std::vector<TRACK*> possible_tracks;

            view->Query( bbox, items );

            for( auto it : items )
            {
                BOARD_ITEM* item = static_cast<BOARD_ITEM*>( it.first );

                if( !(item->GetLayerSet() & lset ).any() )
                    continue;

                if( TRACK* track = dyn_cast<TRACK*>( item ) )
                {
                    if( TestSegmentHit( position, track->GetStart(), track->GetEnd(),
                                        ( track->GetWidth() + aVia->GetWidth() ) / 2 ) )
                        possible_tracks.push_back( track );
                }
            }

            TRACK* return_track = nullptr;
            int min_d = std::numeric_limits<int>::max();

            for( TRACK* track : possible_tracks )
            {
                SEG test( track->GetStart(), track->GetEnd() );
                int dist = ( test.NearestPoint( position ) - position ).EuclideanNorm();

                if( dist < min_d )
                {
                    min_d = dist;
                    return_track = track;
                }
            }

            return return_track;
        }

        bool hasDRCViolation( VIA* aVia, BOARD_ITEM* aOther )
        {
            // It would really be better to know what particular nets a nettie should allow,
            // but for now it is what it is.
            if( DRC_ENGINE::IsNetTie( aOther ) )
                return false;

            BOARD_CONNECTED_ITEM* cItem = dynamic_cast<BOARD_CONNECTED_ITEM*>( aOther );

            if( cItem && cItem->GetNetCode() == aVia->GetNetCode() )
                return false;

            DRC_CONSTRAINT constraint;
            int            clearance;

            for( PCB_LAYER_ID layer : aOther->GetLayerSet().Seq() )
            {
                if( !IsCopperLayer( layer ) )
                    continue;

                constraint = m_drcEngine->EvalRules( CLEARANCE_CONSTRAINT, aVia,  aOther, layer );
                clearance = constraint.GetValue().Min();

                if( clearance >= 0 )
                {
                    std::shared_ptr<SHAPE> viaShape = DRC_ENGINE::GetShape( aVia, layer );
                    std::shared_ptr<SHAPE> otherShape = DRC_ENGINE::GetShape( aOther, layer );

                    if( viaShape->Collide( otherShape.get(), clearance - m_drcEpsilon ) )
                        return true;
                }
            }

            std::unique_ptr<SHAPE_SEGMENT> holeShape;

            if( aOther->Type() == PCB_VIA_T )
            {
                VIA* via = static_cast<VIA*>( aOther );
                wxPoint pos = via->GetPosition();

                holeShape.reset( new SHAPE_SEGMENT( pos, pos, via->GetDrill() ) );
            }
            else if( aOther->Type() == PCB_PAD_T )
            {
                PAD* pad = static_cast<PAD*>( aOther );

                if( pad->GetDrillSize().x )
                    holeShape.reset( new SHAPE_SEGMENT( *pad->GetEffectiveHoleShape() ) );
            }

            if( holeShape )
            {
                constraint = m_drcEngine->EvalRules( HOLE_CLEARANCE_CONSTRAINT, aVia, aOther,
                                                     UNDEFINED_LAYER );
                clearance = constraint.GetValue().Min();

                if( clearance >= 0 )
                {
                    std::shared_ptr<SHAPE> viaShape = DRC_ENGINE::GetShape( aVia, UNDEFINED_LAYER );

                    if( viaShape->Collide( holeShape.get(), clearance - m_drcEpsilon ) )
                        return true;
                }
            }

            return false;
        }

        bool checkDRCViolation( VIA* aVia )
        {
            std::vector<KIGFX::VIEW::LAYER_ITEM_PAIR> items;
            std::set<BOARD_ITEM*> checkedItems;
            BOX2I bbox = aVia->GetBoundingBox();

            bbox.Inflate( m_worstClearance );
            m_frame->GetCanvas()->GetView()->Query( bbox, items );

            for( std::pair<KIGFX::VIEW_ITEM*, int> it : items )
            {
                BOARD_ITEM* item = dynamic_cast<BOARD_ITEM*>( it.first );

                if( !item )
                    continue;

                if( item->Type() == PCB_ZONE_T || item->Type() == PCB_FP_ZONE_T )
                    continue;       // stitching vias bind to zones, so ignore them

                if( item->Type() == PCB_FOOTPRINT_T || item->Type() == PCB_GROUP_T )
                    continue;       // check against children, but not against footprint itself

                if( item->Type() == PCB_FP_TEXT_T && !static_cast<FP_TEXT*>( item )->IsVisible() )
                    continue;       // ignore hidden items

                if( checkedItems.count( item ) )
                    continue;

                if( hasDRCViolation( aVia, item ) )
                    return true;

                checkedItems.insert( item );
            }

            return false;
        }

        PAD* findPad( VIA* aVia )
        {
            const wxPoint position = aVia->GetPosition();
            const LSET    lset = aVia->GetLayerSet();

            for( FOOTPRINT* fp : m_board->Footprints() )
            {
                for(PAD* pad : fp->Pads() )
                {
                    if( pad->HitTest( position ) && ( pad->GetLayerSet() & lset ).any() )
                        if( pad->GetNetCode() > 0 )
                            return pad;
                }
            }

            return nullptr;
        }

        int findStitchedZoneNet( VIA* aVia )
        {
            const wxPoint position = aVia->GetPosition();
            const LSET    lset = aVia->GetLayerSet();

            std::vector<ZONE*> foundZones;

            for( ZONE* zone : m_board->Zones() )
            {
                for( PCB_LAYER_ID layer : LSET( zone->GetLayerSet() & lset ).Seq() )
                {
                    if( zone->HitTestFilledArea( layer, position ) )
                        foundZones.push_back( zone );
                }
            }

            std::sort( foundZones.begin(), foundZones.end(),
                    [] ( const ZONE* a, const ZONE* b )
                    {
                        return a->GetLayer() < b->GetLayer();
                    } );

            // first take the net of the active layer
            for( ZONE* z : foundZones )
            {
                if( m_frame->GetActiveLayer() == z->GetLayer() )
                    return z->GetNetCode();
            }

            // none? take the topmost visible layer
            for( ZONE* z : foundZones )
            {
                if( m_board->IsLayerVisible( z->GetLayer() ) )
                    return z->GetNetCode();
            }

            return -1;
        }

        void SnapItem( BOARD_ITEM *aItem ) override
        {
            // If you place a Via on a track but not on its centerline, the current
            // connectivity algorithm will require us to put a kink in the track when
            // we break it (so that each of the two segments ends on the via center).
            // That's not ideal, and is in fact probably worse than forcing snap in
            // this situation.

            m_gridHelper.SetSnap( !( m_modifiers & MD_SHIFT ) );
            auto    via = static_cast<VIA*>( aItem );
            wxPoint position = via->GetPosition();
            TRACK*  track = findTrack( via );

            if( track )
            {
                SEG      trackSeg( track->GetStart(), track->GetEnd() );
                VECTOR2I snap = m_gridHelper.AlignToSegment( position, trackSeg );

                aItem->SetPosition( (wxPoint) snap );
            }
        }

        bool PlaceItem( BOARD_ITEM* aItem, BOARD_COMMIT& aCommit ) override
        {
            VIA*    via = static_cast<VIA*>( aItem );
            wxPoint viaPos = via->GetPosition();
            TRACK*  track = findTrack( via );
            PAD *   pad = findPad( via );

            if( track )
                via->SetNetCode( track->GetNetCode() );
            else if( pad )
                via->SetNetCode( pad->GetNetCode() );

            if( !m_allowDRCViolations && checkDRCViolation( via ) )
            {
                m_frame->ShowInfoBarError( _( "Via location violates DRC." ) );
                via->SetNetCode( 0 );
                m_flaggedDRC = true;
                return false;
            }
            else if( m_flaggedDRC )
            {
                m_frame->GetInfoBar()->Dismiss();
            }

            if( track )
            {
                if( viaPos != track->GetStart() && viaPos != track->GetEnd() )
                {
                    aCommit.Modify( track );

                    TRACK* newTrack = dynamic_cast<TRACK*>( track->Clone() );
                    const_cast<KIID&>( newTrack->m_Uuid ) = KIID();

                    track->SetEnd( viaPos );
                    newTrack->SetStart( viaPos );
                    aCommit.Add( newTrack );
                }
            }
            else if( !pad )
            {
                via->SetNetCode( findStitchedZoneNet( via ) );
                via->SetIsFree();
            }

            aCommit.Add( aItem );
            return true;
        }

        std::unique_ptr<BOARD_ITEM> CreateItem() override
        {
            auto&   ds = m_board->GetDesignSettings();
            VIA*    via = new VIA( m_board );

            via->SetNetCode( 0 );
            via->SetViaType( ds.m_CurrentViaType );

            // for microvias, the size and hole will be changed later.
            via->SetWidth( ds.GetCurrentViaSize() );
            via->SetDrill( ds.GetCurrentViaDrill() );

            // Usual via is from copper to component.
            // layer pair is B_Cu and F_Cu.
            via->SetLayerPair( B_Cu, F_Cu );

            PCB_LAYER_ID    first_layer = m_frame->GetActiveLayer();
            PCB_LAYER_ID    last_layer;

            // prepare switch to new active layer:
            if( first_layer != m_frame->GetScreen()->m_Route_Layer_TOP )
                last_layer = m_frame->GetScreen()->m_Route_Layer_TOP;
            else
                last_layer = m_frame->GetScreen()->m_Route_Layer_BOTTOM;

            // Adjust the actual via layer pair
            switch( via->GetViaType() )
            {
            case VIATYPE::BLIND_BURIED:
                via->SetLayerPair( first_layer, last_layer );
                break;

            case VIATYPE::MICROVIA: // from external to the near neighbor inner layer
            {
                PCB_LAYER_ID last_inner_layer =
                    ToLAYER_ID( ( m_board->GetCopperLayerCount() - 2 ) );

                if( first_layer == B_Cu )
                    last_layer = last_inner_layer;
                else if( first_layer == F_Cu )
                    last_layer = In1_Cu;
                else if( first_layer == last_inner_layer )
                    last_layer = B_Cu;
                else if( first_layer == In1_Cu )
                    last_layer = F_Cu;

                // else error: will be removed later
                via->SetLayerPair( first_layer, last_layer );

                // Update diameter and hole size, which where set previously for normal vias
                NETCLASS* netClass = via->GetNetClass();

                via->SetWidth( netClass->GetuViaDiameter() );
                via->SetDrill( netClass->GetuViaDrill() );
            }
            break;

            default:
                break;
            }

            return std::unique_ptr<BOARD_ITEM>( via );
        }
    };

    VIA_PLACER placer( frame() );

    SCOPED_DRAW_MODE scopedDrawMode( m_mode, MODE::VIA );

    doInteractiveItemPlacement( aEvent.GetCommandStr().get(), &placer, _( "Place via" ),
                                IPO_REPEAT | IPO_SINGLE_CLICK );

    return 0;
}


int DRAWING_TOOL::getSegmentWidth( PCB_LAYER_ID aLayer ) const
{
    assert( m_board );
    return m_board->GetDesignSettings().GetLineThickness( aLayer );
}


const unsigned int DRAWING_TOOL::WIDTH_STEP = Millimeter2iu( 0.1 );


void DRAWING_TOOL::setTransitions()
{

    Go( &DRAWING_TOOL::PlaceStackup,          PCB_ACTIONS::placeStackup.MakeEvent() );
    Go( &DRAWING_TOOL::PlaceCharacteristics,  PCB_ACTIONS::placeCharacteristics.MakeEvent() );
    Go( &DRAWING_TOOL::DrawLine,              PCB_ACTIONS::drawLine.MakeEvent() );
    Go( &DRAWING_TOOL::DrawZone,              PCB_ACTIONS::drawPolygon.MakeEvent() );
    Go( &DRAWING_TOOL::DrawRectangle,         PCB_ACTIONS::drawRectangle.MakeEvent() );
    Go( &DRAWING_TOOL::DrawCircle,            PCB_ACTIONS::drawCircle.MakeEvent() );
    Go( &DRAWING_TOOL::DrawArc,               PCB_ACTIONS::drawArc.MakeEvent() );
    Go( &DRAWING_TOOL::DrawDimension,         PCB_ACTIONS::drawAlignedDimension.MakeEvent() );
    Go( &DRAWING_TOOL::DrawDimension,         PCB_ACTIONS::drawOrthogonalDimension.MakeEvent() );
    Go( &DRAWING_TOOL::DrawDimension,         PCB_ACTIONS::drawCenterDimension.MakeEvent() );
    Go( &DRAWING_TOOL::DrawDimension,         PCB_ACTIONS::drawLeader.MakeEvent() );
    Go( &DRAWING_TOOL::DrawZone,              PCB_ACTIONS::drawZone.MakeEvent() );
    Go( &DRAWING_TOOL::DrawZone,              PCB_ACTIONS::drawRuleArea.MakeEvent() );
    Go( &DRAWING_TOOL::DrawZone,              PCB_ACTIONS::drawZoneCutout.MakeEvent() );
    Go( &DRAWING_TOOL::DrawZone,              PCB_ACTIONS::drawSimilarZone.MakeEvent() );
    Go( &DRAWING_TOOL::DrawVia,               PCB_ACTIONS::drawVia.MakeEvent() );
    Go( &DRAWING_TOOL::PlaceText,             PCB_ACTIONS::placeText.MakeEvent() );
    Go( &DRAWING_TOOL::PlaceImportedGraphics, PCB_ACTIONS::placeImportedGraphics.MakeEvent() );
    Go( &DRAWING_TOOL::SetAnchor,             PCB_ACTIONS::setAnchor.MakeEvent() );
    Go( &DRAWING_TOOL::ToggleLine45degMode,   PCB_ACTIONS::toggleLine45degMode.MakeEvent() );
}
