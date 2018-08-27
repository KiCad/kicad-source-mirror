/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <eda_dockart.h>
#include <wx/aui/tabart.h>
#include <wx/aui/auibook.h>
//#include <class_draw_panel_gal.h>
//#include <class_drawpanel.h>
#include <draw_frame.h>

void EDA_DOCKART::DrawBorder( wxDC& aDC, wxWindow* aWindow, const wxRect& aRect,
                              wxAuiPaneInfo& aPane )
{
    const wxRect& r = aRect;
    aDC.SetPen( m_borderPen );
    aDC.SetBrush( *wxTRANSPARENT_BRUSH );

    // notebooks draw the border themselves, so they can use native rendering (e.g. tabartgtk)
    wxAuiTabArt* art = nullptr;
    wxAuiNotebook* nb = wxDynamicCast( aWindow, wxAuiNotebook );

    if( nb )
        art = nb->GetArtProvider();

    if( art )
    {
        art->DrawBorder( aDC, aWindow, r );
    }
    else if( aPane.name == "DrawFrame" || aPane.name == "DrawFrameGal" )
    {
        // We don't want to re-write the layout manager, so we give the canvas a single-pixel
        // border and then fill in the top and left with the canvas background colour.
        //
        // This achieves a right-bottom-bordered canvas, which works reasonably well with
        // wxWidgets right-bottom bordered toolbars.

        //wxWindow* window = reinterpret_cast<wxWindow*>( m_frame->GetCanvas() );
        //wxSize scrollbarSize = window->GetSize() - window->GetClientSize();
        // Not sure why it takes a pen twice as wide as the border to fill it in, but it does.
        #if 0
        int borderWidth = GetMetric( wxAUI_DOCKART_PANE_BORDER_SIZE ) * 2;
        int borderAdjust = borderWidth;

        aDC.SetPen( wxPen( m_frame->GetDrawBgColor().ToColour(), borderWidth ) );

        // Yes, this leaves me scratching my head too.
        if( m_frame->IsType( FRAME_PCB )
         || m_frame->IsType( FRAME_PCB_MODULE_EDITOR )
         || m_frame->IsType( FRAME_PCB_MODULE_VIEWER )
         || m_frame->IsType( FRAME_PCB_MODULE_VIEWER_MODAL )
         || m_frame->IsType( FRAME_GERBER ) )
        {
            borderAdjust += 1;
        }

        // left
        aDC.DrawLine( r.x + 1, r.y, r.x + 1, r.y + r.height - borderAdjust - scrollbarSize.y );
        // top
        aDC.DrawLine( r.x + 1, r.y, r.x + r.width - borderAdjust - scrollbarSize.x, r.y );

        aDC.SetPen( m_borderPen );

        // finish off bottom of left side (at end of scrollbar)
        aDC.DrawLine( r.x, r.y + r.height - 1 - scrollbarSize.y, r.x, r.y + r.height - 1 );
        // right
        aDC.DrawLine( r.x + r.width, r.y, r.x + r.width, r.y + r.height - 1 );
        // bottom
        aDC.DrawLine( r.x, r.y + r.height - 1, r.x + r.width - 1, r.y + r.height - 1 );
        #endif
    }
    else
    {
        aDC.DrawRectangle( r );
    }
}


