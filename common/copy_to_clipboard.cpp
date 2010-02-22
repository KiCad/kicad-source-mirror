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

        DrawPanel->UnManageCursor(  );
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
    float   scale = 82.0 / panel->GetParent()->m_InternalUnits;

    if( screen->m_BlockLocate.m_Command != BLOCK_IDLE )
    {
        DrawBlock = TRUE;
        DrawArea.SetX( screen->m_BlockLocate.GetX() );
        DrawArea.SetY( screen->m_BlockLocate.GetY() );
        DrawArea.SetWidth( screen->m_BlockLocate.GetWidth() );
        DrawArea.SetHeight( screen->m_BlockLocate.GetHeight() );
    }

    /* Change frames and local settings. */
    tmp_startvisu = screen->m_StartVisu;
    tmpzoom = screen->GetZoom();
    old_org = screen->m_DrawOrg;
    screen->m_DrawOrg.x   = screen->m_DrawOrg.y = 0;
    screen->m_StartVisu.x = screen->m_StartVisu.y = 0;

    screen->SetZoom( 1 );

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

    panel->PrintPage( &dc, Print_Sheet_Ref, -1, false, NULL );
    screen->m_IsPrinting = false;
    panel->m_ClipBox = tmp;
    wxMetafile* mf = dc.Close();

    if( mf )
    {
        success = mf->SetClipboard( ClipboardSizeX, ClipboardSizeY );
        delete mf;
    }


    GRForceBlackPen( FALSE );

    screen->m_StartVisu = tmp_startvisu;
    screen->m_DrawOrg   = old_org;
    screen->SetZoom( tmpzoom );
#endif

    return success;
}
