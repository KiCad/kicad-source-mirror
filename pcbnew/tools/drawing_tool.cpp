/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014-2017 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
#include "geometry/shape_rect.h"
#include "dialog_table_properties.h"

#include <pgm_base.h>
#include <settings/settings_manager.h>
#include <pcbnew_settings.h>
#include <footprint_editor_settings.h>
#include <dialogs/dialog_barcode_properties.h>
#include <dialogs/dialog_text_properties.h>
#include <dialogs/dialog_track_via_size.h>
#include <gal/graphics_abstraction_layer.h>
#include <geometry/geometry_utils.h>
#include <geometry/shape_segment.h>
#include <import_gfx/dialog_import_graphics.h>
#include <preview_items/arc_assistant.h>
#include <preview_items/bezier_assistant.h>
#include <preview_items/two_point_assistant.h>
#include <preview_items/two_point_geom_manager.h>
#include <ratsnest/ratsnest_data.h>
#include <router/router_tool.h>
#include <status_popup.h>
#include <tool/tool_manager.h>
#include <tools/pcb_actions.h>
#include <tools/pcb_grid_helper.h>
#include <tools/pcb_selection_tool.h>
#include <pcb_barcode.h>
#include <tools/tool_event_utils.h>
#include <tools/zone_create_helper.h>
#include <tools/zone_filler_tool.h>
#include <view/view.h>
#include <widgets/appearance_controls.h>
#include <widgets/wx_infobar.h>
#include <wx/filedlg.h>
#include <wx/msgdlg.h>

#include <bitmaps.h>
#include <board.h>
#include <board_commit.h>
#include <board_design_settings.h>
#include <confirm.h>
#include <footprint.h>
#include <macros.h>
#include <gal/painter.h>
#include <pcb_edit_frame.h>
#include <pcb_group.h>
#include <pcb_point.h>
#include <pcb_reference_image.h>
#include <pcb_text.h>
#include <pcb_textbox.h>
#include <pcb_table.h>
#include <pcb_tablecell.h>
#include <pcb_dimension.h>
#include <pcbnew_id.h>
#include <scoped_set_reset.h>
#include <string_utils.h>
#include <zone.h>
#include <fix_board_shape.h>
#include <view/view_controls.h>

const unsigned int DRAWING_TOOL::COORDS_PADDING = pcbIUScale.mmToIU( 20 );

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
        BOARD_DESIGN_SETTINGS& bds = frame->GetBoard()->GetDesignSettings();
        bool                   useIndex = !bds.m_UseConnectedTrackWidth && !bds.UseCustomTrackViaSize();
        wxString               msg;

        Clear();

        Append( ID_POPUP_PCB_SELECT_CUSTOM_WIDTH, _( "Use Custom Values..." ),
                _( "Specify custom track and via sizes" ), wxITEM_CHECK );
        Check( ID_POPUP_PCB_SELECT_CUSTOM_WIDTH, bds.UseCustomTrackViaSize() );

        AppendSeparator();

        for( int i = 1; i < (int) bds.m_ViasDimensionsList.size(); i++ )
        {
            VIA_DIMENSION via = bds.m_ViasDimensionsList[i];

            if( via.m_Drill > 0 )
            {
                msg.Printf( _("Via %s, hole %s" ),
                            frame->MessageTextFromValue( via.m_Diameter ),
                            frame->MessageTextFromValue( via.m_Drill ) );
            }
            else
            {
                msg.Printf( _( "Via %s" ),
                            frame->MessageTextFromValue( via.m_Diameter ) );
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
        m_inDrawingTool( false ),
        m_layer( UNDEFINED_LAYER ),
        m_stroke( 1, LINE_STYLE::DEFAULT, COLOR4D::UNSPECIFIED ),
        m_pickerItem( nullptr ),
        m_tuningPattern( nullptr )
{
}


DRAWING_TOOL::~DRAWING_TOOL()
{
}


bool DRAWING_TOOL::Init()
{
    auto haveHighlight =
            [this]( const SELECTION& sel )
            {
                KIGFX::RENDER_SETTINGS* cfg = m_toolMgr->GetView()->GetPainter()->GetSettings();

                return !cfg->GetHighlightNetCodes().empty();
            };

    auto activeToolFunctor =
            [this]( const SELECTION& aSel )
            {
                return m_mode != MODE::NONE;
            };

    // some interactive drawing tools can undo the last point
    auto canUndoPoint =
            [this]( const SELECTION& aSel )
            {
                return (   m_mode == MODE::ARC
                        || m_mode == MODE::ZONE
                        || m_mode == MODE::KEEPOUT
                        || m_mode == MODE::GRAPHIC_POLYGON
                        || m_mode == MODE::BEZIER
                        || m_mode == MODE::LINE );
            };

    // functor for tools that can automatically close the outline
    auto canCloseOutline =
            [this]( const SELECTION& aSel )
            {
                 return (   m_mode == MODE::ZONE
                         || m_mode == MODE::KEEPOUT
                         || m_mode == MODE::GRAPHIC_POLYGON );
            };

    auto arcToolActive =
            [this]( const SELECTION& aSel )
            {
                return m_mode == MODE::ARC;
            };

    auto viaToolActive =
            [this]( const SELECTION& aSel )
            {
                return m_mode == MODE::VIA;
            };

    auto tuningToolActive =
            [this]( const SELECTION& aSel )
            {
                return m_mode == MODE::TUNING;
            };

    auto dimensionToolActive =
            [this]( const SELECTION& aSel )
            {
                return m_mode == MODE::DIMENSION;
            };

    CONDITIONAL_MENU& ctxMenu = m_menu->GetMenu();

    // cancel current tool goes in main context menu at the top if present
    ctxMenu.AddItem( ACTIONS::cancelInteractive,         activeToolFunctor, 1 );
    ctxMenu.AddSeparator( 1 );

    ctxMenu.AddItem( PCB_ACTIONS::clearHighlight,        haveHighlight, 2 );
    ctxMenu.AddSeparator(                                haveHighlight, 2 );

    // tool-specific actions
    ctxMenu.AddItem( PCB_ACTIONS::closeOutline,          canCloseOutline, 200 );
    ctxMenu.AddItem( PCB_ACTIONS::deleteLastPoint,       canUndoPoint, 200 );
    ctxMenu.AddItem( PCB_ACTIONS::arcPosture,            arcToolActive, 200 );
    ctxMenu.AddItem( PCB_ACTIONS::spacingIncrease,       tuningToolActive, 200 );
    ctxMenu.AddItem( PCB_ACTIONS::spacingDecrease,       tuningToolActive, 200 );
    ctxMenu.AddItem( PCB_ACTIONS::amplIncrease,          tuningToolActive, 200 );
    ctxMenu.AddItem( PCB_ACTIONS::amplDecrease,          tuningToolActive, 200 );
    ctxMenu.AddItem( PCB_ACTIONS::lengthTunerSettings,   tuningToolActive, 200 );
    ctxMenu.AddItem( PCB_ACTIONS::changeDimensionArrows, dimensionToolActive, 200 );

    ctxMenu.AddSeparator( 500 );

    std::shared_ptr<VIA_SIZE_MENU> viaSizeMenu = std::make_shared<VIA_SIZE_MENU>();
    viaSizeMenu->SetTool( this );
    m_menu->RegisterSubMenu( viaSizeMenu );
    ctxMenu.AddMenu( viaSizeMenu.get(),                  viaToolActive, 500 );

    ctxMenu.AddSeparator( 500 );

    // Type-specific sub-menus will be added for us by other tools
    // For example, zone fill/unfill is provided by the PCB control tool

    // Finally, add the standard zoom/grid items
    getEditFrame<PCB_BASE_FRAME>()->AddStandardSubMenus( *m_menu.get() );

    return true;
}


void DRAWING_TOOL::Reset( RESET_REASON aReason )
{
    // Init variables used by every drawing tool
    m_view = getView();
    m_controls = getViewControls();
    m_board = getModel<BOARD>();
    m_frame = getEditFrame<PCB_BASE_EDIT_FRAME>();

    // Re-initialize session attributes
    const BOARD_DESIGN_SETTINGS& bds = m_frame->GetDesignSettings();

    if( aReason == RESET_REASON::SHUTDOWN )
        return;

    m_layer = m_frame->GetActiveLayer();
    m_stroke.SetWidth( bds.GetLineThickness( m_layer ) );
    m_stroke.SetLineStyle( LINE_STYLE::DEFAULT );
    m_stroke.SetColor( COLOR4D::UNSPECIFIED );

    m_textAttrs.m_Size = bds.GetTextSize( m_layer );
    m_textAttrs.m_StrokeWidth = bds.GetTextThickness( m_layer );
    InferBold( &m_textAttrs );
    m_textAttrs.m_Italic = bds.GetTextItalic( m_layer );
    m_textAttrs.m_KeepUpright = bds.GetTextUpright( m_layer );
    m_textAttrs.m_Mirrored = m_board->IsBackLayer( m_layer );
    m_textAttrs.m_Halign = GR_TEXT_H_ALIGN_LEFT;
    m_textAttrs.m_Valign = GR_TEXT_V_ALIGN_TOP;

    UpdateStatusBar();
}


DRAWING_TOOL::MODE DRAWING_TOOL::GetDrawingMode() const
{
    return m_mode;
}


void DRAWING_TOOL::UpdateStatusBar() const
{
    if( m_frame )
    {
        switch( GetAngleSnapMode() )
        {
        case LEADER_MODE::DEG45:
            m_frame->DisplayConstraintsMsg( _( "Constrain to H, V, 45" ) );
            break;
        case LEADER_MODE::DEG90:
            m_frame->DisplayConstraintsMsg( _( "Constrain to H, V" ) );
            break;
        default:
            m_frame->DisplayConstraintsMsg( wxString( "" ) );
            break;
        }
    }
}


int DRAWING_TOOL::DrawLine( const TOOL_EVENT& aEvent )
{
    if( m_isFootprintEditor && !m_frame->GetModel() )
        return 0;

    if( m_inDrawingTool )
        return 0;

    REENTRANCY_GUARD guard( &m_inDrawingTool );

    BOARD_ITEM*             parent = m_frame->GetModel();
    PCB_SHAPE*              line = new PCB_SHAPE( parent );
    BOARD_COMMIT            commit( m_frame );
    SCOPED_DRAW_MODE        scopedDrawMode( m_mode, MODE::LINE );
    std::optional<VECTOR2D> startingPoint;
    std::stack<PCB_SHAPE*>  committedLines;

    line->SetShape( SHAPE_T::SEGMENT );
    line->SetFlags( IS_NEW );

    if( aEvent.HasPosition() )
        startingPoint = getViewControls()->GetCursorPosition( !aEvent.DisableGridSnapping() );

    m_frame->PushTool( aEvent );
    Activate();

    while( drawShape( aEvent, &line, startingPoint, &committedLines ) )
    {
        if( line )
        {
            commit.Add( line );
            commit.Push( _( "Draw Line" ) );
            startingPoint = VECTOR2D( line->GetEnd() );
            committedLines.push( line );
        }
        else
        {
            startingPoint = std::nullopt;
        }

        line = new PCB_SHAPE( parent );
        line->SetShape( SHAPE_T::SEGMENT );
        line->SetFlags( IS_NEW );
    }

    return 0;
}


int DRAWING_TOOL::DrawRectangle( const TOOL_EVENT& aEvent )
{
    if( m_isFootprintEditor && !m_frame->GetModel() )
        return 0;

    if( m_inDrawingTool )
        return 0;

    REENTRANCY_GUARD guard( &m_inDrawingTool );

    bool                    isTextBox = aEvent.IsAction( &PCB_ACTIONS::drawTextBox );
    PCB_SHAPE*              rect = nullptr;
    BOARD_COMMIT            commit( m_frame );
    BOARD_ITEM*             parent = m_frame->GetModel();
    SCOPED_DRAW_MODE        scopedDrawMode( m_mode, MODE::RECTANGLE );
    std::optional<VECTOR2D> startingPoint;

    rect = isTextBox ? new PCB_TEXTBOX( parent ) : new PCB_SHAPE( parent );
    rect->SetShape( SHAPE_T::RECTANGLE );
    rect->SetFilled( false );
    rect->SetFlags( IS_NEW );

    if( aEvent.HasPosition() )
        startingPoint = getViewControls()->GetCursorPosition( !aEvent.DisableGridSnapping() );

    m_frame->PushTool( aEvent );
    Activate();

    while( drawShape( aEvent, &rect, startingPoint, nullptr ) )
    {
        if( rect )
        {
            bool cancelled = false;

            if( PCB_TEXTBOX* textbox = dynamic_cast<PCB_TEXTBOX*>( rect ) )
                cancelled = m_frame->ShowTextBoxPropertiesDialog( textbox ) != wxID_OK;

            if( cancelled )
            {
                delete rect;
                rect = nullptr;
            }
            else
            {
                rect->Normalize();
                commit.Add( rect );
                commit.Push( isTextBox ? _( "Draw Text Box" ) : _( "Draw Rectangle" ) );

                m_toolMgr->RunAction<EDA_ITEM*>( ACTIONS::selectItem, rect );
            }
        }

        rect = isTextBox ? new PCB_TEXTBOX( parent ) : new PCB_SHAPE( parent );
        rect->SetShape( SHAPE_T::RECTANGLE );
        rect->SetFilled( false );
        rect->SetFlags( IS_NEW );
        startingPoint = std::nullopt;
    }

    return 0;
}


int DRAWING_TOOL::DrawCircle( const TOOL_EVENT& aEvent )
{
    if( m_isFootprintEditor && !m_frame->GetModel() )
        return 0;

    if( m_inDrawingTool )
        return 0;

    REENTRANCY_GUARD guard( &m_inDrawingTool );

    BOARD_ITEM*             parent = m_frame->GetModel();
    PCB_SHAPE*              circle = new PCB_SHAPE( parent );
    BOARD_COMMIT            commit( m_frame );
    SCOPED_DRAW_MODE        scopedDrawMode( m_mode, MODE::CIRCLE );
    std::optional<VECTOR2D> startingPoint;

    circle->SetShape( SHAPE_T::CIRCLE );
    circle->SetFilled( false );
    circle->SetFlags( IS_NEW );

    if( aEvent.HasPosition() )
        startingPoint = getViewControls()->GetCursorPosition( !aEvent.DisableGridSnapping() );

    m_frame->PushTool( aEvent );
    Activate();

    while( drawShape( aEvent, &circle, startingPoint, nullptr ) )
    {
        if( circle )
        {
            commit.Add( circle );
            commit.Push( _( "Draw Circle" ) );

            m_toolMgr->RunAction<EDA_ITEM*>( ACTIONS::selectItem, circle );
        }

        circle = new PCB_SHAPE( parent );
        circle->SetShape( SHAPE_T::CIRCLE );
        circle->SetFilled( false );
        circle->SetFlags( IS_NEW );

        startingPoint = std::nullopt;
    }

    return 0;
}


int DRAWING_TOOL::DrawArc( const TOOL_EVENT& aEvent )
{
    if( m_isFootprintEditor && !m_frame->GetModel() )
        return 0;

    if( m_inDrawingTool )
        return 0;

    REENTRANCY_GUARD guard( &m_inDrawingTool );

    BOARD_ITEM*             parent = m_frame->GetModel();
    PCB_SHAPE*              arc = new PCB_SHAPE( parent );
    BOARD_COMMIT            commit( m_frame );
    SCOPED_DRAW_MODE        scopedDrawMode( m_mode, MODE::ARC );
    std::optional<VECTOR2D> startingPoint;

    arc->SetShape( SHAPE_T::ARC );
    arc->SetFlags( IS_NEW );

    m_frame->PushTool( aEvent );
    Activate();

    if( aEvent.HasPosition() )
        startingPoint = aEvent.Position();

    while( drawArc( aEvent, &arc, startingPoint ) )
    {
        if( arc )
        {
            commit.Add( arc );
            commit.Push( _( "Draw Arc" ) );

            m_toolMgr->RunAction<EDA_ITEM*>( ACTIONS::selectItem, arc );
        }

        arc = new PCB_SHAPE( parent );
        arc->SetShape( SHAPE_T::ARC );
        arc->SetFlags( IS_NEW );

        startingPoint = std::nullopt;
    }

    return 0;
}


int DRAWING_TOOL::DrawBezier( const TOOL_EVENT& aEvent )
{
    if( m_isFootprintEditor && !m_frame->GetModel() )
        return 0;

    if( m_inDrawingTool )
        return 0;

    REENTRANCY_GUARD guard( &m_inDrawingTool );

    BOARD_COMMIT            commit( m_frame );
    SCOPED_DRAW_MODE        scopedDrawMode( m_mode, MODE::BEZIER );
    OPT_VECTOR2I            startingPoint, startingC1;

    m_frame->PushTool( aEvent );
    Activate();

    if( aEvent.HasPosition() )
        startingPoint = aEvent.Position();

    DRAW_ONE_RESULT result = DRAW_ONE_RESULT::ACCEPTED;
    while( result != DRAW_ONE_RESULT::CANCELLED )
    {
        std::unique_ptr<PCB_SHAPE> bezier =
                drawOneBezier( aEvent, startingPoint, startingC1, result );

        // Anyting other than accepted means no chaining
        startingPoint = std::nullopt;
        startingC1 = std::nullopt;

        // If a bezier was created, add it and go again
        if( bezier )
        {
            PCB_SHAPE& bezierRef = *bezier;
            commit.Add( bezier.release() );
            commit.Push( _( "Draw Bezier" ) );

            // Don't chain if reset (or accepted and reset)
            if( result == DRAW_ONE_RESULT::ACCEPTED )
            {
                startingPoint = bezierRef.GetEnd();

                // If the last bezier has a zero C2 control arm, allow the user to define a new C1
                // control arm for the next one.
                if( bezierRef.GetEnd() != bezierRef.GetBezierC2() )
                {
                    // Mirror the control point across the end point to get a tangent control point
                    startingC1 = bezierRef.GetEnd() - ( bezierRef.GetBezierC2() - bezierRef.GetEnd() );
                }
            }
        }
    }

    return 0;
}


int DRAWING_TOOL::PlaceReferenceImage( const TOOL_EVENT& aEvent )
{
    if( m_inDrawingTool )
        return 0;

    REENTRANCY_GUARD     guard( &m_inDrawingTool );

    PCB_REFERENCE_IMAGE* image = aEvent.Parameter<PCB_REFERENCE_IMAGE*>();
    bool                 immediateMode = image != nullptr;
    PCB_GRID_HELPER      grid( m_toolMgr, m_frame->GetMagneticItemsSettings() );
    bool                 ignorePrimePosition = false;
    COMMON_SETTINGS*     common_settings = Pgm().GetCommonSettings();

    VECTOR2I             cursorPos = getViewControls()->GetCursorPosition();
    PCB_SELECTION_TOOL*  selectionTool = m_toolMgr->GetTool<PCB_SELECTION_TOOL>();
    BOARD_COMMIT         commit( m_frame );

    m_toolMgr->RunAction( ACTIONS::selectionClear );

    // Add all the drawable symbols to preview
    if( image )
    {
        image->SetPosition( cursorPos );
        m_view->ClearPreview();
        m_view->AddToPreview( image, false );   // Add, but not give ownership
    }

    m_frame->PushTool( aEvent );

    auto setCursor =
            [&]()
            {
                if( image )
                    m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::MOVING );
                else
                    m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::ARROW );
            };

    auto cleanup =
            [&] ()
            {
                m_toolMgr->RunAction( ACTIONS::selectionClear );
                m_view->ClearPreview();
                m_view->RecacheAllItems();
                delete image;
                image = nullptr;
            };

    Activate();

    // Must be done after Activate() so that it gets set into the correct context
    getViewControls()->ShowCursor( true );

    // Set initial cursor
    setCursor();

    // Prime the pump
    if( image )
    {
        m_toolMgr->PostAction( ACTIONS::refreshPreview );
    }
    else if( aEvent.HasPosition() )
    {
        m_toolMgr->PrimeTool( aEvent.Position() );
    }
    else if( common_settings->m_Input.immediate_actions && !aEvent.IsReactivate() )
    {
        m_toolMgr->PrimeTool( { 0, 0 } );
        ignorePrimePosition = true;
    }

    // Main loop: keep receiving events
    while( TOOL_EVENT* evt = Wait() )
    {
        setCursor();

        grid.SetSnap( !evt->Modifier( MD_SHIFT ) );
        grid.SetUseGrid( getView()->GetGAL()->GetGridSnapping() && !evt->DisableGridSnapping() );
        cursorPos = GetClampedCoords( grid.BestSnapAnchor( m_controls->GetMousePosition(),
                                                           { m_frame->GetActiveLayer() }, GRID_GRAPHICS ),
                                      COORDS_PADDING );
        m_controls->ForceCursorPosition( true, cursorPos );

        if( evt->IsCancelInteractive() || ( image && evt->IsAction( &ACTIONS::undo ) ) )
        {
            if( image )
            {
                cleanup();
            }
            else
            {
                m_frame->PopTool( aEvent );
                break;
            }

            if( immediateMode )
            {
                m_frame->PopTool( aEvent );
                break;
            }
        }
        else if( evt->IsActivate() )
        {
            if( image && evt->IsMoveTool() )
            {
                // We're already moving our own item; ignore the move tool
                evt->SetPassEvent( false );
                continue;
            }

            if( image )
            {
                m_frame->ShowInfoBarMsg( _( "Press <ESC> to cancel image creation." ) );
                evt->SetPassEvent( false );
                continue;
            }

            if( evt->IsMoveTool() )
            {
                // Leave ourselves on the stack so we come back after the move
                break;
            }
            else
            {
                m_frame->PopTool( aEvent );
                break;
            }
        }
        else if( evt->IsClick( BUT_LEFT ) || evt->IsDblClick( BUT_LEFT ) )
        {
            if( !image )
            {
                m_toolMgr->RunAction( ACTIONS::selectionClear );

                wxFileDialog dlg( m_frame, _( "Choose Image" ), wxEmptyString, wxEmptyString,
                                  _( "Image Files" ) + wxS( " " ) + wxImage::GetImageExtWildcard(),
                                  wxFD_OPEN );
                bool         cancelled = false;

                RunMainStack(
                        [&]()
                        {
                            cancelled = dlg.ShowModal() != wxID_OK;
                        } );

                if( cancelled )
                    continue;

                // If we started with a hotkey which has a position then warp back to that.
                // Otherwise update to the current mouse position pinned inside the autoscroll
                // boundaries.
                if( evt->IsPrime() && !ignorePrimePosition )
                {
                    cursorPos = grid.Align( evt->Position() );
                    getViewControls()->WarpMouseCursor( cursorPos, true );
                }
                else
                {
                    getViewControls()->PinCursorInsideNonAutoscrollArea( true );
                    cursorPos = getViewControls()->GetMousePosition();
                }

                cursorPos = getViewControls()->GetMousePosition( true );

                wxString fullFilename = dlg.GetPath();

                if( wxFileExists( fullFilename ) )
                    image = new PCB_REFERENCE_IMAGE( m_frame->GetModel(), cursorPos );

                if( !image || !image->GetReferenceImage().ReadImageFile( fullFilename ) )
                {
                    wxMessageBox( wxString::Format(_( "Could not load image from '%s'." ), fullFilename ) );
                    delete image;
                    image = nullptr;
                    continue;
                }

                image->SetFlags( IS_NEW | IS_MOVING );
                image->SetLayer( m_frame->GetActiveLayer() );

                m_view->ClearPreview();
                m_view->AddToPreview( image, false );   // Add, but not give ownership
                m_view->RecacheAllItems(); // Bitmaps are cached in Opengl
                selectionTool->AddItemToSel( image, false );

                getViewControls()->SetCursorPosition( cursorPos, false );
                setCursor();
                m_view->ShowPreview( true );
            }
            else
            {
                commit.Add( image );
                commit.Push( _( "Place Image" ) );

                m_toolMgr->RunAction<EDA_ITEM*>( ACTIONS::selectItem, image );

                image = nullptr;
                m_toolMgr->PostAction( ACTIONS::activatePointEditor );

                m_view->ClearPreview();

                if( immediateMode )
                {
                    m_frame->PopTool( aEvent );
                    break;
                }
            }
        }
        else if( evt->IsClick( BUT_RIGHT ) )
        {
            // Warp after context menu only if dragging...
            if( !image )
                m_toolMgr->VetoContextMenuMouseWarp();

            m_menu->ShowContextMenu( selectionTool->GetSelection() );
        }
        else if( image && (   evt->IsAction( &ACTIONS::refreshPreview )
                           || evt->IsMotion() ) )
        {
            image->SetPosition( cursorPos );
            m_view->ClearPreview();
            m_view->AddToPreview( image, false );   // Add, but not give ownership
            m_view->RecacheAllItems(); // Bitmaps are cached in Opengl
        }
        else if( image && evt->IsAction( &ACTIONS::doDelete ) )
        {
            cleanup();
        }
        else if( image && (   ZONE_FILLER_TOOL::IsZoneFillAction( evt )
                           || evt->IsAction( &ACTIONS::redo ) ) )
        {
            wxBell();
        }
        else
        {
            evt->SetPassEvent();
        }

        // Enable autopanning and cursor capture only when there is an image to be placed
        getViewControls()->SetAutoPan( image != nullptr );
        getViewControls()->CaptureCursor( image != nullptr );
    }

    getViewControls()->SetAutoPan( false );
    getViewControls()->CaptureCursor( false );
    m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::ARROW );

    return 0;
}

struct POINT_PLACER : public INTERACTIVE_PLACER_BASE
{
    POINT_PLACER( DRAWING_TOOL& aDrawingTool, PCB_BASE_EDIT_FRAME& aFrame ) :
            m_drawingTool( aDrawingTool ),
            m_frame( aFrame ),
            m_gridHelper( aDrawingTool.GetManager(), aFrame.GetMagneticItemsSettings() )
    {
    }

    std::unique_ptr<BOARD_ITEM> CreateItem() override
    {
        std::unique_ptr<PCB_POINT> new_point = std::make_unique<PCB_POINT>( m_frame.GetModel() );

        PCB_LAYER_ID layer = m_frame.GetActiveLayer();
        new_point->SetLayer( layer );

        return new_point;
    }

    void SnapItem( BOARD_ITEM* aItem ) override
    {
        m_gridHelper.SetSnap( !( m_modifiers & MD_SHIFT ) );
        m_gridHelper.SetUseGrid( !( m_modifiers & MD_CTRL ) );

        KIGFX::VIEW_CONTROLS& viewControls = *m_drawingTool.GetManager()->GetViewControls();
        const VECTOR2I        position = viewControls.GetMousePosition();

        VECTOR2I cursorPos = m_gridHelper.BestSnapAnchor( position, aItem->GetLayerSet() );
        viewControls.ForceCursorPosition( true, cursorPos );
        aItem->SetPosition( cursorPos );
    }

    DRAWING_TOOL&        m_drawingTool;
    PCB_BASE_EDIT_FRAME& m_frame;
    PCB_GRID_HELPER      m_gridHelper;
};


int DRAWING_TOOL::PlacePoint( const TOOL_EVENT& aEvent )
{
    if( m_isFootprintEditor && !m_frame->GetModel() )
        return 0;

    if( m_inDrawingTool )
        return 0;

    REENTRANCY_GUARD guard( &m_inDrawingTool );

    POINT_PLACER placer( *this, *frame() );

    doInteractiveItemPlacement( aEvent, &placer, _( "Place point" ), IPO_REPEAT | IPO_SINGLE_CLICK );

    return 0;
}


int DRAWING_TOOL::PlaceText( const TOOL_EVENT& aEvent )
{
    if( m_isFootprintEditor && !m_frame->GetModel() )
        return 0;

    if( m_inDrawingTool )
        return 0;

    REENTRANCY_GUARD guard( &m_inDrawingTool );

    COMMON_SETTINGS*             common_settings = Pgm().GetCommonSettings();
    PCB_TEXT*                    text = nullptr;
    bool                         ignorePrimePosition = false;
    const BOARD_DESIGN_SETTINGS& bds = m_frame->GetDesignSettings();
    BOARD_COMMIT                 commit( m_frame );
    SCOPED_DRAW_MODE             scopedDrawMode( m_mode, MODE::TEXT );
    PCB_GRID_HELPER              grid( m_toolMgr, m_frame->GetMagneticItemsSettings() );

    auto setCursor =
            [&]()
            {
                if( text )
                    m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::MOVING );
                else
                    m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::TEXT );
            };

    auto cleanup =
            [&]()
            {
                m_toolMgr->RunAction( ACTIONS::selectionClear );
                m_controls->ForceCursorPosition( false );
                m_controls->ShowCursor( true );
                m_controls->SetAutoPan( false );
                m_controls->CaptureCursor( false );
                delete text;
                text = nullptr;
            };

    m_toolMgr->RunAction( ACTIONS::selectionClear );

    m_frame->PushTool( aEvent );

    Activate();
    // Must be done after Activate() so that it gets set into the correct context
    m_controls->ShowCursor( true );
    m_controls->ForceCursorPosition( false );
    // do not capture or auto-pan until we start placing some text
    // Set initial cursor
    setCursor();

    if( aEvent.HasPosition() )
    {
        m_toolMgr->PrimeTool( aEvent.Position() );
    }
    else if( common_settings->m_Input.immediate_actions && !aEvent.IsReactivate() )
    {
        m_toolMgr->PrimeTool( { 0, 0 } );
        ignorePrimePosition = true;
    }

    // Main loop: keep receiving events
    while( TOOL_EVENT* evt = Wait() )
    {
        setCursor();

        grid.SetSnap( !evt->Modifier( MD_SHIFT ) );
        grid.SetUseGrid( getView()->GetGAL()->GetGridSnapping() && !evt->DisableGridSnapping() );
        VECTOR2I cursorPos = GetClampedCoords( grid.BestSnapAnchor( m_controls->GetMousePosition(),
                                                                    { m_frame->GetActiveLayer() }, GRID_TEXT ),
                                               COORDS_PADDING );
        m_controls->ForceCursorPosition( true, cursorPos );

        if( evt->IsDrag() )
        {
            continue;
        }
        else if( evt->IsCancelInteractive() || ( text && evt->IsAction( &ACTIONS::undo ) ) )
        {
            if( text )
            {
                cleanup();
            }
            else
            {
                m_frame->PopTool( aEvent );
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
                m_frame->PopTool( aEvent );
                break;
            }
        }
        else if( evt->IsClick( BUT_RIGHT ) )
        {
            if( !text )
                m_toolMgr->VetoContextMenuMouseWarp();

            m_menu->ShowContextMenu( selection() );
        }
        else if( evt->IsClick( BUT_LEFT ) )
        {
            bool placing = text != nullptr;

            if( !text )
            {
                m_toolMgr->RunAction( ACTIONS::selectionClear );

                m_controls->ForceCursorPosition( true, m_controls->GetCursorPosition() );

                PCB_LAYER_ID    layer = m_frame->GetActiveLayer();
                TEXT_ATTRIBUTES textAttrs;

                textAttrs.m_Size = bds.GetTextSize( layer );
                textAttrs.m_StrokeWidth = bds.GetTextThickness( layer );
                InferBold( &textAttrs );
                textAttrs.m_Italic = bds.GetTextItalic( layer );
                textAttrs.m_KeepUpright = bds.GetTextUpright( layer );
                textAttrs.m_Mirrored = m_board->IsBackLayer( layer );
                textAttrs.m_Halign = GR_TEXT_H_ALIGN_LEFT;
                textAttrs.m_Valign = GR_TEXT_V_ALIGN_BOTTOM;

                if( m_isFootprintEditor )
                    text = new PCB_TEXT( static_cast<FOOTPRINT*>( m_frame->GetModel() ) );
                else
                    text = new PCB_TEXT( m_frame->GetModel() );

                text->SetLayer( layer );
                text->SetAttributes( textAttrs );
                text->SetTextPos( cursorPos );
                text->SetFlags( IS_NEW ); // Prevent double undo commits

                DIALOG_TEXT_PROPERTIES textDialog( m_frame, text );
                bool                   cancelled;

                RunMainStack(
                        [&]()
                        {
                            // QuasiModal required for Scintilla auto-complete
                            cancelled = textDialog.ShowQuasiModal() != wxID_OK;
                        } );

                if( cancelled || NoPrintableChars( text->GetText() ) )
                {
                    delete text;
                    text = nullptr;
                }
                else if( text->GetTextPos() != cursorPos )
                {
                    // If the user modified the location then go ahead and place it there.
                    // Otherwise we'll drag.
                    placing = true;
                }

                if( text )
                {
                    if( !m_view->IsLayerVisible( text->GetLayer() ) )
                    {
                        m_frame->GetAppearancePanel()->SetLayerVisible( text->GetLayer(), true );
                        m_frame->GetCanvas()->Refresh();
                    }

                    m_toolMgr->RunAction<EDA_ITEM*>( ACTIONS::selectItem, text );
                    m_view->Update( &selection() );

                    // update the cursor so it looks correct before another event
                    setCursor();
                }
            }

            if( placing )
            {
                text->ClearFlags();
                m_toolMgr->RunAction( ACTIONS::selectionClear );

                commit.Add( text );
                commit.Push( _( "Draw Text" ) );

                m_toolMgr->RunAction<EDA_ITEM*>( ACTIONS::selectItem, text );

                text = nullptr;
            }

            m_controls->ForceCursorPosition( false );

            // If we started with a hotkey which has a position then warp back to that.
            // Otherwise update to the current mouse position pinned inside the autoscroll
            // boundaries.
            if( evt->IsPrime() && !ignorePrimePosition )
            {
                cursorPos = evt->Position();
                m_controls->WarpMouseCursor( cursorPos, true );
            }
            else
            {
                m_controls->PinCursorInsideNonAutoscrollArea( true );
                cursorPos = m_controls->GetMousePosition();
            }

            m_toolMgr->PostAction( PCB_ACTIONS::refreshPreview );

            m_controls->ShowCursor( true );
            m_controls->CaptureCursor( text != nullptr );
            m_controls->SetAutoPan( text != nullptr );
        }
        else if( text && ( evt->IsMotion() || evt->IsAction( &PCB_ACTIONS::refreshPreview ) ) )
        {
            text->SetPosition( cursorPos );
            selection().SetReferencePoint( cursorPos );
            m_view->Update( &selection() );
        }
        else if( text
                 && ( ZONE_FILLER_TOOL::IsZoneFillAction( evt )
                      || evt->IsAction( &ACTIONS::redo ) ) )
        {
            wxBell();
        }
        else if( text && evt->IsAction( &PCB_ACTIONS::properties ) )
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

    m_controls->SetAutoPan( false );
    m_controls->CaptureCursor( false );
    m_controls->ForceCursorPosition( false );
    m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::ARROW );

    if( selection().Empty() )
        m_frame->SetMsgPanel( board() );

    return 0;
}


int DRAWING_TOOL::DrawTable( const TOOL_EVENT& aEvent )
{
    if( m_inDrawingTool )
        return 0;

    REENTRANCY_GUARD guard( &m_inDrawingTool );

    PCB_TABLE*                   table = nullptr;
    const BOARD_DESIGN_SETTINGS& bds = m_frame->GetDesignSettings();
    BOARD_COMMIT                 commit( m_frame );
    PCB_GRID_HELPER              grid( m_toolMgr, m_frame->GetMagneticItemsSettings() );

    // We might be running as the same shape in another co-routine.  Make sure that one
    // gets whacked.
    m_toolMgr->DeactivateTool();

    auto setCursor =
            [&]()
            {
                if( table )
                    m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::MOVING );
                else
                    m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::PENCIL );
            };

    auto cleanup =
            [&] ()
            {
                m_toolMgr->RunAction( ACTIONS::selectionClear );
                m_controls->ForceCursorPosition( false );
                m_controls->ShowCursor( true );
                m_controls->SetAutoPan( false );
                m_controls->CaptureCursor( false );
                delete table;
                table = nullptr;
            };

    m_toolMgr->RunAction( ACTIONS::selectionClear );

    m_frame->PushTool( aEvent );

    Activate();
    // Must be done after Activate() so that it gets set into the correct context
    getViewControls()->ShowCursor( true );
    m_controls->ForceCursorPosition( false );
    // Set initial cursor
    setCursor();

    if( aEvent.HasPosition() )
        m_toolMgr->PrimeTool( aEvent.Position() );

    // Main loop: keep receiving events
    while( TOOL_EVENT* evt = Wait() )
    {
        setCursor();
        grid.SetSnap( !evt->Modifier( MD_SHIFT ) );
        grid.SetUseGrid( getView()->GetGAL()->GetGridSnapping() && !evt->DisableGridSnapping() );
        VECTOR2I cursorPos = GetClampedCoords( grid.BestSnapAnchor( m_controls->GetMousePosition(),
                                                                    { m_frame->GetActiveLayer() }, GRID_TEXT ),
                                               COORDS_PADDING );
        m_controls->ForceCursorPosition( true, cursorPos );

        if( evt->IsCancelInteractive() || ( table && evt->IsAction( &ACTIONS::undo ) )
            || evt->IsDrag() )
        {
            if( table )
            {
                cleanup();
            }
            else
            {
                m_frame->PopTool( aEvent );
                break;
            }
        }
        else if( evt->IsActivate() )
        {
            if( table )
                cleanup();

            if( evt->IsMoveTool() )
            {
                // leave ourselves on the stack so we come back after the move
                break;
            }
            else
            {
                m_frame->PopTool( aEvent );
                break;
            }
        }
        else if( evt->IsClick( BUT_RIGHT ) )
        {
            // Warp after context menu only if dragging...
            if( !table )
                m_toolMgr->VetoContextMenuMouseWarp();

            m_menu->ShowContextMenu( selection() );
        }
        else if( evt->IsClick( BUT_LEFT ) )
        {
            if( !table )
            {
                m_toolMgr->RunAction( ACTIONS::selectionClear );

                PCB_LAYER_ID layer = m_frame->GetActiveLayer();

                table = new PCB_TABLE( m_frame->GetModel(), bds.GetLineThickness( layer ) );
                table->SetFlags( IS_NEW );
                table->SetLayer( layer );
                table->SetColCount( 1 );
                table->AddCell( new PCB_TABLECELL( table ) );

                table->SetLayer( layer );
                table->SetPosition( cursorPos );

                if( !m_view->IsLayerVisible( layer ) )
                {
                    m_frame->GetAppearancePanel()->SetLayerVisible( layer, true );
                    m_frame->GetCanvas()->Refresh();
                }

                m_toolMgr->RunAction<EDA_ITEM*>( ACTIONS::selectItem, table );
                m_view->Update( &selection() );

                // update the cursor so it looks correct before another event
                setCursor();
            }
            else
            {
                m_toolMgr->RunAction( ACTIONS::selectionClear );

                table->Normalize();

                DIALOG_TABLE_PROPERTIES dlg( m_frame, table );
                bool                    cancelled;

                RunMainStack(
                        [&]()
                        {
                            // QuasiModal required for Scintilla auto-complete
                            cancelled = dlg.ShowQuasiModal() != wxID_OK;
                        } );

                if( cancelled )
                {
                    delete table;
                }
                else
                {
                    commit.Add( table, m_frame->GetScreen() );
                    commit.Push( _( "Draw Table" ) );

                    m_toolMgr->RunAction<EDA_ITEM*>( ACTIONS::selectItem, table );
                    m_toolMgr->PostAction( ACTIONS::activatePointEditor );
                }

                table = nullptr;
            }
        }
        else if( table && ( evt->IsAction( &ACTIONS::refreshPreview ) || evt->IsMotion() ) )
        {
            VECTOR2I fontSize = bds.GetTextSize( table->GetLayer() );
            VECTOR2I gridSize = grid.GetGridSize( grid.GetItemGrid( table ) );
            VECTOR2I origin( table->GetPosition() );
            VECTOR2I requestedSize( cursorPos - origin );

            int colCount = std::max( 1, requestedSize.x / ( fontSize.x * 15 ) );
            int rowCount = std::max( 1, requestedSize.y / ( fontSize.y * 3  ) );

            VECTOR2I cellSize( std::max( fontSize.x * 5, requestedSize.x / colCount ),
                               std::max( fontSize.y * 3, requestedSize.y / rowCount ) );

            cellSize.x = KiROUND( (double) cellSize.x / gridSize.x ) * gridSize.x;
            cellSize.y = KiROUND( (double) cellSize.y / gridSize.y ) * gridSize.y;

            table->ClearCells();
            table->SetColCount( colCount );

            for( int col = 0; col < colCount; ++col )
                table->SetColWidth( col, cellSize.x );

            for( int row = 0; row < rowCount; ++row )
            {
                table->SetRowHeight( row, cellSize.y );

                for( int col = 0; col < colCount; ++col )
                {
                    PCB_TABLECELL* cell = new PCB_TABLECELL( table );
                    cell->SetPosition( origin + VECTOR2I( col * cellSize.x, row * cellSize.y ) );
                    cell->SetEnd( cell->GetPosition() + cellSize );
                    table->AddCell( cell );
                }
            }

            selection().SetReferencePoint( cursorPos );
            m_view->Update( &selection() );
            m_frame->SetMsgPanel( table );
        }
        else if( table && evt->IsAction( &PCB_ACTIONS::properties ) )
        {
            frame()->OnEditItemRequest( table );
            m_view->Update( &selection() );
            frame()->SetMsgPanel( table );
        }
        else if( table && (   ZONE_FILLER_TOOL::IsZoneFillAction( evt )
                          || evt->IsAction( &ACTIONS::redo ) ) )
        {
            wxBell();
        }
        else
        {
            evt->SetPassEvent();
        }

        // Enable autopanning and cursor capture only when there is a shape being drawn
        getViewControls()->SetAutoPan( table != nullptr );
        getViewControls()->CaptureCursor( table != nullptr );
    }

    getViewControls()->SetAutoPan( false );
    getViewControls()->CaptureCursor( false );
    m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::ARROW );
    return 0;
}


void DRAWING_TOOL::constrainDimension( PCB_DIMENSION_BASE* aDim )
{
    const VECTOR2I lineVector{ aDim->GetEnd() - aDim->GetStart() };

    aDim->SetEnd( aDim->GetStart() + GetVectorSnapped45( lineVector ) );
    aDim->Update();
}

int DRAWING_TOOL::DrawBarcode( const TOOL_EVENT& aEvent )
{
    if( m_inDrawingTool )
        return 0;

    REENTRANCY_GUARD guard( &m_inDrawingTool );

    PCB_BARCODE*                 barcode = nullptr;
    BOARD_COMMIT                 commit( m_frame );
    PCB_GRID_HELPER              grid( m_toolMgr, m_frame->GetMagneticItemsSettings() );
    const BOARD_DESIGN_SETTINGS& bds = m_frame->GetDesignSettings();

    auto setCursor =
            [&]()
            {
                if( barcode )
                    m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::MOVING );
                else
                    m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::PENCIL );
            };

    auto cleanup =
            [&]()
            {
                m_toolMgr->RunAction( ACTIONS::selectionClear );
                m_controls->ForceCursorPosition( false );
                m_controls->ShowCursor( true );
                m_controls->SetAutoPan( false );
                m_controls->CaptureCursor( false );
                delete barcode;
                barcode = nullptr;
            };

    m_toolMgr->RunAction( ACTIONS::selectionClear );

    m_frame->PushTool( aEvent );

    Activate();
    // Must be done after Activate() so that it gets set into the correct context
    getViewControls()->ShowCursor( true );
    m_controls->ForceCursorPosition( false );
    setCursor();

    if( aEvent.HasPosition() )
        m_toolMgr->PrimeTool( aEvent.Position() );

    while( TOOL_EVENT* evt = Wait() )
    {
        setCursor();

        grid.SetSnap( !evt->Modifier( MD_SHIFT ) );
        grid.SetUseGrid( getView()->GetGAL()->GetGridSnapping() && !evt->DisableGridSnapping() );
        VECTOR2I cursorPos = GetClampedCoords( grid.BestSnapAnchor( m_controls->GetMousePosition(),
                                                                    { m_frame->GetActiveLayer() }, GRID_TEXT ),
                                               COORDS_PADDING );
        m_controls->ForceCursorPosition( true, cursorPos );

        if( evt->IsCancelInteractive()
            || evt->IsDrag()
            || ( barcode && evt->IsAction( &ACTIONS::undo ) ) )
        {
            if( barcode )
            {
                cleanup();
            }
            else
            {
                m_frame->PopTool( aEvent );
                break;
            }
        }
        else if( evt->IsActivate() )
        {
            if( barcode )
                cleanup();

            if( evt->IsMoveTool() )
            {
                // leave ourselves on the stack so we come back after the move
                break;
            }
            else
            {
                m_frame->PopTool( aEvent );
                break;
            }
        }
        else if( evt->IsClick( BUT_RIGHT ) )
        {
            if( !barcode )
                m_toolMgr->VetoContextMenuMouseWarp();

            m_menu->ShowContextMenu( selection() );
        }
        else if( evt->IsClick( BUT_LEFT ) )
        {
            m_toolMgr->RunAction( ACTIONS::selectionClear );

            PCB_LAYER_ID layer = m_frame->GetActiveLayer();

            barcode = new PCB_BARCODE( m_frame->GetModel() );
            barcode->SetFlags( IS_NEW );
            barcode->SetLayer( layer );
            barcode->SetPosition( cursorPos );
            barcode->SetTextSize( bds.GetTextSize( layer ).y );

            DIALOG_BARCODE_PROPERTIES dlg( m_frame, barcode );
            bool                      cancelled;

            RunMainStack(
                    [&]()
                    {
                        cancelled = dlg.ShowModal() != wxID_OK;
                    } );

            if( cancelled )
            {
                delete barcode;
            }
            else
            {
                if( !m_view->IsLayerVisible( layer ) )
                {
                    m_frame->GetAppearancePanel()->SetLayerVisible( layer, true );
                    m_frame->GetCanvas()->Refresh();
                }

                commit.Add( barcode );
                commit.Push( _( "Draw Barcode" ) );

                m_toolMgr->RunAction<EDA_ITEM*>( ACTIONS::selectItem, barcode );
                m_view->Update( &selection() );
            }

            barcode = nullptr;
        }
        else
        {
            evt->SetPassEvent();
        }

        getViewControls()->SetAutoPan( false );
        getViewControls()->CaptureCursor( false );
    }

    getViewControls()->SetAutoPan( false );
    getViewControls()->CaptureCursor( false );
    m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::ARROW );
    return 0;
}


int DRAWING_TOOL::DrawDimension( const TOOL_EVENT& aEvent )
{
    if( m_isFootprintEditor && !m_frame->GetModel() )
        return 0;

    if( m_inDrawingTool )
        return 0;

    REENTRANCY_GUARD guard( &m_inDrawingTool );

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
    KICAD_T                t = PCB_DIMENSION_T;

    m_view->Add( &preview );

    auto cleanup =
            [&]()
            {
                m_controls->SetAutoPan( false );
                m_controls->CaptureCursor( false );
                m_controls->ForceCursorPosition( false );

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

    m_toolMgr->RunAction( ACTIONS::selectionClear );

    m_frame->PushTool( aEvent );

    Activate();
    // Must be done after Activate() so that it gets set into the correct context
    m_controls->ShowCursor( true );
    m_controls->ForceCursorPosition( false );
    // Set initial cursor
    setCursor();

    m_toolMgr->PostAction( ACTIONS::refreshPreview );

    if( aEvent.HasPosition() )
        m_toolMgr->PrimeTool( aEvent.Position() );

    // Main loop: keep receiving events
    while( TOOL_EVENT* evt = Wait() )
    {
        if( step > SET_ORIGIN )
            frame()->SetMsgPanel( dimension );

        setCursor();

        grid.SetSnap( !evt->Modifier( MD_SHIFT ) );
        auto angleSnap = GetAngleSnapMode();
        if( evt->Modifier( MD_CTRL ) )
            angleSnap = LEADER_MODE::DIRECT;
        bool constrained = angleSnap != LEADER_MODE::DIRECT;
        grid.SetUseGrid( getView()->GetGAL()->GetGridSnapping() && !evt->DisableGridSnapping() );

        if( step == SET_HEIGHT && t != PCB_DIM_ORTHOGONAL_T )
        {
            if( dimension->GetStart().x != dimension->GetEnd().x
                    && dimension->GetStart().y != dimension->GetEnd().y )
            {
                // Not cardinal.  Grid snapping doesn't make sense for height.
                grid.SetUseGrid( false );
            }
        }

        VECTOR2I cursorPos = evt->HasPosition() ? evt->Position() : m_controls->GetMousePosition();
        cursorPos = GetClampedCoords( grid.BestSnapAnchor( cursorPos, nullptr, GRID_GRAPHICS ),
                                      COORDS_PADDING );

        m_controls->ForceCursorPosition( true, cursorPos );

        if( evt->IsCancelInteractive() || ( dimension && evt->IsAction( &ACTIONS::undo ) ) )
        {
            m_controls->SetAutoPan( false );

            if( step != SET_ORIGIN )    // start from the beginning
            {
                cleanup();
            }
            else
            {
                m_frame->PopTool( aEvent );
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
                m_frame->PopTool( aEvent );
                break;
            }
        }
        else if( evt->IsAction( &PCB_ACTIONS::incWidth ) && step != SET_ORIGIN )
        {
            m_stroke.SetWidth( m_stroke.GetWidth() + WIDTH_STEP );
            dimension->SetLineThickness( m_stroke.GetWidth() );
            m_view->Update( &preview );
            frame()->SetMsgPanel( dimension );
        }
        else if( evt->IsAction( &PCB_ACTIONS::decWidth ) && step != SET_ORIGIN )
        {
            if( (unsigned) m_stroke.GetWidth() > WIDTH_STEP )
            {
                m_stroke.SetWidth( m_stroke.GetWidth() - WIDTH_STEP );
                dimension->SetLineThickness( m_stroke.GetWidth() );
                m_view->Update( &preview );
                frame()->SetMsgPanel( dimension );
            }
        }
        else if( evt->IsClick( BUT_RIGHT ) )
        {
            if( !dimension )
                m_toolMgr->VetoContextMenuMouseWarp();

            m_menu->ShowContextMenu( selection() );
        }
        else if( evt->IsClick( BUT_LEFT ) || evt->IsDblClick( BUT_LEFT ) )
        {
            switch( step )
            {
            case SET_ORIGIN:
            {
                m_toolMgr->RunAction( ACTIONS::selectionClear );

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
                        };

                if( originalEvent.IsAction( &PCB_ACTIONS::drawAlignedDimension ) )
                {
                    dimension = new PCB_DIM_ALIGNED( m_frame->GetModel() );
                    setMeasurementAttributes( dimension );
                }
                else if( originalEvent.IsAction( &PCB_ACTIONS::drawOrthogonalDimension ) )
                {
                    dimension = new PCB_DIM_ORTHOGONAL( m_frame->GetModel() );
                    setMeasurementAttributes( dimension );
                }
                else if( originalEvent.IsAction( &PCB_ACTIONS::drawCenterDimension ) )
                {
                    dimension = new PCB_DIM_CENTER( m_frame->GetModel() );
                }
                else if( originalEvent.IsAction( &PCB_ACTIONS::drawRadialDimension ) )
                {
                    dimension = new PCB_DIM_RADIAL( m_frame->GetModel() );
                    setMeasurementAttributes( dimension );
                }
                else if( originalEvent.IsAction( &PCB_ACTIONS::drawLeader ) )
                {
                    dimension = new PCB_DIM_LEADER( m_frame->GetModel() );
                    dimension->SetTextPos( cursorPos );
                }
                else
                {
                    wxFAIL_MSG( wxT( "Unhandled action in DRAWING_TOOL::DrawDimension" ) );
                }

                t = dimension->Type();

                dimension->SetLayer( layer );
                dimension->SetMirrored( m_board->IsBackLayer( layer ) );
                dimension->SetTextSize( boardSettings.GetTextSize( layer ) );
                dimension->SetTextThickness( boardSettings.GetTextThickness( layer ) );
                dimension->SetItalic( boardSettings.GetTextItalic( layer ) );
                dimension->SetLineThickness( boardSettings.GetLineThickness( layer ) );
                dimension->SetArrowLength( boardSettings.m_DimensionArrowLength );
                dimension->SetExtensionOffset( boardSettings.m_DimensionExtensionOffset );
                dimension->SetStart( cursorPos );
                dimension->SetEnd( cursorPos );
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
                break;
            }

            case SET_END:
                // Dimensions that have origin and end in the same spot are not valid
                if( dimension->GetStart() == dimension->GetEnd() )
                {
                    --step;
                    break;
                }

                if( t != PCB_DIM_CENTER_T && t != PCB_DIM_RADIAL_T && t != PCB_DIM_LEADER_T )
                {
                    break;
                }

                ++step;
                KI_FALLTHROUGH;
            case SET_HEIGHT:
                assert( dimension->GetStart() != dimension->GetEnd() );
                assert( dimension->GetLineThickness() > 0 );

                preview.Remove( dimension );

                commit.Add( dimension );
                commit.Push( _( "Draw Dimension" ) );

                // Run the edit immediately to set the leader text
                if( t == PCB_DIM_LEADER_T )
                    frame()->OnEditItemRequest( dimension );

                m_toolMgr->RunAction<EDA_ITEM*>( ACTIONS::selectItem, dimension );

                break;
            }

            if( ++step >= FINISHED )
            {
                dimension = nullptr;
                step = SET_ORIGIN;
                m_controls->SetAutoPan( false );
                m_controls->CaptureCursor( false );
            }
            else if( evt->IsDblClick( BUT_LEFT ) )
            {
                m_toolMgr->PostAction( PCB_ACTIONS::cursorClick );
            }
        }
        else if( evt->IsMotion() )
        {
            switch( step )
            {
            case SET_END:
                dimension->SetEnd( cursorPos );

                if( constrained || t == PCB_DIM_CENTER_T )
                    constrainDimension( dimension );

                if( t == PCB_DIM_ORTHOGONAL_T )
                {
                    PCB_DIM_ORTHOGONAL* ortho = static_cast<PCB_DIM_ORTHOGONAL*>( dimension );

                    BOX2I bounds( dimension->GetStart(),
                                  dimension->GetEnd() - dimension->GetStart() );

                    // Create a nice preview by measuring the longer dimension
                    bool vert = bounds.GetWidth() < bounds.GetHeight();

                    ortho->SetOrientation( vert ? PCB_DIM_ORTHOGONAL::DIR::VERTICAL
                                                : PCB_DIM_ORTHOGONAL::DIR::HORIZONTAL );
                }
                else if( t == PCB_DIM_RADIAL_T )
                {
                    PCB_DIM_RADIAL* radialDim = static_cast<PCB_DIM_RADIAL*>( dimension );
                    VECTOR2I        textOffset( radialDim->GetArrowLength() * 10, 0 );

                    if( radialDim->GetEnd().x < radialDim->GetStart().x )
                        textOffset = -textOffset;

                    radialDim->SetTextPos( radialDim->GetKnee() + textOffset );
                }
                else if( t == PCB_DIM_LEADER_T )
                {
                    VECTOR2I textOffset( dimension->GetArrowLength() * 10, 0 );

                    if( dimension->GetEnd().x < dimension->GetStart().x )
                        textOffset = -textOffset;

                    dimension->SetTextPos( dimension->GetEnd() + textOffset );
                }

                dimension->Update();
                break;

            case SET_HEIGHT:
                if( t == PCB_DIM_ALIGNED_T )
                {
                    PCB_DIM_ALIGNED* aligned = static_cast<PCB_DIM_ALIGNED*>( dimension );

                    // Calculating the direction of travel perpendicular to the selected axis
                    double angle = aligned->GetAngle() + ( M_PI / 2 );

                    VECTOR2I delta( (VECTOR2I) cursorPos - dimension->GetEnd() );
                    double  height = ( delta.x * cos( angle ) ) + ( delta.y * sin( angle ) );
                    aligned->SetHeight( height );
                    aligned->Update();
                }
                else if( t == PCB_DIM_ORTHOGONAL_T )
                {
                    PCB_DIM_ORTHOGONAL* ortho = static_cast<PCB_DIM_ORTHOGONAL*>( dimension );

                    BOX2I    bbox( dimension->GetStart(),
                                   dimension->GetEnd() - dimension->GetStart() );
                    VECTOR2I direction( cursorPos - bbox.Centre() );
                    bool     vert;

                    // Only change the orientation when we move outside the bbox
                    if( !bbox.Contains( cursorPos ) )
                    {
                        // If the dimension is horizontal or vertical, set correct orientation
                        // otherwise, test if we're left/right of the bounding box or above/below it
                        if( bbox.GetWidth() == 0 )
                            vert = true;
                        else if( bbox.GetHeight() == 0 )
                            vert = false;
                        else if( cursorPos.x > bbox.GetLeft() && cursorPos.x < bbox.GetRight() )
                            vert = false;
                        else if( cursorPos.y > bbox.GetTop() && cursorPos.y < bbox.GetBottom() )
                            vert = true;
                        else
                            vert = std::abs( direction.y ) < std::abs( direction.x );

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

                break;
            }

            // Show a preview of the item
            m_view->Update( &preview );
        }
        else if( dimension && evt->IsAction( &PCB_ACTIONS::layerChanged ) )
        {
            PCB_LAYER_ID layer = m_frame->GetActiveLayer();

            if( !m_view->IsLayerVisible( layer ) )
            {
                m_frame->GetAppearancePanel()->SetLayerVisible( layer, true );
                m_frame->GetCanvas()->Refresh();
            }

            dimension->SetLayer( layer );
            dimension->SetTextSize( boardSettings.GetTextSize( layer ) );
            dimension->SetTextThickness( boardSettings.GetTextThickness( layer ) );
            dimension->SetItalic( boardSettings.GetTextItalic( layer ) );
            dimension->SetLineThickness( boardSettings.GetLineThickness( layer ) );
            dimension->Update();

            m_view->Update( &preview );
            frame()->SetMsgPanel( dimension );
        }
        else if( dimension && evt->IsAction( &PCB_ACTIONS::properties ) )
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
                wxBell();
            }
        }
        else if( dimension && evt->IsAction( &PCB_ACTIONS::changeDimensionArrows ) )
        {
            switch( dimension->Type() )
            {
            case PCB_DIM_ALIGNED_T:
            case PCB_DIM_ORTHOGONAL_T:
            case PCB_DIM_RADIAL_T:
                if( dimension->GetArrowDirection() == DIM_ARROW_DIRECTION::INWARD )
                    dimension->SetArrowDirection( DIM_ARROW_DIRECTION::OUTWARD );
                else
                    dimension->SetArrowDirection( DIM_ARROW_DIRECTION::INWARD );
                break;
            default:
                // Other dimension types don't have arrows that can swap
                wxBell();
            }

            m_view->Update( &preview );
        }
        else if( dimension && ( ZONE_FILLER_TOOL::IsZoneFillAction( evt )
                               || evt->IsAction( &ACTIONS::redo ) ) )
        {
            wxBell();
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

    if( selection().Empty() )
        m_frame->SetMsgPanel( board() );

    return 0;
}


int DRAWING_TOOL::PlaceImportedGraphics( const TOOL_EVENT& aEvent )
{
    if( !m_frame->GetModel() )
        return 0;

    if( m_inDrawingTool )
        return 0;

    REENTRANCY_GUARD guard( &m_inDrawingTool );

    DIALOG_IMPORT_GRAPHICS dlg( m_frame );

    // Set filename on drag-and-drop
    if( aEvent.HasParameter() )
        dlg.SetFilenameOverride( *aEvent.Parameter<wxString*>() );

    int dlgResult = dlg.ShowModal();

    std::list<std::unique_ptr<EDA_ITEM>>& list = dlg.GetImportedItems();

    if( dlgResult != wxID_OK )
        return 0;

    // Ensure the list is not empty:
    if( list.empty() )
    {
        wxMessageBox( _( "No graphic items found in file.") );
        return 0;
    }

    m_toolMgr->RunAction( ACTIONS::cancelInteractive );

    std::vector<BOARD_ITEM*> newItems;          // all new items, including group
    std::vector<BOARD_ITEM*> selectedItems;     // the group, or newItems if no group
    PCB_SELECTION            preview;
    BOARD_COMMIT             commit( m_frame );
    PCB_GROUP*               group = nullptr;
    PCB_LAYER_ID             layer = F_Cu;

    if( dlg.ShouldGroupItems() )
    {
        group = new PCB_GROUP( m_frame->GetModel() );

        newItems.push_back( group );
        selectedItems.push_back( group );
        preview.Add( group );
    }

    if( dlg.ShouldFixDiscontinuities() )
    {
        std::vector<PCB_SHAPE*> shapeList;

        for( const std::unique_ptr<EDA_ITEM>& ptr : list )
        {
            if( PCB_SHAPE* shape = dynamic_cast<PCB_SHAPE*>( ptr.get() ) )
                shapeList.push_back( shape );
        }

        ConnectBoardShapes( shapeList, dlg.GetTolerance() );
    }

    for( std::unique_ptr<EDA_ITEM>& ptr : list )
    {
        EDA_ITEM* eda_item = ptr.release();

        if( eda_item->IsBOARD_ITEM() )
        {
            BOARD_ITEM* item = static_cast<BOARD_ITEM*>( eda_item );

            newItems.push_back( item );

            if( group )
                group->AddItem( item );
            else
                selectedItems.push_back( item );

            layer = item->GetLayer();
        }

        preview.Add( eda_item );
    }

    // Clear the current selection then select the drawings so that edit tools work on them
    m_toolMgr->RunAction( ACTIONS::selectionClear );

    EDA_ITEMS selItems( selectedItems.begin(), selectedItems.end() );
    m_toolMgr->RunAction<EDA_ITEMS*>( ACTIONS::selectItems, &selItems );

    if( !dlg.IsPlacementInteractive() )
    {
        for( BOARD_ITEM* item : newItems )
            commit.Add( item );

        commit.Push( _( "Import Graphics" ) );

        return 0;
    }

    // Turn shapes on if they are off, so that the created object will be visible after completion
    m_frame->SetObjectVisible( LAYER_FILLED_SHAPES );

    if( !m_view->IsLayerVisible( layer ) )
    {
        m_frame->GetAppearancePanel()->SetLayerVisible( layer, true );
        m_frame->GetCanvas()->Refresh();
    }

    m_view->Add( &preview );

    m_frame->PushTool( aEvent );

    auto setCursor =
            [&]()
            {
                m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::MOVING );
            };

    Activate();
    // Must be done after Activate() so that it gets set into the correct context
    m_controls->ShowCursor( true );
    m_controls->ForceCursorPosition( false );
    // Set initial cursor
    setCursor();

    SCOPED_DRAW_MODE scopedDrawMode( m_mode, MODE::DXF );
    PCB_GRID_HELPER  grid( m_toolMgr, m_frame->GetMagneticItemsSettings() );

    // Now move the new items to the current cursor position:
    VECTOR2I cursorPos = m_controls->GetCursorPosition( !aEvent.DisableGridSnapping() );
    VECTOR2I delta = cursorPos - static_cast<BOARD_ITEM*>( preview.GetTopLeftItem() )->GetPosition();

    for( BOARD_ITEM* item : selectedItems )
        item->Move( delta );

    m_view->Update( &preview );

    // Main loop: keep receiving events
    while( TOOL_EVENT* evt = Wait() )
    {
        setCursor();

        grid.SetSnap( !evt->Modifier( MD_SHIFT ) );
        grid.SetUseGrid( getView()->GetGAL()->GetGridSnapping() && !evt->DisableGridSnapping() );
        cursorPos = GetClampedCoords( grid.BestSnapAnchor( m_controls->GetMousePosition(), { layer }, GRID_GRAPHICS ),
                                      COORDS_PADDING );
        m_controls->ForceCursorPosition( true, cursorPos );

        if( evt->IsCancelInteractive() || evt->IsActivate() )
        {
            m_toolMgr->RunAction( ACTIONS::selectionClear );

            if( group )
                preview.Remove( group );

            for( BOARD_ITEM* item : newItems )
                delete item;

            break;
        }
        else if( evt->IsMotion() )
        {
            delta = cursorPos - static_cast<BOARD_ITEM*>( preview.GetTopLeftItem() )->GetPosition();

            for( BOARD_ITEM* item : selectedItems )
                item->Move( delta );

            m_view->Update( &preview );
        }
        else if( evt->IsClick( BUT_RIGHT ) )
        {
            m_menu->ShowContextMenu( selection() );
        }
        else if( evt->IsClick( BUT_LEFT ) || evt->IsDblClick( BUT_LEFT ) )
        {
            // Place the imported drawings
            for( BOARD_ITEM* item : newItems )
                commit.Add( item );

            commit.Push( _( "Import Graphics" ) );

            break;   // This is a one-shot command, not a tool
        }
        else if( ZONE_FILLER_TOOL::IsZoneFillAction( evt ) )
        {
            wxBell();
        }
        else
        {
            evt->SetPassEvent();
        }
    }

    preview.Clear();
    m_view->Remove( &preview );

    m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::ARROW );
    m_controls->ForceCursorPosition( false );

    m_frame->PopTool( aEvent );

    return 0;
}


int DRAWING_TOOL::SetAnchor( const TOOL_EVENT& aEvent )
{
    // Make sense only in FP editor
    if( !m_isFootprintEditor )
        return 0;

    if( !m_frame->GetModel() )
        return 0;

    if( m_inDrawingTool )
        return 0;

    REENTRANCY_GUARD guard( &m_inDrawingTool );

    SCOPED_DRAW_MODE scopedDrawMode( m_mode, MODE::ANCHOR );
    PCB_GRID_HELPER  grid( m_toolMgr, m_frame->GetMagneticItemsSettings() );

    m_toolMgr->RunAction( ACTIONS::selectionClear );

    m_frame->PushTool( aEvent );

    auto setCursor =
            [&]()
            {
                m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::BULLSEYE );
            };

    Activate();
    // Must be done after Activate() so that it gets set into the correct context
    m_controls->ShowCursor( true );
    m_controls->SetAutoPan( true );
    m_controls->CaptureCursor( false );
    m_controls->ForceCursorPosition( false );
    // Set initial cursor
    setCursor();

    while( TOOL_EVENT* evt = Wait() )
    {
        setCursor();

        grid.SetSnap( !evt->Modifier( MD_SHIFT ) );
        grid.SetUseGrid( getView()->GetGAL()->GetGridSnapping() && !evt->DisableGridSnapping() );
        VECTOR2I cursorPos = grid.BestSnapAnchor( m_controls->GetMousePosition(), LSET::AllLayersMask() );
        m_controls->ForceCursorPosition( true, cursorPos );

        if( evt->IsClick( BUT_LEFT ) || evt->IsDblClick( BUT_LEFT ) )
        {
            FOOTPRINT*   footprint = (FOOTPRINT*) m_frame->GetModel();
            BOARD_COMMIT commit( m_frame );
            commit.Modify( footprint );

            // set the new relative internal local coordinates of footprint items
            VECTOR2I moveVector = footprint->GetPosition() - cursorPos;
            footprint->MoveAnchorPosition( moveVector );

            commit.Push( _( "Move Footprint Anchor" ) );

            // Usually, we do not need to change twice the anchor position,
            // so deselect the active tool
            m_frame->PopTool( aEvent );
            break;
        }
        else if( evt->IsClick( BUT_RIGHT ) )
        {
            m_menu->ShowContextMenu( selection() );
        }
        else if( evt->IsCancelInteractive() || evt->IsActivate() )
        {
            m_frame->PopTool( aEvent );
            break;
        }
        else
        {
            evt->SetPassEvent();
        }
    }

    m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::ARROW );
    m_controls->ForceCursorPosition( false );

    return 0;
}


/**
 * Update a #PCB_SHAPE from the current state of a #TWO_POINT_GEOMETRY_MANAGER.
 */
static void updateSegmentFromGeometryMgr( const KIGFX::PREVIEW::TWO_POINT_GEOMETRY_MANAGER& aMgr,
                                          PCB_SHAPE* aGraphic )
{
    if( !aMgr.IsReset() )
    {
        aGraphic->SetStart( aMgr.GetOrigin() );
        aGraphic->SetEnd( aMgr.GetEnd() );
    }
}


bool DRAWING_TOOL::drawShape( const TOOL_EVENT& aTool, PCB_SHAPE** aGraphic,
                              std::optional<VECTOR2D> aStartingPoint,
                              std::stack<PCB_SHAPE*>* aCommittedGraphics )
{
    SHAPE_T shape = ( *aGraphic )->GetShape();

    // Only three shapes are currently supported
    wxASSERT( shape == SHAPE_T::SEGMENT || shape == SHAPE_T::CIRCLE || shape == SHAPE_T::RECTANGLE );

    const BOARD_DESIGN_SETTINGS& bds = m_frame->GetDesignSettings();
    EDA_UNITS                    userUnits = m_frame->GetUserUnits();
    PCB_GRID_HELPER              grid( m_toolMgr, m_frame->GetMagneticItemsSettings() );
    PCB_SHAPE*&                  graphic = *aGraphic;

    if( m_layer != m_frame->GetActiveLayer() )
    {
        m_layer = m_frame->GetActiveLayer();
        m_stroke.SetWidth( bds.GetLineThickness( m_layer ) );
        m_stroke.SetLineStyle( LINE_STYLE::DEFAULT );
        m_stroke.SetColor( COLOR4D::UNSPECIFIED );

        m_textAttrs.m_Size = bds.GetTextSize( m_layer );
        m_textAttrs.m_StrokeWidth = bds.GetTextThickness( m_layer );
        InferBold( &m_textAttrs );
        m_textAttrs.m_Italic = bds.GetTextItalic( m_layer );
        m_textAttrs.m_KeepUpright = bds.GetTextUpright( m_layer );
        m_textAttrs.m_Mirrored = m_board->IsBackLayer( m_layer );
        m_textAttrs.m_Halign = GR_TEXT_H_ALIGN_LEFT;
        m_textAttrs.m_Valign = GR_TEXT_V_ALIGN_TOP;
    }

    // Turn shapes on if they are off, so that the created object will be visible after completion
    m_frame->SetObjectVisible( LAYER_FILLED_SHAPES );

    // geometric construction manager
    KIGFX::PREVIEW::TWO_POINT_GEOMETRY_MANAGER twoPointMgr;

    // drawing assistant overlay
    // TODO: workaround because EDA_SHAPE_TYPE_T is not visible from commons.
    KIGFX::PREVIEW::GEOM_SHAPE geomShape( static_cast<KIGFX::PREVIEW::GEOM_SHAPE>( shape ) );
    KIGFX::PREVIEW::TWO_POINT_ASSISTANT twoPointAsst( twoPointMgr, pcbIUScale, userUnits, geomShape );

    // Add a VIEW_GROUP that serves as a preview for the new item
    m_preview.Clear();
    m_view->Add( &m_preview );
    m_view->Add( &twoPointAsst );

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
                m_preview.Clear();
                m_view->Update( &m_preview );
                delete graphic;
                graphic = nullptr;

                if( !isLocalOriginSet )
                    m_frame->GetScreen()->m_LocalOrigin = VECTOR2D( 0, 0 );
            };

    m_controls->ShowCursor( true );
    m_controls->ForceCursorPosition( false );
    // Set initial cursor
    setCursor();

    m_toolMgr->PostAction( ACTIONS::refreshPreview );

    if( aStartingPoint )
        m_toolMgr->PrimeTool( *aStartingPoint );

    // Main loop: keep receiving events
    while( TOOL_EVENT* evt = Wait() )
    {
        setCursor();

        if( started )
            m_frame->SetMsgPanel( graphic );

        grid.SetSnap( !evt->Modifier( MD_SHIFT ) );
        auto angleSnap = GetAngleSnapMode();
        if( evt->Modifier( MD_CTRL ) )
            angleSnap = LEADER_MODE::DIRECT;

        // Rectangular shapes never get zero-size snapping
        if( shape == SHAPE_T::RECTANGLE && angleSnap == LEADER_MODE::DEG90 )
            angleSnap = LEADER_MODE::DEG45;

        grid.SetUseGrid( getView()->GetGAL()->GetGridSnapping() && !evt->DisableGridSnapping() );
        cursorPos = GetClampedCoords( grid.BestSnapAnchor( m_controls->GetMousePosition(), { m_layer }, GRID_GRAPHICS ),
                                      COORDS_PADDING );
        m_controls->ForceCursorPosition( true, cursorPos );

        if( evt->IsCancelInteractive() || ( started && evt->IsAction( &ACTIONS::undo ) ) )
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
            if( m_layer != m_frame->GetActiveLayer() )
            {
                m_layer = m_frame->GetActiveLayer();
                m_stroke.SetWidth( bds.GetLineThickness( m_layer ) );
                m_stroke.SetLineStyle( LINE_STYLE::DEFAULT );
                m_stroke.SetColor( COLOR4D::UNSPECIFIED );

                m_textAttrs.m_Size = bds.GetTextSize( m_layer );
                m_textAttrs.m_StrokeWidth = bds.GetTextThickness( m_layer );
                InferBold( &m_textAttrs );
                m_textAttrs.m_Italic = bds.GetTextItalic( m_layer );
                m_textAttrs.m_KeepUpright = bds.GetTextUpright( m_layer );
                m_textAttrs.m_Mirrored = m_board->IsBackLayer( m_layer );
                m_textAttrs.m_Halign = GR_TEXT_H_ALIGN_LEFT;
                m_textAttrs.m_Valign = GR_TEXT_V_ALIGN_TOP;
            }

            if( graphic )
            {
                if( !m_view->IsLayerVisible( m_layer ) )
                {
                    m_frame->GetAppearancePanel()->SetLayerVisible( m_layer, true );
                    m_frame->GetCanvas()->Refresh();
                }

                graphic->SetLayer( m_layer );
                graphic->SetStroke( m_stroke );

                if( PCB_TEXTBOX* pcb_textbox = dynamic_cast<PCB_TEXTBOX*>( graphic ) )
                    pcb_textbox->SetAttributes( m_textAttrs );

                m_view->Update( &m_preview );
                frame()->SetMsgPanel( graphic );
            }
            else
            {
                evt->SetPassEvent();
            }
        }
        else if( evt->IsClick( BUT_RIGHT ) )
        {
            if( !graphic )
                m_toolMgr->VetoContextMenuMouseWarp();

            m_menu->ShowContextMenu( selection() );
        }
        else if( evt->IsClick( BUT_LEFT ) || evt->IsDblClick( BUT_LEFT ) )
        {
            if( !graphic )
                break;

            if( !started )
            {
                m_toolMgr->RunAction( ACTIONS::selectionClear );

                if( aStartingPoint )
                {
                    cursorPos = *aStartingPoint;
                    aStartingPoint = std::nullopt;
                }

                // Init the new item attributes
                if( graphic )   // always true, but Coverity can't seem to figure that out
                {
                    graphic->SetShape( static_cast<SHAPE_T>( shape ) );
                    graphic->SetFilled( false );
                    graphic->SetStroke( m_stroke );
                    graphic->SetLayer( m_layer );
                }

                if( PCB_TEXTBOX* pcb_textbox = dynamic_cast<PCB_TEXTBOX*>( graphic ) )
                    pcb_textbox->SetAttributes( m_textAttrs );

                grid.SetSkipPoint( cursorPos );

                twoPointMgr.SetOrigin( cursorPos );
                twoPointMgr.SetEnd( cursorPos );

                if( !isLocalOriginSet )
                    m_frame->GetScreen()->m_LocalOrigin = cursorPos;

                m_preview.Add( graphic );
                frame()->SetMsgPanel( graphic );
                m_controls->SetAutoPan( true );
                m_controls->CaptureCursor( true );

                if( !m_view->IsLayerVisible( m_layer ) )
                {
                    m_frame->GetAppearancePanel()->SetLayerVisible( m_layer, true );
                    m_frame->GetCanvas()->Refresh();
                }

                updateSegmentFromGeometryMgr( twoPointMgr, graphic );

                started = true;
            }
            else
            {
                PCB_SHAPE* snapItem = dynamic_cast<PCB_SHAPE*>( grid.GetSnapped() );

                if( shape == SHAPE_T::SEGMENT && snapItem && graphic->GetLength() > 0 )
                {
                    // User has clicked on the end of an existing segment, closing a path
                    BOARD_COMMIT commit( m_frame );

                    commit.Add( graphic );
                    commit.Push( _( "Draw Line" ) );
                    m_toolMgr->RunAction<EDA_ITEM*>( ACTIONS::selectItem, graphic );

                    graphic = nullptr;
                }
                else if( twoPointMgr.IsEmpty() || evt->IsDblClick( BUT_LEFT ) )
                {
                    // User has clicked twice in the same spot, meaning we're finished
                    delete graphic;
                    graphic = nullptr;
                }

                m_preview.Clear();
                twoPointMgr.Reset();
                break;
            }

            twoPointMgr.SetEnd( GetClampedCoords( cursorPos ) );
        }
        else if( evt->IsMotion() )
        {
            VECTOR2I clampedCursorPos = cursorPos;

            if( shape == SHAPE_T::CIRCLE || shape == SHAPE_T::ARC )
                clampedCursorPos = getClampedRadiusEnd( twoPointMgr.GetOrigin(), cursorPos );
            else
                clampedCursorPos = getClampedDifferenceEnd( twoPointMgr.GetOrigin(), cursorPos );

            // constrained lines
            if( started && angleSnap != LEADER_MODE::DIRECT )
            {
                const VECTOR2I lineVector( clampedCursorPos - VECTOR2I( twoPointMgr.GetOrigin() ) );

                VECTOR2I newEnd;
                if( angleSnap == LEADER_MODE::DEG90 )
                    newEnd = GetVectorSnapped90( lineVector );
                else
                    newEnd = GetVectorSnapped45( lineVector, ( shape == SHAPE_T::RECTANGLE ) );

                m_controls->ForceCursorPosition( true, VECTOR2I( twoPointMgr.GetEnd() ) );
                twoPointMgr.SetEnd( twoPointMgr.GetOrigin() + newEnd );
                twoPointMgr.SetAngleSnap( angleSnap );
            }
            else
            {
                twoPointMgr.SetEnd( clampedCursorPos );
                twoPointMgr.SetAngleSnap( LEADER_MODE::DIRECT );
            }

            updateSegmentFromGeometryMgr( twoPointMgr, graphic );
            m_view->Update( &m_preview );
            m_view->Update( &twoPointAsst );
        }
        else if( started && (   evt->IsAction( &PCB_ACTIONS::doDelete )
                             || evt->IsAction( &PCB_ACTIONS::deleteLastPoint ) ) )
        {
            if( aCommittedGraphics && !aCommittedGraphics->empty() )
            {
                twoPointMgr.SetOrigin( aCommittedGraphics->top()->GetStart() );
                twoPointMgr.SetEnd( aCommittedGraphics->top()->GetEnd() );
                aCommittedGraphics->pop();

                getViewControls()->WarpMouseCursor( twoPointMgr.GetEnd(), true );

                if( PICKED_ITEMS_LIST* undo = m_frame->PopCommandFromUndoList() )
                {
                    m_frame->PutDataInPreviousState( undo );
                    m_frame->ClearListAndDeleteItems( undo );
                    delete undo;
                }

                updateSegmentFromGeometryMgr( twoPointMgr, graphic );
                m_view->Update( &m_preview );
                m_view->Update( &twoPointAsst );
            }
            else
            {
                cleanup();
                break;
            }
        }
        else if( graphic && evt->IsAction( &PCB_ACTIONS::incWidth ) )
        {
            m_stroke.SetWidth( m_stroke.GetWidth() + WIDTH_STEP );
            graphic->SetStroke( m_stroke );
            m_view->Update( &m_preview );
            frame()->SetMsgPanel( graphic );
        }
        else if( graphic && evt->IsAction( &PCB_ACTIONS::decWidth ) )
        {
            if( (unsigned) m_stroke.GetWidth() > WIDTH_STEP )
            {
                m_stroke.SetWidth( m_stroke.GetWidth() - WIDTH_STEP );
                graphic->SetStroke( m_stroke );
                m_view->Update( &m_preview );
                frame()->SetMsgPanel( graphic );
            }
        }
        else if( started && evt->IsAction( &PCB_ACTIONS::properties ) )
        {
            frame()->OnEditItemRequest( graphic );
            m_view->Update( &m_preview );
            frame()->SetMsgPanel( graphic );
        }
        else if( started && (   ZONE_FILLER_TOOL::IsZoneFillAction( evt )
                             || evt->IsAction( &ACTIONS::redo ) ) )
        {
            wxBell();
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
    m_view->Remove( &m_preview );

    if( selection().Empty() )
        m_frame->SetMsgPanel( board() );

    m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::ARROW );
    m_controls->SetAutoPan( false );
    m_controls->CaptureCursor( false );
    m_controls->ForceCursorPosition( false );

    return !cancelled;
}


/**
 * Update an arc PCB_SHAPE from the current state of an Arc Geometry Manager.
 */
static void updateArcFromConstructionMgr( const KIGFX::PREVIEW::ARC_GEOM_MANAGER& aMgr,
                                          PCB_SHAPE& aArc )
{
    VECTOR2I vec = aMgr.GetOrigin();

    aArc.SetCenter( vec );

    if( aMgr.GetSubtended() < ANGLE_0 )
    {
        vec = aMgr.GetStartRadiusEnd();
        aArc.SetStart( vec );
        vec = aMgr.GetEndRadiusEnd();
        aArc.SetEnd( vec );
    }
    else
    {
        vec = aMgr.GetEndRadiusEnd();
        aArc.SetStart( vec );
        vec = aMgr.GetStartRadiusEnd();
        aArc.SetEnd( vec );
    }
}


bool DRAWING_TOOL::drawArc( const TOOL_EVENT& aTool, PCB_SHAPE** aGraphic,
                            std::optional<VECTOR2D> aStartingPoint )
{
    wxCHECK( aGraphic, false );

    PCB_SHAPE*&  graphic = *aGraphic;

    wxCHECK( graphic, false );

    if( m_layer != m_frame->GetActiveLayer() )
    {
        m_layer = m_frame->GetActiveLayer();
        m_stroke.SetWidth( m_frame->GetDesignSettings().GetLineThickness( m_layer ) );
        m_stroke.SetLineStyle( LINE_STYLE::DEFAULT );
        m_stroke.SetColor( COLOR4D::UNSPECIFIED );
    }

    // Arc geometric construction manager
    KIGFX::PREVIEW::ARC_GEOM_MANAGER arcManager;

    // Arc drawing assistant overlay
    KIGFX::PREVIEW::ARC_ASSISTANT arcAsst( arcManager, pcbIUScale, m_frame->GetUserUnits() );

    // Add a VIEW_GROUP that serves as a preview for the new item
    PCB_SELECTION preview;
    m_view->Add( &preview );
    m_view->Add( &arcAsst );
    PCB_GRID_HELPER grid( m_toolMgr, m_frame->GetMagneticItemsSettings() );

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

    m_controls->ShowCursor( true );
    m_controls->ForceCursorPosition( false );
    // Set initial cursor
    setCursor();

    bool started = false;
    bool cancelled = false;

    m_toolMgr->PostAction( ACTIONS::refreshPreview );

    if( aStartingPoint )
        m_toolMgr->PrimeTool( *aStartingPoint );

    // Main loop: keep receiving events
    while( TOOL_EVENT* evt = Wait() )
    {
        if( started )
            m_frame->SetMsgPanel( graphic );

        setCursor();

        graphic->SetLayer( m_layer );

        grid.SetSnap( !evt->Modifier( MD_SHIFT ) );
        auto angleSnap = GetAngleSnapMode();
        if( evt->Modifier( MD_CTRL ) )
            angleSnap = LEADER_MODE::DIRECT;
        grid.SetUseGrid( getView()->GetGAL()->GetGridSnapping() && !evt->DisableGridSnapping() );
        VECTOR2I cursorPos = GetClampedCoords( grid.BestSnapAnchor( m_controls->GetMousePosition(), graphic,
                                                                    GRID_GRAPHICS ),
                                               COORDS_PADDING );
        m_controls->ForceCursorPosition( true, cursorPos );

        if( evt->IsCancelInteractive() || ( started && evt->IsAction( &ACTIONS::undo ) ) )
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
        else if( evt->IsClick( BUT_LEFT ) )
        {
            if( !started )
            {
                m_toolMgr->RunAction( ACTIONS::selectionClear );

                m_controls->SetAutoPan( true );
                m_controls->CaptureCursor( true );

                // Init the new item attributes
                // (non-geometric, those are handled by the manager)
                graphic->SetShape( SHAPE_T::ARC );
                graphic->SetStroke( m_stroke );

                if( !m_view->IsLayerVisible( m_layer ) )
                {
                    m_frame->GetAppearancePanel()->SetLayerVisible( m_layer, true );
                    m_frame->GetCanvas()->Refresh();
                }

                preview.Add( graphic );
                frame()->SetMsgPanel( graphic );
                started = true;
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
            arcManager.SetAngleSnap( angleSnap != LEADER_MODE::DIRECT );

            // update, but don't step the manager state
            arcManager.AddPoint( cursorPos, false );
        }
        else if( evt->IsAction( &PCB_ACTIONS::layerChanged ) )
        {
            if( m_layer != m_frame->GetActiveLayer() )
            {
                m_layer = m_frame->GetActiveLayer();
                m_stroke.SetWidth( m_frame->GetDesignSettings().GetLineThickness( m_layer ) );
                m_stroke.SetLineStyle( LINE_STYLE::DEFAULT );
                m_stroke.SetColor( COLOR4D::UNSPECIFIED );
            }

            if( graphic )
            {
                if( !m_view->IsLayerVisible( m_layer ) )
                {
                    m_frame->GetAppearancePanel()->SetLayerVisible( m_layer, true );
                    m_frame->GetCanvas()->Refresh();
                }

                graphic->SetLayer( m_layer );
                graphic->SetStroke( m_stroke );
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
                graphic->SetArcAngleAndEnd( ANGLE_90 );
                frame()->OnEditItemRequest( graphic );
                m_view->Update( &preview );
                frame()->SetMsgPanel( graphic );
                break;
            }
            // Don't show the edit panel if we can't represent the arc with it
            else if( ( arcManager.GetStep() == KIGFX::PREVIEW::ARC_GEOM_MANAGER::SET_ANGLE )
                    && ( arcManager.GetStartRadiusEnd() != arcManager.GetEndRadiusEnd() ) )
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
            if( !graphic )
                m_toolMgr->VetoContextMenuMouseWarp();

            m_menu->ShowContextMenu( selection() );
        }
        else if( evt->IsAction( &PCB_ACTIONS::incWidth ) )
        {
            m_stroke.SetWidth( m_stroke.GetWidth() + WIDTH_STEP );

            if( graphic )
            {
                graphic->SetStroke( m_stroke );
                m_view->Update( &preview );
                frame()->SetMsgPanel( graphic );
            }
        }
        else if( evt->IsAction( &PCB_ACTIONS::decWidth ) )
        {
            if( (unsigned) m_stroke.GetWidth() > WIDTH_STEP )
            {
                m_stroke.SetWidth( m_stroke.GetWidth() - WIDTH_STEP );

                if( graphic )
                {
                    graphic->SetStroke( m_stroke );
                    m_view->Update( &preview );
                    frame()->SetMsgPanel( graphic );
                }
            }
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
        else if( started && (   ZONE_FILLER_TOOL::IsZoneFillAction( evt )
                             || evt->IsAction( &ACTIONS::redo ) ) )
        {
            wxBell();
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

            if( started )
                frame()->SetMsgPanel( graphic );
            else
                frame()->SetMsgPanel( board() );
        }
    }

    preview.Remove( graphic );
    m_view->Remove( &arcAsst );
    m_view->Remove( &preview );

    if( selection().Empty() )
        m_frame->SetMsgPanel( board() );

    m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::ARROW );
    m_controls->SetAutoPan( false );
    m_controls->CaptureCursor( false );
    m_controls->ForceCursorPosition( false );

    return !cancelled;
}


std::unique_ptr<PCB_SHAPE> DRAWING_TOOL::drawOneBezier( const TOOL_EVENT&   aTool,
                                                        const OPT_VECTOR2I& aStartingPoint,
                                                        const OPT_VECTOR2I& aStartingControl1Point,
                                                        DRAW_ONE_RESULT&    aResult )
{
    int maxError = board()->GetDesignSettings().m_MaxError;

    std::unique_ptr<PCB_SHAPE> bezier = std::make_unique<PCB_SHAPE>( m_frame->GetModel() );
    bezier->SetShape( SHAPE_T::BEZIER );
    bezier->SetFlags( IS_NEW );

    if( m_layer != m_frame->GetActiveLayer() )
    {
        m_layer = m_frame->GetActiveLayer();
        m_stroke.SetWidth( m_frame->GetDesignSettings().GetLineThickness( m_layer ) );
        m_stroke.SetLineStyle( LINE_STYLE::DEFAULT );
        m_stroke.SetColor( COLOR4D::UNSPECIFIED );
    }

    // Arc geometric construction manager
    KIGFX::PREVIEW::BEZIER_GEOM_MANAGER bezierManager;

    // Arc drawing assistant overlay
    KIGFX::PREVIEW::BEZIER_ASSISTANT bezierAsst( bezierManager, pcbIUScale, m_frame->GetUserUnits() );

    // Add a VIEW_GROUP that serves as a preview for the new item
    PCB_SELECTION preview;
    m_view->Add( &preview );
    m_view->Add( &bezierAsst );
    PCB_GRID_HELPER grid( m_toolMgr, m_frame->GetMagneticItemsSettings() );

    const auto setCursor =
            [&]()
            {
                m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::PENCIL );
            };

    const auto resetProgress =
            [&]()
            {
                preview.Clear();
                bezier.reset();
            };

    m_controls->ShowCursor( true );
    m_controls->ForceCursorPosition( false );
    // Set initial cursor
    setCursor();

    const auto started =
            [&]()
            {
                return bezierManager.GetStep() > KIGFX::PREVIEW::BEZIER_GEOM_MANAGER::SET_START;
            };

    aResult = DRAW_ONE_RESULT::ACCEPTED;
    bool priming = false;

    m_toolMgr->PostAction( ACTIONS::refreshPreview );

    // Load in one or two points if they were passed in
    if( aStartingPoint )
    {
        priming = true;

        if( aStartingControl1Point )
        {
            bezierManager.AddPoint( *aStartingPoint, true );
            bezierManager.AddPoint( *aStartingControl1Point, true );
            m_toolMgr->PrimeTool( *aStartingControl1Point );
        }
        else
        {
            bezierManager.AddPoint( *aStartingPoint, true );
            m_toolMgr->PrimeTool( *aStartingPoint );
        }
    }

    // Main loop: keep receiving events
    while( TOOL_EVENT* evt = Wait() )
    {
        if( started() )
            m_frame->SetMsgPanel( bezier.get() );

        setCursor();

        // Init the new item attributes
        // (non-geometric, those are handled by the manager)
        bezier->SetShape( SHAPE_T::BEZIER );
        bezier->SetStroke( m_stroke );
        bezier->SetLayer( m_layer );

        grid.SetSnap( !evt->Modifier( MD_SHIFT ) );
        grid.SetUseGrid( getView()->GetGAL()->GetGridSnapping() && !evt->DisableGridSnapping() );
        VECTOR2I cursorPos = GetClampedCoords( grid.BestSnapAnchor( m_controls->GetMousePosition(), bezier.get(),
                                                                    GRID_GRAPHICS ),
                                               COORDS_PADDING );
        m_controls->ForceCursorPosition( true, cursorPos );

        if( evt->IsCancelInteractive() || ( started() && evt->IsAction( &ACTIONS::undo ) ) )
        {
            resetProgress();

            if( !started() )
            {
                // We've handled the cancel event.  Don't cancel other tools
                evt->SetPassEvent( false );
                m_frame->PopTool( aTool );
                aResult = DRAW_ONE_RESULT::CANCELLED;
            }
            else
            {
                // We're not cancelling, but we're also not returning a finished bezier
                // So we'll be called again.
                aResult = DRAW_ONE_RESULT::RESET;
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
                resetProgress();
                // leave ourselves on the stack so we come back after the move
                aResult = DRAW_ONE_RESULT::CANCELLED;
                break;
            }
            else
            {
                resetProgress();
                m_frame->PopTool( aTool );
                aResult = DRAW_ONE_RESULT::CANCELLED;
                break;
            }
        }
        else if( evt->IsClick( BUT_LEFT ) || evt->IsDblClick( BUT_LEFT ) )
        {
            if( !started() )
            {
                m_toolMgr->RunAction( ACTIONS::selectionClear );

                m_controls->SetAutoPan( true );
                m_controls->CaptureCursor( true );

                if( !m_view->IsLayerVisible( m_layer ) )
                {
                    m_frame->GetAppearancePanel()->SetLayerVisible( m_layer, true );
                    m_frame->GetCanvas()->Refresh();
                }

                frame()->SetMsgPanel( bezier.get() );
            }

            if( !priming )
                bezierManager.AddPoint( cursorPos, true );
            else
                priming = false;

            const bool doubleClick = evt->IsDblClick( BUT_LEFT );

            if( doubleClick )
            {
                // Use the current point for all remaining points
                while( bezierManager.GetStep() < KIGFX::PREVIEW::BEZIER_GEOM_MANAGER::SET_END )
                    bezierManager.AddPoint( cursorPos, true );
            }

            if( bezierManager.GetStep() == KIGFX::PREVIEW::BEZIER_GEOM_MANAGER::SET_END )
                preview.Add( bezier.get() );

            // Return to the caller for a reset
            if( doubleClick )
            {
                // Don't chain to this one
                aResult = DRAW_ONE_RESULT::ACCEPTED_AND_RESET;
                break;
            }
        }
        else if( evt->IsAction( &PCB_ACTIONS::deleteLastPoint ) )
        {
            bezierManager.RemoveLastPoint();

            if( bezierManager.GetStep() < KIGFX::PREVIEW::BEZIER_GEOM_MANAGER::SET_END )
                preview.Remove( bezier.get() );
        }
        else if( evt->IsMotion() )
        {
            // set angle snap
            // bezierManager.SetAngleSnap( Is45Limited() );

            // update, but don't step the manager state
            bezierManager.AddPoint( cursorPos, false );
        }
        else if( evt->IsAction( &PCB_ACTIONS::layerChanged ) )
        {
            if( m_layer != m_frame->GetActiveLayer() )
            {
                m_layer = m_frame->GetActiveLayer();
                m_stroke.SetWidth( m_frame->GetDesignSettings().GetLineThickness( m_layer ) );
                m_stroke.SetLineStyle( LINE_STYLE::DEFAULT );
                m_stroke.SetColor( COLOR4D::UNSPECIFIED );
            }

            if( !m_view->IsLayerVisible( m_layer ) )
            {
                m_frame->GetAppearancePanel()->SetLayerVisible( m_layer, true );
                m_frame->GetCanvas()->Refresh();
            }

            bezier->SetLayer( m_layer );
            bezier->SetStroke( m_stroke );
            m_view->Update( &preview );
            frame()->SetMsgPanel( bezier.get() );
        }
        else if( evt->IsAction( &PCB_ACTIONS::properties ) )
        {
            // Don't show the edit panel if we can't represent the arc with it
            if( ( bezierManager.GetStep() >= KIGFX::PREVIEW::BEZIER_GEOM_MANAGER::SET_END ) )
            {
                frame()->OnEditItemRequest( bezier.get() );
                m_view->Update( &preview );
                frame()->SetMsgPanel( bezier.get() );
                break;
            }
            else
            {
                evt->SetPassEvent();
            }
        }
        else if( evt->IsClick( BUT_RIGHT ) )
        {
            m_menu->ShowContextMenu( selection() );
        }
        else if( evt->IsAction( &PCB_ACTIONS::incWidth ) )
        {
            m_stroke.SetWidth( m_stroke.GetWidth() + WIDTH_STEP );

            bezier->SetStroke( m_stroke );
            m_view->Update( &preview );
            frame()->SetMsgPanel( bezier.get() );
        }
        else if( evt->IsAction( &PCB_ACTIONS::decWidth ) )
        {
            if( (unsigned) m_stroke.GetWidth() > WIDTH_STEP )
            {
                m_stroke.SetWidth( m_stroke.GetWidth() - WIDTH_STEP );

                bezier->SetStroke( m_stroke );
                m_view->Update( &preview );
                frame()->SetMsgPanel( bezier.get() );
            }
        }
        else if( evt->IsAction( &ACTIONS::updateUnits ) )
        {
            bezierAsst.SetUnits( frame()->GetUserUnits() );
            m_view->Update( &bezierAsst );
            evt->SetPassEvent();
        }
        else if( started() && (   ZONE_FILLER_TOOL::IsZoneFillAction( evt )
                               || evt->IsAction( &ACTIONS::redo ) ) )
        {
            wxBell();
        }
        else
        {
            evt->SetPassEvent();
        }

        if( bezierManager.IsComplete() )
        {
            break;
        }
        else if( bezierManager.HasGeometryChanged() )
        {
            bezier->SetStart( bezierManager.GetStart() );
            bezier->SetBezierC1( bezierManager.GetControlC1() );
            bezier->SetEnd( bezierManager.GetEnd() );
            bezier->SetBezierC2( bezierManager.GetControlC2() );
            bezier->RebuildBezierToSegmentsPointsList( maxError );

            m_view->Update( &preview );
            m_view->Update( &bezierAsst );

            // Once we are receiving end points, we can show the bezier in the preview
            if( bezierManager.GetStep() >= KIGFX::PREVIEW::BEZIER_GEOM_MANAGER::SET_END )
                frame()->SetMsgPanel( bezier.get() );
            else
                frame()->SetMsgPanel( board() );
        }
    }

    preview.Remove( bezier.get() );
    m_view->Remove( &bezierAsst );
    m_view->Remove( &preview );

    if( selection().Empty() )
        m_frame->SetMsgPanel( board() );

    m_frame->GetCanvas()->SetCurrentCursor( KICURSOR::ARROW );
    m_controls->SetAutoPan( false );
    m_controls->CaptureCursor( false );
    m_controls->ForceCursorPosition( false );

    return bezier;
};


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
        m_toolMgr->RunAction( ACTIONS::selectionCursor );
    }

    // we want a single zone
    if( selection.Size() == 1 && selection[0]->Type() == PCB_ZONE_T )
        *aZone = static_cast<ZONE*>( selection[0] );

    // expected a zone, but didn't get one
    if( !*aZone )
    {
        if( clearSelection )
            m_toolMgr->RunAction( ACTIONS::selectionClear );

        return false;
    }

    return true;
}


int DRAWING_TOOL::DrawZone( const TOOL_EVENT& aEvent )
{
    if( m_isFootprintEditor && !m_frame->GetModel() )
        return 0;

    if( m_inDrawingTool )
        return 0;

    REENTRANCY_GUARD guard( &m_inDrawingTool );

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
    params.m_layer = m_frame->GetActiveLayer();

    if( zoneMode == ZONE_MODE::SIMILAR && !sourceZone->IsOnLayer( params.m_layer ) )
        params.m_layer = sourceZone->GetFirstLayer();

    ZONE_CREATE_HELPER   zoneTool( *this, params );
    // the geometry manager which handles the zone geometry, and hands the calculated points
    // over to the zone creator tool
    POLYGON_GEOM_MANAGER polyGeomMgr( zoneTool );
    bool                 started     = false;
    PCB_GRID_HELPER      grid( m_toolMgr, m_frame->GetMagneticItemsSettings() );

    m_frame->PushTool( aEvent );

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

    Activate();
    // Must be done after Activate() so that it gets set into the correct context
    m_controls->ShowCursor( true );
    m_controls->ForceCursorPosition( false );
    // Set initial cursor
    setCursor();

    if( aEvent.HasPosition() )
        m_toolMgr->PrimeTool( aEvent.Position() );

    // Main loop: keep receiving events
    while( TOOL_EVENT* evt = Wait() )
    {
        setCursor();

        LSET layers( { m_frame->GetActiveLayer() } );
        grid.SetSnap( !evt->Modifier( MD_SHIFT ) );
        LEADER_MODE angleSnap = GetAngleSnapMode();

        if( evt->Modifier( MD_CTRL ) )
            angleSnap = LEADER_MODE::DIRECT;

        grid.SetUseGrid( getView()->GetGAL()->GetGridSnapping() && !evt->DisableGridSnapping() );

        VECTOR2I cursorPos = evt->HasPosition() ? evt->Position() : m_controls->GetMousePosition();
        cursorPos = GetClampedCoords( grid.BestSnapAnchor( cursorPos, layers, GRID_GRAPHICS ), COORDS_PADDING );

        m_controls->ForceCursorPosition( true, cursorPos );

        polyGeomMgr.SetLeaderMode( angleSnap );

        if( evt->IsCancelInteractive() )
        {
            if( started )
            {
                cleanup();
            }
            else
            {
                m_frame->PopTool( aEvent );

                // We've handled the cancel event.  Don't cancel other tools
                evt->SetPassEvent( false );
                break;
            }
        }
        else if( evt->IsActivate() )
        {
            if( started )
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
                m_frame->PopTool( aEvent );
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
            if( !started )
                m_toolMgr->VetoContextMenuMouseWarp();

            m_menu->ShowContextMenu( selection() );
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

                cleanup();
                m_frame->PopTool( aEvent );
                break;
            }
            // adding a corner
            else if( polyGeomMgr.AddPoint( cursorPos ) )
            {
                if( !started )
                {
                    started = true;

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
        else if( started && (   evt->IsAction( &PCB_ACTIONS::deleteLastPoint )
                             || evt->IsAction( &ACTIONS::doDelete )
                             || evt->IsAction( &ACTIONS::undo ) ) )
        {
            if( std::optional<VECTOR2I> last = polyGeomMgr.DeleteLastCorner() )
            {
                cursorPos = last.value();
                getViewControls()->WarpMouseCursor( cursorPos, true );
                m_controls->ForceCursorPosition( true, cursorPos );
                polyGeomMgr.SetCursorPosition( cursorPos );
            }
            else
            {
                cleanup();
            }
        }
        else if( started && (   evt->IsMotion()
                             || evt->IsDrag( BUT_LEFT ) ) )
        {
            polyGeomMgr.SetCursorPosition( cursorPos );
        }
        else if( started && (   ZONE_FILLER_TOOL::IsZoneFillAction( evt )
                             || evt->IsAction( &ACTIONS::redo ) ) )
        {
            wxBell();
        }
        else if( started && evt->IsAction( &PCB_ACTIONS::properties ) )
        {
            frame()->OnEditItemRequest( zoneTool.GetZone() );
            zoneTool.OnGeometryChange( polyGeomMgr );
            frame()->SetMsgPanel( zoneTool.GetZone() );
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
    if( m_isFootprintEditor )
        return 0;

    if( m_inDrawingTool )
        return 0;

    REENTRANCY_GUARD guard( &m_inDrawingTool );

    struct VIA_PLACER : public INTERACTIVE_PLACER_BASE
    {
        PCB_BASE_EDIT_FRAME*        m_frame;
        PCB_GRID_HELPER             m_gridHelper;
        std::shared_ptr<DRC_ENGINE> m_drcEngine;
        int                         m_drcEpsilon;
        int                         m_worstClearance;
        bool                        m_allowDRCViolations;

        VIA_PLACER( PCB_BASE_EDIT_FRAME* aFrame ) :
                m_frame( aFrame ),
                m_gridHelper( aFrame->GetToolManager(), aFrame->GetMagneticItemsSettings() ),
                m_drcEngine( aFrame->GetBoard()->GetDesignSettings().m_DRCEngine ),
                m_drcEpsilon( aFrame->GetBoard()->GetDesignSettings().GetDRCEpsilon() ),
                m_worstClearance( 0 )
        {
            ROUTER_TOOL* router = m_frame->GetToolManager()->GetTool<ROUTER_TOOL>();

            if( router )
                m_allowDRCViolations = router->Router()->Settings().AllowDRCViolations();

            try
            {
                if( aFrame )
                    m_drcEngine->InitEngine( aFrame->GetDesignRulesPath() );

                DRC_CONSTRAINT constraint;

                if( m_drcEngine->QueryWorstConstraint( CLEARANCE_CONSTRAINT, constraint ) )
                    m_worstClearance = constraint.GetValue().Min();

                if( m_drcEngine->QueryWorstConstraint( HOLE_CLEARANCE_CONSTRAINT, constraint ) )
                    m_worstClearance = std::max( m_worstClearance, constraint.GetValue().Min() );

                for( FOOTPRINT* footprint : aFrame->GetBoard()->Footprints() )
                {
                    for( PAD* pad : footprint->Pads() )
                    {
                        std::optional<int> padOverride = pad->GetClearanceOverrides( nullptr );

                        if( padOverride.has_value() )
                            m_worstClearance = std::max( m_worstClearance, padOverride.value() );
                    }
                }
            }
            catch( PARSE_ERROR& )
            {
            }
        }

        virtual ~VIA_PLACER()
        {
        }

        /**
         * Get the bounding box the via would have if placed at the given position
         * (the via's bounding box is relative to its own position).
         */
        static BOX2I getEffectiveBoundingBox( const PCB_VIA& aVia, const VECTOR2I& aPosition )
        {
            BOX2I bbox = aVia.GetBoundingBox();
            bbox.Move( aPosition - aVia.GetPosition() );
            return bbox;
        }

        PCB_TRACK* findTrack( const PCB_VIA* aVia, const VECTOR2I& aPosition ) const
        {
            const LSET  lset = aVia->GetLayerSet();
            const BOX2I bbox = getEffectiveBoundingBox( *aVia, aPosition );

            std::vector<KIGFX::VIEW::LAYER_ITEM_PAIR> items;
            KIGFX::PCB_VIEW*        view = m_frame->GetCanvas()->GetView();
            std::vector<PCB_TRACK*> possible_tracks;

            wxCHECK( view, nullptr );

            view->Query( bbox, items );

            for( const KIGFX::VIEW::LAYER_ITEM_PAIR& it : items )
            {
                if( !it.first->IsBOARD_ITEM() )
                    continue;

                BOARD_ITEM* item = static_cast<BOARD_ITEM*>( it.first );

                if( !( item->GetLayerSet() & lset ).any() )
                    continue;

                if( item->Type() == PCB_TRACE_T )
                {
                    PCB_TRACK* track = static_cast<PCB_TRACK*>( item );

                    if( TestSegmentHit( aPosition, track->GetStart(), track->GetEnd(),
                                        ( track->GetWidth() + aVia->GetWidth( track->GetLayer() ) ) / 2 ) )
                    {
                        possible_tracks.push_back( track );
                    }
                }
                else if( item->Type() == PCB_ARC_T )
                {
                    PCB_ARC* arc = static_cast<PCB_ARC*>( item );

                    if( arc->HitTest( aPosition, aVia->GetWidth( arc->GetLayer() ) / 2 ) )
                        possible_tracks.push_back( arc );
                }
            }

            PCB_TRACK* return_track = nullptr;
            int min_d = std::numeric_limits<int>::max();

            for( PCB_TRACK* track : possible_tracks )
            {
                SEG test( track->GetStart(), track->GetEnd() );
                int dist = ( test.NearestPoint( aPosition ) - aPosition ).EuclideanNorm();

                if( dist < min_d )
                {
                    min_d = dist;
                    return_track = track;
                }
            }

            return return_track;
        }

        bool hasDRCViolation( PCB_VIA* aVia, BOARD_ITEM* aOther )
        {
            DRC_CONSTRAINT        constraint;
            int                   clearance;
            BOARD_CONNECTED_ITEM* connectedItem = dynamic_cast<BOARD_CONNECTED_ITEM*>( aOther );
            ZONE*                 zone = dynamic_cast<ZONE*>( aOther );

            if( zone && zone->GetIsRuleArea() )
            {
                if( zone->GetDoNotAllowVias() )
                {
                    bool hit = false;

                    aVia->Padstack().ForEachUniqueLayer(
                            [&]( PCB_LAYER_ID aLayer )
                            {
                                if( hit )
                                    return;

                                if( zone->Outline()->Collide( aVia->GetPosition(), aVia->GetWidth( aLayer ) / 2 ) )
                                    hit = true;
                            } );

                    return hit;
                }

                return false;
            }

            if( connectedItem )
            {
                int connectedItemNet = connectedItem->GetNetCode();

                if( connectedItemNet == 0 || connectedItemNet == aVia->GetNetCode() )
                    return false;
            }

            for( PCB_LAYER_ID layer : aOther->GetLayerSet() )
            {
                // Reference images are "on" a copper layer but are not actually part of it
                if( !IsCopperLayer( layer ) || aOther->Type() == PCB_REFERENCE_IMAGE_T )
                    continue;

                constraint = m_drcEngine->EvalRules( CLEARANCE_CONSTRAINT, aVia,  aOther, layer );
                clearance = constraint.GetValue().Min();

                if( clearance >= 0 )
                {
                    std::shared_ptr<SHAPE> viaShape = aVia->GetEffectiveShape( layer );
                    std::shared_ptr<SHAPE> otherShape = aOther->GetEffectiveShape( layer );

                    if( viaShape->Collide( otherShape.get(), clearance - m_drcEpsilon ) )
                        return true;
                }
            }

            if( aOther->HasHole() )
            {
                constraint = m_drcEngine->EvalRules( HOLE_CLEARANCE_CONSTRAINT, aVia, aOther, UNDEFINED_LAYER );
                clearance = constraint.GetValue().Min();

                if( clearance >= 0 )
                {
                    std::shared_ptr<SHAPE> viaShape = aVia->GetEffectiveShape( UNDEFINED_LAYER );

                    if( viaShape->Collide( aOther->GetEffectiveHoleShape().get(), clearance - m_drcEpsilon ) )
                        return true;
                }
            }

            return false;
        }

        bool checkDRCViolation( PCB_VIA* aVia )
        {
            std::vector<KIGFX::VIEW::LAYER_ITEM_PAIR> items;
            std::set<BOARD_ITEM*> checkedItems;
            BOX2I bbox = aVia->GetBoundingBox();

            bbox.Inflate( m_worstClearance );
            m_frame->GetCanvas()->GetView()->Query( bbox, items );

            for( std::pair<KIGFX::VIEW_ITEM*, int> it : items )
            {
                if( !it.first->IsBOARD_ITEM() )
                    continue;

                BOARD_ITEM* item = static_cast<BOARD_ITEM*>( it.first );

                if( item->Type() == PCB_ZONE_T && !static_cast<ZONE*>( item )->GetIsRuleArea() )
                {
                    continue;       // stitching vias bind to zones, so ignore them
                }
                else if( item->Type() == PCB_FOOTPRINT_T || item->Type() == PCB_GROUP_T )
                {
                    continue;       // check against children, but not against footprint itself
                }
                else if( ( item->Type() == PCB_FIELD_T || item->Type() == PCB_TEXT_T )
                         && !static_cast<PCB_TEXT*>( item )->IsVisible() )
                {
                    continue;       // ignore hidden items
                }
                else if( checkedItems.count( item ) )
                {
                    continue;       // already checked
                }

                if( hasDRCViolation( aVia, item ) )
                    return true;

                checkedItems.insert( item );
            }

            DRC_CONSTRAINT constraint = m_drcEngine->EvalRules( DISALLOW_CONSTRAINT, aVia, nullptr,
                                                                UNDEFINED_LAYER );

            if( constraint.m_DisallowFlags && constraint.GetSeverity() != RPT_SEVERITY_IGNORE )
                return true;

            return false;
        }

        PAD* findPad( const PCB_VIA* aVia, const VECTOR2I& aPosition ) const
        {
            const LSET  lset = aVia->GetLayerSet();
            const BOX2I bbox = getEffectiveBoundingBox( *aVia, aPosition );

            const KIGFX::PCB_VIEW&                    view = *m_frame->GetCanvas()->GetView();
            std::vector<KIGFX::VIEW::LAYER_ITEM_PAIR> items;

            view.Query( bbox, items );

            for( const KIGFX::VIEW::LAYER_ITEM_PAIR& it : items )
            {
                if( !it.first->IsBOARD_ITEM() )
                    continue;

                BOARD_ITEM& item = static_cast<BOARD_ITEM&>( *it.first );

                if( item.Type() == PCB_PAD_T && ( item.GetLayerSet() & lset ).any() )
                {
                    PAD& pad = static_cast<PAD&>( item );

                    if( pad.HitTest( aPosition ) )
                        return &pad;
                }
            }

            return nullptr;
        }

        PCB_SHAPE* findGraphic( const PCB_VIA* aVia, const VECTOR2I& aPosition ) const
        {
            const LSET lset = aVia->GetLayerSet() & LSET::AllCuMask();
            BOX2I      bbox = getEffectiveBoundingBox( *aVia, aPosition );

            std::vector<KIGFX::VIEW::LAYER_ITEM_PAIR> items;
            KIGFX::PCB_VIEW* view = m_frame->GetCanvas()->GetView();
            PCB_LAYER_ID activeLayer = m_frame->GetActiveLayer();
            std::vector<PCB_SHAPE*> possible_shapes;

            view->Query( bbox, items );

            for( const KIGFX::VIEW::LAYER_ITEM_PAIR& it : items )
            {
                if( !it.first->IsBOARD_ITEM() )
                    continue;

                BOARD_ITEM* item = static_cast<BOARD_ITEM*>( it.first );

                if( !( item->GetLayerSet() & lset ).any() )
                    continue;

                if( item->Type() == PCB_SHAPE_T )
                {
                    PCB_SHAPE* shape = static_cast<PCB_SHAPE*>( item );

                    if( shape->HitTest( aPosition, aVia->GetWidth( activeLayer ) / 2 ) )
                        possible_shapes.push_back( shape );
                }
            }

            PCB_SHAPE* return_shape = nullptr;
            int min_d = std::numeric_limits<int>::max();

            for( PCB_SHAPE* shape : possible_shapes )
            {
                int dist = ( shape->GetPosition() - aPosition ).EuclideanNorm();

                if( dist < min_d )
                {
                    min_d = dist;
                    return_shape = shape;
                }
            }

            return return_shape;
        }

        std::optional<int> selectPossibleNetsByPopupMenu( std::set<int>& aNetcodeList )
        {
            ACTION_MENU         menu( true );
            const NETINFO_LIST& netInfo = m_board->GetNetInfo();
            std::map<int, int>  menuIDNetCodeMap;
            int                 menuID = 1;

            for( int netcode : aNetcodeList )
            {
                wxString menuText;
                if( menuID < 10 )
                {
#ifdef __WXMAC__
                    menuText = wxString::Format( "%s\t",
                                                 netInfo.GetNetItem( netcode )->GetNetname() );
#else
                    menuText = wxString::Format( "&%d  %s\t",
                                                 menuID,
                                                 netInfo.GetNetItem( netcode )->GetNetname() );
#endif
                }
                else
                {
                    menuText = netInfo.GetNetItem( netcode )->GetNetname();
                }

                menu.Add( menuText, menuID, BITMAPS::INVALID_BITMAP );
                menuIDNetCodeMap[ menuID ] = netcode;
                menuID++;
            }

            menu.SetTitle( _( "Select Net:" ) );
            menu.DisplayTitle( true );

            DRAWING_TOOL* drawingTool = m_frame->GetToolManager()->GetTool<DRAWING_TOOL>();
            drawingTool->SetContextMenu( &menu, CMENU_NOW );

            int  selectedNetCode = -1;
            bool cancelled = false;

            while( TOOL_EVENT* evt = drawingTool->Wait() )
            {
                if( evt->Action() == TA_CHOICE_MENU_UPDATE )
                {
                    evt->SetPassEvent();
                }
                else if( evt->Action() == TA_CHOICE_MENU_CHOICE )
                {
                    std::optional<int> id = evt->GetCommandId();

                    // User has selected an item, so this one will be returned
                    if( id && ( *id > 0 ) && ( *id < menuID ) )
                    {
                        selectedNetCode = menuIDNetCodeMap.at( *id );
                    }
                    // User has cancelled the menu (either by <esc> or clicking out of it),
                    else
                    {
                        cancelled = true;
                    }
                }
                else if( evt->Action() == TA_CHOICE_MENU_CLOSED )
                {
                    break;
                }
            }

            if( cancelled )
                return std::optional<int>();
            else
                return selectedNetCode;
        }

        std::optional<int> findStitchedZoneNet( PCB_VIA* aVia )
        {
            const VECTOR2I      position = aVia->GetPosition();
            PCB_DISPLAY_OPTIONS opts = m_frame->GetDisplayOptions();
            std::set<int>       netcodeList;

            // See if there are any connections available on a high-contrast layer
            if( opts.m_ContrastModeDisplay == HIGH_CONTRAST_MODE::DIMMED
                || opts.m_ContrastModeDisplay == HIGH_CONTRAST_MODE::HIDDEN )
            {
                if( aVia->GetLayerSet().test( m_frame->GetActiveLayer() ) )
                {
                    for( ZONE* z : m_board->Zones() )
                    {
                        if( z->IsOnLayer( m_frame->GetActiveLayer() ) )
                        {
                            if( z->HitTestFilledArea( m_frame->GetActiveLayer(), position ) )
                                netcodeList.insert( z->GetNetCode() );
                        }
                    }
                }
            }

            // If there's only one, return it.
            if( netcodeList.size() == 1 )
                return *netcodeList.begin();

            // See if there are any connections available on a visible layer
            LSET lset = LSET( m_board->GetVisibleLayers() & aVia->GetLayerSet() );

            for( ZONE* z : m_board->Zones() )
            {
                if( z->GetIsRuleArea() )
                    continue; // ignore rule areas

                for( PCB_LAYER_ID layer : lset )
                {
                    if( z->IsOnLayer( layer ) )
                    {
                        if( z->HitTestFilledArea( layer, position ) )
                            netcodeList.insert( z->GetNetCode() );
                    }
                }
            }

            // If there's only one, return it.
            if( netcodeList.size() == 1 )
                return *netcodeList.begin();

            if( netcodeList.size() > 1 )
            {
                // The net assignment is ambiguous.  Let the user decide.
                return selectPossibleNetsByPopupMenu( netcodeList );
            }
            else
            {
                return NETINFO_LIST::ORPHANED;
            }
        }

        void SnapItem( BOARD_ITEM *aItem ) override
        {
            m_gridHelper.SetSnap( !( m_modifiers & MD_SHIFT ) );

            MAGNETIC_SETTINGS* settings = m_frame->GetMagneticItemsSettings();
            PCB_VIA*           via = static_cast<PCB_VIA*>( aItem );

            // When snapping, use the mouse position, not the item position, which may be
            // grid-snapped, so that we can get the cursor within snap-range of snap points.
            // If we don't get a snap, the via will be left as it is (i.e. maybe grid-snapped).
            KIGFX::VIEW_CONTROLS& viewControls = *m_frame->GetCanvas()->GetViewControls();
            const VECTOR2I        position = viewControls.GetMousePosition();

            if( settings->tracks != MAGNETIC_OPTIONS::NO_EFFECT && m_gridHelper.GetSnap() )
            {
                if( PCB_TRACK* track = findTrack( via, position ) )
                {
                    SEG      trackSeg( track->GetStart(), track->GetEnd() );
                    VECTOR2I snap = m_gridHelper.AlignToSegment( position, trackSeg );

                    aItem->SetPosition( snap );
                    return;
                }
            }

            if( settings->pads != MAGNETIC_OPTIONS::NO_EFFECT && m_gridHelper.GetSnap() )
            {
                if( PAD* pad = findPad( via, position ) )
                {
                    aItem->SetPosition( pad->GetPosition() );
                    return;
                }
            }

            if( settings->graphics && m_gridHelper.GetSnap() )
            {
                if( PCB_SHAPE* shape = findGraphic( via, position ) )
                {
                    if( shape->IsAnyFill() )
                    {
                        // Is this shape something to be replaced by the via, or something to be
                        // stitched by multiple vias?  Use an area-based test to make a guess.
                        SHAPE_POLY_SET poly;
                        shape->TransformShapeToPolygon( poly, shape->GetLayer(), 0, ARC_LOW_DEF, ERROR_INSIDE );
                        double shapeArea = poly.Area();

                        int    R = via->GetWidth( shape->GetLayer() ) / 2;
                        double viaArea = M_PI * R * R;

                        if( viaArea * 4 > shapeArea )
                            aItem->SetPosition( shape->GetPosition() );
                    }
                    else
                    {
                        switch( shape->GetShape() )
                        {
                        case SHAPE_T::SEGMENT:
                        {
                            SEG seg( shape->GetStart(), shape->GetEnd() );
                            VECTOR2I snap = m_gridHelper.AlignToSegment( position, seg );
                            aItem->SetPosition( snap );
                            break;
                        }

                        case SHAPE_T::ARC:
                        {
                            if( ( shape->GetEnd() - position ).SquaredEuclideanNorm() <
                                ( shape->GetStart() - position ).SquaredEuclideanNorm() )
                            {
                                aItem->SetPosition( shape->GetEnd() );
                            }
                            else
                            {
                                aItem->SetPosition( shape->GetStart() );
                            }

                            break;
                        }

                        case SHAPE_T::POLY:
                        {
                            if( !shape->IsPolyShapeValid() )
                            {
                                aItem->SetPosition( shape->GetPosition() );
                                break;
                            }

                            const SHAPE_POLY_SET& polySet = shape->GetPolyShape();
                            std::optional<SEG> nearestSeg;
                            int minDist = std::numeric_limits<int>::max();

                            for( int ii = 0; ii < polySet.OutlineCount(); ++ii )
                            {
                                const SHAPE_LINE_CHAIN& poly = polySet.Outline( ii );

                                for( int jj = 0; jj < poly.SegmentCount(); ++jj )
                                {
                                    const SEG& seg = poly.GetSegment( jj );
                                    int dist = seg.Distance( position );

                                    if( dist < minDist )
                                    {
                                        minDist = dist;
                                        nearestSeg = seg;
                                    }
                                }
                            }

                            if( nearestSeg )
                            {
                                VECTOR2I snap = m_gridHelper.AlignToSegment( position, *nearestSeg );
                                aItem->SetPosition( snap );
                            }

                            break;
                        }

                        default:
                            aItem->SetPosition( shape->GetPosition() );
                        }

                    }
                }

            }
        }

        bool PlaceItem( BOARD_ITEM* aItem, BOARD_COMMIT& aCommit ) override
        {
            WX_INFOBAR* infobar = m_frame->GetInfoBar();
            PCB_VIA*    via = static_cast<PCB_VIA*>( aItem );
            VECTOR2I    viaPos = via->GetPosition();
            PCB_TRACK*  track = findTrack( via, via->GetPosition() );
            PAD*        pad = findPad( via, via->GetPosition() );
            PCB_SHAPE*  shape = findGraphic( via, via->GetPosition() );

            if( track )
            {
                via->SetNetCode( track->GetNetCode() );
                via->SetIsFree( false );
            }
            else if( pad )
            {
                via->SetNetCode( pad->GetNetCode() );
                via->SetIsFree( false );
            }
            else if( shape && shape->GetNetCode() > 0 )
            {
                via->SetNetCode( shape->GetNetCode() );
                via->SetIsFree( false );
            }
            else
            {
                std::optional<int> netcode = findStitchedZoneNet( via );

                if( !netcode.has_value() )  // user cancelled net disambiguation menu
                    return false;

                via->SetNetCode( netcode.value() );
                via->SetIsFree( via->GetNetCode() > 0 );
            }

            if( checkDRCViolation( via ) )
            {
                m_frame->ShowInfoBarError( _( "Via location violates DRC." ), true,
                                           WX_INFOBAR::MESSAGE_TYPE::DRC_VIOLATION );

                if( !m_allowDRCViolations )
                    return false;
            }
            else
            {
                if( infobar->GetMessageType() == WX_INFOBAR::MESSAGE_TYPE::DRC_VIOLATION )
                    infobar->Dismiss();
            }

            aCommit.Add( via );

            // If the user explicitly disables snap (using shift), then don't break the tracks.
            // This will prevent PNS from being able to connect the via and track but
            // it is explicitly requested by the user
            if( track && m_gridHelper.GetSnap() )
            {
                VECTOR2I trackStart = track->GetStart();
                VECTOR2I trackEnd = track->GetEnd();
                SEG      trackSeg( trackStart, trackEnd );

                if( viaPos == trackStart || viaPos == trackEnd )
                    return true;

                if( !trackSeg.Contains( viaPos ) )
                    return true;

                aCommit.Modify( track );
                track->SetStart( trackStart );
                track->SetEnd( viaPos );

                PCB_TRACK* newTrack = dynamic_cast<PCB_TRACK*>( track->Clone() );
                const_cast<KIID&>( newTrack->m_Uuid ) = KIID();

                newTrack->SetStart( viaPos );
                newTrack->SetEnd( trackEnd );
                aCommit.Add( newTrack );
            }

            return true;
        }

        std::unique_ptr<BOARD_ITEM> CreateItem() override
        {
            BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();
            PCB_VIA*               via = new PCB_VIA( m_board );

            via->SetNetCode( 0 );
            via->SetViaType( bds.m_CurrentViaType );

            if( via->GetViaType() == VIATYPE::THROUGH )
            {
                via->SetLayerPair( B_Cu, F_Cu );
            }
            else
            {
                PCB_LAYER_ID first_layer = m_frame->GetActiveLayer();
                PCB_LAYER_ID last_layer;

                // prepare switch to new active layer:
                if( first_layer != m_frame->GetScreen()->m_Route_Layer_TOP )
                    last_layer = m_frame->GetScreen()->m_Route_Layer_TOP;
                else
                    last_layer = m_frame->GetScreen()->m_Route_Layer_BOTTOM;

                via->SetLayerPair( first_layer, last_layer );
            }

            if( via->GetViaType() == VIATYPE::MICROVIA )
            {
                via->SetWidth( PADSTACK::ALL_LAYERS,
                               via->GetEffectiveNetClass()->GetuViaDiameter() );
                via->SetDrill( via->GetEffectiveNetClass()->GetuViaDrill() );
            }
            else
            {
                via->SetWidth( PADSTACK::ALL_LAYERS, bds.GetCurrentViaSize() );
                via->SetDrill( bds.GetCurrentViaDrill() );
            }

            return std::unique_ptr<BOARD_ITEM>( via );
        }
    };

    VIA_PLACER placer( frame() );

    SCOPED_DRAW_MODE scopedDrawMode( m_mode, MODE::VIA );

    doInteractiveItemPlacement( aEvent, &placer, _( "Place via" ), IPO_REPEAT | IPO_SINGLE_CLICK );

    return 0;
}


const unsigned int DRAWING_TOOL::WIDTH_STEP = pcbIUScale.mmToIU( 0.1 );


void DRAWING_TOOL::setTransitions()
{
    // clang-format off
    Go( &DRAWING_TOOL::DrawLine,              PCB_ACTIONS::drawLine.MakeEvent() );
    Go( &DRAWING_TOOL::DrawZone,              PCB_ACTIONS::drawPolygon.MakeEvent() );
    Go( &DRAWING_TOOL::DrawRectangle,         PCB_ACTIONS::drawRectangle.MakeEvent() );
    Go( &DRAWING_TOOL::DrawCircle,            PCB_ACTIONS::drawCircle.MakeEvent() );
    Go( &DRAWING_TOOL::DrawArc,               PCB_ACTIONS::drawArc.MakeEvent() );
    Go( &DRAWING_TOOL::DrawBezier,            PCB_ACTIONS::drawBezier.MakeEvent() );
    Go( &DRAWING_TOOL::DrawDimension,         PCB_ACTIONS::drawAlignedDimension.MakeEvent() );
    Go( &DRAWING_TOOL::DrawDimension,         PCB_ACTIONS::drawOrthogonalDimension.MakeEvent() );
    Go( &DRAWING_TOOL::DrawDimension,         PCB_ACTIONS::drawCenterDimension.MakeEvent() );
    Go( &DRAWING_TOOL::DrawDimension,         PCB_ACTIONS::drawRadialDimension.MakeEvent() );
    Go( &DRAWING_TOOL::DrawDimension,         PCB_ACTIONS::drawLeader.MakeEvent() );
    Go( &DRAWING_TOOL::DrawBarcode,           PCB_ACTIONS::placeBarcode.MakeEvent() );
    Go( &DRAWING_TOOL::DrawZone,              PCB_ACTIONS::drawZone.MakeEvent() );
    Go( &DRAWING_TOOL::DrawZone,              PCB_ACTIONS::drawRuleArea.MakeEvent() );
    Go( &DRAWING_TOOL::DrawZone,              PCB_ACTIONS::drawZoneCutout.MakeEvent() );
    Go( &DRAWING_TOOL::DrawZone,              PCB_ACTIONS::drawSimilarZone.MakeEvent() );
    Go( &DRAWING_TOOL::DrawVia,               PCB_ACTIONS::drawVia.MakeEvent() );
    Go( &DRAWING_TOOL::PlacePoint,            PCB_ACTIONS::placePoint.MakeEvent() );
    Go( &DRAWING_TOOL::PlaceReferenceImage,   PCB_ACTIONS::placeReferenceImage.MakeEvent() );
    Go( &DRAWING_TOOL::PlaceText,             PCB_ACTIONS::placeText.MakeEvent() );
    Go( &DRAWING_TOOL::DrawTable,             PCB_ACTIONS::drawTable.MakeEvent() );
    Go( &DRAWING_TOOL::DrawRectangle,         PCB_ACTIONS::drawTextBox.MakeEvent() );
    Go( &DRAWING_TOOL::PlaceImportedGraphics, PCB_ACTIONS::placeImportedGraphics.MakeEvent() );
    Go( &DRAWING_TOOL::PlaceImportedGraphics, PCB_ACTIONS::ddImportGraphics.MakeEvent() );
    Go( &DRAWING_TOOL::SetAnchor,             PCB_ACTIONS::setAnchor.MakeEvent() );

    Go( &DRAWING_TOOL::PlaceTuningPattern,    PCB_ACTIONS::tuneSingleTrack.MakeEvent() );
    Go( &DRAWING_TOOL::PlaceTuningPattern,    PCB_ACTIONS::tuneDiffPair.MakeEvent() );
    Go( &DRAWING_TOOL::PlaceTuningPattern,    PCB_ACTIONS::tuneSkew.MakeEvent() );
    // clang-format on
}
