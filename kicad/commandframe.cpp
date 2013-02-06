/**
 * @file commandframe.cpp
 * @brief Frame showing fast launch buttons and messages box
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2012 Jean-Pierre Charras
 * Copyright (C) 2004-2012 KiCad Developers, see change_log.txt for contributors.
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


#include <fctsys.h>
#include <macros.h>

#include <kicad.h>
#include <menus_helpers.h>


RIGHT_KM_FRAME::RIGHT_KM_FRAME( KICAD_MANAGER_FRAME* parent ) :
    wxSashLayoutWindow( parent, wxID_ANY )
{
    m_Parent    = parent;
    m_MessagesBox = NULL;
    m_bitmapButtons_maxHeigth = 0;
    m_ButtonSeparation = 10;        // control of command buttons position
    m_ButtonsListPosition.x = m_ButtonSeparation;
    m_ButtonsListPosition.y = m_ButtonSeparation;
    m_ButtonLastPosition    = m_ButtonsListPosition;

    // Add bitmap buttons to launch KiCad utilities:
    m_ButtPanel = new wxPanel( this, wxID_ANY );
    CreateCommandToolbar();
    m_ButtonsPanelHeight    = m_ButtonsListPosition.y + m_bitmapButtons_maxHeigth + m_ButtonSeparation;

    // Add the wxTextCtrl showing all messages from KiCad:
    m_MessagesBox = new wxTextCtrl( this, wxID_ANY, wxEmptyString,
                                  wxDefaultPosition, wxDefaultSize,
                                  wxTE_MULTILINE | wxSUNKEN_BORDER | wxTE_READONLY );
}


void RIGHT_KM_FRAME::OnSize( wxSizeEvent& event )
{
    #define EXTRA_MARGE 4
    wxSize  wsize;
    wsize.x = GetClientSize().x;

    // Fix size of buttons area
    wsize.y = m_ButtonsPanelHeight;
    m_ButtPanel->SetSize( wsize );

    // Fix position and size of the message area, below the buttons area
    wsize.y = GetClientSize().y - m_ButtonsPanelHeight;
    m_MessagesBox->SetSize( wsize );
    m_MessagesBox->SetPosition( wxPoint( 0, m_ButtonsPanelHeight ) );

    event.Skip();
}


BEGIN_EVENT_TABLE( RIGHT_KM_FRAME, wxSashLayoutWindow )
EVT_SIZE( RIGHT_KM_FRAME::OnSize )
END_EVENT_TABLE()


/**
 * Function CreateCommandToolbar
 * create the buttons to call Eeschema CvPcb, Pcbnew and GerbView
 */
void RIGHT_KM_FRAME::CreateCommandToolbar( void )
{
    wxBitmapButton* btn;

    btn = AddBitmapButton( ID_TO_EESCHEMA, KiBitmap( icon_eeschema_xpm ) );
    btn->SetToolTip( _( "Eeschema (Schematic editor)" ) );

    btn = AddBitmapButton( ID_TO_CVPCB, KiBitmap( icon_cvpcb_xpm ) );
    btn->SetToolTip( _( "CvPcb (Components to modules)" ) );

    btn = AddBitmapButton( ID_TO_PCB, KiBitmap( icon_pcbnew_xpm ) );
    btn->SetToolTip( _( "Pcbnew (PCB editor)" ) );

    btn = AddBitmapButton( ID_TO_GERBVIEW, KiBitmap( icon_gerbview_xpm ) );
    btn->SetToolTip( _( "GerbView (Gerber viewer)" ) );

    btn = AddBitmapButton( ID_TO_BITMAP_CONVERTER, KiBitmap( icon_bitmap2component_xpm ) );
    btn->SetToolTip( _(
                        "Bitmap2Component (a tool to build a logo from a bitmap)\n\
Creates a component (for Eeschema) or a footprint (for Pcbnew) that shows a B&W picture"                                                                                     ) );

    btn = AddBitmapButton( ID_TO_PCB_CALCULATOR, KiBitmap( icon_pcbcalculator_xpm ) );
    btn->SetToolTip( _( "Pcb calculator, the Swiss army knife..." ) );
}


/**
 * Function AddBitmapButton
 * add a  Bitmap Button (fast launch button) to the buttons panel
 * @param aId = the button id
 * @param aBitmap = the wxBitmap used to create the button
 */
wxBitmapButton* RIGHT_KM_FRAME::AddBitmapButton( wxWindowID aId, const wxBitmap& aBitmap  )
{
    wxPoint buttPos = m_ButtonLastPosition;
    wxSize  buttSize;
    int     btn_margin = 10;     // extra margin around the bitmap.

    buttSize.x = aBitmap.GetWidth() + btn_margin;
    buttSize.y = aBitmap.GetHeight() + btn_margin;

    if( m_bitmapButtons_maxHeigth < buttSize.y )
        m_bitmapButtons_maxHeigth = buttSize.y;

    wxBitmapButton* btn = new wxBitmapButton( m_ButtPanel, aId, aBitmap, buttPos, buttSize );
    m_ButtonLastPosition.x += buttSize.x + m_ButtonSeparation;

    return btn;
}
