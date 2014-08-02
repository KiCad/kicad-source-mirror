/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2006 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 1992-2011 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file copy_to_clipboard.cpp
 */

#include <wx/metafile.h>
#include <fctsys.h>
#include <gr_basic.h>
#include <common.h>
#include <id.h>
#include <class_drawpanel.h>
#include <class_base_screen.h>
#include <confirm.h>
#include <draw_frame.h>

static bool DrawPageOnClipboard( EDA_DRAW_FRAME* aFrame );


void EDA_DRAW_FRAME::CopyToClipboard( wxCommandEvent& event )
{
    DrawPageOnClipboard( this );

    if( event.GetId() == ID_GEN_COPY_BLOCK_TO_CLIPBOARD )
    {
        if( GetScreen()->IsBlockActive() )
            m_canvas->SetCursor( wxCursor( (wxStockCursor) m_canvas->GetDefaultCursor() ) );

        m_canvas->EndMouseCapture();
    }
}


/* copy the current page or block to the clipboard ,
 * to export drawings to other applications (word processing ...)
 * This is not suitable for copy command within Eeschema or Pcbnew
 */
bool DrawPageOnClipboard( EDA_DRAW_FRAME* aFrame )
{
    bool    success = true;

#ifdef __WINDOWS__
    int     tmpzoom;
    wxPoint tmp_startvisu;
    wxPoint old_org;
    wxPoint DrawOffset;
    int     ClipboardSizeX, ClipboardSizeY;
    bool    DrawBlock = false;
    wxRect  DrawArea;
    BASE_SCREEN* screen = aFrame->GetCanvas()->GetScreen();

    // scale is the ratio resolution/internal units
    double  scale = 82.0 / 1000.0 / (double) screen->MilsToIuScalar();

    if( screen->IsBlockActive() )
    {
        DrawBlock = true;
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

    EDA_RECT tmp = *aFrame->GetCanvas()->GetClipBox();
    GRResetPenAndBrush( &dc );
    const bool plotBlackAndWhite = false;
    GRForceBlackPen( plotBlackAndWhite );
    screen->m_IsPrinting = true;
    dc.SetUserScale( scale, scale );
    ClipboardSizeX = dc.MaxX() + 10;
    ClipboardSizeY = dc.MaxY() + 10;
    aFrame->GetCanvas()->SetClipBox( EDA_RECT( wxPoint( 0, 0 ), wxSize( 0x7FFFFF0, 0x7FFFFF0 ) ) );

    if( DrawBlock )
    {
        dc.SetClippingRegion( DrawArea );
    }

    const LSET allLayersMask = LSET().set();
    aFrame->PrintPage( &dc, allLayersMask, false );
    screen->m_IsPrinting = false;
    aFrame->GetCanvas()->SetClipBox( tmp );
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
