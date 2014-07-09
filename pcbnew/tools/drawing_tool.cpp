/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 CERN
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

#include <boost/bind.hpp>
#include <cstdio>

#include "drawing_tool.h"
#include "common_actions.h"

#include <wxPcbStruct.h>
#include <class_draw_panel_gal.h>
#include <project.h>
#include <id.h>
#include <pcbnew_id.h>
#include <confirm.h>
#include <dialog_edit_module_text.h>
#include <import_dxf/dialog_dxf_import.h>

#include <view/view_group.h>
#include <view/view_controls.h>
#include <gal/graphics_abstraction_layer.h>
#include <tool/tool_manager.h>
#include <router/direction.h>

#include <class_board.h>
#include <class_edge_mod.h>
#include <class_pcb_text.h>
#include <class_dimension.h>
#include <class_mire.h>
#include <class_zone.h>
#include <class_module.h>

DRAWING_TOOL::DRAWING_TOOL() :
    TOOL_INTERACTIVE( "pcbnew.InteractiveDrawing" ), m_editModules( false )
{
}


DRAWING_TOOL::~DRAWING_TOOL()
{
}


void DRAWING_TOOL::Reset( RESET_REASON aReason )
{
    // Init variables used by every drawing tool
    m_view = getView();
    m_controls = getViewControls();
    m_board = getModel<BOARD>();
    m_frame = getEditFrame<PCB_EDIT_FRAME>();

    setTransitions();
}


int DRAWING_TOOL::DrawLine( TOOL_EVENT& aEvent )
{
    if( m_editModules )
    {
        m_frame->SetToolID( ID_MODEDIT_LINE_TOOL, wxCURSOR_PENCIL, _( "Add graphic line" ) );

        MODULE* module = m_board->m_Modules;
        EDGE_MODULE* line = new EDGE_MODULE( module );

        while( drawSegment( S_SEGMENT, line ) )
        {
            m_frame->OnModify();
            m_frame->SaveCopyInUndoList( module, UR_MODEDIT );
            line->SetLocalCoord();
            line->SetParent( module );
            module->GraphicalItems().PushFront( line );

            line = new EDGE_MODULE( module );
        }
    }
    else
    {
        m_frame->SetToolID( ID_PCB_ADD_LINE_BUTT, wxCURSOR_PENCIL, _( "Add graphic line" ) );

        DRAWSEGMENT* line = new DRAWSEGMENT;

        while( drawSegment( S_SEGMENT, line ) )
        {
            m_board->Add( line );
            m_frame->OnModify();
            m_frame->SaveCopyInUndoList( line, UR_NEW );

            line = new DRAWSEGMENT;
        }
    }

    setTransitions();
    m_frame->SetToolID( ID_NO_TOOL_SELECTED, wxCURSOR_DEFAULT, wxEmptyString );

    return 0;
}


int DRAWING_TOOL::DrawCircle( TOOL_EVENT& aEvent )
{
    if( m_editModules )
    {
        m_frame->SetToolID( ID_MODEDIT_CIRCLE_TOOL, wxCURSOR_PENCIL, _( "Add graphic circle" ) );

        MODULE* module = m_board->m_Modules;
        EDGE_MODULE* circle = new EDGE_MODULE( module );

        while( drawSegment( S_CIRCLE, circle ) )
        {
            m_frame->OnModify();
            m_frame->SaveCopyInUndoList( module, UR_MODEDIT );
            circle->SetLocalCoord();
            circle->SetParent( module );
            module->GraphicalItems().PushFront( circle );

            circle = new EDGE_MODULE( module );
        }
    }
    else
    {
        m_frame->SetToolID( ID_PCB_CIRCLE_BUTT, wxCURSOR_PENCIL, _( "Add graphic circle" ) );

        DRAWSEGMENT* circle = new DRAWSEGMENT;

        while( drawSegment( S_CIRCLE, circle ) )
        {
            m_board->Add( circle );
            m_frame->OnModify();
            m_frame->SaveCopyInUndoList( circle, UR_NEW );

            circle = new DRAWSEGMENT;
        }
    }

    setTransitions();
    m_frame->SetToolID( ID_NO_TOOL_SELECTED, wxCURSOR_DEFAULT, wxEmptyString );

    return 0;
}


int DRAWING_TOOL::DrawArc( TOOL_EVENT& aEvent )
{
    if( m_editModules )
    {
        m_frame->SetToolID( ID_MODEDIT_ARC_TOOL, wxCURSOR_PENCIL, _( "Add graphic arc" ) );

        MODULE* module = m_board->m_Modules;
        EDGE_MODULE* arc = new EDGE_MODULE( module );

        while( drawArc( arc ) )
        {
            m_frame->OnModify();
            m_frame->SaveCopyInUndoList( module, UR_MODEDIT );
            arc->SetLocalCoord();
            arc->SetParent( module );
            module->GraphicalItems().PushFront( arc );

            arc = new EDGE_MODULE( module );
        }
    }
    else
    {
        m_frame->SetToolID( ID_PCB_ARC_BUTT, wxCURSOR_PENCIL, _( "Add graphic arc" ) );

        DRAWSEGMENT* arc = new DRAWSEGMENT;

        while( drawArc( arc ) )
        {
            m_board->Add( arc );
            m_frame->OnModify();
            m_frame->SaveCopyInUndoList( arc, UR_NEW );

            arc = new DRAWSEGMENT;
        }
    }

    setTransitions();
    m_frame->SetToolID( ID_NO_TOOL_SELECTED, wxCURSOR_DEFAULT, wxEmptyString );

    return 0;
}


int DRAWING_TOOL::PlaceText( TOOL_EVENT& aEvent )
{
    if( m_editModules )
        return placeTextModule();
    else
        return placeTextPcb();
}


int DRAWING_TOOL::DrawDimension( TOOL_EVENT& aEvent )
{
    DIMENSION* dimension = NULL;
    int width, maxThickness;

    // Add a VIEW_GROUP that serves as a preview for the new item
    KIGFX::VIEW_GROUP preview( m_view );
    m_view->Add( &preview );

    m_toolMgr->RunAction( COMMON_ACTIONS::selectionClear );
    m_controls->ShowCursor( true );
    m_controls->SetSnapping( true );

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
        VECTOR2I cursorPos = m_controls->GetCursorPosition();

        if( evt->IsCancel() || evt->IsActivate() )
        {
            if( step != SET_ORIGIN )    // start from the beginning
            {
                preview.Clear();
                preview.ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY );

                delete dimension;
                step = SET_ORIGIN;
            }
            else
                break;

            if( evt->IsActivate() )  // now finish unconditionally
                break;
        }

        else if( evt->IsKeyPressed() && step != SET_ORIGIN )
        {
            width = dimension->GetWidth();

            // Modify the new item width
            if( evt->KeyCode() == '-' && width > WIDTH_STEP )
                dimension->SetWidth( width - WIDTH_STEP );
            else if( evt->KeyCode() == '=' )
                dimension->SetWidth( width + WIDTH_STEP );

            preview.ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY );
        }

        else if( evt->IsClick( BUT_LEFT ) )
        {
            switch( step )
            {
            case SET_ORIGIN:
                {
                    LAYER_ID layer = m_frame->GetScreen()->m_Active_Layer;

                    if( IsCopperLayer( layer ) || layer == Edge_Cuts )
                    {
                        DisplayInfoMessage( NULL, _( "Dimension not allowed on Copper or Edge Cut layers" ) );
                        --step;
                    }
                    else
                    {
                        // Init the new item attributes
                        dimension = new DIMENSION( m_board );
                        dimension->SetLayer( layer );
                        dimension->SetOrigin( wxPoint( cursorPos.x, cursorPos.y ) );
                        dimension->SetEnd( wxPoint( cursorPos.x, cursorPos.y ) );
                        dimension->Text().SetSize( m_board->GetDesignSettings().m_PcbTextSize );

                        width = m_board->GetDesignSettings().m_PcbTextWidth;
                        maxThickness = Clamp_Text_PenSize( width, dimension->Text().GetSize() );

                        if( width > maxThickness )
                            width = maxThickness;

                        dimension->Text().SetThickness( width );
                        dimension->SetWidth( width );
                        dimension->AdjustDimensionDetails();

                        preview.Add( dimension );

                        m_controls->SetAutoPan( true );
                    }
                }
                break;

            case SET_END:
                dimension->SetEnd( wxPoint( cursorPos.x, cursorPos.y ) );

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

                        m_view->Add( dimension );
                        m_board->Add( dimension );
                        dimension->ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY );

                        m_frame->OnModify();
                        m_frame->SaveCopyInUndoList( dimension, UR_NEW );

                        preview.Remove( dimension );
                    }
                }
                break;
            }

            if( ++step == FINISHED )
            {
                step = SET_ORIGIN;
                m_controls->SetAutoPan( false );
            }
        }

        else if( evt->IsMotion() )
        {
            switch( step )
            {
            case SET_END:
                dimension->SetEnd( wxPoint( cursorPos.x, cursorPos.y ) );
                break;

            case SET_HEIGHT:
            {
                // Calculating the direction of travel perpendicular to the selected axis
                double angle = dimension->GetAngle() + ( M_PI / 2 );

                wxPoint pos( cursorPos.x, cursorPos.y );
                wxPoint delta( pos - dimension->m_featureLineDO );
                double height  = ( delta.x * cos( angle ) ) + ( delta.y * sin( angle ) );
                dimension->SetHeight( height );
            }
            break;
            }

            // Show a preview of the item
            preview.ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY );
        }
    }

    if( step != SET_ORIGIN )
        delete dimension;

    m_controls->ShowCursor( false );
    m_controls->SetSnapping( false );
    m_controls->SetAutoPan( false );
    m_view->Remove( &preview );

    setTransitions();
    m_frame->SetToolID( ID_NO_TOOL_SELECTED, wxCURSOR_DEFAULT, wxEmptyString );

    return 0;
}


int DRAWING_TOOL::DrawZone( TOOL_EVENT& aEvent )
{
    m_frame->SetToolID( ID_PCB_ZONES_BUTT, wxCURSOR_PENCIL, _( "Add zones" ) );

    return drawZone( false );
}


int DRAWING_TOOL::DrawKeepout( TOOL_EVENT& aEvent )
{
    m_frame->SetToolID( ID_PCB_KEEPOUT_AREA_BUTT, wxCURSOR_PENCIL, _( "Add keepout" ) );

    return drawZone( true );
}


int DRAWING_TOOL::PlaceTarget( TOOL_EVENT& aEvent )
{
    PCB_TARGET* target = new PCB_TARGET( m_board );

    // Init the new item attributes
    target->SetLayer( Edge_Cuts );
    target->SetWidth( m_board->GetDesignSettings().m_EdgeSegmentWidth );
    target->SetSize( Millimeter2iu( 5 ) );
    VECTOR2I cursorPos = m_controls->GetCursorPosition();
    target->SetPosition( wxPoint( cursorPos.x, cursorPos.y ) );

    // Add a VIEW_GROUP that serves as a preview for the new item
    KIGFX::VIEW_GROUP preview( m_view );
    preview.Add( target );
    m_view->Add( &preview );
    preview.ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY );

    m_toolMgr->RunAction( COMMON_ACTIONS::selectionClear );
    m_controls->SetSnapping( true );
    m_controls->SetAutoPan( true );

    Activate();
    m_frame->SetToolID( ID_PCB_MIRE_BUTT, wxCURSOR_PENCIL, _( "Add layer alignment target" ) );

    // Main loop: keep receiving events
    while( OPT_TOOL_EVENT evt = Wait() )
    {
        cursorPos = m_controls->GetCursorPosition();

        if( evt->IsCancel() || evt->IsActivate() )
            break;

        else if( evt->IsKeyPressed() )
        {
            int width = target->GetWidth();

            // Modify the new item width
            if( evt->KeyCode() == '-' && width > WIDTH_STEP )
                target->SetWidth( width - WIDTH_STEP );
            else if( evt->KeyCode() == '=' )
                target->SetWidth( width + WIDTH_STEP );

            preview.ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY );
        }

        else if( evt->IsClick( BUT_LEFT ) )
        {
            assert( target->GetSize() > 0 );
            assert( target->GetWidth() > 0 );

            m_view->Add( target );
            m_board->Add( target );
            target->ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY );

            m_frame->OnModify();
            m_frame->SaveCopyInUndoList( target, UR_NEW );

            preview.Remove( target );

            // Create next PCB_TARGET
            target = new PCB_TARGET( *target );
            preview.Add( target );
        }

        else if( evt->IsMotion() )
        {
            target->SetPosition( wxPoint( cursorPos.x, cursorPos.y ) );
            preview.ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY );
        }
    }

    delete target;

    m_controls->SetSnapping( false );
    m_controls->SetAutoPan( false );
    m_view->Remove( &preview );

    setTransitions();
    m_frame->SetToolID( ID_NO_TOOL_SELECTED, wxCURSOR_DEFAULT, wxEmptyString );

    return 0;
}


int DRAWING_TOOL::PlaceModule( TOOL_EVENT& aEvent )
{
    MODULE* module = NULL;

    // Add a VIEW_GROUP that serves as a preview for the new item
    KIGFX::VIEW_GROUP preview( m_view );
    m_view->Add( &preview );

    m_toolMgr->RunAction( COMMON_ACTIONS::selectionClear );
    m_controls->ShowCursor( true );
    m_controls->SetSnapping( true );
    m_controls->SetAutoPan( true );

    Activate();
    m_frame->SetToolID( ID_PCB_MODULE_BUTT, wxCURSOR_HAND, _( "Add module" ) );

    // Main loop: keep receiving events
    while( OPT_TOOL_EVENT evt = Wait() )
    {
        VECTOR2I cursorPos = m_controls->GetCursorPosition();

        if( evt->IsCancel() || evt->IsActivate() )
        {
            if( module )
            {
                m_board->Delete( module );  // it was added by LoadModuleFromLibrary()
                module = NULL;

                preview.Clear();
                preview.ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY );
                m_controls->ShowCursor( true );
            }
            else
                break;

            if( evt->IsActivate() )  // now finish unconditionally
                break;
        }

        else if( module && evt->Category() == TC_COMMAND )
        {
            if( evt->IsAction( &COMMON_ACTIONS::rotate ) )
            {
                module->Rotate( module->GetPosition(), m_frame->GetRotationAngle() );
                preview.ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY );
            }
            else if( evt->IsAction( &COMMON_ACTIONS::flip ) )
            {
                module->Flip( module->GetPosition() );
                preview.ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY );
            }
        }

        else if( evt->IsClick( BUT_LEFT ) )
        {
            if( !module )
            {
                // Init the new item attributes
                module = m_frame->LoadModuleFromLibrary( wxEmptyString,
                                                         m_frame->Prj().PcbFootprintLibs(),
                                                         true, NULL );
                if( module == NULL )
                    continue;

                m_controls->ShowCursor( false );
                module->SetPosition( wxPoint( cursorPos.x, cursorPos.y ) );

                // Add all the drawable parts to preview
                preview.Add( module );
                module->RunOnChildren( boost::bind( &KIGFX::VIEW_GROUP::Add, &preview, _1 ) );

                preview.ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY );
            }
            else
            {
                module->RunOnChildren( boost::bind( &KIGFX::VIEW::Add, m_view, _1 ) );
                m_view->Add( module );
                module->ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY );

                m_frame->OnModify();
                m_frame->SaveCopyInUndoList( module, UR_NEW );

                // Remove from preview
                preview.Remove( module );
                module->RunOnChildren( boost::bind( &KIGFX::VIEW_GROUP::Remove, &preview, _1 ) );
                module = NULL;  // to indicate that there is no module that we currently modify

                m_controls->ShowCursor( true );
            }
        }

        else if( module && evt->IsMotion() )
        {
            module->SetPosition( wxPoint( cursorPos.x, cursorPos.y ) );
            preview.ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY );
        }
    }

    m_controls->ShowCursor( false );
    m_controls->SetSnapping( false );
    m_controls->SetAutoPan( false );
    m_view->Remove( &preview );

    setTransitions();
    m_frame->SetToolID( ID_NO_TOOL_SELECTED, wxCURSOR_DEFAULT, wxEmptyString );

    return 0;
}


int DRAWING_TOOL::PlacePad( TOOL_EVENT& aEvent )
{
    assert( m_editModules );

    m_frame->SetToolID( ID_MODEDIT_PAD_TOOL, wxCURSOR_PENCIL, _( "Add pads" ) );

    MODULE* module = m_board->m_Modules;
    assert( module );

    D_PAD* pad = new D_PAD( module );
    m_frame->Import_Pad_Settings( pad, false );     // use the global settings for pad

    VECTOR2I cursorPos = m_controls->GetCursorPosition();
    pad->SetPosition( wxPoint( cursorPos.x, cursorPos.y ) );

    // Add a VIEW_GROUP that serves as a preview for the new item
    KIGFX::VIEW_GROUP preview( m_view );
    preview.Add( pad );
    m_view->Add( &preview );

    m_toolMgr->RunAction( COMMON_ACTIONS::selectionClear );
    m_controls->ShowCursor( true );
    m_controls->SetSnapping( true );

    Activate();

    // Main loop: keep receiving events
    while( OPT_TOOL_EVENT evt = Wait() )
    {
        cursorPos = m_controls->GetCursorPosition();

        if( evt->IsMotion() )
        {
            pad->SetPosition( wxPoint( cursorPos.x, cursorPos.y ) );
            preview.ViewUpdate();
        }

        else if( evt->Category() == TC_COMMAND )
        {
            if( evt->IsAction( &COMMON_ACTIONS::rotate ) )
            {
                pad->Rotate( pad->GetPosition(), m_frame->GetRotationAngle() );
                preview.ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY );
            }
            else if( evt->IsAction( &COMMON_ACTIONS::flip ) )
            {
                pad->Flip( pad->GetPosition() );
                preview.ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY );
            }
            else if( evt->IsCancel() || evt->IsActivate() )
            {
                preview.Clear();
                delete pad;
                break;
            }
        }

        else if( evt->IsClick( BUT_LEFT ) )
        {
            m_frame->OnModify();
            m_frame->SaveCopyInUndoList( module, UR_MODEDIT );

            m_board->m_Status_Pcb = 0;    // I have no clue why, but it is done in the legacy view
            module->SetLastEditTime();
            module->Pads().PushBack( pad );

            pad->SetNetCode( NETINFO_LIST::UNCONNECTED );

            // Set the relative pad position
            // ( pad position for module orient, 0, and relative to the module position)
            pad->SetLocalCoord();

            /* NPTH pads take empty pad number (since they can't be connected),
             * other pads get incremented from the last one edited */
            wxString padName;

            if( pad->GetAttribute() != PAD_HOLE_NOT_PLATED )
                padName = getNextPadName();

            pad->SetPadName( padName );

            // Handle the view aspect
            preview.Remove( pad );
            m_view->Add( pad );

            // Start placing next pad
            pad = new D_PAD( module );
            m_frame->Import_Pad_Settings( pad, false );
            preview.Add( pad );
        }
    }

    m_controls->ShowCursor( false );
    m_controls->SetSnapping( false );
    m_controls->SetAutoPan( false );
    m_view->Remove( &preview );

    setTransitions();
    m_frame->SetToolID( ID_NO_TOOL_SELECTED, wxCURSOR_DEFAULT, wxEmptyString );

    return 0;
}


int DRAWING_TOOL::PlaceDXF( TOOL_EVENT& aEvent )
{
    DIALOG_DXF_IMPORT dlg( m_frame );
    int dlgResult = dlg.ShowModal();

    const std::list<BOARD_ITEM*>& list = dlg.GetImportedItems();
    MODULE* module = m_board->m_Modules;

    if( dlgResult != wxID_OK || module == NULL || list.empty() )
    {
        setTransitions();

        return 0;
    }

    VECTOR2I cursorPos = m_controls->GetCursorPosition();
    VECTOR2I delta = cursorPos - (*list.begin())->GetPosition();

    // Add a VIEW_GROUP that serves as a preview for the new item
    KIGFX::VIEW_GROUP preview( m_view );

    // Build the undo list & add items to the current view
    std::list<BOARD_ITEM*>::const_iterator it, itEnd;
    for( it = list.begin(), itEnd = list.end(); it != itEnd; ++it )
    {
        BOARD_ITEM* item = *it;
        BOARD_ITEM* converted = NULL;

        // Modules use different types for the same things,
        // so we need to convert imported items to appropriate classes.
        switch( item->Type() )
        {
        case PCB_LINE_T:
        {
            if( m_editModules )
            {
                converted = new EDGE_MODULE( module );
                *static_cast<DRAWSEGMENT*>( converted ) = *static_cast<DRAWSEGMENT*>( item );
                converted->Move( wxPoint( delta.x, delta.y ) );
                preview.Add( converted );
                delete item;
            }
            else
            {
                preview.Add( item );
            }

            break;
        }

        case PCB_TEXT_T:
        {
            if( m_editModules )
            {
                converted = new TEXTE_MODULE( module );
                *static_cast<TEXTE_PCB*>( converted ) = *static_cast<TEXTE_PCB*>( item );
                converted->Move( wxPoint( delta.x, delta.y ) );
                preview.Add( converted );
                delete item;
            }
            else
            {
                preview.Add( item );
            }
            break;
        }

        default:
            assert( false );    // there is a type that is currently not handled here
            break;
        }
    }

    BOARD_ITEM* firstItem = static_cast<BOARD_ITEM*>( *preview.Begin() );
    m_view->Add( &preview );

    m_toolMgr->RunAction( COMMON_ACTIONS::selectionClear );
    m_controls->ShowCursor( true );
    m_controls->SetSnapping( true );

    Activate();

    // Main loop: keep receiving events
    while( OPT_TOOL_EVENT evt = Wait() )
    {
        cursorPos = m_controls->GetCursorPosition();

        if( evt->IsMotion() )
        {
            delta = cursorPos - firstItem->GetPosition();

            for( KIGFX::VIEW_GROUP::iter it = preview.Begin(), end = preview.End(); it != end; ++it )
                static_cast<BOARD_ITEM*>( *it )->Move( wxPoint( delta.x, delta.y ) );

            preview.ViewUpdate();
        }

        else if( evt->Category() == TC_COMMAND )
        {
            if( evt->IsAction( &COMMON_ACTIONS::rotate ) )
            {
                for( KIGFX::VIEW_GROUP::iter it = preview.Begin(), end = preview.End(); it != end; ++it )
                    static_cast<BOARD_ITEM*>( *it )->Rotate( wxPoint( cursorPos.x, cursorPos.y ),
                                                             m_frame->GetRotationAngle() );

                preview.ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY );
            }
            else if( evt->IsAction( &COMMON_ACTIONS::flip ) )
            {
                for( KIGFX::VIEW_GROUP::iter it = preview.Begin(), end = preview.End(); it != end; ++it )
                    static_cast<BOARD_ITEM*>( *it )->Flip( wxPoint( cursorPos.x, cursorPos.y ) );

                preview.ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY );
            }
            else if( evt->IsCancel() || evt->IsActivate() )
            {
                preview.FreeItems();
                break;
            }
        }

        else if( evt->IsClick( BUT_LEFT ) )
        {
            // Place the drawing

            if( m_editModules )
            {
                m_frame->SaveCopyInUndoList( module, UR_MODEDIT );
                module->SetLastEditTime();

                for( KIGFX::VIEW_GROUP::iter it = preview.Begin(), end = preview.End(); it != end; ++it )
                {
                    BOARD_ITEM* item = static_cast<BOARD_ITEM*>( *it );
                    module->Add( item );

                    switch( item->Type() )
                    {
                    case PCB_MODULE_TEXT_T:
                        static_cast<TEXTE_MODULE*>( item )->SetLocalCoord();
                        break;

                    case PCB_MODULE_EDGE_T:
                        static_cast<EDGE_MODULE*>( item )->SetLocalCoord();
                        break;

                    default:
                        assert( false );
                        break;
                    }

                    m_view->Add( item );
                }
            }
            else
            {
                PICKED_ITEMS_LIST picklist;

                for( KIGFX::VIEW_GROUP::iter it = preview.Begin(), end = preview.End(); it != end; ++it )
                {
                    BOARD_ITEM* item = static_cast<BOARD_ITEM*>( *it );
                    m_board->Add( item );

                    ITEM_PICKER itemWrapper( item, UR_NEW );
                    picklist.PushItem( itemWrapper );

                    m_view->Add( item );
                }

                m_frame->SaveCopyInUndoList( picklist, UR_NEW );
            }

            m_frame->OnModify();

            break;
        }
    }

    preview.Clear();

    m_controls->ShowCursor( false );
    m_controls->SetSnapping( false );
    m_controls->SetAutoPan( false );
    m_view->Remove( &preview );

    setTransitions();

    return 0;
}


int DRAWING_TOOL::SetAnchor( TOOL_EVENT& aEvent )
{
    assert( m_editModules );

    Activate();
    m_frame->SetToolID( ID_MODEDIT_ANCHOR_TOOL, wxCURSOR_PENCIL,
                        _( "Place the footprint anchor" ) );

    KIGFX::VIEW_CONTROLS* controls = getViewControls();
    controls->ShowCursor( true );
    controls->SetSnapping( true );
    controls->SetAutoPan( true );

    while( OPT_TOOL_EVENT evt = Wait() )
    {
        if( evt->IsClick( BUT_LEFT ) )
        {
            MODULE* module = m_board->m_Modules;

            m_frame->SaveCopyInUndoList( module, UR_MODEDIT );

            // set the new relative internal local coordinates of footprint items
            VECTOR2I cursorPos = controls->GetCursorPosition();
            wxPoint moveVector = module->GetPosition() - wxPoint( cursorPos.x, cursorPos.y );
            module->MoveAnchorPosition( moveVector );

            // Usually, we do not need to change twice the anchor position,
            // so deselect the active tool
            break;
        }

        else if( evt->IsCancel() || evt->IsActivate() )
            break;
    }

    controls->SetAutoPan( false );
    controls->SetSnapping( false );
    controls->ShowCursor( false );

    setTransitions();
    m_frame->SetToolID( ID_NO_TOOL_SELECTED, wxCURSOR_DEFAULT, wxEmptyString );

    return 0;
}


bool DRAWING_TOOL::drawSegment( int aShape, DRAWSEGMENT* aGraphic )
{
    // Only two shapes are currently supported
    assert( aShape == S_SEGMENT || aShape == S_CIRCLE );

    DRAWSEGMENT line45;

    // Add a VIEW_GROUP that serves as a preview for the new item
    KIGFX::VIEW_GROUP preview( m_view );
    m_view->Add( &preview );

    m_toolMgr->RunAction( COMMON_ACTIONS::selectionClear );
    m_controls->ShowCursor( true );
    m_controls->SetSnapping( true );

    Activate();

    bool direction45 = false;       // 45 degrees only mode
    bool started = false;

    // Main loop: keep receiving events
    while( OPT_TOOL_EVENT evt = Wait() )
    {
        bool updatePreview = false;            // should preview be updated
        VECTOR2I cursorPos = m_controls->GetCursorPosition();

        // Enable 45 degrees lines only mode by holding control
        if( direction45 != evt->Modifier( MD_CTRL ) && started && aShape == S_SEGMENT )
        {
            direction45 = evt->Modifier( MD_CTRL );

            if( direction45 )
            {
                preview.Add( &line45 );
                make45DegLine( aGraphic, &line45 );
            }
            else
            {
                preview.Remove( &line45 );
                aGraphic->SetEnd( wxPoint( cursorPos.x, cursorPos.y ) );
            }

            updatePreview = true;
        }

        if( evt->IsCancel() || evt->IsActivate() )
        {
            preview.Clear();
            updatePreview = true;
            delete aGraphic;
            break;
        }

        else if( evt->IsKeyPressed() )
        {
            int width = aGraphic->GetWidth();

            // Modify the new item width
            if( evt->KeyCode() == '-' && width > WIDTH_STEP )   // TODO change it to TOOL_ACTIONs
                aGraphic->SetWidth( width - WIDTH_STEP );
            else if( evt->KeyCode() == '=' )
                aGraphic->SetWidth( width + WIDTH_STEP );

            updatePreview = true;
        }

        else if( evt->IsClick( BUT_LEFT ) )
        {
            if( !started )
            {
                LAYER_ID layer = m_frame->GetScreen()->m_Active_Layer;

                if( IsCopperLayer( layer ) )
                {
                    DisplayInfoMessage( NULL, _( "Graphic not allowed on Copper layers" ) );
                }
                else
                {
                    // Init the new item attributes
                    aGraphic->SetShape( (STROKE_T) aShape );
                    aGraphic->SetWidth( m_board->GetDesignSettings().m_DrawSegmentWidth );
                    aGraphic->SetStart( wxPoint( cursorPos.x, cursorPos.y ) );
                    aGraphic->SetEnd( wxPoint( cursorPos.x, cursorPos.y ) );
                    aGraphic->SetLayer( layer );

                    if( aShape == S_SEGMENT )
                    {
                        line45 = *aGraphic; // used only for direction 45 mode with lines
                        line45.SetLayer( layer );
                    }

                    preview.Add( aGraphic );
                    m_controls->SetAutoPan( true );

                    started = true;
                }
            }
            else
            {
                if( aGraphic->GetEnd() != aGraphic->GetStart() )
                {
                    assert( aGraphic->GetLength() > 0 );
                    assert( aGraphic->GetWidth() > 0 );

                    m_view->Add( aGraphic );
                    aGraphic->ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY );
                }
                else                            // User has clicked twice in the same spot
                {                               // a clear sign that the current drawing is finished
                    delete aGraphic;            // but only if at least one graphic was created
                    started = false;            // otherwise - force user to draw more or cancel
                }

                preview.Clear();
                break;
            }
        }

        else if( evt->IsMotion() )
        {
            // 45 degree lines
            if( direction45 && aShape == S_SEGMENT )
                make45DegLine( aGraphic, &line45 );
            else
                aGraphic->SetEnd( wxPoint( cursorPos.x, cursorPos.y ) );

            updatePreview = true;
        }

        if( updatePreview )
            preview.ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY );
    }

    m_controls->ShowCursor( false );
    m_controls->SetSnapping( false );
    m_controls->SetAutoPan( false );
    m_view->Remove( &preview );

    return started;
}


bool DRAWING_TOOL::drawArc( DRAWSEGMENT* aGraphic )
{
    bool clockwise = true;      // drawing direction of the arc
    double startAngle = 0.0f;   // angle of the first arc line
    VECTOR2I cursorPos = m_controls->GetCursorPosition();

    DRAWSEGMENT helperLine;
    helperLine.SetShape( S_SEGMENT );
    helperLine.SetLayer( Dwgs_User );
    helperLine.SetWidth( 1 );

    // Add a VIEW_GROUP that serves as a preview for the new item
    KIGFX::VIEW_GROUP preview( m_view );
    m_view->Add( &preview );

    m_toolMgr->RunAction( COMMON_ACTIONS::selectionClear );
    m_controls->ShowCursor( true );
    m_controls->SetSnapping( true );

    Activate();

    enum ARC_STEPS
    {
        SET_ORIGIN = 0,
        SET_END,
        SET_ANGLE,
        FINISHED
    };
    int step = SET_ORIGIN;

    // Main loop: keep receiving events
    while( OPT_TOOL_EVENT evt = Wait() )
    {
        cursorPos = m_controls->GetCursorPosition();

        if( evt->IsCancel() || evt->IsActivate() )
        {
            preview.Clear();
            preview.ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY );
            delete aGraphic;
            break;
        }

        else if( evt->IsKeyPressed() && step != SET_ORIGIN )
        {
            int width = aGraphic->GetWidth();

            // Modify the new item width
            if( evt->KeyCode() == '-' && width > WIDTH_STEP )       // TODO convert to tool actions
                aGraphic->SetWidth( width - WIDTH_STEP );
            else if( evt->KeyCode() == '=' )
                aGraphic->SetWidth( width + WIDTH_STEP );
            else if( evt->KeyCode() == '/' )
            {
                if( clockwise )
                    aGraphic->SetAngle( aGraphic->GetAngle() - 3600.0 );
                else
                    aGraphic->SetAngle( aGraphic->GetAngle() + 3600.0 );

                clockwise = !clockwise;
            }

            preview.ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY );
        }

        else if( evt->IsClick( BUT_LEFT ) )
        {
            switch( step )
            {
            case SET_ORIGIN:
            {
                LAYER_ID layer = m_frame->GetScreen()->m_Active_Layer;

                if( IsCopperLayer( layer ) )
                {
                    DisplayInfoMessage( NULL, _( "Graphic not allowed on Copper layers" ) );
                    --step;
                }
                else
                {
                    // Init the new item attributes
                    aGraphic->SetShape( S_ARC );
                    aGraphic->SetAngle( 0.0 );
                    aGraphic->SetWidth( m_board->GetDesignSettings().m_DrawSegmentWidth );
                    aGraphic->SetCenter( wxPoint( cursorPos.x, cursorPos.y ) );
                    aGraphic->SetLayer( layer );

                    helperLine.SetStart( aGraphic->GetCenter() );
                    helperLine.SetEnd( aGraphic->GetCenter() );

                    preview.Add( aGraphic );
                    preview.Add( &helperLine );

                    m_controls->SetAutoPan( true );
                }
            }
            break;

            case SET_END:
            {
                if( wxPoint( cursorPos.x, cursorPos.y ) != aGraphic->GetCenter() )
                {
                    VECTOR2D startLine( aGraphic->GetArcStart() - aGraphic->GetCenter() );
                    startAngle = startLine.Angle();
                    aGraphic->SetArcStart( wxPoint( cursorPos.x, cursorPos.y ) );
                }
                else
                    --step;     // one another chance to draw a proper arc
            }
            break;

            case SET_ANGLE:
            {
                if( wxPoint( cursorPos.x, cursorPos.y ) != aGraphic->GetArcStart() )
                {
                    assert( aGraphic->GetAngle() != 0 );
                    assert( aGraphic->GetArcStart() != aGraphic->GetArcEnd() );
                    assert( aGraphic->GetWidth() > 0 );

                    m_view->Add( aGraphic );
                    aGraphic->ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY );

                    preview.Remove( aGraphic );
                    preview.Remove( &helperLine );
                }
                else
                    --step;     // one another chance to draw a proper arc
            }
            break;
            }

            if( ++step == FINISHED )
                break;
        }

        else if( evt->IsMotion() )
        {
            switch( step )
            {
            case SET_END:
                helperLine.SetEnd( wxPoint( cursorPos.x, cursorPos.y ) );
                aGraphic->SetArcStart( wxPoint( cursorPos.x, cursorPos.y ) );
                break;

            case SET_ANGLE:
            {
                VECTOR2D endLine( wxPoint( cursorPos.x, cursorPos.y ) - aGraphic->GetCenter() );
                double newAngle = RAD2DECIDEG( endLine.Angle() - startAngle );

                // Adjust the new angle to (counter)clockwise setting
                if( clockwise && newAngle < 0.0 )
                    newAngle += 3600.0;
                else if( !clockwise && newAngle > 0.0 )
                    newAngle -= 3600.0;

                aGraphic->SetAngle( newAngle );
            }
            break;
            }

            // Show a preview of the item
            preview.ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY );
        }
    }

    m_controls->ShowCursor( false );
    m_controls->SetSnapping( false );
    m_controls->SetAutoPan( false );
    m_view->Remove( &preview );

    return ( step > SET_ORIGIN );
}


int DRAWING_TOOL::drawZone( bool aKeepout )
{
    ZONE_CONTAINER* zone = NULL;
    DRAWSEGMENT line45;
    DRAWSEGMENT* helperLine = NULL;  // we will need more than one helper line

    // Add a VIEW_GROUP that serves as a preview for the new item
    KIGFX::VIEW_GROUP preview( m_view );
    m_view->Add( &preview );

    m_toolMgr->RunAction( COMMON_ACTIONS::selectionClear );
    m_controls->ShowCursor( true );
    m_controls->SetSnapping( true );

    Activate();

    VECTOR2I lastCursorPos = m_controls->GetCursorPosition();
    VECTOR2I origin;
    int numPoints = 0;
    bool direction45 = false;       // 45 degrees only mode

    // Main loop: keep receiving events
    while( OPT_TOOL_EVENT evt = Wait() )
    {
        bool updatePreview = false;            // should preview be updated
        VECTOR2I cursorPos = m_controls->GetCursorPosition();

        // Enable 45 degrees lines only mode by holding control
        if( direction45 != ( evt->Modifier( MD_CTRL ) && numPoints > 0 ) )
        {
            direction45 = evt->Modifier( MD_CTRL );

            if( direction45 )
            {
                preview.Add( &line45 );
                make45DegLine( helperLine, &line45 );
            }
            else
            {
                preview.Remove( &line45 );
                helperLine->SetEnd( wxPoint( cursorPos.x, cursorPos.y ) );
            }

            updatePreview = true;
        }

        if( evt->IsCancel() || evt->IsActivate() )
        {
            if( numPoints > 0 )         // cancel the current zone
            {
                delete zone;
                zone = NULL;
                m_controls->SetAutoPan( false );

                if( direction45 )
                {
                    preview.Remove( &line45 );
                    direction45 = false;
                }

                preview.FreeItems();
                updatePreview = true;

                numPoints = 0;
            }
            else                        // there is no zone currently drawn - just stop the tool
                break;

            if( evt->IsActivate() )  // now finish unconditionally
                break;
        }

        else if( evt->IsClick( BUT_LEFT ) )
        {
            // Check if it is double click / closing line (so we have to finish the zone)
            if( lastCursorPos == cursorPos || ( numPoints > 0 && cursorPos == origin ) )
            {
                if( numPoints > 2 )     // valid zone consists of more than 2 points
                {
                    assert( zone->GetNumCorners() > 2 );

                    // Finish the zone
                    zone->Outline()->CloseLastContour();
                    zone->Outline()->RemoveNullSegments();

                    m_board->Add( zone );
                    m_view->Add( zone );

                    if( !aKeepout )
                        static_cast<PCB_EDIT_FRAME*>( m_frame )->Fill_Zone( zone );

                    zone->ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY );

                    m_frame->OnModify();
                    m_frame->SaveCopyInUndoList( zone, UR_NEW );

                    zone = NULL;
                }
                else
                {
                    delete zone;
                    zone = NULL;
                }

                numPoints = 0;
                m_controls->SetAutoPan( false );

                if( direction45 )
                {
                    preview.Remove( &line45 );
                    direction45 = false;
                }

                preview.FreeItems();
                updatePreview = true;
            }
            else
            {
                if( numPoints == 0 )        // it's the first click
                {
                    // Get the current default settings for zones
                    ZONE_SETTINGS zoneInfo = m_frame->GetZoneSettings();
                    zoneInfo.m_CurrentZone_Layer = m_frame->GetScreen()->m_Active_Layer;

                    m_controls->SetAutoPan( true );

                    // Show options dialog
                    ZONE_EDIT_T dialogResult;
                    if( aKeepout )
                        dialogResult = InvokeKeepoutAreaEditor( m_frame, &zoneInfo );
                    else
                    {
                        if( IsCopperLayer( zoneInfo.m_CurrentZone_Layer ) )
                            dialogResult = InvokeCopperZonesEditor( m_frame, &zoneInfo );
                        else
                            dialogResult = InvokeNonCopperZonesEditor( m_frame, NULL, &zoneInfo );
                    }

                    if( dialogResult == ZONE_ABORT )
                    {
                        m_controls->SetAutoPan( false );
                        continue;
                    }

                    // Apply the selected settings
                    zone = new ZONE_CONTAINER( m_board );
                    zoneInfo.ExportSetting( *zone );
                    m_frame->GetGalCanvas()->SetTopLayer( zoneInfo.m_CurrentZone_Layer );

                    // Add the first point
                    zone->Outline()->Start( zoneInfo.m_CurrentZone_Layer,
                                            cursorPos.x, cursorPos.y,
                                            zone->GetHatchStyle() );
                    origin = cursorPos;

                    // Helper line represents the currently drawn line of the zone polygon
                    helperLine = new DRAWSEGMENT;
                    helperLine->SetShape( S_SEGMENT );
                    helperLine->SetWidth( 1 );
                    helperLine->SetLayer( zoneInfo.m_CurrentZone_Layer );
                    helperLine->SetStart( wxPoint( cursorPos.x, cursorPos.y ) );
                    helperLine->SetEnd( wxPoint( cursorPos.x, cursorPos.y ) );
                    line45 = *helperLine;

                    preview.Add( helperLine );
                }
                else
                {
                    zone->AppendCorner( helperLine->GetEnd() );
                    helperLine = new DRAWSEGMENT( *helperLine );
                    helperLine->SetStart( helperLine->GetEnd() );
                    preview.Add( helperLine );
                }

                ++numPoints;
                updatePreview = true;
            }

            lastCursorPos = cursorPos;
        }

        else if( evt->IsMotion() && numPoints > 0 )
        {
            // 45 degree lines
            if( direction45 )
                make45DegLine( helperLine, &line45 );
            else
                helperLine->SetEnd( wxPoint( cursorPos.x, cursorPos.y ) );

            // Show a preview of the item
            updatePreview = true;
        }

        if( updatePreview )
            preview.ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY );
    }

    m_controls->ShowCursor( false );
    m_controls->SetSnapping( false );
    m_controls->SetAutoPan( false );
    m_view->Remove( &preview );

    setTransitions();
    m_frame->SetToolID( ID_NO_TOOL_SELECTED, wxCURSOR_DEFAULT, wxEmptyString );

    return 0;
}


int DRAWING_TOOL::placeTextModule()
{
    TEXTE_MODULE* text = new TEXTE_MODULE( NULL );
    const BOARD_DESIGN_SETTINGS& dsnSettings = m_frame->GetDesignSettings();
    MODULE* module = m_board->m_Modules;

    // Add a VIEW_GROUP that serves as a preview for the new item
    KIGFX::VIEW_GROUP preview( m_view );
    m_view->Add( &preview );

    m_toolMgr->RunAction( COMMON_ACTIONS::selectionClear );
    m_controls->ShowCursor( true );
    m_controls->SetSnapping( true );
    m_controls->SetAutoPan( true );

    Activate();
    m_frame->SetToolID( ID_PCB_ADD_TEXT_BUTT, wxCURSOR_PENCIL, _( "Add text" ) );
    bool placing = false;

    // Main loop: keep receiving events
    while( OPT_TOOL_EVENT evt = Wait() )
    {
        VECTOR2I cursorPos = m_controls->GetCursorPosition();

        if( evt->IsCancel() || evt->IsActivate() )
        {
            preview.Clear();
            preview.ViewUpdate();
            m_controls->ShowCursor( true );

            if( !placing || evt->IsActivate() )
            {
                delete text;
                break;
            }
            else
            {
                placing = false;  // start from the beginning
            }
        }

        else if( text && evt->Category() == TC_COMMAND )
        {
            if( evt->IsAction( &COMMON_ACTIONS::rotate ) )
            {
                text->Rotate( text->GetPosition(), m_frame->GetRotationAngle() );
                preview.ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY );
            }
            else if( evt->IsAction( &COMMON_ACTIONS::flip ) )
            {
                text->Flip( text->GetPosition() );
                preview.ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY );
            }
        }

        else if( evt->IsClick( BUT_LEFT ) )
        {
            if( !placing )
            {
                text->SetSize( dsnSettings.m_ModuleTextSize );
                text->SetThickness( dsnSettings.m_ModuleTextWidth );
                text->SetTextPosition( wxPoint( cursorPos.x, cursorPos.y ) );

                DialogEditModuleText textDialog( m_frame, text, NULL );
                placing = textDialog.ShowModal() && ( text->GetText().Length() > 0 );

                if( !placing )
                    continue;

                m_controls->ShowCursor( false );
                text->SetParent( module );   // it has to set after the settings dialog
                                             // otherwise the dialog stores it in undo buffer
                preview.Add( text );
            }
            else
            {
                assert( text->GetText().Length() > 0 );
                assert( text->GetSize().x > 0 && text->GetSize().y > 0 );

                text->SetLocalCoord();
                text->ClearFlags();

                // Module has to be saved before any modification is made
                m_frame->SaveCopyInUndoList( module, UR_MODEDIT );
                module->GraphicalItems().PushFront( text );

                m_view->Add( text );
                text->ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY );

                m_frame->OnModify();

                preview.Remove( text );
                m_controls->ShowCursor( true );

                text = new TEXTE_MODULE( NULL );
                placing = false;
            }
        }

        else if( text && evt->IsMotion() )
        {
            text->SetTextPosition( wxPoint( cursorPos.x, cursorPos.y ) );

            // Show a preview of the item
            preview.ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY );
        }
    }

    m_controls->ShowCursor( false );
    m_controls->SetSnapping( false );
    m_controls->SetAutoPan( false );
    m_view->Remove( &preview );

    setTransitions();
    m_frame->SetToolID( ID_NO_TOOL_SELECTED, wxCURSOR_DEFAULT, wxEmptyString );

    return 0;
}


int DRAWING_TOOL::placeTextPcb()
{
    TEXTE_PCB* text = NULL;

    // Add a VIEW_GROUP that serves as a preview for the new item
    KIGFX::VIEW_GROUP preview( m_view );
    m_view->Add( &preview );

    m_toolMgr->RunAction( COMMON_ACTIONS::selectionClear );
    m_controls->ShowCursor( true );
    m_controls->SetSnapping( true );
    m_controls->SetAutoPan( true );

    Activate();
    m_frame->SetToolID( ID_PCB_ADD_TEXT_BUTT, wxCURSOR_PENCIL, _( "Add text" ) );

    // Main loop: keep receiving events
    while( OPT_TOOL_EVENT evt = Wait() )
    {
        VECTOR2I cursorPos = m_controls->GetCursorPosition();

        if( evt->IsCancel() || evt->IsActivate() )
        {
            if( text )
            {
                // Delete the old text and have another try
                m_board->Delete( text );        // it was already added by CreateTextPcb()
                text = NULL;

                preview.Clear();
                preview.ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY );
                m_controls->ShowCursor( true );
            }
            else
                break;

            if( evt->IsActivate() )  // now finish unconditionally
                break;
        }

        else if( text && evt->Category() == TC_COMMAND )
        {
            if( evt->IsAction( &COMMON_ACTIONS::rotate ) )
            {
                text->Rotate( text->GetPosition(), m_frame->GetRotationAngle() );
                preview.ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY );
            }
            else if( evt->IsAction( &COMMON_ACTIONS::flip ) )
            {
                text->Flip( text->GetPosition() );
                preview.ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY );
            }
        }

        else if( evt->IsClick( BUT_LEFT ) )
        {
            if( !text )
            {
                // Init the new item attributes
                text = static_cast<PCB_EDIT_FRAME*>( m_frame )->CreateTextePcb( NULL );

                if( text == NULL )
                    continue;

                m_controls->ShowCursor( false );
                preview.Add( text );
            }
            else
            {
                assert( text->GetText().Length() > 0 );
                assert( text->GetSize().x > 0 && text->GetSize().y > 0 );

                text->ClearFlags();
                m_view->Add( text );
                // m_board->Add( text );        // it is already added by CreateTextePcb()
                text->ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY );

                m_frame->OnModify();
                m_frame->SaveCopyInUndoList( text, UR_NEW );

                preview.Remove( text );
                m_controls->ShowCursor( true );

                text = NULL;
            }
        }

        else if( text && evt->IsMotion() )
        {
            text->SetTextPosition( wxPoint( cursorPos.x, cursorPos.y ) );

            // Show a preview of the item
            preview.ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY );
        }
    }

    m_controls->ShowCursor( false );
    m_controls->SetSnapping( false );
    m_controls->SetAutoPan( false );
    m_view->Remove( &preview );

    setTransitions();
    m_frame->SetToolID( ID_NO_TOOL_SELECTED, wxCURSOR_DEFAULT, wxEmptyString );

    return 0;
}


void DRAWING_TOOL::make45DegLine( DRAWSEGMENT* aSegment, DRAWSEGMENT* aHelper ) const
{
    VECTOR2I cursorPos = m_controls->GetCursorPosition();
    VECTOR2I origin( aSegment->GetStart() );
    DIRECTION_45 direction( origin - cursorPos );
    SHAPE_LINE_CHAIN newChain = direction.BuildInitialTrace( origin, cursorPos );

    if( newChain.PointCount() > 2 )
    {
        aSegment->SetEnd( wxPoint( newChain.Point( -2 ).x, newChain.Point( -2 ).y ) );
        aHelper->SetStart( wxPoint( newChain.Point( -2 ).x, newChain.Point( -2 ).y ) );
        aHelper->SetEnd( wxPoint( newChain.Point( -1 ).x, newChain.Point( -1 ).y ) );
    }
    else
    {
        aSegment->SetEnd( wxPoint( cursorPos.x, cursorPos.y ) );
        aHelper->SetStart( wxPoint( cursorPos.x, cursorPos.y ) );
        aHelper->SetEnd( wxPoint( cursorPos.x, cursorPos.y ) );
    }
}


bool isNotDigit( char aChar )
{
    return ( aChar < '0' || aChar > '9' );
}


wxString DRAWING_TOOL::getNextPadName() const
{
    std::set<int> usedNumbers;

    // Find the first, not used pad number
    for( MODULE* module = m_board->m_Modules; module; module = module->Next() )
    {
        for( D_PAD* pad = module->Pads(); pad; pad = pad->Next() )
        {
            wxString padName = pad->GetPadName();
            int padNumber = 0;
            int base = 1;

            // Trim and extract the trailing numeric part
            while( padName.Len() && padName.Last() >= '0' && padName.Last() <= '9' )
            {
                padNumber += ( padName.Last() - '0' ) * base;
                padName.RemoveLast();
                base *= 10;
            }

            usedNumbers.insert( padNumber );
        }
    }

    int candidate = *usedNumbers.begin();

    // Look for a gap in pad numbering
    for( std::set<int>::iterator it = usedNumbers.begin(),
            itEnd = usedNumbers.end(); it != itEnd; ++it )
    {
        if( *it - candidate > 1 )
            break;

        candidate = *it;
    }

    return wxString::Format( wxT( "%i" ), ++candidate );
}


void DRAWING_TOOL::setTransitions()
{
    Go( &DRAWING_TOOL::DrawLine,         COMMON_ACTIONS::drawLine.MakeEvent() );
    Go( &DRAWING_TOOL::DrawCircle,       COMMON_ACTIONS::drawCircle.MakeEvent() );
    Go( &DRAWING_TOOL::DrawArc,          COMMON_ACTIONS::drawArc.MakeEvent() );
    Go( &DRAWING_TOOL::DrawDimension,    COMMON_ACTIONS::drawDimension.MakeEvent() );
    Go( &DRAWING_TOOL::DrawZone,         COMMON_ACTIONS::drawZone.MakeEvent() );
    Go( &DRAWING_TOOL::DrawKeepout,      COMMON_ACTIONS::drawKeepout.MakeEvent() );
    Go( &DRAWING_TOOL::PlaceText,        COMMON_ACTIONS::placeText.MakeEvent() );
    Go( &DRAWING_TOOL::PlaceTarget,      COMMON_ACTIONS::placeTarget.MakeEvent() );
    Go( &DRAWING_TOOL::PlaceModule,      COMMON_ACTIONS::placeModule.MakeEvent() );
    Go( &DRAWING_TOOL::PlacePad,         COMMON_ACTIONS::placePad.MakeEvent() );
    Go( &DRAWING_TOOL::PlaceDXF,         COMMON_ACTIONS::placeDXF.MakeEvent() );
    Go( &DRAWING_TOOL::SetAnchor,        COMMON_ACTIONS::setAnchor.MakeEvent() );
}
