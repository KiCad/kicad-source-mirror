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

#include <wx/clipbrd.h>
//#include <wx/metafile.h>
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
    bool    DrawBlock = false;
    wxRect  DrawArea;
    BASE_SCREEN* screen = aFrame->GetCanvas()->GetScreen();

    if( screen->IsBlockActive() )
    {
        DrawBlock = true;
        DrawArea.SetX( screen->m_BlockLocate.GetX() );
        DrawArea.SetY( screen->m_BlockLocate.GetY() );
        DrawArea.SetWidth( screen->m_BlockLocate.GetWidth() );
        DrawArea.SetHeight( screen->m_BlockLocate.GetHeight() );
    }
    else
        DrawArea.SetSize( aFrame->GetPageSizeIU() );

    // Calculate a reasonable dc size, in pixels, and the dc scale to fit
    // the drawings into the dc size
    // scale is the ratio resolution (in PPI) / internal units
    double ppi = 300;   // Use 300 pixels per inch to create bitmap images on start
    double inch2Iu = 1000.0 * (double) screen->MilsToIuScalar();
    double  scale = ppi / inch2Iu;

    wxSize dcsize = DrawArea.GetSize();

    int maxdim = std::max( dcsize.x, dcsize.y );
    // the max size in pixels of the bitmap used to byuild the sheet copy
    const int maxbitmapsize = 3000;

    while( int( maxdim * scale ) > maxbitmapsize )
    {
        ppi = ppi / 1.5;
        scale = ppi / inch2Iu;
    }

    dcsize.x *= scale;
    dcsize.y *= scale;

    // Set draw offset, zoom... to values needed to draw in the memory DC
    // after saving initial values:
    wxPoint tmp_startvisu = screen->m_StartVisu;
    double tmpzoom = screen->GetZoom();
    wxPoint old_org = screen->m_DrawOrg;
    screen->m_DrawOrg.x   = screen->m_DrawOrg.y = 0;
    screen->m_StartVisu.x = screen->m_StartVisu.y = 0;

    screen->SetZoom( 1 );   // we use zoom = 1 in draw functions.

    wxMemoryDC dc;
    wxBitmap image( dcsize );
    dc.SelectObject( image );

    EDA_RECT tmp = *aFrame->GetCanvas()->GetClipBox();
    GRResetPenAndBrush( &dc );
    GRForceBlackPen( false );
    screen->m_IsPrinting = true;
    dc.SetUserScale( scale, scale );

    aFrame->GetCanvas()->SetClipBox( EDA_RECT( wxPoint( 0, 0 ),
                                     wxSize( 0x7FFFFF0, 0x7FFFFF0 ) ) );

    if( DrawBlock )
    {
        dc.SetClippingRegion( DrawArea );
    }

    dc.Clear();
    aFrame->GetCanvas()->EraseScreen( &dc );
    const LSET allLayersMask = LSET().set();
    aFrame->PrintPage( &dc, allLayersMask, false );
    screen->m_IsPrinting = false;
    aFrame->GetCanvas()->SetClipBox( tmp );

    bool    success = true;

    if( wxTheClipboard->Open() )
    {
        // This data objects are held by the clipboard,
        // so do not delete them in the app.
        wxBitmapDataObject* clipbrd_data = new wxBitmapDataObject( image );
        wxTheClipboard->SetData( clipbrd_data );
        wxTheClipboard->Close();
    }
    else
        success = false;

    // Deselect Bitmap from DC in order to delete the MemoryDC safely
    // without deleting the bitmap
    dc.SelectObject( wxNullBitmap );

    GRForceBlackPen( false );

    screen->m_StartVisu = tmp_startvisu;
    screen->m_DrawOrg   = old_org;
    screen->SetZoom( tmpzoom );

    return success;
}
