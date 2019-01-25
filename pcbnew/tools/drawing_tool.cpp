/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014-2017 CERN
 * Copyright (C) 2018-2019 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <class_draw_panel_gal.h>
#include <project.h>
#include <id.h>
#include <pcbnew_id.h>
#include <confirm.h>
#include <import_gfx/dialog_import_gfx.h>

#include <view/view_group.h>
#include <view/view_controls.h>
#include <view/view.h>
#include <gal/graphics_abstraction_layer.h>
#include <tool/tool_manager.h>
#include <geometry/geometry_utils.h>
#include <ratsnest_data.h>
#include <board_commit.h>
#include <scoped_set_reset.h>
#include <bitmaps.h>
#include <hotkeys.h>
#include <painter.h>
#include <status_popup.h>
#include "grid_helper.h"
#include <dialogs/dialog_text_properties.h>
#include <preview_items/arc_assistant.h>

#include <class_board.h>
#include <class_edge_mod.h>
#include <class_pcb_text.h>
#include <class_dimension.h>
#include <class_zone.h>
#include <class_module.h>

#include <tools/selection_tool.h>
#include <tools/tool_event_utils.h>
#include <tools/zone_create_helper.h>

using SCOPED_DRAW_MODE = SCOPED_SET_RESET<DRAWING_TOOL::MODE>;

// Drawing tool actions
TOOL_ACTION PCB_ACTIONS::drawLine( "pcbnew.InteractiveDrawing.line",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_ADD_LINE ),
        _( "Draw Line" ), _( "Draw a line" ), NULL, AF_ACTIVATE );

TOOL_ACTION PCB_ACTIONS::drawGraphicPolygon( "pcbnew.InteractiveDrawing.graphicPolygon",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_ADD_POLYGON ),
        _( "Draw Graphic Polygon" ), _( "Draw a graphic polygon" ), NULL, AF_ACTIVATE );

TOOL_ACTION PCB_ACTIONS::drawCircle( "pcbnew.InteractiveDrawing.circle",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_ADD_CIRCLE ),
        _( "Draw Circle" ), _( "Draw a circle" ), NULL, AF_ACTIVATE );

TOOL_ACTION PCB_ACTIONS::drawArc( "pcbnew.InteractiveDrawing.arc",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_ADD_ARC ),
        _( "Draw Arc" ), _( "Draw an arc" ), NULL, AF_ACTIVATE );

TOOL_ACTION PCB_ACTIONS::placeText( "pcbnew.InteractiveDrawing.text",
        AS_GLOBAL,  TOOL_ACTION::LegacyHotKey( HK_ADD_TEXT ),
        _( "Add Text" ), _( "Add a text" ), NULL, AF_ACTIVATE );

TOOL_ACTION PCB_ACTIONS::drawDimension( "pcbnew.InteractiveDrawing.dimension",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_ADD_DIMENSION ),
        _( "Add Dimension" ), _( "Add a dimension" ), NULL, AF_ACTIVATE );

TOOL_ACTION PCB_ACTIONS::drawZone( "pcbnew.InteractiveDrawing.zone",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_ADD_ZONE ),
        _( "Add Filled Zone" ), _( "Add a filled zone" ), NULL, AF_ACTIVATE );

TOOL_ACTION PCB_ACTIONS::drawVia( "pcbnew.InteractiveDrawing.via",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_ADD_FREE_VIA ),
        _( "Add Vias" ), _( "Add free-standing vias" ), NULL, AF_ACTIVATE );

TOOL_ACTION PCB_ACTIONS::drawZoneKeepout( "pcbnew.InteractiveDrawing.keepout",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_ADD_KEEPOUT ),
        _( "Add Keepout Area" ), _( "Add a keepout area" ), NULL, AF_ACTIVATE );

TOOL_ACTION PCB_ACTIONS::drawZoneCutout( "pcbnew.InteractiveDrawing.zoneCutout",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_ADD_CUTOUT ),
        _( "Add a Zone Cutout" ), _( "Add a cutout area of an existing zone" ),
        add_zone_cutout_xpm, AF_ACTIVATE );

TOOL_ACTION PCB_ACTIONS::drawSimilarZone( "pcbnew.InteractiveDrawing.similarZone",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_ADD_SIMILAR_ZONE ),
        _( "Add a Similar Zone" ), _( "Add a zone with the same settings as an existing zone" ),
        add_zone_xpm, AF_ACTIVATE );

TOOL_ACTION PCB_ACTIONS::placeImportedGraphics( "pcbnew.InteractiveDrawing.placeImportedGraphics",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_ADD_DXF ),
        "Place Imported Graphics", "", NULL, AF_ACTIVATE );

TOOL_ACTION PCB_ACTIONS::setAnchor( "pcbnew.InteractiveDrawing.setAnchor",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_ADD_ANCHOR ),
        _( "Place the Footprint Anchor" ), _( "Place the footprint anchor" ),
        NULL, AF_ACTIVATE );

TOOL_ACTION PCB_ACTIONS::incWidth( "pcbnew.InteractiveDrawing.incWidth",
        AS_CONTEXT, TOOL_ACTION::LegacyHotKey( HK_INC_LINE_WIDTH ),
        _( "Increase Line Width" ), _( "Increase the line width" ) );

TOOL_ACTION PCB_ACTIONS::decWidth( "pcbnew.InteractiveDrawing.decWidth",
        AS_CONTEXT, TOOL_ACTION::LegacyHotKey( HK_DEC_LINE_WIDTH ),
        _( "Decrease Line Width" ), _( "Decrease the line width" ) );

TOOL_ACTION PCB_ACTIONS::arcPosture( "pcbnew.InteractiveDrawing.arcPosture",
        AS_CONTEXT, TOOL_ACTION::LegacyHotKey( HK_SWITCH_TRACK_POSTURE ),
        _( "Switch Arc Posture" ), _( "Switch the arc posture" ) );

/*
 * Contextual actions
 */

static TOOL_ACTION deleteLastPoint( "pcbnew.InteractiveDrawing.deleteLastPoint",
        AS_CONTEXT, WXK_BACK,
        _( "Delete Last Point" ), _( "Delete the last point added to the current item" ),
        undo_xpm );

static TOOL_ACTION closeZoneOutline( "pcbnew.InteractiveDrawing.closeZoneOutline",
        AS_CONTEXT, 0,
        _( "Close Zone Outline" ), _( "Close the outline of a zone in progress" ),
        checked_ok_xpm );


DRAWING_TOOL::DRAWING_TOOL() :
    PCB_TOOL( "pcbnew.InteractiveDrawing" ),
    m_view( nullptr ), m_controls( nullptr ),
    m_board( nullptr ), m_frame( nullptr ), m_mode( MODE::NONE ),
    m_lineWidth( 1 )
{
}


DRAWING_TOOL::~DRAWING_TOOL()
{
}


bool DRAWING_TOOL::Init()
{
    auto activeToolFunctor = [ this ] ( const SELECTION& aSel ) {
                                 return m_mode != MODE::NONE;
                             };

    // some interactive drawing tools can undo the last point
    auto canUndoPoint = [ this ] ( const SELECTION& aSel ) {
                            return m_mode == MODE::ARC || m_mode == MODE::ZONE;
                        };

    // functor for zone-only actions
    auto zoneActiveFunctor = [this ] ( const SELECTION& aSel ) {
                                 return m_mode == MODE::ZONE;
                             };

    auto& ctxMenu = m_menu.GetMenu();

    // cancel current tool goes in main context menu at the top if present
    ctxMenu.AddItem( ACTIONS::cancelInteractive, activeToolFunctor, 1 );
    ctxMenu.AddSeparator( activeToolFunctor, 1 );

    // tool-specific actions
    ctxMenu.AddItem( closeZoneOutline, zoneActiveFunctor, 200 );
    ctxMenu.AddItem( deleteLastPoint, canUndoPoint, 200 );

    ctxMenu.AddSeparator( canUndoPoint, 500 );

    // Type-specific sub-menus will be added for us by other tools
    // For example, zone fill/unfill is provided by the PCB control tool

    // Finally, add the standard zoom/grid items
    m_menu.AddStandardSubMenus( *getEditFrame<PCB_BASE_FRAME>() );

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
    if( m_editModules && !m_frame->GetModel() )
        return 0;

    BOARD_ITEM_CONTAINER* parent = m_frame->GetModel();
    DRAWSEGMENT* line = m_editModules ? new EDGE_MODULE( (MODULE*) parent ) : new DRAWSEGMENT;

    auto startingPoint = boost::make_optional<VECTOR2D>( false, VECTOR2D( 0, 0 ) );
    BOARD_COMMIT commit( m_frame );

    SCOPED_DRAW_MODE scopedDrawMode( m_mode, MODE::LINE );

    m_frame->SetToolID( m_editModules ? ID_MODEDIT_LINE_TOOL : ID_PCB_ADD_LINE_BUTT,
            wxCURSOR_PENCIL, _( "Add graphic line" ) );

    while( drawSegment( S_SEGMENT, line, startingPoint ) )
    {
        if( line )
        {
            if( m_editModules )
                static_cast<EDGE_MODULE*>( line )->SetLocalCoord();

            commit.Add( line );
            commit.Push( _( "Draw a line segment" ) );
            startingPoint = VECTOR2D( line->GetEnd() );
        }
        else
        {
            startingPoint = NULLOPT;
        }

        line = m_editModules ? new EDGE_MODULE( (MODULE*) parent ) : new DRAWSEGMENT;
    }

    m_frame->SetNoToolSelected();

    return 0;
}


int DRAWING_TOOL::DrawCircle( const TOOL_EVENT& aEvent )
{
    if( m_editModules && !m_frame->GetModel() )
        return 0;

    BOARD_ITEM_CONTAINER* parent = m_frame->GetModel();
    DRAWSEGMENT*    circle = m_editModules ? new EDGE_MODULE( (MODULE*) parent ) : new DRAWSEGMENT;
    BOARD_COMMIT    commit( m_frame );

    SCOPED_DRAW_MODE scopedDrawMode( m_mode, MODE::CIRCLE );

    m_frame->SetToolID( m_editModules ? ID_MODEDIT_CIRCLE_TOOL : ID_PCB_CIRCLE_BUTT,
            wxCURSOR_PENCIL, _( "Add graphic circle" ) );

    while( drawSegment( S_CIRCLE, circle ) )
    {
        if( circle )
        {
            if( m_editModules )
                static_cast<EDGE_MODULE*>( circle )->SetLocalCoord();

            commit.Add( circle );
            commit.Push( _( "Draw a circle" ) );
        }

        circle = m_editModules ? new EDGE_MODULE( (MODULE*) parent ) : new DRAWSEGMENT;
    }

    m_frame->SetNoToolSelected();

    return 0;
}


int DRAWING_TOOL::DrawArc( const TOOL_EVENT& aEvent )
{
    if( m_editModules && !m_frame->GetModel() )
        return 0;

    BOARD_ITEM_CONTAINER* parent = m_frame->GetModel();
    DRAWSEGMENT*    arc = m_editModules ? new EDGE_MODULE( (MODULE*) parent ) : new DRAWSEGMENT;
    BOARD_COMMIT    commit( m_frame );

    SCOPED_DRAW_MODE scopedDrawMode( m_mode, MODE::ARC );

    m_frame->SetToolID( m_editModules ? ID_MODEDIT_ARC_TOOL : ID_PCB_ARC_BUTT,
            wxCURSOR_PENCIL, _( "Add graphic arc" ) );

    while( drawArc( arc ) )
    {
        if( arc )
        {
            if( m_editModules )
                static_cast<EDGE_MODULE*>( arc )->SetLocalCoord();

            commit.Add( arc );
            commit.Push( _( "Draw an arc" ) );
        }

        arc = m_editModules ? new EDGE_MODULE( (MODULE*) parent ) : new DRAWSEGMENT;
    }

    m_frame->SetNoToolSelected();

    return 0;
}


int DRAWING_TOOL::PlaceText( const TOOL_EVENT& aEvent )
{
    if( m_editModules && !m_frame->GetModel() )
        return 0;

    BOARD_ITEM* text = NULL;
    const BOARD_DESIGN_SETTINGS& dsnSettings = m_frame->GetDesignSettings();
    SELECTION_TOOL* selTool = m_toolMgr->GetTool<SELECTION_TOOL>();
    SELECTION& selection = selTool->GetSelection();
    BOARD_COMMIT commit( m_frame );

    m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );
    m_controls->ShowCursor( true );
    m_controls->SetSnapping( true );
    // do not capture or auto-pan until we start placing some text

    SCOPED_DRAW_MODE scopedDrawMode( m_mode, MODE::TEXT );

    Activate();
    m_frame->SetToolID( m_editModules ? ID_MODEDIT_TEXT_TOOL : ID_PCB_ADD_TEXT_BUTT,
            wxCURSOR_PENCIL, _( "Add text" ) );

    bool reselect = false;

    // Main loop: keep receiving events
    while( OPT_TOOL_EVENT evt = Wait() )
    {
        VECTOR2I cursorPos = m_controls->GetCursorPosition();

        if( reselect && text )
            m_toolMgr->RunAction( PCB_ACTIONS::selectItem, true, text );

        if( TOOL_EVT_UTILS::IsCancelInteractive( *evt ) )
        {
            if( text )
            {
                m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );

                // Delete the old text and have another try
                delete text;
                text = NULL;

                m_controls->SetAutoPan( false );
                m_controls->CaptureCursor( false );
                m_controls->ShowCursor( true );
            }
            else
                break;

            if( evt->IsActivate() ) // now finish unconditionally
                break;
        }
        else if( evt->IsClick( BUT_RIGHT ) )
        {
            m_menu.ShowContextMenu();
        }
        else if( evt->IsClick( BUT_LEFT ) )
        {
            if( !text )
            {
                m_controls->ForceCursorPosition( true, m_controls->GetCursorPosition() );
                PCB_LAYER_ID layer = m_frame->GetActiveLayer();

                // Init the new item attributes
                if( m_editModules )
                {
                    TEXTE_MODULE* textMod = new TEXTE_MODULE( (MODULE*) m_frame->GetModel() );

                    textMod->SetLayer( layer );
                    textMod->SetTextSize( dsnSettings.GetTextSize( layer ) );
                    textMod->SetThickness( dsnSettings.GetTextThickness( layer ) );
                    textMod->SetItalic( dsnSettings.GetTextItalic( layer ) );
                    textMod->SetKeepUpright( dsnSettings.GetTextUpright( layer ) );
                    textMod->SetTextPos( wxPoint( cursorPos.x, cursorPos.y ) );

                    DIALOG_TEXT_PROPERTIES textDialog( m_frame, textMod, NULL );
                    bool placing;

                    RunMainStack([&]() {
                        placing = textDialog.ShowModal() && ( textMod->GetText().Length() > 0 );
                    } );

                    if( placing )
                        text = textMod;
                    else
                        delete textMod;
                }
                else
                {
                    TEXTE_PCB* textPcb = new TEXTE_PCB( m_frame->GetModel() );
                    // TODO we have to set IS_NEW, otherwise InstallTextPCB.. creates an undo entry :| LEGACY_CLEANUP
                    textPcb->SetFlags( IS_NEW );

                    textPcb->SetLayer( layer );

                    // Set the mirrored option for layers on the BACK side of the board
                    if( IsBackLayer( layer ) )
                        textPcb->SetMirrored( true );

                    textPcb->SetTextSize( dsnSettings.GetTextSize( layer ) );
                    textPcb->SetThickness( dsnSettings.GetTextThickness( layer ) );
                    textPcb->SetItalic( dsnSettings.GetTextItalic( layer ) );
                    textPcb->SetTextPos( wxPoint( cursorPos.x, cursorPos.y ) );

                    RunMainStack([&]() {
                        m_frame->InstallTextOptionsFrame( textPcb, NULL );
                    } );

                    if( textPcb->GetText().IsEmpty() )
                        delete textPcb;
                    else
                        text = textPcb;
                }

                if( text == NULL )
                    continue;

                m_controls->WarpCursor( text->GetPosition(), true );
                m_controls->ForceCursorPosition( false );
                m_controls->CaptureCursor( true );
                m_controls->SetAutoPan( true );

                m_toolMgr->RunAction( PCB_ACTIONS::selectItem, true, text );
            }
            else
            {
                text->ClearFlags();
                m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );

                commit.Add( text );
                commit.Push( _( "Place a text" ) );

                m_controls->CaptureCursor( false );
                m_controls->SetAutoPan( false );
                m_controls->ShowCursor( true );

                text = NULL;
            }
        }
        else if( text && evt->IsMotion() )
        {
            text->SetPosition( wxPoint( cursorPos.x, cursorPos.y ) );
            selection.SetReferencePoint( cursorPos );
            m_view->Update( &selection );
            frame()->SetMsgPanel( text );
        }

        else if( text && evt->IsAction( &PCB_ACTIONS::properties ) )
        {
            // Calling 'Properties' action clears the selection, so we need to restore it
            reselect = true;
        }
    }

    frame()->SetMsgPanel( board() );
    frame()->SetNoToolSelected();

    return 0;
}


void DRAWING_TOOL::constrainDimension( DIMENSION* dimension )
{
    const VECTOR2I lineVector{ dimension->GetEnd() - dimension->GetOrigin() };

    dimension->SetEnd( wxPoint(
        VECTOR2I( dimension->GetOrigin() ) + GetVectorSnapped45( lineVector ) ) );
}


int DRAWING_TOOL::DrawDimension( const TOOL_EVENT& aEvent )
{
    if( m_editModules && !m_frame->GetModel() )
        return 0;

    DIMENSION* dimension = NULL;
    BOARD_COMMIT commit( m_frame );
    GRID_HELPER grid( m_frame );

    // Add a VIEW_GROUP that serves as a preview for the new item
    SELECTION preview;

    m_view->Add( &preview );

    m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );
    m_controls->ShowCursor( true );
    m_controls->SetSnapping( true );

    SCOPED_DRAW_MODE scopedDrawMode( m_mode, MODE::DIMENSION );

    Activate();
    m_frame->SetToolID( ID_PCB_DIMENSION_BUTT, wxCURSOR_PENCIL, _( "Add dimension" ) );

    enum DIMENSION_STEPS
    {
        SET_ORIGIN = 0,
        SET_END,
        SET_HEIGHT,
        FINISHED
    };
    int step = SET_ORIGIN;

    // Main loop: keep receiving events
    while( OPT_TOOL_EVENT evt = Wait() )
    {
        grid.SetSnap( !evt->Modifier( MD_SHIFT ) );
        grid.SetUseGrid( !evt->Modifier( MD_ALT ) );
        m_controls->SetSnapping( !evt->Modifier( MD_ALT ) );
        VECTOR2I cursorPos = grid.BestSnapAnchor( m_controls->GetMousePosition(), nullptr );
        m_controls->ForceCursorPosition( true, cursorPos );

        if( TOOL_EVT_UTILS::IsCancelInteractive( *evt ) )
        {
            m_controls->SetAutoPan( false );

            if( step != SET_ORIGIN )    // start from the beginning
            {
                preview.Clear();

                delete dimension;
                step = SET_ORIGIN;
            }
            else
                break;

            if( evt->IsActivate() ) // now finish unconditionally
                break;
        }
        else if( evt->IsAction( &PCB_ACTIONS::incWidth ) && step != SET_ORIGIN )
        {
            m_lineWidth += WIDTH_STEP;
            dimension->SetWidth( m_lineWidth );
            m_view->Update( &preview );
            frame()->SetMsgPanel( dimension );
        }
        else if( evt->IsAction( &PCB_ACTIONS::decWidth ) && step != SET_ORIGIN )
        {
            if( m_lineWidth > WIDTH_STEP )
            {
                m_lineWidth -= WIDTH_STEP;
                dimension->SetWidth( m_lineWidth );
                m_view->Update( &preview );
                frame()->SetMsgPanel( dimension );
            }
        }
        else if( evt->IsClick( BUT_RIGHT ) )
        {
            m_menu.ShowContextMenu();
        }
        else if( evt->IsClick( BUT_LEFT ) )
        {
            switch( step )
            {
            case SET_ORIGIN:
            {
                PCB_LAYER_ID layer = getDrawingLayer();
                const BOARD_DESIGN_SETTINGS& boardSettings = m_board->GetDesignSettings();

                if( layer == Edge_Cuts )        // dimensions are not allowed on EdgeCuts
                    layer = Dwgs_User;

                // Init the new item attributes
                dimension = new DIMENSION( m_board );
                dimension->SetLayer( layer );
                dimension->SetOrigin( wxPoint( cursorPos.x, cursorPos.y ) );
                dimension->SetEnd( wxPoint( cursorPos.x, cursorPos.y ) );
                dimension->Text().SetTextSize( boardSettings.GetTextSize( layer ) );
                dimension->Text().SetThickness( boardSettings.GetTextThickness( layer ) );
                dimension->Text().SetItalic( boardSettings.GetTextItalic( layer ) );
                dimension->SetWidth( boardSettings.GetLineThickness( layer ) );
                dimension->SetUnits( m_frame->GetUserUnits(), false );
                dimension->AdjustDimensionDetails();

                preview.Add( dimension );
                frame()->SetMsgPanel( dimension );

                m_controls->SetAutoPan( true );
                m_controls->CaptureCursor( true );
            }
            break;

            case SET_END:
                dimension->SetEnd( wxPoint( cursorPos.x, cursorPos.y ) );

                if( !!evt->Modifier( MD_CTRL ) )
                    constrainDimension( dimension );

                // Dimensions that have origin and end in the same spot are not valid
                if( dimension->GetOrigin() == dimension->GetEnd() )
                    --step;

                break;

            case SET_HEIGHT:
            {
                if( wxPoint( cursorPos.x, cursorPos.y ) != dimension->GetPosition() )
                {
                    assert( dimension->GetOrigin() != dimension->GetEnd() );
                    assert( dimension->GetWidth() > 0 );

                    preview.Remove( dimension );

                    commit.Add( dimension );
                    commit.Push( _( "Draw a dimension" ) );
                }
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
                dimension->SetEnd( wxPoint( cursorPos.x, cursorPos.y ) );

                if( !!evt->Modifier( MD_CTRL ) )
                    constrainDimension( dimension );

                break;

            case SET_HEIGHT:
            {
                // Calculating the direction of travel perpendicular to the selected axis
                double angle = dimension->GetAngle() + ( M_PI / 2 );

                wxPoint pos( cursorPos.x, cursorPos.y );
                wxPoint delta( pos - dimension->m_featureLineDO );
                double  height = ( delta.x * cos( angle ) ) + ( delta.y * sin( angle ) );
                dimension->SetHeight( height );
            }
            break;
            }

            // Show a preview of the item
            m_view->Update( &preview );
            if( step )
                frame()->SetMsgPanel( dimension );
            else
                frame()->SetMsgPanel( board() );
        }
    }

    if( step != SET_ORIGIN )
        delete dimension;

    m_controls->SetAutoPan( false );

    m_view->Remove( &preview );
    frame()->SetMsgPanel( board() );
    m_frame->SetNoToolSelected();

    return 0;
}


int DRAWING_TOOL::DrawZone( const TOOL_EVENT& aEvent )
{
    SCOPED_DRAW_MODE scopedDrawMode( m_mode, MODE::ZONE );

    m_frame->SetToolID( ID_PCB_ZONES_BUTT, wxCURSOR_PENCIL, _( "Add zones" ) );

    return drawZone( false, ZONE_MODE::ADD );
}


int DRAWING_TOOL::DrawZoneKeepout( const TOOL_EVENT& aEvent )
{
    SCOPED_DRAW_MODE scopedDrawMode( m_mode, MODE::KEEPOUT );

    m_frame->SetToolID( ID_PCB_KEEPOUT_AREA_BUTT, wxCURSOR_PENCIL, _( "Add keepout" ) );

    return drawZone( true, ZONE_MODE::ADD );
}


int DRAWING_TOOL::DrawZoneCutout( const TOOL_EVENT& aEvent )
{
    SCOPED_DRAW_MODE scopedDrawMode( m_mode, MODE::ZONE );

    m_frame->SetToolID( ID_PCB_ZONES_BUTT, wxCURSOR_PENCIL, _( "Add zone cutout" ) );

    return drawZone( false, ZONE_MODE::CUTOUT );
}


int DRAWING_TOOL::DrawGraphicPolygon( const TOOL_EVENT& aEvent )
{
    if( m_editModules && !m_frame->GetModel() )
        return 0;

    SCOPED_DRAW_MODE scopedDrawMode( m_mode, MODE::GRAPHIC_POLYGON );

    m_frame->SetToolID( m_editModules ? ID_MODEDIT_POLYGON_TOOL : ID_PCB_ADD_POLYGON_BUTT,
                        wxCURSOR_PENCIL, _( "Add graphic polygon" ) );

    return drawZone( false, ZONE_MODE::GRAPHIC_POLYGON );
}


int DRAWING_TOOL::DrawSimilarZone( const TOOL_EVENT& aEvent )
{
    SCOPED_DRAW_MODE scopedDrawMode( m_mode, MODE::ZONE );

    m_frame->SetToolID( ID_PCB_ZONES_BUTT, wxCURSOR_PENCIL, _( "Add similar zone" ) );

    return drawZone( false, ZONE_MODE::SIMILAR );
}


int DRAWING_TOOL::PlaceImportedGraphics( const TOOL_EVENT& aEvent )
{
    if( !m_frame->GetModel() )
        return 0;

    // Note: PlaceImportedGraphics() will convert  PCB_LINE_T and PCB_TEXT_T to module graphic items
    // if needed
    DIALOG_IMPORT_GFX dlg( m_frame, m_editModules );
    int dlgResult = dlg.ShowModal();

    auto& list = dlg.GetImportedItems();

    if( dlgResult != wxID_OK )
        return 0;

    // Ensure the list is not empty:
    if( list.empty() )
    {
        wxMessageBox( _( "No graphic items found in file to import") );
        return 0;
    }


    m_frame->SetNoToolSelected();

    // Add a VIEW_GROUP that serves as a preview for the new item
    SELECTION preview;
    BOARD_COMMIT commit( m_frame );

    // Build the undo list & add items to the current view
    for( auto it = list.begin(), itEnd = list.end(); it != itEnd; ++it )
    {
        EDA_ITEM* item = it->get();

        if( m_editModules )
        {
            wxASSERT( item->Type() == PCB_MODULE_EDGE_T || item->Type() == PCB_MODULE_TEXT_T );
        }
        else
        {
            wxASSERT( item->Type() == PCB_LINE_T || item->Type() == PCB_TEXT_T );
        }

        if( dlg.IsPlacementInteractive() )
            preview.Add( item );
        else
            commit.Add( item );

        it->release();
    }

    if( !dlg.IsPlacementInteractive() )
    {
        commit.Push( _( "Place a DXF_SVG drawing" ) );
        return 0;
    }

    BOARD_ITEM* firstItem = static_cast<BOARD_ITEM*>( preview.Front() );
    m_view->Add( &preview );

    m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );
    m_controls->ShowCursor( true );
    m_controls->SetSnapping( true );
    m_controls->ForceCursorPosition( false );

    SCOPED_DRAW_MODE scopedDrawMode( m_mode, MODE::DXF );

    // Now move the new items to the current cursor position:
    VECTOR2I cursorPos = m_controls->GetCursorPosition();
    VECTOR2I delta = cursorPos - firstItem->GetPosition();

    for( auto item : preview )
        static_cast<BOARD_ITEM*>( item )->Move( wxPoint( delta.x, delta.y ) );

    m_view->Update( &preview );

    Activate();

    // Main loop: keep receiving events
    while( OPT_TOOL_EVENT evt = Wait() )
    {
        cursorPos = m_controls->GetCursorPosition();

        if( evt->IsMotion() )
        {
            delta = cursorPos - firstItem->GetPosition();

            for( auto item : preview )
                static_cast<BOARD_ITEM*>( item )->Move( wxPoint( delta.x, delta.y ) );

            m_view->Update( &preview );
        }
        else if( evt->Category() == TC_COMMAND )
        {
            // TODO it should be handled by EDIT_TOOL, so add items and select?
            if( TOOL_EVT_UTILS::IsRotateToolEvt( *evt ) )
            {
                const auto  rotationPoint   = wxPoint( cursorPos.x, cursorPos.y );
                const auto  rotationAngle   = TOOL_EVT_UTILS::GetEventRotationAngle(
                        *m_frame, *evt );

                for( auto item : preview )
                {
                    static_cast<BOARD_ITEM*>( item )->Rotate( rotationPoint, rotationAngle );
                }

                m_view->Update( &preview );
            }
            else if( evt->IsAction( &PCB_ACTIONS::flip ) )
            {
                for( auto item : preview )
                    static_cast<BOARD_ITEM*>( item )->Flip( wxPoint( cursorPos.x, cursorPos.y ) );

                m_view->Update( &preview );
            }
            else if( TOOL_EVT_UTILS::IsCancelInteractive( *evt ) )
            {
                preview.FreeItems();
                break;
            }
        }
        else if( evt->IsClick( BUT_RIGHT ) )
        {
            m_menu.ShowContextMenu();
        }
        else if( evt->IsClick( BUT_LEFT ) )
        {
            // Place the imported drawings
            for( auto item : preview )
                commit.Add( item );

            commit.Push( _( "Place a DXF_SVG drawing" ) );
            break;
        }
    }

    preview.Clear();
    m_view->Remove( &preview );

    return 0;
}


int DRAWING_TOOL::SetAnchor( const TOOL_EVENT& aEvent )
{
    assert( m_editModules );

    if( !m_frame->GetModel() )
        return 0;

    SCOPED_DRAW_MODE scopedDrawMode( m_mode, MODE::ANCHOR );

    Activate();
    m_frame->SetToolID( ID_MODEDIT_ANCHOR_TOOL, wxCURSOR_PENCIL,
            _( "Place the footprint anchor" ) );

    m_controls->ShowCursor( true );
    m_controls->SetSnapping( true );
    m_controls->SetAutoPan( true );
    m_controls->CaptureCursor( false );

    while( OPT_TOOL_EVENT evt = Wait() )
    {
        if( evt->IsClick( BUT_LEFT ) )
        {
            MODULE* module = (MODULE*) m_frame->GetModel();
            BOARD_COMMIT commit( m_frame );
            commit.Modify( module );

            // set the new relative internal local coordinates of footprint items
            VECTOR2I    cursorPos = m_controls->GetCursorPosition();
            wxPoint     moveVector = module->GetPosition() - wxPoint( cursorPos.x, cursorPos.y );
            module->MoveAnchorPosition( moveVector );

            commit.Push( _( "Move the footprint reference anchor" ) );

            // Usually, we do not need to change twice the anchor position,
            // so deselect the active tool
            break;
        }
        else if( evt->IsClick( BUT_RIGHT ) )
        {
            m_menu.ShowContextMenu();
        }
        else if( TOOL_EVT_UTILS::IsCancelInteractive( *evt ) )
            break;
    }

    m_frame->SetNoToolSelected();

    return 0;
}


bool DRAWING_TOOL::drawSegment( int aShape, DRAWSEGMENT*& aGraphic,
        OPT<VECTOR2D> aStartingPoint )
{
    // Only two shapes are currently supported
    assert( aShape == S_SEGMENT || aShape == S_CIRCLE );
    GRID_HELPER grid( m_frame );

    m_lineWidth = getSegmentWidth( getDrawingLayer() );
    m_frame->SetActiveLayer( getDrawingLayer() );

    // Add a VIEW_GROUP that serves as a preview for the new item
    SELECTION preview;
    m_view->Add( &preview );

    m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );
    m_controls->ShowCursor( true );

    Activate();

    bool    direction45 = false;    // 45 degrees only mode
    bool    started = false;
    bool    IsOCurseurSet = ( m_frame->GetScreen()->m_O_Curseur != wxPoint( 0, 0 ) );
    VECTOR2I cursorPos = m_controls->GetMousePosition();

    if( aStartingPoint )
    {
        // Init the new item attributes
        aGraphic->SetShape( (STROKE_T) aShape );
        aGraphic->SetWidth( m_lineWidth );
        aGraphic->SetLayer( getDrawingLayer() );
        aGraphic->SetStart( wxPoint( aStartingPoint->x, aStartingPoint->y ) );

        cursorPos = grid.BestSnapAnchor( cursorPos, aGraphic );
        m_controls->ForceCursorPosition( true, cursorPos );
        aGraphic->SetEnd( wxPoint( cursorPos.x, cursorPos.y ) );

        preview.Add( aGraphic );
        m_controls->SetAutoPan( true );
        m_controls->CaptureCursor( true );

        if( !IsOCurseurSet )
            m_frame->GetScreen()->m_O_Curseur = wxPoint( aStartingPoint->x, aStartingPoint->y );

        started = true;
    }

    // Main loop: keep receiving events
    while( OPT_TOOL_EVENT evt = Wait() )
    {
        grid.SetSnap( !evt->Modifier( MD_SHIFT ) );
        grid.SetUseGrid( !evt->Modifier( MD_ALT ) );
        m_controls->SetSnapping( !evt->Modifier( MD_ALT ) );
        cursorPos = grid.BestSnapAnchor( m_controls->GetMousePosition(), getDrawingLayer() );
        m_controls->ForceCursorPosition( true, cursorPos );

        // 45 degree angle constraint enabled with an option and toggled with Ctrl
        const bool limit45 = ( frame()->Settings().m_use45DegreeGraphicSegments != !!( evt->Modifier( MD_CTRL ) ) );

        if( direction45 != limit45 && started && aShape == S_SEGMENT )
        {
            direction45 = limit45;

            if( direction45 )
            {
                const VECTOR2I lineVector( cursorPos - VECTOR2I( aGraphic->GetStart() ) );

                // get a restricted 45/H/V line from the last fixed point to the cursor
                auto newEnd = GetVectorSnapped45( lineVector );
                aGraphic->SetEnd( aGraphic->GetStart() + wxPoint( newEnd.x, newEnd.y ) );
                m_controls->ForceCursorPosition( true, VECTOR2I( aGraphic->GetEnd() ) );
            }
            else
            {
                aGraphic->SetEnd( wxPoint( cursorPos.x, cursorPos.y ) );
            }

            m_view->Update( &preview );
            frame()->SetMsgPanel( aGraphic );
        }

        if( TOOL_EVT_UTILS::IsCancelInteractive( *evt ) )
        {
            preview.Clear();
            m_view->Update( &preview );
            delete aGraphic;
            aGraphic = NULL;
            if( !IsOCurseurSet )
                m_frame->GetScreen()->m_O_Curseur = wxPoint( 0, 0 );
            break;
        }
        else if( evt->IsAction( &PCB_ACTIONS::layerChanged ) )
        {
            m_lineWidth = getSegmentWidth( getDrawingLayer() );
            aGraphic->SetLayer( getDrawingLayer() );
            aGraphic->SetWidth( m_lineWidth );
            m_view->Update( &preview );
            frame()->SetMsgPanel( aGraphic );
        }
        else if( evt->IsClick( BUT_RIGHT ) )
        {
            m_menu.ShowContextMenu();
        }
        else if( evt->IsClick( BUT_LEFT ) || evt->IsDblClick( BUT_LEFT ) )
        {
            if( !started )
            {
                m_lineWidth = getSegmentWidth( getDrawingLayer() );

                // Init the new item attributes
                aGraphic->SetShape( (STROKE_T) aShape );
                aGraphic->SetWidth( m_lineWidth );
                aGraphic->SetStart( wxPoint( cursorPos.x, cursorPos.y ) );
                aGraphic->SetEnd( wxPoint( cursorPos.x, cursorPos.y ) );
                aGraphic->SetLayer( getDrawingLayer() );

                if( !IsOCurseurSet )
                    m_frame->GetScreen()->m_O_Curseur = wxPoint( cursorPos.x, cursorPos.y );

                preview.Add( aGraphic );
                frame()->SetMsgPanel( aGraphic );
                m_controls->SetAutoPan( true );
                m_controls->CaptureCursor( true );

                started = true;
            }
            else
            {
                auto snapItem = dyn_cast<DRAWSEGMENT*>( grid.GetSnapped() );
                auto mod = dyn_cast<MODULE*>( m_frame->GetModel() );

                if( aGraphic->GetEnd() == aGraphic->GetStart()
                    || ( evt->IsDblClick( BUT_LEFT ) && aShape == S_SEGMENT )
                    || snapItem )
                // User has clicked twice in the same spot
                //  or clicked on the end of an existing segment (closing a path)
                {
                    BOARD_COMMIT commit( m_frame );

                    // If the user clicks on an existing snap point from a drawsegment
                    //  we finish the segment as they are likely closing a path
                    if( snapItem && aGraphic->GetLength() > 0.0 )
                    {
                        DRAWSEGMENT* l = m_editModules ? new EDGE_MODULE( mod ) : new DRAWSEGMENT;

                        *l = *aGraphic;
                        commit.Add( l );
                    }

                    if( !commit.Empty() )
                        commit.Push( _( "Draw a line" ) );

                    delete aGraphic;
                    aGraphic = NULL;
                }

                preview.Clear();
                break;
            }
        }
        else if( evt->IsMotion() )
        {
            // 45 degree lines
            if( direction45 && aShape == S_SEGMENT )
            {
                const VECTOR2I lineVector( cursorPos - VECTOR2I( aGraphic->GetStart() ) );

                // get a restricted 45/H/V line from the last fixed point to the cursor
                auto newEnd = GetVectorSnapped45( lineVector );
                aGraphic->SetEnd( aGraphic->GetStart() + wxPoint( newEnd.x, newEnd.y ) );
                m_controls->ForceCursorPosition( true, VECTOR2I( aGraphic->GetEnd() ) );
            }
            else
                aGraphic->SetEnd( wxPoint( cursorPos.x, cursorPos.y ) );

            m_view->Update( &preview );

            if( started )
                frame()->SetMsgPanel( aGraphic );
            else
                frame()->SetMsgPanel( board() );
        }
        else if( evt->IsAction( &PCB_ACTIONS::incWidth ) )
        {
            m_lineWidth += WIDTH_STEP;
            aGraphic->SetWidth( m_lineWidth );
            m_view->Update( &preview );
            frame()->SetMsgPanel( aGraphic );
        }
        else if( evt->IsAction( &PCB_ACTIONS::decWidth ) && ( m_lineWidth > WIDTH_STEP ) )
        {
            m_lineWidth -= WIDTH_STEP;
            aGraphic->SetWidth( m_lineWidth );
            m_view->Update( &preview );
            frame()->SetMsgPanel( aGraphic );
        }
        else if( evt->IsAction( &PCB_ACTIONS::resetCoords ) )
        {
            IsOCurseurSet = true;
        }
    }

    if( !IsOCurseurSet ) // reset the relative coordinte if it was not set before
        m_frame->GetScreen()->m_O_Curseur = wxPoint( 0, 0 );

    m_view->Remove( &preview );
    frame()->SetMsgPanel( board() );
    m_controls->SetAutoPan( false );
    m_controls->CaptureCursor( false );
    m_controls->ForceCursorPosition( false );

    return started;
}


/**
 * Update an arc DRAWSEGMENT from the current state
 * of an Arc Geometry Manager
 */
static void updateArcFromConstructionMgr( const KIGFX::PREVIEW::ARC_GEOM_MANAGER& aMgr,
        DRAWSEGMENT& aArc )
{
    auto vec = aMgr.GetOrigin();

    aArc.SetCenter( { vec.x, vec.y } );

    vec = aMgr.GetStartRadiusEnd();
    aArc.SetArcStart( { vec.x, vec.y } );

    aArc.SetAngle( RAD2DECIDEG( -aMgr.GetSubtended() ) );
}


bool DRAWING_TOOL::drawArc( DRAWSEGMENT*& aGraphic )
{
    m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );

    m_lineWidth = getSegmentWidth( getDrawingLayer() );

    // Arc geometric construction manager
    KIGFX::PREVIEW::ARC_GEOM_MANAGER arcManager;

    // Arc drawing assistant overlay
    KIGFX::PREVIEW::ARC_ASSISTANT arcAsst( arcManager, m_frame->GetUserUnits() );

    // Add a VIEW_GROUP that serves as a preview for the new item
    SELECTION preview;
    m_view->Add( &preview );
    m_view->Add( &arcAsst );
    GRID_HELPER grid( m_frame );

    m_controls->ShowCursor( true );
    m_controls->SetSnapping( true );

    Activate();

    bool firstPoint = false;

    // Main loop: keep receiving events
    while( OPT_TOOL_EVENT evt = Wait() )
    {
        PCB_LAYER_ID layer = getDrawingLayer();
        aGraphic->SetLayer( layer );

        grid.SetSnap( !evt->Modifier( MD_SHIFT ) );
        grid.SetUseGrid( !evt->Modifier( MD_ALT ) );
        m_controls->SetSnapping( !evt->Modifier( MD_ALT ) );
        VECTOR2I cursorPos = grid.BestSnapAnchor( m_controls->GetMousePosition(), aGraphic );
        m_controls->ForceCursorPosition( true, cursorPos );

        if( evt->IsClick( BUT_LEFT ) )
        {
            if( !firstPoint )
            {
                m_controls->SetAutoPan( true );
                m_controls->CaptureCursor( true );

                m_lineWidth = getSegmentWidth( getDrawingLayer() );

                // Init the new item attributes
                // (non-geometric, those are handled by the manager)
                aGraphic->SetShape( S_ARC );
                aGraphic->SetWidth( m_lineWidth );

                preview.Add( aGraphic );
                firstPoint = true;
            }

            arcManager.AddPoint( cursorPos, true );
        }
        else if( evt->IsAction( &deleteLastPoint ) )
        {
            arcManager.RemoveLastPoint();
        }
        else if( evt->IsMotion() )
        {
            // set angle snap
            arcManager.SetAngleSnap( evt->Modifier( MD_CTRL ) );

            // update, but don't step the manager state
            arcManager.AddPoint( cursorPos, false );
        }
        else if( TOOL_EVT_UTILS::IsCancelInteractive( *evt ) )
        {
            preview.Clear();
            delete aGraphic;
            aGraphic = nullptr;
            break;
        }
        else if( evt->IsAction( &PCB_ACTIONS::layerChanged ) )
        {
            m_lineWidth = getSegmentWidth( getDrawingLayer() );
            aGraphic->SetLayer( getDrawingLayer() );
            aGraphic->SetWidth( m_lineWidth );
            m_view->Update( &preview );
            frame()->SetMsgPanel( aGraphic );
        }
        else if( evt->IsClick( BUT_RIGHT ) )
        {
            m_menu.ShowContextMenu();
        }
        else if( evt->IsAction( &PCB_ACTIONS::incWidth ) )
        {
            m_lineWidth += WIDTH_STEP;
            aGraphic->SetWidth( m_lineWidth );
            m_view->Update( &preview );
            frame()->SetMsgPanel( aGraphic );
        }
        else if( evt->IsAction( &PCB_ACTIONS::decWidth ) && m_lineWidth > WIDTH_STEP )
        {
            m_lineWidth -= WIDTH_STEP;
            aGraphic->SetWidth( m_lineWidth );
            m_view->Update( &preview );
            frame()->SetMsgPanel( aGraphic );
        }
        else if( evt->IsAction( &PCB_ACTIONS::arcPosture ) )
        {
            arcManager.ToggleClockwise();
        }

        if( arcManager.IsComplete() )
        {
            break;
        }
        else if( arcManager.HasGeometryChanged() )
        {
            updateArcFromConstructionMgr( arcManager, *aGraphic );
            m_view->Update( &preview );
            m_view->Update( &arcAsst );

            if(firstPoint)
                frame()->SetMsgPanel( aGraphic );
            else
                frame()->SetMsgPanel( board() );
        }
    }

    preview.Remove( aGraphic );
    m_view->Remove( &arcAsst );
    m_view->Remove( &preview );
    frame()->SetMsgPanel( board() );
    m_controls->SetAutoPan( false );
    m_controls->CaptureCursor( false );
    m_controls->ForceCursorPosition( false );

    return !arcManager.IsReset();
}


bool DRAWING_TOOL::getSourceZoneForAction( ZONE_MODE aMode, ZONE_CONTAINER*& aZone )
{
    bool clearSelection = false;
    aZone = nullptr;

    // not an action that needs a source zone
    if( aMode == ZONE_MODE::ADD || aMode == ZONE_MODE::GRAPHIC_POLYGON )
        return true;

    SELECTION_TOOL* selTool = m_toolMgr->GetTool<SELECTION_TOOL>();
    const SELECTION& selection = selTool->GetSelection();

    if( selection.Empty() )
    {
        clearSelection = true;
        m_toolMgr->RunAction( PCB_ACTIONS::selectionCursor, true );
    }

    // we want a single zone
    if( selection.Size() == 1 )
        aZone = dyn_cast<ZONE_CONTAINER*>( selection[0] );

    // expected a zone, but didn't get one
    if( !aZone )
    {
        if( clearSelection )
            m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );

        return false;
    }

    return true;
}

int DRAWING_TOOL::drawZone( bool aKeepout, ZONE_MODE aMode )
{
    // get a source zone, if we need one. We need it for:
    // ZONE_MODE::CUTOUT (adding a hole to the source zone)
    // ZONE_MODE::SIMILAR (creating a new zone using settings of source zone
    ZONE_CONTAINER* sourceZone = nullptr;

    if( !getSourceZoneForAction( aMode, sourceZone ) )
    {
        m_frame->SetNoToolSelected();
        return 0;
    }

    ZONE_CREATE_HELPER::PARAMS params;

    params.m_keepout = aKeepout;
    params.m_mode = aMode;
    params.m_sourceZone = sourceZone;

    if( aMode == ZONE_MODE::GRAPHIC_POLYGON )
        params.m_layer = getDrawingLayer();
    else if( aMode == ZONE_MODE::SIMILAR )
        params.m_layer = sourceZone->GetLayer();
    else
        params.m_layer = m_frame->GetActiveLayer();

    ZONE_CREATE_HELPER zoneTool( *this, params );

    // the geometry manager which handles the zone geometry, and
    // hands the calculated points over to the zone creator tool
    POLYGON_GEOM_MANAGER polyGeomMgr( zoneTool );

    Activate();    // register for events

    m_controls->ShowCursor( true );
    m_controls->SetSnapping( true );

    bool    started     = false;
    GRID_HELPER grid( m_frame );
    STATUS_TEXT_POPUP status( m_frame );
    status.SetTextColor( wxColour( 255, 0, 0 ) );
    status.SetText( _( "Self-intersecting polygons are not allowed" ) );

    while( OPT_TOOL_EVENT evt = Wait() )
    {
        LSET layers( m_frame->GetActiveLayer() );
        grid.SetSnap( !evt->Modifier( MD_SHIFT ) );
        grid.SetUseGrid( !evt->Modifier( MD_ALT ) );
        m_controls->SetSnapping( !evt->Modifier( MD_ALT ) );
        VECTOR2I cursorPos = grid.BestSnapAnchor( m_controls->GetMousePosition(), layers );
        m_controls->ForceCursorPosition( true, cursorPos );

        if( TOOL_EVT_UTILS::IsCancelInteractive( *evt ) )
        {
            // pre-empted by another tool, give up
            // cancelled without an inprogress polygon, give up
            if( !polyGeomMgr.IsPolygonInProgress() || evt->IsActivate() )
            {
                break;
            }

            polyGeomMgr.Reset();
            // start again
            started = false;

            m_controls->SetAutoPan( false );
            m_controls->CaptureCursor( false );
        }
        else if( evt->IsAction( &PCB_ACTIONS::layerChanged ) )
        {
            if( aMode == ZONE_MODE::GRAPHIC_POLYGON )
                params.m_layer = getDrawingLayer();
            else if( aMode == ZONE_MODE::ADD || aMode == ZONE_MODE::CUTOUT )
                params.m_layer = frame()->GetActiveLayer();
        }
        else if( evt->IsClick( BUT_RIGHT ) )
        {
            m_menu.ShowContextMenu();
        }
        // events that lock in nodes
        else if( evt->IsClick( BUT_LEFT )
                 || evt->IsDblClick( BUT_LEFT )
                 || evt->IsAction( &closeZoneOutline ) )
        {
            // Check if it is double click / closing line (so we have to finish the zone)
            const bool endPolygon = evt->IsDblClick( BUT_LEFT )
                                    || evt->IsAction( &closeZoneOutline )
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
                    m_controls->SetAutoPan( true );
                    m_controls->CaptureCursor( true );
                }
            }

        }
        else if( evt->IsAction( &deleteLastPoint ) )
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
            polyGeomMgr.SetCursorPosition( cursorPos, evt->Modifier( MD_CTRL )
                                                      ? POLYGON_GEOM_MANAGER::LEADER_MODE::DEG45
                                                      : POLYGON_GEOM_MANAGER::LEADER_MODE::DIRECT );

            if( polyGeomMgr.IsSelfIntersecting( true ) )
            {
                wxPoint p = wxGetMousePosition() + wxPoint( 20, 20 );
                status.Move( p );
                status.Popup( m_frame );
                status.Expire( 1500 );
            }
            else
            {
                status.Hide();
            }
        }
    }    // end while

    m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );
    m_frame->SetNoToolSelected();
    m_controls->ForceCursorPosition( false );

    return 0;
}


int DRAWING_TOOL::DrawVia( const TOOL_EVENT& aEvent )
{
    struct VIA_PLACER : public INTERACTIVE_PLACER_BASE
    {
        GRID_HELPER m_gridHelper;

        VIA_PLACER( PCB_EDIT_FRAME* aFrame ) : m_gridHelper( aFrame )
        {}

        TRACK* findTrack( VIA* aVia )
        {
            const LSET lset = aVia->GetLayerSet();
            std::vector<KIGFX::VIEW::LAYER_ITEM_PAIR> items;
            BOX2I bbox = aVia->GetBoundingBox();
            auto view = m_frame->GetGalCanvas()->GetView();
            std::vector<TRACK*> possible_tracks;

            view->Query( bbox, items );

            for( auto it : items )
            {
                BOARD_ITEM* item = static_cast<BOARD_ITEM*>( it.first );

                if( !(item->GetLayerSet() & lset ).any() )
                    continue;

                if( auto track = dyn_cast<TRACK*>( item ) )
                {
                    if( TestSegmentHit( aVia->GetPosition(), track->GetStart(), track->GetEnd(),
                                        ( track->GetWidth() + aVia->GetWidth() ) / 2 ) )
                        possible_tracks.push_back( track );
                }
            }

            TRACK* return_track = nullptr;
            int min_d = std::numeric_limits<int>::max();
            for( auto track : possible_tracks )
            {
                SEG test( track->GetStart(), track->GetEnd() );
                auto dist = ( test.NearestPoint( aVia->GetPosition() ) -
                        VECTOR2I( aVia->GetPosition() ) ).EuclideanNorm();

                if( dist < min_d )
                {
                    min_d = dist;
                    return_track = track;
                }
            }

            return return_track;
        }


        bool hasDRCViolation( VIA* aVia )
        {
            const LSET lset = aVia->GetLayerSet();
            std::vector<KIGFX::VIEW::LAYER_ITEM_PAIR> items;
            int net = 0;
            int clearance = 0;
            BOX2I bbox = aVia->GetBoundingBox();
            auto view = m_frame->GetGalCanvas()->GetView();

            view->Query( bbox, items );

            for( auto it : items )
            {
                BOARD_ITEM* item = static_cast<BOARD_ITEM*>( it.first );

                if( !(item->GetLayerSet() & lset ).any() )
                    continue;

                if( auto track = dyn_cast<TRACK*>( item ) )
                {
                    int max_clearance = std::max( clearance, track->GetClearance() );

                    if( TestSegmentHit( aVia->GetPosition(), track->GetStart(), track->GetEnd(),
                            ( track->GetWidth() + aVia->GetWidth() ) / 2  + max_clearance ) )
                    {
                        if( net && track->GetNetCode() != net )
                            return true;

                        net = track->GetNetCode();
                        clearance = track->GetClearance();
                    }
                }

                if( auto mod = dyn_cast<MODULE*>( item ) )
                {
                    for( auto pad : mod->Pads() )
                    {
                        int max_clearance = std::max( clearance, pad->GetClearance() );

                        if( pad->HitTest( aVia->GetBoundingBox(), false, max_clearance ) )
                        {
                            if( net && pad->GetNetCode() != net )
                                return true;

                            net = pad->GetNetCode();
                            clearance = pad->GetClearance();
                        }
                    }
                }
            }

            return false;
        }


        int findStitchedZoneNet( VIA* aVia )
        {
            const auto  pos     = aVia->GetPosition();
            const auto  lset    = aVia->GetLayerSet();

            for( auto mod : m_board->Modules() )
            {
                for( D_PAD* pad : mod->Pads() )
                {
                    if( pad->HitTest( pos ) && ( pad->GetLayerSet() & lset ).any() )
                        return -1;
                }
            }

            std::vector<ZONE_CONTAINER*> foundZones;

            for( auto zone : m_board->Zones() )
            {
                if( zone->HitTestFilledArea( pos ) )
                {
                    foundZones.push_back( zone );
                }
            }

            std::sort( foundZones.begin(), foundZones.end(),
                    [] ( const ZONE_CONTAINER* a, const ZONE_CONTAINER* b ) {
                return a->GetLayer() < b->GetLayer();
            } );

            // first take the net of the active layer
            for( auto z : foundZones )
            {
                if( m_frame->GetActiveLayer() == z->GetLayer() )
                    return z->GetNetCode();
            }

            // none? take the topmost visible layer
            for( auto z : foundZones )
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
            wxPoint pos = via->GetPosition();
            TRACK*  track = findTrack( via );

            if( track )
            {
                SEG         trackSeg( track->GetStart(), track->GetEnd() );
                VECTOR2I    snap = m_gridHelper.AlignToSegment( pos, trackSeg );

                aItem->SetPosition( wxPoint( snap.x, snap.y ) );
            }
        }

        bool PlaceItem( BOARD_ITEM* aItem, BOARD_COMMIT& aCommit ) override
        {
            auto    via = static_cast<VIA*>( aItem );
            int     newNet;
            TRACK*  track = findTrack( via );

            if( hasDRCViolation( via ) )
                return false;

            if( track )
            {
                aCommit.Modify( track );
                TRACK* newTrack = dynamic_cast<TRACK*>( track->Clone() );
                track->SetEnd( via->GetPosition() );
                newTrack->SetStart( via->GetPosition() );
                aCommit.Add( newTrack );

                newNet = track->GetNetCode();
            }
            else
                newNet = findStitchedZoneNet( via );

            if( newNet > 0 )
                via->SetNetCode( newNet );

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
            case VIA_BLIND_BURIED:
                via->SetLayerPair( first_layer, last_layer );
                break;

            case VIA_MICROVIA:    // from external to the near neighbor inner layer
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

                // Update diameter and hole size, which where set previously
                // for normal vias
                NETINFO_ITEM* net = via->GetNet();

                if( net )
                {
                    via->SetWidth( net->GetMicroViaSize() );
                    via->SetDrill( net->GetMicroViaDrillSize() );
                }
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

    frame()->SetToolID( ID_PCB_DRAW_VIA_BUTT, wxCURSOR_PENCIL, _( "Add vias" ) );

    doInteractiveItemPlacement( &placer, _( "Place via" ),
            IPO_REPEAT | IPO_SINGLE_CLICK | IPO_ROTATE | IPO_FLIP );

    frame()->SetToolID( ID_NO_TOOL_SELECTED, wxCURSOR_DEFAULT, wxEmptyString );

    return 0;
}


void DRAWING_TOOL::setTransitions()
{
    Go( &DRAWING_TOOL::DrawLine, PCB_ACTIONS::drawLine.MakeEvent() );
    Go( &DRAWING_TOOL::DrawGraphicPolygon, PCB_ACTIONS::drawGraphicPolygon.MakeEvent() );
    Go( &DRAWING_TOOL::DrawCircle, PCB_ACTIONS::drawCircle.MakeEvent() );
    Go( &DRAWING_TOOL::DrawArc, PCB_ACTIONS::drawArc.MakeEvent() );
    Go( &DRAWING_TOOL::DrawDimension, PCB_ACTIONS::drawDimension.MakeEvent() );
    Go( &DRAWING_TOOL::DrawZone, PCB_ACTIONS::drawZone.MakeEvent() );
    Go( &DRAWING_TOOL::DrawZoneKeepout, PCB_ACTIONS::drawZoneKeepout.MakeEvent() );
    Go( &DRAWING_TOOL::DrawZoneCutout, PCB_ACTIONS::drawZoneCutout.MakeEvent() );
    Go( &DRAWING_TOOL::DrawSimilarZone, PCB_ACTIONS::drawSimilarZone.MakeEvent() );
    Go( &DRAWING_TOOL::DrawVia, PCB_ACTIONS::drawVia.MakeEvent() );
    Go( &DRAWING_TOOL::PlaceText, PCB_ACTIONS::placeText.MakeEvent() );
    Go( &DRAWING_TOOL::PlaceImportedGraphics, PCB_ACTIONS::placeImportedGraphics.MakeEvent() );
    Go( &DRAWING_TOOL::SetAnchor, PCB_ACTIONS::setAnchor.MakeEvent() );
}


int DRAWING_TOOL::getSegmentWidth( PCB_LAYER_ID aLayer ) const
{
    assert( m_board );
    return m_board->GetDesignSettings().GetLineThickness( aLayer );
}


PCB_LAYER_ID DRAWING_TOOL::getDrawingLayer() const
{
    PCB_LAYER_ID layer = m_frame->GetActiveLayer();

    if( IsCopperLayer( layer ) )
    {
        if( layer == F_Cu )
            layer = F_SilkS;
        else if( layer == B_Cu )
            layer = B_SilkS;
        else
            layer = Dwgs_User;

        m_frame->SetActiveLayer( layer );
    }

    return layer;
}


const unsigned int DRAWING_TOOL::WIDTH_STEP = 100000;
