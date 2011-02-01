/****************/
/* controle.cpp */
/****************/

#include "fctsys.h"
#include "common.h"
#include "class_drawpanel.h"
#include "gerbview.h"

GERBER_DRAW_ITEM* WinEDA_GerberFrame::GerberGeneralLocateAndDisplay()
{
    return Locate( CURSEUR_OFF_GRILLE );
}


void WinEDA_GerberFrame::GeneralControle( wxDC* aDC, wxPoint aPosition )
{
    wxRealPoint gridSize;
    wxPoint     oldpos;
    int         hotkey = 0;
    wxPoint     pos = aPosition;

    PutOnGrid( &pos );

    if( GetScreen()->IsRefreshReq() )
    {
        DrawPanel->Refresh( );
        wxSafeYield();

        // We must return here, instead of proceeding.
        // If we let the cursor move during a refresh request,
        // the cursor be displayed in the wrong place
        // during delayed repaint events that occur when
        // you move the mouse when a message dialog is on
        // the screen, and then you dismiss the dialog by
        // typing the Enter key.
        return;
    }

    oldpos = GetScreen()->m_Curseur;
    gridSize = GetScreen()->GetGridSize();

    switch( g_KeyPressed )
    {
    case WXK_NUMPAD8:
    case WXK_UP:
        pos.y -= wxRound( gridSize.y );
        DrawPanel->MoveCursor( pos );
        break;

    case WXK_NUMPAD2:
    case WXK_DOWN:
        pos.y += wxRound( gridSize.y );
        DrawPanel->MoveCursor( pos );
        break;

    case WXK_NUMPAD4:
    case WXK_LEFT:
        pos.x -= wxRound( gridSize.x );
        DrawPanel->MoveCursor( pos );
        break;

    case WXK_NUMPAD6:
    case WXK_RIGHT:
        pos.x += wxRound( gridSize.x );
        DrawPanel->MoveCursor( pos );
        break;

    default:
        hotkey = g_KeyPressed;
        break;
    }

    GetScreen()->m_Curseur = pos;

    if( oldpos != GetScreen()->m_Curseur )
    {
        pos = GetScreen()->m_Curseur;
        GetScreen()->m_Curseur = oldpos;
        DrawPanel->CursorOff( aDC );
        GetScreen()->m_Curseur = pos;
        DrawPanel->CursorOn( aDC );

        if( DrawPanel->ManageCurseur )
        {
            DrawPanel->ManageCurseur( DrawPanel, aDC, TRUE );
        }
    }

    if( hotkey )
    {
        OnHotKey( aDC, hotkey, NULL );
    }

    if( GetScreen()->IsRefreshReq() )
    {
        DrawPanel->Refresh( );
        wxSafeYield();
    }

    UpdateStatusBar();
}
