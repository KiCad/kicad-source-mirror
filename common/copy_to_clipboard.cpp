/////////////////////////////////////////////////////////////////////////////
// Name:        copy_to_clipboard.cpp
// Author:      jean-pierre Charras
// Created:     18 aug 2006
// Licence:     License GNU
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


/* calls the function to copy the current page or the current bock to
 * the clipboard
 */
void WinEDA_DrawFrame::CopyToClipboard( wxCommandEvent& event )
{
    DrawPage( DrawPanel );

    if(  event.GetId() == ID_GEN_COPY_BLOCK_TO_CLIPBOARD )
    {
        if( GetBaseScreen()->m_BlockLocate.m_Command != BLOCK_IDLE )
            DrawPanel->SetCursor( wxCursor( DrawPanel->m_PanelCursor =
                        DrawPanel->m_PanelDefaultCursor ) );

        if( DrawPanel->ManageCurseur && DrawPanel->ForceCloseManageCurseur )
        {
            KicadGraphicContext dc( DrawPanel );
            DrawPanel->ForceCloseManageCurseur( DrawPanel, &dc );
        }
    }
}


/* copy the current page or block to the clipboard ,
 * to export drawings to other applications (word processing ...)
 * This is not suitable for copy command within eeschema or pcbnew
 */
bool DrawPage( WinEDA_DrawPanel* panel )
{
    bool    success = TRUE;

#ifdef __WINDOWS__
    int     tmpzoom;
    wxPoint tmp_startvisu;
    wxPoint old_org;
    wxPoint DrawOffset;
    int     ClipboardSizeX, ClipboardSizeY;
    bool    DrawBlock = FALSE;
    wxRect  DrawArea;
    BASE_SCREEN* screen = panel->GetScreen();

    /* scale is the ratio resolution/internal units */
    float   scale = 82.0 / panel->m_Parent->m_InternalUnits;

    if( ActiveScreen->m_BlockLocate.m_Command != BLOCK_IDLE )
    {
        DrawBlock = TRUE;
        DrawArea.SetX( ActiveScreen->m_BlockLocate.GetX() );
        DrawArea.SetY( ActiveScreen->m_BlockLocate.GetY() );
        DrawArea.SetWidth( ActiveScreen->m_BlockLocate.GetWidth() );
        DrawArea.SetHeight( ActiveScreen->m_BlockLocate.GetHeight() );
    }

    /* Change frames and local settings. */
    tmp_startvisu = ActiveScreen->m_StartVisu;
    tmpzoom = ActiveScreen->GetZoom();
    old_org = ActiveScreen->m_DrawOrg;
    ActiveScreen->m_DrawOrg.x   = ActiveScreen->m_DrawOrg.y = 0;
    ActiveScreen->m_StartVisu.x = ActiveScreen->m_StartVisu.y = 0;

    ActiveScreen->SetZoom( 1 );

    wxMetafileDC dc /*(wxT(""), DrawArea.GetWidth(), DrawArea.GetHeight())*/;

    EDA_Rect tmp = panel->m_ClipBox;
    GRResetPenAndBrush( &dc );
    GRForceBlackPen( s_PlotBlackAndWhite );
    screen->m_IsPrinting = true;
    dc.SetUserScale( scale, scale );
    ClipboardSizeX = dc.MaxX() + 10;
    ClipboardSizeY = dc.MaxY() + 10;
    panel->m_ClipBox.SetX( 0 ); panel->m_ClipBox.SetY( 0 );
    panel->m_ClipBox.SetWidth( 0x7FFFFF0 );
    panel->m_ClipBox.SetHeight( 0x7FFFFF0 );

    if( DrawBlock )
    {
        dc.SetClippingRegion( DrawArea );
    }

    panel->PrintPage( &dc, Print_Sheet_Ref, -1, false );
    screen->m_IsPrinting = false;
    panel->m_ClipBox = tmp;
    wxMetafile* mf = dc.Close();

    if( mf )
    {
        success = mf->SetClipboard( ClipboardSizeX, ClipboardSizeY );
        delete mf;
    }


    GRForceBlackPen( FALSE );
    SetPenMinWidth( 1 );

    ActiveScreen->m_StartVisu = tmp_startvisu;
    ActiveScreen->m_DrawOrg   = old_org;
    ActiveScreen->SetZoom( tmpzoom );
#endif

    return success;
}
