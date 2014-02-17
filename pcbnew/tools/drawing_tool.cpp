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
#include <router/direction.h>

#include <class_board.h>
#include <class_drawsegment.h>
#include <class_pcb_text.h>
#include <class_dimension.h>
#include <class_mire.h>
#include <class_zone.h>
#include <class_module.h>

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
    bool clockwise = true;  // drawing direction of the arc
    double startAngle;      // angle of the first arc line
    VECTOR2I cursorPos = m_controls->GetCursorPosition();

    // Init the new item attributes
    DRAWSEGMENT* arc = new DRAWSEGMENT( m_board );
    arc->SetShape( S_ARC );
    arc->SetAngle( 0.0 );
    arc->SetWidth( m_board->GetDesignSettings().m_DrawSegmentWidth );
    arc->SetCenter( wxPoint( cursorPos.x, cursorPos.y ) );

    DRAWSEGMENT helperLine;
    helperLine.SetShape( S_SEGMENT );
    helperLine.SetLayer( DRAW_N );
    helperLine.SetWidth( 1 );

    // Add a VIEW_GROUP that serves as a preview for the new item
    KIGFX::VIEW_GROUP preview( m_view );
    m_view->Add( &preview );

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

        if( evt->IsCancel() )
        {
            delete arc;
            break;
        }

        else if( evt->IsKeyUp() )
        {
            int width = arc->GetWidth();

            // Modify the new item width
            if( evt->KeyCode() == '-' && width > WIDTH_STEP )
                arc->SetWidth( width - WIDTH_STEP );
            else if( evt->KeyCode() == '=' )
                arc->SetWidth( width + WIDTH_STEP );
            else if( evt->KeyCode() == '/' )
            {
                if( clockwise )
                    arc->SetAngle( arc->GetAngle() - 3600.0 );
                else
                    arc->SetAngle( arc->GetAngle() + 3600.0 );

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
                    helperLine.SetStart( arc->GetCenter() );
                    arc->SetCenter( wxPoint( cursorPos.x, cursorPos.y ) );
                    arc->SetLayer( layer );
                    preview.Add( arc );
                    preview.Add( &helperLine );

                    m_controls->SetAutoPan( true );
                }
            }
            break;

            case SET_END:
            {
                if( wxPoint( cursorPos.x, cursorPos.y ) != arc->GetCenter() )
                {
                    VECTOR2D startLine( arc->GetArcStart() - arc->GetCenter() );
                    startAngle = startLine.Angle();
                    arc->SetArcStart( wxPoint( cursorPos.x, cursorPos.y ) );
                }
                else
                    --step;     // one another chance to draw a proper arc

            }
            break;

            case SET_ANGLE:
            {
                if( wxPoint( cursorPos.x, cursorPos.y ) != arc->GetArcStart() )
                {
                    assert( arc->GetAngle() > 0 );
                    assert( arc->GetArcStart() != arc->GetArcEnd() );
                    assert( arc->GetWidth() > 0 );

                    m_view->Add( arc );
                    m_board->Add( arc );
                    arc->ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY );
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
            case SET_ORIGIN:
                arc->SetCenter( wxPoint( cursorPos.x, cursorPos.y ) );
                break;

            case SET_END:
                helperLine.SetEnd( wxPoint( cursorPos.x, cursorPos.y ) );
                arc->SetArcStart( wxPoint( cursorPos.x, cursorPos.y ) );
                break;

            case SET_ANGLE:
            {
                // Compute the current angle
                VECTOR2D endLine( wxPoint( cursorPos.x, cursorPos.y ) - arc->GetCenter() );
                double newAngle = RAD2DECIDEG( endLine.Angle() - startAngle );

                if( clockwise && newAngle < 0.0 )
                    newAngle += 3600.0;
                else if( !clockwise && newAngle > 0.0 )
                    newAngle -= 3600.0;

                arc->SetAngle( newAngle );
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
    TEXTE_PCB* text = m_frame->CreateTextePcb( NULL );
    if( text == NULL )
    {
        setTransitions();
        return 0;
    }

    // Add a VIEW_GROUP that serves as a preview for the new item
    KIGFX::VIEW_GROUP preview( m_view );
    preview.Add( text );
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
            m_board->Delete( text );
            break;
        }

        else if( evt->Category() == TC_COMMAND )
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
            assert( text->GetText().Length() > 0 );
            assert( text->GetSize().x > 0 && text->GetSize().y > 0 );

            text->ClearFlags();
            m_view->Add( text );
            // m_board->Add( text );        // it is already added by CreateTextePcb()
            text->ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY );
            break;
        }

        else if( evt->IsMotion() )
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

            case SET_END:
                // Dimensions that have origin and end in the same spot are not valid
                if( dimension->GetOrigin() == wxPoint( cursorPos.x, cursorPos.y ) )
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
            assert( target->GetSize() > 0 );
            assert( target->GetWidth() > 0 );

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

    // Init the new item attributes
    DRAWSEGMENT* graphic = new DRAWSEGMENT( m_board );
    graphic->SetShape( (STROKE_T) aShape );
    graphic->SetWidth( m_board->GetDesignSettings().m_DrawSegmentWidth );

    DRAWSEGMENT line45( *graphic ); // used only for direction 45 mode with lines

    // Add a VIEW_GROUP that serves as a preview for the new item
    KIGFX::VIEW_GROUP preview( m_view );
    m_view->Add( &preview );

    m_controls->ShowCursor( true );
    m_controls->SetSnapping( true );

    Activate();

    bool started = false;
    bool direction45 = false;       // 45 degrees only mode
    int addedSegments = 0;

    // Main loop: keep receiving events
    while( OPT_TOOL_EVENT evt = Wait() )
    {
        bool updatePreview = false;            // should preview be updated
        VECTOR2I cursorPos = m_controls->GetCursorPosition();

        // Enable 45 degrees lines only mode by holding control
        if( direction45 != ( evt->Modifier( MD_CTRL ) && aShape == S_SEGMENT && started ) )
        {
            direction45 = evt->Modifier( MD_CTRL );

            if( direction45 )
            {
                preview.Add( &line45 );
                make45DegLine( graphic, &line45 );
            }
            else
            {
                preview.Remove( &line45 );
                graphic->SetEnd( wxPoint( cursorPos.x, cursorPos.y ) );
            }

            updatePreview = true;
        }

        if( evt->IsCancel() )
        {
            if( direction45 )
                preview.Remove( &line45 );

            preview.FreeItems();
            if( !started )
                delete graphic;

            break;
        }

        else if( evt->IsKeyUp() )
        {
            int width = graphic->GetWidth();

            // Modify the new item width
            if( evt->KeyCode() == '-' && width > WIDTH_STEP )
                graphic->SetWidth( width - WIDTH_STEP );
            else if( evt->KeyCode() == '=' )
                graphic->SetWidth( width + WIDTH_STEP );

            updatePreview = true;
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

                    graphic->SetStart( wxPoint( cursorPos.x, cursorPos.y ) );
                    graphic->SetEnd( wxPoint( cursorPos.x, cursorPos.y ) );
                    graphic->SetLayer( layer );
                    line45.SetLayer( layer );
                    preview.Add( graphic );

                    started = true;
                }
            }
            else
            {
                if( graphic->GetEnd() != graphic->GetStart() )
                {
                    assert( graphic->GetLength() > 0 );
                    assert( graphic->GetWidth() > 0 );

                    m_view->Add( graphic );
                    m_board->Add( graphic );
                    graphic->ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY );
                    preview.Remove( graphic );
                    ++addedSegments;

                    if( aContinous )
                    {
                        graphic = new DRAWSEGMENT( *graphic );
                        // Start the new line in the same spot where the previous one has ended
                        graphic->SetStart( graphic->GetEnd() );

                        if( direction45 )
                            graphic->SetEnd( line45.GetEnd() );

                        preview.Add( graphic );
                    }
                    else
                    {
                        break;
                    }
                }
                else if( addedSegments > 0 )   // User has clicked twice in the same spot
                {
                    delete graphic;  // seems like a clear sign that the drawing is finished
                    break;           // and we should remove the latest DRAWSEGMENT we have created
                }
            }
        }

        else if( evt->IsMotion() && started )
        {
            // 45 degree lines
            if( direction45 && aShape == S_SEGMENT )
                make45DegLine( graphic, &line45 );
            else
                graphic->SetEnd( wxPoint( cursorPos.x, cursorPos.y ) );

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
    DRAWSEGMENT line45( *helperLine );

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
    bool direction45 = false;       // 45 degrees only mode
    bool cancelled = false;

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

        if( evt->IsCancel() )
        {
            cancelled = true;
            break;
        }

        else if( evt->IsClick( BUT_LEFT ) )
        {
            if( lastCursorPos == cursorPos || ( numPoints > 0 && cursorPos == origin ) )
            {
                if( numPoints > 2 )
                {
                    assert( zone->GetNumCorners() > 2 );

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
                    cancelled = true;
                }

                break;
            }
            else
            {
                if( numPoints == 0 )
                {
                    // Add the first point
                    zone->Outline()->Start( zoneInfo.m_CurrentZone_Layer,
                                            cursorPos.x, cursorPos.y,
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

                updatePreview = true;
            }

            lastCursorPos = cursorPos;
        }

        else if( evt->IsMotion() )
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

    // Clean
    if( cancelled )
        delete zone;
    if( direction45 )
        preview.Remove( &line45 );
    preview.FreeItems();

    setTransitions();

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
