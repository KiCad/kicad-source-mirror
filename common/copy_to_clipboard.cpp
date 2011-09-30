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

static bool DrawPageOnClipboard( EDA_DRAW_FRAME* aFrame );


/* calls the function to copy the current page or the current bock to
 * the clipboard
 */
void EDA_DRAW_FRAME::CopyToClipboard( wxCommandEvent& event )
{
    DrawPageOnClipboard( this );

    if( event.GetId() == ID_GEN_COPY_BLOCK_TO_CLIPBOARD )
    {
        if( GetScreen()->IsBlockActive() )
            DrawPanel->SetCursor( wxCursor( DrawPanel->GetDefaultCursor() ) );

        DrawPanel->EndMouseCapture();
    }
}


/* copy the current page or block to the clipboard ,
 * to export drawings to other applications (word processing ...)
 * This is not suitable for copy command within Eeschema or Pcbnew
 */
bool DrawPageOnClipboard( EDA_DRAW_FRAME* aFrame )
{
    bool    success = TRUE;

#ifdef __WINDOWS__
    int     tmpzoom;
    wxPoint tmp_startvisu;
    wxPoint old_org;
    wxPoint DrawOffset;
    int     ClipboardSizeX, ClipboardSizeY;
    bool    DrawBlock = false;
    wxRect  DrawArea;
    BASE_SCREEN* screen = aFrame->DrawPanel->GetScreen();

    /* scale is the ratio resolution/internal units */
    float   scale = 82.0 / aFrame->m_InternalUnits;

    if( screen->IsBlockActive() )
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

    wxMetafileDC dc;

    EDA_RECT tmp = aFrame->DrawPanel->m_ClipBox;
    GRResetPenAndBrush( &dc );
    const bool plotBlackAndWhite = false;
    GRForceBlackPen( plotBlackAndWhite );
    screen->m_IsPrinting = true;
    dc.SetUserScale( scale, scale );
    ClipboardSizeX = dc.MaxX() + 10;
    ClipboardSizeY = dc.MaxY() + 10;
    aFrame->DrawPanel->m_ClipBox.SetX( 0 );
    aFrame->DrawPanel->m_ClipBox.SetY( 0 );
    aFrame->DrawPanel->m_ClipBox.SetWidth( 0x7FFFFF0 );
    aFrame->DrawPanel->m_ClipBox.SetHeight( 0x7FFFFF0 );

    if( DrawBlock )
    {
        dc.SetClippingRegion( DrawArea );
    }

    const int maskLayer = 0xFFFFFFFF;
    aFrame->PrintPage( &dc, maskLayer, false );
    screen->m_IsPrinting = false;
    aFrame->DrawPanel->m_ClipBox = tmp;
    wxMetafile* mf = dc.Close();

    if( mf )
    {
        success = mf->SetClipboard( ClipboardSizeX, ClipboardSizeY );
        delete mf;
    }


    GRForceBlackPen( false );

    screen->m_StartVisu = tmp_startvisu;
    screen->m_DrawOrg   = old_org;
    screen->SetZoom( tmpzoom );
#endif

    return success;
}
