/****************/
/* controle.cpp */
/****************/

#include "fctsys.h"
#include "common.h"
#include "class_drawpanel.h"
#include "gerbview.h"


void GERBVIEW_FRAME::GeneralControl( wxDC* aDC, const wxPoint& aPosition, int aHotKey )
{
    wxRealPoint gridSize;
    wxPoint     oldpos;
    wxPoint     pos = aPosition;

    pos = GetScreen()->GetNearestGridPosition( pos );

    oldpos = GetScreen()->GetCrossHairPosition();
    gridSize = GetScreen()->GetGridSize();

    switch( aHotKey )
    {
    case WXK_NUMPAD8:
    case WXK_UP:
        pos.y -= wxRound( gridSize.y );
        m_canvas->MoveCursor( pos );
        break;

    case WXK_NUMPAD2:
    case WXK_DOWN:
        pos.y += wxRound( gridSize.y );
        m_canvas->MoveCursor( pos );
        break;

    case WXK_NUMPAD4:
    case WXK_LEFT:
        pos.x -= wxRound( gridSize.x );
        m_canvas->MoveCursor( pos );
        break;

    case WXK_NUMPAD6:
    case WXK_RIGHT:
        pos.x += wxRound( gridSize.x );
        m_canvas->MoveCursor( pos );
        break;

    default:
        break;
    }

    GetScreen()->SetCrossHairPosition( pos );

    if( oldpos != GetScreen()->GetCrossHairPosition() )
    {
        pos = GetScreen()->GetCrossHairPosition();
        GetScreen()->SetCrossHairPosition( oldpos );
        m_canvas->CrossHairOff( aDC );
        GetScreen()->SetCrossHairPosition( pos );
        m_canvas->CrossHairOn( aDC );

        if( m_canvas->IsMouseCaptured() )
        {
            m_canvas->m_mouseCaptureCallback( m_canvas, aDC, aPosition, true );
        }
    }

    if( aHotKey )
    {
        OnHotKey( aDC, aHotKey, NULL );
    }

    UpdateStatusBar();
}
