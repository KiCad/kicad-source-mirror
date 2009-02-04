/////////////////////////////////////////////////////////////////////////////

// Name:        copy_to_clipboard.cpp
// Author:      jean-pierre Charras
// Created:     18 aug 2006
// Licence:   License GNU
/////////////////////////////////////////////////////////////////////////////

#include "wx/metafile.h"
#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "id.h"
#include "class_drawpanel.h"
#include "class_base_screen.h"
#include "confirm.h"
#include "wxstruct.h"


extern BASE_SCREEN* ActiveScreen;

static const bool   s_PlotBlackAndWhite = FALSE;
static const bool   Print_Sheet_Ref = TRUE;

static bool DrawPage( WinEDA_DrawPanel* panel );

/************************************************************/
void WinEDA_DrawFrame::CopyToClipboard( wxCommandEvent& event )
/************************************************************/

/* calls the function to copy the current page or the current bock to the clipboard
 */
{
    DrawPage( DrawPanel );

    if(  event.GetId() == ID_GEN_COPY_BLOCK_TO_CLIPBOARD )
    {
        if( GetBaseScreen()->BlockLocate.m_Command != BLOCK_IDLE )
            DrawPanel->SetCursor( wxCursor( DrawPanel->m_PanelCursor =
                        DrawPanel->m_PanelDefaultCursor ) );

        if( DrawPanel->ManageCurseur && DrawPanel->ForceCloseManageCurseur )
        {
            wxClientDC dc( DrawPanel );

            DrawPanel->PrepareGraphicContext( &dc );
            DrawPanel->ForceCloseManageCurseur( DrawPanel, &dc );
        }
    }
}


/*****************************************************************/
bool DrawPage( WinEDA_DrawPanel* panel )
/*****************************************************************/

/* copy the current page or block to the clipboard ,
 * to export drawings to other applications (word processing ...)
 * Thi is not suitable for copy command within eeschema or pcbnew
 */
{
    bool    success = TRUE;

#ifdef __WINDOWS__
    int     tmpzoom;
    wxPoint tmp_startvisu;
    wxPoint old_org;
    wxPoint DrawOffset; // Offset de trace
    int     ClipboardSizeX, ClipboardSizeY;
    bool    DrawBlock = FALSE;
    wxRect  DrawArea;

    /* scale is the ratio resolution/internal units */
    float   scale = 82.0 / panel->m_Parent->m_InternalUnits;

    if( ActiveScreen->BlockLocate.m_Command != BLOCK_IDLE )
    {
        DrawBlock = TRUE;
        DrawArea.SetX( (int) ( ActiveScreen->BlockLocate.GetX() ) );
        DrawArea.SetY( (int) ( ActiveScreen->BlockLocate.GetY() ) );
        DrawArea.SetWidth( (int) ( ActiveScreen->BlockLocate.GetWidth() ) );
        DrawArea.SetHeight( (int) ( ActiveScreen->BlockLocate.GetHeight() ) );
    }

    /* modification des cadrages et reglages locaux */
    tmp_startvisu = ActiveScreen->m_StartVisu;
    tmpzoom = ActiveScreen->GetZoom();
    old_org = ActiveScreen->m_DrawOrg;
    ActiveScreen->m_DrawOrg.x   = ActiveScreen->m_DrawOrg.y = 0;
    ActiveScreen->m_StartVisu.x = ActiveScreen->m_StartVisu.y = 0;

    ActiveScreen->SetZoom( 1 );

    wxMetafileDC dc /*(wxT(""), DrawArea.GetWidth(), DrawArea.GetHeight())*/;

    if( !dc.Ok() )
    {
        DisplayError( NULL, wxT( "CopyToClipboard: DrawPage error: wxMetafileDC not OK" ) );
        success = FALSE;
    }
    else
    {
        EDA_Rect tmp = panel->m_ClipBox;
        GRResetPenAndBrush( &dc );
        GRForceBlackPen( s_PlotBlackAndWhite );
        g_IsPrinting = TRUE;
        dc.SetUserScale( scale, scale );
        ClipboardSizeX = dc.MaxX() + 10;
        ClipboardSizeY = dc.MaxY() + 10;
        panel->m_ClipBox.SetX( 0 ); panel->m_ClipBox.SetY( 0 );
        panel->m_ClipBox.SetWidth( 0x7FFFFF0 ); panel->m_ClipBox.SetHeight( 0x7FFFFF0 );

        if( DrawBlock )
        {
            dc.SetClippingRegion( DrawArea );
        }
        panel->PrintPage( &dc, Print_Sheet_Ref, -1, false );
        g_IsPrinting     = FALSE;
        panel->m_ClipBox = tmp;
        wxMetafile* mf = dc.Close();
        if( mf )
        {
            success = mf->SetClipboard( ClipboardSizeX, ClipboardSizeY );
            delete mf;
        }
    }


    GRForceBlackPen( FALSE );
    SetPenMinWidth( 1 );

    ActiveScreen->m_StartVisu = tmp_startvisu;
    ActiveScreen->m_DrawOrg   = old_org;
    ActiveScreen->SetZoom( tmpzoom );
#endif

    return success;
}
