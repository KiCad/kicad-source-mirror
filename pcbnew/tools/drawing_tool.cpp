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

#include "drawing_tool.h"
#include "common_actions.h"

#include <wxPcbStruct.h>
#include <confirm.h>

#include <view/view_group.h>
#include <view/view_controls.h>
#include <gal/graphics_abstraction_layer.h>
#include <tool/tool_manager.h>

#include <class_board.h>
#include <class_drawsegment.h>
#include <class_pcb_text.h>
#include <class_dimension.h>
#include <class_mire.h>
#include <class_zone.h>
#include <class_module.h>
#include <class_netinfo.h>          // TODO to be removed if we do not need the flags

DRAWING_TOOL::DRAWING_TOOL() :
    TOOL_INTERACTIVE( "pcbnew.InteractiveDrawing" )
{
}


DRAWING_TOOL::~DRAWING_TOOL()
{
}


void DRAWING_TOOL::Reset( RESET_REASON aReason )
{
    setTransitions();
}


int DRAWING_TOOL::DrawLine( TOOL_EVENT& aEvent )
{
    m_continous = true;

    return draw( S_SEGMENT );
}


int DRAWING_TOOL::DrawCircle( TOOL_EVENT& aEvent )
{
    m_continous = false;

    return draw( S_CIRCLE );
}


int DRAWING_TOOL::DrawArc( TOOL_EVENT& aEvent )
{
    m_continous = false;

    int step = 0;

    KIGFX::VIEW* view = getView();
    KIGFX::VIEW_CONTROLS* controls = getViewControls();
    BOARD* board = getModel<BOARD>( PCB_T );
    DRAWSEGMENT graphic;
    DRAWSEGMENT helperLine;
    bool positive = true;

    // Init the new item attributes
    graphic.SetShape( S_ARC );
    graphic.SetAngle( 0.0 );
    graphic.SetWidth( board->GetDesignSettings().m_DrawSegmentWidth );

    helperLine.SetShape( S_SEGMENT );
    helperLine.SetLayer( DRAW_N );
    helperLine.SetWidth( 1 );

    // Add a VIEW_GROUP that serves as a preview for the new item
    KIGFX::VIEW_GROUP preview( view );
    view->Add( &preview );

    controls->ShowCursor( true );
    controls->SetSnapping( true );

    Activate();

    // Main loop: keep receiving events
    while( OPT_TOOL_EVENT evt = Wait() )
    {
        VECTOR2D cursorPos = view->ToWorld( controls->GetCursorPosition() );

        if( evt->IsCancel() )
            break;

        else if( evt->IsKeyUp() )
        {
            int width = graphic.GetWidth();

            // Modify the new item width
            if( evt->KeyCode() == '-' && width > WIDTH_STEP )
                graphic.SetWidth( width - WIDTH_STEP );
            else if( evt->KeyCode() == '=' )
                graphic.SetWidth( width + WIDTH_STEP );
            else if( evt->KeyCode() == ' ' )
            {
                if( positive )
                    graphic.SetAngle( graphic.GetAngle() - 3600.0 );
                else
                    graphic.SetAngle( graphic.GetAngle() + 3600.0 );

                positive = !positive;
            }

            preview.ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY );
        }

        else if( evt->IsClick( BUT_LEFT ) )
        {
            switch( step )
            {
            case 0:
            {
                LAYER_NUM layer = getEditFrame<PCB_EDIT_FRAME>()->GetScreen()->m_Active_Layer;

                if( IsCopperLayer( layer ) )
                {
                    DisplayInfoMessage( NULL, _( "Graphic not allowed on Copper layers" ) );
                    --step;
                }
                else
                {
                    controls->SetAutoPan( true );

                    helperLine.SetStart( graphic.GetCenter() );
                    graphic.SetLayer( layer );
                    preview.Add( &graphic );
                    preview.Add( &helperLine );
                }
            }
            break;

            case 2:
            {
                if( wxPoint( cursorPos.x, cursorPos.y ) != graphic.GetCenter() )
                {
                    DRAWSEGMENT* newItem = new DRAWSEGMENT( graphic );
                    view->Add( newItem );
                    board->Add( newItem );
                    newItem->ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY );
                }
            }
            break;

            }

            if( ++step == 3 )
                break;
        }

        else if( evt->IsMotion() )
        {
            switch( step )
            {
            case 0:
                graphic.SetCenter( wxPoint( cursorPos.x, cursorPos.y ) );
                break;

            case 1:
                helperLine.SetEnd( wxPoint( cursorPos.x, cursorPos.y ) );
                graphic.SetArcStart( wxPoint( cursorPos.x, cursorPos.y ) );
                break;

            case 2:
            {
                VECTOR2D firstLine( graphic.GetArcStart() - graphic.GetCenter() );
                double firstAngle = firstLine.Angle();

                VECTOR2D secondLine( wxPoint( cursorPos.x, cursorPos.y ) - graphic.GetCenter() );
                double secondAngle = secondLine.Angle();

                double angle = RAD2DECIDEG( secondAngle - firstAngle );

                if( positive && angle < 0.0 )
                    angle += 3600.0;
                else if( !positive && angle > 0.0 )
                    angle -= 3600.0;

                graphic.SetAngle( angle );
            }
            break;
            }

            // Show a preview of the item
            preview.ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY );
        }
    }

    controls->ShowCursor( false );
    controls->SetSnapping( false );
    controls->SetAutoPan( false );
    view->Remove( &preview );

    setTransitions();

    return 0;
}


int DRAWING_TOOL::draw( STROKE_T aShape )
{
    bool started = false;
    KIGFX::VIEW* view = getView();
    KIGFX::VIEW_CONTROLS* controls = getViewControls();
    BOARD* board = getModel<BOARD>( PCB_T );
    DRAWSEGMENT graphic;

    // Init the new item attributes
    graphic.SetShape( aShape );
    graphic.SetWidth( board->GetDesignSettings().m_DrawSegmentWidth );

    // Add a VIEW_GROUP that serves as a preview for the new item
    KIGFX::VIEW_GROUP preview( view );
    view->Add( &preview );

    controls->ShowCursor( true );
    controls->SetSnapping( true );

    Activate();

    // Main loop: keep receiving events
    while( OPT_TOOL_EVENT evt = Wait() )
    {
        // Enable 45 degrees lines only mode by holding shift
        bool linesAngle45 = evt->Modifier( MD_SHIFT );
        VECTOR2D cursorPos = view->ToWorld( controls->GetCursorPosition() );

        if( evt->IsCancel() )
            break;

        else if( evt->IsKeyUp() )
        {
            int width = graphic.GetWidth();

            // Modify the new item width
            if( evt->KeyCode() == '-' && width > WIDTH_STEP )
                graphic.SetWidth( width - WIDTH_STEP );
            else if( evt->KeyCode() == '=' )
                graphic.SetWidth( width + WIDTH_STEP );

            preview.ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY );
        }

        else if( evt->IsClick( BUT_LEFT ) )
        {
            if( !started )
            {
                LAYER_NUM layer = getEditFrame<PCB_EDIT_FRAME>()->GetScreen()->m_Active_Layer;

                if( IsCopperLayer( layer ) )
                {
                    DisplayInfoMessage( NULL, _( "Graphic not allowed on Copper layers" ) );
                }
                else
                {
                    controls->SetAutoPan( true );

                    graphic.SetStart( wxPoint( cursorPos.x, cursorPos.y ) );
                    graphic.SetLayer( layer );
                    preview.Add( &graphic );

                    started = true;
                }
            }
            else
            {
                if( wxPoint( cursorPos.x, cursorPos.y ) != graphic.GetStart() )
                {
                    DRAWSEGMENT* newItem = new DRAWSEGMENT( graphic );
                    view->Add( newItem );
                    board->Add( newItem );
                    newItem->ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY );

                    if( m_continous )
                        graphic.SetStart( graphic.GetEnd() ); // This is the origin point for a new item
                    else
                        break;
                }
                else            // User has clicked twice in the same spot
                    break;      // seems like a clear sign that the drawing is finished
            }
        }

        else if( evt->IsMotion() && started )
        {
            // 45 degree lines
            if( linesAngle45 && aShape == S_SEGMENT )
            {
                VECTOR2D lineVector( wxPoint( cursorPos.x, cursorPos.y ) - graphic.GetStart() );
                double angle = lineVector.Angle();

                double newAngle = round( angle / ( M_PI / 4.0 ) ) * M_PI / 4.0;
                VECTOR2D newLineVector = lineVector.Rotate( newAngle - angle );

                // Snap the new line to the grid // TODO fix it, does not work good..
                VECTOR2D newLineEnd = VECTOR2D( graphic.GetStart() ) + newLineVector;
                VECTOR2D snapped = view->GetGAL()->GetGridPoint( newLineEnd );

                graphic.SetEnd( wxPoint( snapped.x, snapped.y ) );
            }
            else
            {
                graphic.SetEnd( wxPoint( cursorPos.x, cursorPos.y ) );
            }

            // Show a preview of the item
            preview.ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY );
        }
    }

    controls->ShowCursor( false );
    controls->SetSnapping( false );
    controls->SetAutoPan( false );
    view->Remove( &preview );

    setTransitions();

    return 0;
}


int DRAWING_TOOL::DrawText( TOOL_EVENT& aEvent )
{
    KIGFX::VIEW* view = getView();
    KIGFX::VIEW_CONTROLS* controls = getViewControls();

    // Init the new item attributes
    TEXTE_PCB* newText = getEditFrame<PCB_EDIT_FRAME>()->CreateTextePcb( NULL );
    if( newText == NULL )
    {
        setTransitions();
        return 0;
    }

    // Add a VIEW_GROUP that serves as a preview for the new item
    KIGFX::VIEW_GROUP preview( view );
    preview.Add( newText );
    view->Add( &preview );

    controls->ShowCursor( true );
    controls->SetSnapping( true );
    controls->SetAutoPan( true );

    Activate();

    // Main loop: keep receiving events
    while( OPT_TOOL_EVENT evt = Wait() )
    {
        VECTOR2D cursorPos = view->ToWorld( controls->GetCursorPosition() );

        if( evt->IsCancel() )
        {
            // it was already added by CreateTextPcb()
            getModel<BOARD>( PCB_T )->Delete( newText );
            break;
        }

        else if( evt->Category() == TC_COMMAND )
        {
            if( evt->IsAction( &COMMON_ACTIONS::rotate ) )
            {
                newText->Rotate( newText->GetPosition(), getEditFrame<PCB_EDIT_FRAME>()->GetRotationAngle() );
                preview.ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY );
            }
            else if( evt->IsAction( &COMMON_ACTIONS::flip ) )
            {
                newText->Flip( newText->GetPosition() );
                preview.ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY );
            }
        }

        else if( evt->IsClick( BUT_LEFT ) )
        {
            newText->ClearFlags();
            view->Add( newText );
            // board->Add( newText );        // it is already added by CreateTextePcb()
            newText->ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY );
            break;
        }

        else if( evt->IsMotion() )
        {
            newText->SetTextPosition( wxPoint( cursorPos.x, cursorPos.y ) );

            // Show a preview of the item
            preview.ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY );
        }
    }

    controls->ShowCursor( false );
    controls->SetSnapping( false );
    controls->SetAutoPan( false );
    view->Remove( &preview );

    setTransitions();

    return 0;
}


int DRAWING_TOOL::DrawDimension( TOOL_EVENT& aEvent )
{
    m_continous = false;

    int step = 0;

    KIGFX::VIEW* view = getView();
    KIGFX::VIEW_CONTROLS* controls = getViewControls();
    BOARD* board = getModel<BOARD>( PCB_T );
    DIMENSION* dimension = new DIMENSION( board );

    // Init the new item attributes
    dimension->Text().SetSize( board->GetDesignSettings().m_PcbTextSize );
    int width = board->GetDesignSettings().m_PcbTextWidth;
    int maxthickness = Clamp_Text_PenSize( width, dimension->Text().GetSize() );

    if( width > maxthickness )
        width = maxthickness;

    dimension->Text().SetThickness( width );
    dimension->SetWidth( width );
    dimension->SetFlags( IS_NEW );
    dimension->AdjustDimensionDetails();

    // Add a VIEW_GROUP that serves as a preview for the new item
    KIGFX::VIEW_GROUP preview( view );
    view->Add( &preview );

    controls->ShowCursor( true );
    controls->SetSnapping( true );

    Activate();

    // Main loop: keep receiving events
    while( OPT_TOOL_EVENT evt = Wait() )
    {
        VECTOR2D cursorPos = view->ToWorld( controls->GetCursorPosition() );

        if( evt->IsCancel() )
        {
            delete dimension;
            break;
        }

        else if( evt->IsKeyUp() )
        {
            int width = dimension->GetWidth();

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
            case 0:
            {
                LAYER_NUM layer = getEditFrame<PCB_EDIT_FRAME>()->GetScreen()->m_Active_Layer;

                if( IsCopperLayer( layer ) )
                {
                    DisplayInfoMessage( NULL, _( "Graphic not allowed on Copper layers" ) );
                    --step;
                }
                else
                {
                    controls->SetAutoPan( true );

                    dimension->SetLayer( layer );
                    dimension->SetOrigin( wxPoint( cursorPos.x, cursorPos.y ) );

                    preview.Add( dimension );
                }
            }
            break;

            case 2:
            {
                if( wxPoint( cursorPos.x, cursorPos.y ) != dimension->GetPosition() )
                {
                    view->Add( dimension );
                    board->Add( dimension );
                    dimension->ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY );
                }
            }
            break;
            }

            if( ++step == 3 )
                break;
        }

        else if( evt->IsMotion() )
        {
            switch( step )
            {
            case 1:
                dimension->SetEnd( wxPoint( cursorPos.x, cursorPos.y ) );
                break;

            case 2:
            {
                /* Calculating the direction of travel perpendicular to the selected axis. */
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

    controls->ShowCursor( false );
    controls->SetSnapping( false );
    controls->SetAutoPan( false );
    view->Remove( &preview );

    setTransitions();

    return 0;
}


int DRAWING_TOOL::DrawZone( TOOL_EVENT& aEvent )
{
    KIGFX::VIEW* view = getView();
    KIGFX::VIEW_CONTROLS* controls = getViewControls();
    BOARD* board = getModel<BOARD>( PCB_T );
    ZONE_CONTAINER* zone = new ZONE_CONTAINER( board );
    PCB_EDIT_FRAME* editFrame = getEditFrame<PCB_EDIT_FRAME>();

    // Get the current, default settings for zones
    ZONE_SETTINGS zoneInfo = editFrame->GetZoneSettings();

    ZONE_EDIT_T dialogResult;
    if( IsCopperLayer( editFrame->GetScreen()->m_Active_Layer ) )
        dialogResult = InvokeCopperZonesEditor( editFrame, &zoneInfo );
    else
        dialogResult = InvokeNonCopperZonesEditor( editFrame, zone, &zoneInfo );

    if( dialogResult == ZONE_ABORT )
    {
        delete zone;
        setTransitions();
        return 0;
    }

    zoneInfo.ExportSetting( *zone );
    editFrame->SetTopLayer( zoneInfo.m_CurrentZone_Layer );

    DRAWSEGMENT* helperLine = new DRAWSEGMENT;
    helperLine->SetShape( S_SEGMENT );
    helperLine->SetLayer( zoneInfo.m_CurrentZone_Layer );
    helperLine->SetWidth( 1 );

    // Add a VIEW_GROUP that serves as a preview for the new item
    KIGFX::VIEW_GROUP preview( view );
//    preview.Add( zone );
    view->Add( &preview );

    controls->ShowCursor( true );
    controls->SetSnapping( true );
    controls->SetAutoPan( true );

    Activate();

    VECTOR2D lastCursorPos = view->ToWorld( controls->GetCursorPosition() );

    // Main loop: keep receiving events
    while( OPT_TOOL_EVENT evt = Wait() )
    {
        // Enable 45 degrees lines only mode by holding shift
        bool linesAngle45 = evt->Modifier( MD_SHIFT );
        VECTOR2D cursorPos = view->ToWorld( controls->GetCursorPosition() );

        if( evt->IsCancel() )
        {
            delete zone;
            break;
        }

        else if( evt->IsClick( BUT_LEFT ) )
        {
            if( lastCursorPos == cursorPos ||
                ( zone->GetNumCorners() > 0 && wxPoint( cursorPos.x, cursorPos.y ) == zone->Outline()->GetPos( 0 ) ) )   // TODO better conditions
            {
                if( zone->GetNumCorners() > 2 )
                {
                    // Finish the zone
                    zone->Outline()->CloseLastContour();
                    zone->Outline()->RemoveNullSegments();

                    board->Add( zone );
                    view->Add( zone );

                    editFrame->Fill_Zone( zone );
                    zone->ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY );
                }
                else
                {
                    // That is not a valid zone
                    delete zone;
                }

                break;
            }
            else
            {
                if( zone->GetNumCorners() == 0 )
                {
                    zone->Outline()->Start( zoneInfo.m_CurrentZone_Layer,
                                            cursorPos.x,
                                            cursorPos.y,
                                            zone->GetHatchStyle() );
                    helperLine->SetStart( wxPoint( cursorPos.x, cursorPos.y ) );
                    preview.Add( helperLine );
                }
                else
                {
                    zone->AppendCorner( helperLine->GetEnd() );
                    helperLine = new DRAWSEGMENT( *helperLine );
                    preview.Add( helperLine );
                    helperLine->SetStart( helperLine->GetEnd() );
                }
                preview.ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY );
            }

            lastCursorPos = cursorPos;
        }

        else if( evt->IsMotion() )
        {
            helperLine->SetEnd( wxPoint( cursorPos.x, cursorPos.y ) );

            // 45 degree lines
            if( linesAngle45 )
            {
                VECTOR2D lineVector( wxPoint( cursorPos.x, cursorPos.y ) - helperLine->GetStart() );
                double angle = lineVector.Angle();

                double newAngle = round( angle / ( M_PI / 4.0 ) ) * M_PI / 4.0;
                VECTOR2D newLineVector = lineVector.Rotate( newAngle - angle );

                // Snap the new line to the grid // TODO fix it, does not work good..
                VECTOR2D newLineEnd = VECTOR2D( helperLine->GetStart() ) + newLineVector;
                VECTOR2D snapped = view->GetGAL()->GetGridPoint( newLineEnd );

                helperLine->SetEnd( wxPoint( snapped.x, snapped.y ) );
            }
            else
            {
                helperLine->SetEnd( wxPoint( cursorPos.x, cursorPos.y ) );
            }

            // Show a preview of the item
            preview.ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY );
        }
    }

    controls->ShowCursor( false );
    controls->SetSnapping( false );
    controls->SetAutoPan( false );
    view->Remove( &preview );

    // remove helper lines only
//    preview.Remove( zone );
    preview.FreeItems();

    setTransitions();

    return 0;
}


int DRAWING_TOOL::PlaceTarget( TOOL_EVENT& aEvent )
{
    KIGFX::VIEW* view = getView();
    KIGFX::VIEW_CONTROLS* controls = getViewControls();
    BOARD* board = getModel<BOARD>( PCB_T );
    PCB_TARGET* target = new PCB_TARGET( board );

    // Init the new item attributes
    target->SetLayer( EDGE_N );
    target->SetWidth( board->GetDesignSettings().m_EdgeSegmentWidth );
    target->SetSize( Millimeter2iu( 5 ) );
    VECTOR2D cursorPos = view->ToWorld( controls->GetCursorPosition() );
    target->SetPosition( wxPoint( cursorPos.x, cursorPos.y ) );

    // Add a VIEW_GROUP that serves as a preview for the new item
    KIGFX::VIEW_GROUP preview( view );
    preview.Add( target );
    view->Add( &preview );
    preview.ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY );

    controls->SetSnapping( true );

    Activate();

    // Main loop: keep receiving events
    while( OPT_TOOL_EVENT evt = Wait() )
    {
        VECTOR2D cursorPos = view->ToWorld( controls->GetCursorPosition() );

        if( evt->IsCancel() )
        {
            delete target;
            break;
        }

        else if( evt->IsKeyUp() )
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
            view->Add( target );
            board->Add( target );
            target->ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY );
            break;
        }

        else if( evt->IsMotion() )
        {
            target->SetPosition( wxPoint( cursorPos.x, cursorPos.y ) );
            preview.ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY );
        }
    }

    controls->SetSnapping( false );
    controls->SetAutoPan( false );
    view->Remove( &preview );

    setTransitions();

    return 0;
}


int DRAWING_TOOL::PlaceModule( TOOL_EVENT& aEvent )
{
    KIGFX::VIEW* view = getView();
    KIGFX::VIEW_CONTROLS* controls = getViewControls();
    BOARD* board = getModel<BOARD>( PCB_T );
    PCB_EDIT_FRAME* editFrame = getEditFrame<PCB_EDIT_FRAME>();
    MODULE* module = editFrame->LoadModuleFromLibrary( wxEmptyString,
                                               editFrame->GetFootprintLibraryTable(), true, NULL );
    if( module == NULL )
    {
        setTransitions();
        return 0;
    }

    // Init the new item attributes
    VECTOR2D cursorPos = view->ToWorld( controls->GetCursorPosition() );
    module->SetPosition( wxPoint( cursorPos.x, cursorPos.y ) );

    // Add a VIEW_GROUP that serves as a preview for the new item
    KIGFX::VIEW_GROUP preview( view );
    preview.Add( module );
    module->RunOnChildren( std::bind1st( std::mem_fun( &KIGFX::VIEW_GROUP::Add ), &preview ) );
    view->Add( &preview );
    preview.ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY );

    controls->SetSnapping( true );

    Activate();

    // Main loop: keep receiving events
    while( OPT_TOOL_EVENT evt = Wait() )
    {
        cursorPos = view->ToWorld( controls->GetCursorPosition() );

        if( evt->IsCancel() )
        {
            board->Delete( module );
            break;
        }

        else if( evt->Category() == TC_COMMAND )
        {
            if( evt->IsAction( &COMMON_ACTIONS::rotate ) )
            {
                module->Rotate( module->GetPosition(), getEditFrame<PCB_EDIT_FRAME>()->GetRotationAngle() );
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
            module->RunOnChildren( std::bind1st( std::mem_fun( &KIGFX::VIEW::Add ), view ) );
            view->Add( module );
            module->ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY );
            break;
        }

        else if( evt->IsMotion() )
        {
            module->SetPosition( wxPoint( cursorPos.x, cursorPos.y ) );
            preview.ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY );
        }
    }

    controls->SetSnapping( false );
    controls->SetAutoPan( false );
    view->Remove( &preview );

    setTransitions();

    return 0;
}


void DRAWING_TOOL::setTransitions()
{
    Go( &DRAWING_TOOL::DrawLine, COMMON_ACTIONS::drawLine.MakeEvent() );
    Go( &DRAWING_TOOL::DrawCircle, COMMON_ACTIONS::drawCircle.MakeEvent() );
    Go( &DRAWING_TOOL::DrawArc, COMMON_ACTIONS::drawArc.MakeEvent() );
    Go( &DRAWING_TOOL::DrawText, COMMON_ACTIONS::drawText.MakeEvent() );
    Go( &DRAWING_TOOL::DrawDimension, COMMON_ACTIONS::drawDimension.MakeEvent() );
    Go( &DRAWING_TOOL::DrawZone, COMMON_ACTIONS::drawZone.MakeEvent() );
    Go( &DRAWING_TOOL::PlaceTarget, COMMON_ACTIONS::placeTarget.MakeEvent() );
    Go( &DRAWING_TOOL::PlaceModule, COMMON_ACTIONS::placeModule.MakeEvent() );
}
