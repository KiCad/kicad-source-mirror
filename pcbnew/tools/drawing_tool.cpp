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
    // Init variables used by every drawing tool
    m_view = getView();
    m_controls = getViewControls();
    m_board = getModel<BOARD>( PCB_T );
    m_frame = getEditFrame<PCB_EDIT_FRAME>();

    setTransitions();
}


int DRAWING_TOOL::DrawLine( TOOL_EVENT& aEvent )
{
    return drawSegment( S_SEGMENT, true );
}


int DRAWING_TOOL::DrawCircle( TOOL_EVENT& aEvent )
{
    return drawSegment( S_CIRCLE, false );
}


int DRAWING_TOOL::DrawArc( TOOL_EVENT& aEvent )
{
    bool clockwise = true;
    double startAngle;      // angle of the first arc line
    VECTOR2I cursorPos = m_controls->GetCursorPosition();

    // Init the new item attributes
    DRAWSEGMENT graphic;
    graphic.SetShape( S_ARC );
    graphic.SetAngle( 0.0 );
    graphic.SetWidth( m_board->GetDesignSettings().m_DrawSegmentWidth );
    graphic.SetCenter( wxPoint( cursorPos.x, cursorPos.y ) );

    DRAWSEGMENT helperLine;
    helperLine.SetShape( S_SEGMENT );
    helperLine.SetLayer( DRAW_N );
    helperLine.SetWidth( 1 );

    // Add a VIEW_GROUP that serves as a preview for the new item
    KIGFX::VIEW_GROUP preview( m_view );
    m_view->Add( &preview );

    m_controls->ShowCursor( true );
    m_controls->SetSnapping( true );
    m_controls->SetAutoPan( true );

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
            else if( evt->KeyCode() == '/' )
            {
                if( clockwise )
                    graphic.SetAngle( graphic.GetAngle() - 3600.0 );
                else
                    graphic.SetAngle( graphic.GetAngle() + 3600.0 );

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
                LAYER_NUM layer = m_frame->GetScreen()->m_Active_Layer;

                if( IsCopperLayer( layer ) )
                {
                    DisplayInfoMessage( NULL, _( "Graphic not allowed on Copper layers" ) );
                    --step;
                }
                else
                {
                    helperLine.SetStart( graphic.GetCenter() );
                    graphic.SetLayer( layer );
                    preview.Add( &graphic );
                    preview.Add( &helperLine );
                }
            }
            break;

            case SET_END:
            {
                VECTOR2D startLine( graphic.GetArcStart() - graphic.GetCenter() );
                startAngle = startLine.Angle();
            }
            break;

            case SET_ANGLE:
            {
                if( wxPoint( cursorPos.x, cursorPos.y ) != graphic.GetCenter() )
                {
                    DRAWSEGMENT* newItem = new DRAWSEGMENT( graphic );
                    m_view->Add( newItem );
                    m_board->Add( newItem );
                    newItem->ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY );
                }
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
            case SET_ORIGIN:
                graphic.SetCenter( wxPoint( cursorPos.x, cursorPos.y ) );
                break;

            case SET_END:
                helperLine.SetEnd( wxPoint( cursorPos.x, cursorPos.y ) );
                graphic.SetArcStart( wxPoint( cursorPos.x, cursorPos.y ) );
                break;

            case SET_ANGLE:
            {
                // Compute the current angle
                VECTOR2D endLine( wxPoint( cursorPos.x, cursorPos.y ) - graphic.GetCenter() );
                double newAngle = RAD2DECIDEG( endLine.Angle() - startAngle );

                if( clockwise && newAngle < 0.0 )
                    newAngle += 3600.0;
                else if( !clockwise && newAngle > 0.0 )
                    newAngle -= 3600.0;

                graphic.SetAngle( newAngle );
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

    setTransitions();

    return 0;
}


int DRAWING_TOOL::DrawText( TOOL_EVENT& aEvent )
{
    // Init the new item attributes
    TEXTE_PCB* newText = m_frame->CreateTextePcb( NULL );
    if( newText == NULL )
    {
        setTransitions();
        return 0;
    }

    // Add a VIEW_GROUP that serves as a preview for the new item
    KIGFX::VIEW_GROUP preview( m_view );
    preview.Add( newText );
    m_view->Add( &preview );

    m_controls->ShowCursor( true );
    m_controls->SetSnapping( true );
    m_controls->SetAutoPan( true );

    Activate();

    // Main loop: keep receiving events
    while( OPT_TOOL_EVENT evt = Wait() )
    {
        VECTOR2I cursorPos = m_controls->GetCursorPosition();

        if( evt->IsCancel() )
        {
            // it was already added by CreateTextPcb()
            m_board->Delete( newText );
            break;
        }

        else if( evt->Category() == TC_COMMAND )
        {
            if( evt->IsAction( &COMMON_ACTIONS::rotate ) )
            {
                newText->Rotate( newText->GetPosition(), m_frame->GetRotationAngle() );
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
            m_view->Add( newText );
            // m_board->Add( newText );        // it is already added by CreateTextePcb()
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

    m_controls->ShowCursor( false );
    m_controls->SetSnapping( false );
    m_controls->SetAutoPan( false );
    m_view->Remove( &preview );

    setTransitions();

    return 0;
}


int DRAWING_TOOL::DrawDimension( TOOL_EVENT& aEvent )
{
    DIMENSION* dimension = new DIMENSION( m_board );

    // Init the new item attributes
    dimension->Text().SetSize( m_board->GetDesignSettings().m_PcbTextSize );
    int width = m_board->GetDesignSettings().m_PcbTextWidth;
    int maxThickness = Clamp_Text_PenSize( width, dimension->Text().GetSize() );

    if( width > maxThickness )
        width = maxThickness;

    dimension->Text().SetThickness( width );
    dimension->SetWidth( width );
    dimension->AdjustDimensionDetails();

    // Add a VIEW_GROUP that serves as a preview for the new item
    KIGFX::VIEW_GROUP preview( m_view );
    m_view->Add( &preview );

    m_controls->ShowCursor( true );
    m_controls->SetSnapping( true );

    Activate();

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

        if( evt->IsCancel() )
        {
            delete dimension;
            break;
        }

        else if( evt->IsKeyUp() )
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
                LAYER_NUM layer = m_frame->GetScreen()->m_Active_Layer;

                if( IsCopperLayer( layer ) )
                {
                    DisplayInfoMessage( NULL, _( "Graphic not allowed on Copper layers" ) );
                    --step;
                }
                else
                {
                    m_controls->SetAutoPan( true );

                    dimension->SetLayer( layer );
                    dimension->SetOrigin( wxPoint( cursorPos.x, cursorPos.y ) );

                    preview.Add( dimension );
                }
            }
            break;

            case SET_HEIGHT:
            {
                if( wxPoint( cursorPos.x, cursorPos.y ) != dimension->GetPosition() )
                {
                    m_view->Add( dimension );
                    m_board->Add( dimension );
                    dimension->ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY );
                }
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

    m_controls->ShowCursor( false );
    m_controls->SetSnapping( false );
    m_controls->SetAutoPan( false );
    m_view->Remove( &preview );

    setTransitions();

    return 0;
}


int DRAWING_TOOL::DrawZone( TOOL_EVENT& aEvent )
{
    return drawZone( false );
}


int DRAWING_TOOL::DrawKeepout( TOOL_EVENT& aEvent )
{
    return drawZone( true );
}


int DRAWING_TOOL::PlaceTarget( TOOL_EVENT& aEvent )
{
    PCB_TARGET* target = new PCB_TARGET( m_board );

    // Init the new item attributes
    target->SetLayer( EDGE_N );
    target->SetWidth( m_board->GetDesignSettings().m_EdgeSegmentWidth );
    target->SetSize( Millimeter2iu( 5 ) );
    VECTOR2I cursorPos = m_controls->GetCursorPosition();
    target->SetPosition( wxPoint( cursorPos.x, cursorPos.y ) );

    // Add a VIEW_GROUP that serves as a preview for the new item
    KIGFX::VIEW_GROUP preview( m_view );
    preview.Add( target );
    m_view->Add( &preview );
    preview.ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY );

    m_controls->SetSnapping( true );

    Activate();

    // Main loop: keep receiving events
    while( OPT_TOOL_EVENT evt = Wait() )
    {
        cursorPos = m_controls->GetCursorPosition();

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
            m_view->Add( target );
            m_board->Add( target );
            target->ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY );
            break;
        }

        else if( evt->IsMotion() )
        {
            target->SetPosition( wxPoint( cursorPos.x, cursorPos.y ) );
            preview.ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY );
        }
    }

    m_controls->SetSnapping( false );
    m_controls->SetAutoPan( false );
    m_view->Remove( &preview );

    setTransitions();

    return 0;
}


int DRAWING_TOOL::PlaceModule( TOOL_EVENT& aEvent )
{
    MODULE* module = m_frame->LoadModuleFromLibrary( wxEmptyString,
                                               m_frame->GetFootprintLibraryTable(), true, NULL );
    if( module == NULL )
    {
        setTransitions();
        return 0;
    }

    // Init the new item attributes
    VECTOR2I cursorPos = m_controls->GetCursorPosition();
    module->SetPosition( wxPoint( cursorPos.x, cursorPos.y ) );

    // Add a VIEW_GROUP that serves as a preview for the new item
    KIGFX::VIEW_GROUP preview( m_view );
    preview.Add( module );
    module->RunOnChildren( std::bind1st( std::mem_fun( &KIGFX::VIEW_GROUP::Add ), &preview ) );
    m_view->Add( &preview );
    preview.ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY );

    m_controls->SetSnapping( true );

    Activate();

    // Main loop: keep receiving events
    while( OPT_TOOL_EVENT evt = Wait() )
    {
        cursorPos = m_controls->GetCursorPosition();

        if( evt->IsCancel() )
        {
            m_board->Delete( module );
            break;
        }

        else if( evt->Category() == TC_COMMAND )
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
            module->RunOnChildren( std::bind1st( std::mem_fun( &KIGFX::VIEW::Add ), m_view ) );
            m_view->Add( module );
            module->ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY );
            break;
        }

        else if( evt->IsMotion() )
        {
            module->SetPosition( wxPoint( cursorPos.x, cursorPos.y ) );
            preview.ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY );
        }
    }

    m_controls->SetSnapping( false );
    m_controls->SetAutoPan( false );
    m_view->Remove( &preview );

    setTransitions();

    return 0;
}


int DRAWING_TOOL::drawSegment( int aShape, bool aContinous )
{
    // Only two shapes are currently supported
    assert( aShape == S_SEGMENT || aShape == S_CIRCLE );

    bool started = false;
    DRAWSEGMENT graphic;

    // Init the new item attributes
    graphic.SetShape( (STROKE_T) aShape );
    graphic.SetWidth( m_board->GetDesignSettings().m_DrawSegmentWidth );

    // Add a VIEW_GROUP that serves as a preview for the new item
    KIGFX::VIEW_GROUP preview( m_view );
    m_view->Add( &preview );

    m_controls->ShowCursor( true );
    m_controls->SetSnapping( true );

    Activate();

    // Main loop: keep receiving events
    while( OPT_TOOL_EVENT evt = Wait() )
    {
        // Enable 45 degrees lines only mode by holding shift
        bool linesAngle45 = evt->Modifier( MD_SHIFT );
        VECTOR2I cursorPos = m_controls->GetCursorPosition();

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
                LAYER_NUM layer = m_frame->GetScreen()->m_Active_Layer;

                if( IsCopperLayer( layer ) )
                {
                    DisplayInfoMessage( NULL, _( "Graphic not allowed on Copper layers" ) );
                }
                else
                {
                    m_controls->SetAutoPan( true );

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
                    m_view->Add( newItem );
                    m_board->Add( newItem );
                    newItem->ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY );

                    if( aContinous )
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
                make45DegLine( &graphic );
            else
                graphic.SetEnd( wxPoint( cursorPos.x, cursorPos.y ) );

            // Show a preview of the item
            preview.ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY );
        }
    }

    m_controls->ShowCursor( false );
    m_controls->SetSnapping( false );
    m_controls->SetAutoPan( false );
    m_view->Remove( &preview );
    setTransitions();

    return 0;
}


int DRAWING_TOOL::drawZone( bool aKeepout )
{
    ZONE_CONTAINER* zone = new ZONE_CONTAINER( m_board );

    // Get the current, default settings for zones
    ZONE_SETTINGS zoneInfo = m_frame->GetZoneSettings();
    zoneInfo.m_CurrentZone_Layer = m_frame->GetScreen()->m_Active_Layer;

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
        delete zone;
        setTransitions();

        return 0;
    }

    // Apply the selected settings
    zoneInfo.ExportSetting( *zone );
    m_frame->SetTopLayer( zoneInfo.m_CurrentZone_Layer );

    // Helper line represents the currently drawn line of the zone polygon
    DRAWSEGMENT* helperLine = new DRAWSEGMENT;
    helperLine->SetShape( S_SEGMENT );
    helperLine->SetLayer( zoneInfo.m_CurrentZone_Layer );
    helperLine->SetWidth( 1 );

    // Add a VIEW_GROUP that serves as a preview for the new item
    KIGFX::VIEW_GROUP preview( m_view );
    m_view->Add( &preview );

    m_controls->ShowCursor( true );
    m_controls->SetSnapping( true );
    m_controls->SetAutoPan( true );

    Activate();

    VECTOR2I lastCursorPos = m_controls->GetCursorPosition();
    VECTOR2I origin;
    int numPoints = 0;

    // Main loop: keep receiving events
    while( OPT_TOOL_EVENT evt = Wait() )
    {
        // Enable 45 degrees lines only mode by holding shift
        bool linesAngle45 = evt->Modifier( MD_SHIFT );
        VECTOR2I cursorPos = m_controls->GetCursorPosition();

        if( evt->IsCancel() )
        {
            delete zone;
            break;
        }

        else if( evt->IsClick( BUT_LEFT ) )
        {
            if( lastCursorPos == cursorPos || ( numPoints > 0 && cursorPos == origin ) )
            {
                if( numPoints > 2 )
                {
                    // Finish the zone
                    zone->Outline()->CloseLastContour();
                    zone->Outline()->RemoveNullSegments();

                    m_board->Add( zone );
                    m_view->Add( zone );

                    if( !aKeepout )
                        m_frame->Fill_Zone( zone );

                    zone->ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY );
                }
                else
                {
                    // If there are less than 3 points, then it is not a valid zone
                    delete zone;
                }

                break;
            }
            else
            {
                if( numPoints == 0 )
                {
                    // Add the first point
                    zone->Outline()->Start( zoneInfo.m_CurrentZone_Layer,
                                            cursorPos.x,
                                            cursorPos.y,
                                            zone->GetHatchStyle() );
                    helperLine->SetStart( wxPoint( cursorPos.x, cursorPos.y ) );
                    origin = cursorPos;
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

                preview.ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY );
            }

            lastCursorPos = cursorPos;
        }

        else if( evt->IsMotion() )
        {
            // 45 degree lines
            if( linesAngle45 )
                make45DegLine( helperLine );
            else
                helperLine->SetEnd( wxPoint( cursorPos.x, cursorPos.y ) );

            // Show a preview of the item
            preview.ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY );
        }
    }

    m_controls->ShowCursor( false );
    m_controls->SetSnapping( false );
    m_controls->SetAutoPan( false );
    m_view->Remove( &preview );

    // Delete helper lines
    preview.FreeItems();

    setTransitions();

    return 0;
}


void DRAWING_TOOL::make45DegLine( DRAWSEGMENT* aSegment ) const
{
    VECTOR2I cursorPos = m_controls->GetCursorPosition();

    // Current line vector
    VECTOR2D lineVector( wxPoint( cursorPos.x, cursorPos.y ) - aSegment->GetStart() );
    double angle = lineVector.Angle();

    // Find the closest angle, which is a multiple of 45 degrees
    double newAngle = round( angle / ( M_PI / 4.0 ) ) * M_PI / 4.0;

    VECTOR2D newLineVector = lineVector.Rotate( newAngle - angle );
    VECTOR2D newLineEnd = VECTOR2D( aSegment->GetStart() ) + newLineVector;

    // Snap the new line to the grid
    newLineEnd = m_view->GetGAL()->GetGridPoint( newLineEnd );

    aSegment->SetEnd( wxPoint( newLineEnd.x, newLineEnd.y ) );
}


void DRAWING_TOOL::setTransitions()
{
    Go( &DRAWING_TOOL::DrawLine, COMMON_ACTIONS::drawLine.MakeEvent() );
    Go( &DRAWING_TOOL::DrawCircle, COMMON_ACTIONS::drawCircle.MakeEvent() );
    Go( &DRAWING_TOOL::DrawArc, COMMON_ACTIONS::drawArc.MakeEvent() );
    Go( &DRAWING_TOOL::DrawText, COMMON_ACTIONS::drawText.MakeEvent() );
    Go( &DRAWING_TOOL::DrawDimension, COMMON_ACTIONS::drawDimension.MakeEvent() );
    Go( &DRAWING_TOOL::DrawZone, COMMON_ACTIONS::drawZone.MakeEvent() );
    Go( &DRAWING_TOOL::DrawKeepout, COMMON_ACTIONS::drawKeepout.MakeEvent() );
    Go( &DRAWING_TOOL::PlaceTarget, COMMON_ACTIONS::placeTarget.MakeEvent() );
    Go( &DRAWING_TOOL::PlaceModule, COMMON_ACTIONS::placeModule.MakeEvent() );
}
