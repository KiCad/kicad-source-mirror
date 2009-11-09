/****************/
/* controle.cpp */
/****************/

#include "fctsys.h"
#include "common.h"
#include "class_drawpanel.h"

#include "pcbnew.h"
#include "gerbview.h"
#include "protos.h"


BOARD_ITEM* WinEDA_GerberFrame::GerberGeneralLocateAndDisplay()
{
    return Locate( CURSEUR_OFF_GRILLE );
}


void WinEDA_GerberFrame::GeneralControle( wxDC* DC, wxPoint Mouse )
{
    wxRealPoint  delta;
    wxPoint curpos, oldpos;
    int     hotkey = 0;

    if( GetScreen()->IsRefreshReq() )
    {
        RedrawActiveWindow( DC, TRUE );

        // We must return here, instead of proceeding.
        // If we let the cursor move during a refresh request,
        // the cursor be displayed in the wrong place
        // during delayed repaint events that occur when
        // you move the mouse when a message dialog is on
        // the screen, and then you dismiss the dialog by
        // typing the Enter key.
        return;
    }

    curpos = DrawPanel->CursorRealPosition( Mouse );
    oldpos = GetScreen()->m_Curseur;

    delta = GetScreen()->GetGridSize();
    GetScreen()->Scale( delta );

    if( delta.x == 0 )
        delta.x = 1;
    if( delta.y == 0 )
        delta.y = 1;

    switch( g_KeyPressed )
    {
    case WXK_NUMPAD8:
    case WXK_UP:
        Mouse.y -= wxRound(delta.y);
        DrawPanel->MouseTo( Mouse );
        break;

    case WXK_NUMPAD2:
    case WXK_DOWN:
        Mouse.y += wxRound(delta.y);
        DrawPanel->MouseTo( Mouse );
        break;

    case WXK_NUMPAD4:
    case WXK_LEFT:
        Mouse.x -= wxRound(delta.x);
        DrawPanel->MouseTo( Mouse );
        break;

    case WXK_NUMPAD6:
    case WXK_RIGHT:
        Mouse.x += wxRound(delta.x);
        DrawPanel->MouseTo( Mouse );
        break;

    default:
        hotkey = g_KeyPressed;
        break;
    }

    GetScreen()->m_Curseur = curpos;

    PutOnGrid( &GetScreen()->m_Curseur );

    if( oldpos != GetScreen()->m_Curseur )
    {
        curpos = GetScreen()->m_Curseur;
        GetScreen()->m_Curseur = oldpos;
        DrawPanel->CursorOff( DC );

        GetScreen()->m_Curseur = curpos;
        DrawPanel->CursorOn( DC );

        if( DrawPanel->ManageCurseur )
        {
            DrawPanel->ManageCurseur( DrawPanel, DC, TRUE );
        }
    }

    if( hotkey )
    {
        OnHotKey( DC, hotkey, NULL );
    }

    if( GetScreen()->IsRefreshReq() )
    {
        RedrawActiveWindow( DC, TRUE );
    }

    SetToolbars();
    UpdateStatusBar();
}
