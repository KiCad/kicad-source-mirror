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


LAUNCHER_PANEL::LAUNCHER_PANEL( wxWindow* parent ) :
    wxPanel( parent, wxID_ANY )
{
    m_bitmapButtons_maxHeigth = 0;
    m_buttonSeparation = 10;        // control of command buttons position
    m_buttonsListPosition.x = m_buttonSeparation;
    m_buttonsListPosition.y = m_buttonSeparation;
    m_buttonLastPosition    = m_buttonsListPosition;

    // Add bitmap buttons to launch KiCad utilities:
    CreateCommandToolbar();
}

int LAUNCHER_PANEL::GetPanelHeight() const
{
    int height = m_buttonsListPosition.y + m_bitmapButtons_maxHeigth
                 + m_buttonSeparation;
    return height;
}

/**
 * Function CreateCommandToolbar
 * create the buttons to call Eeschema CvPcb, Pcbnew and GerbView
 */
void LAUNCHER_PANEL::CreateCommandToolbar()
{
    wxBitmapButton* btn;

    btn = AddBitmapButton( ID_TO_SCH, KiBitmap( icon_eeschema_xpm ) );
    btn->SetToolTip( _( "Eeschema - Electronic schematic editor" ) );

    btn = AddBitmapButton( ID_TO_SCH_LIB_EDITOR, KiBitmap( libedit_icon_xpm ) );
    btn->SetToolTip( _( "Schematic library editor" ) );

#if 0
    btn = AddBitmapButton( ID_TO_CVPCB, KiBitmap( icon_cvpcb_xpm ) );
    btn->SetToolTip( _( "CvPcb - Associate footprint to components" ) );
#endif

    btn = AddBitmapButton( ID_TO_PCB, KiBitmap( icon_pcbnew_xpm ) );
    btn->SetToolTip( _( "Pcbnew - Printed circuit board editor" ) );

    btn = AddBitmapButton( ID_TO_PCB_FP_EDITOR, KiBitmap( icon_modedit_xpm ) );
    btn->SetToolTip( _( "PCB footprint editor" ) );

    btn = AddBitmapButton( ID_TO_GERBVIEW, KiBitmap( icon_gerbview_xpm ) );
    btn->SetToolTip( _( "GerbView - Gerber viewer" ) );

    btn = AddBitmapButton( ID_TO_BITMAP_CONVERTER, KiBitmap( icon_bitmap2component_xpm ) );
    btn->SetToolTip( _(
                        "Bitmap2Component - Convert bitmap images to Eeschema\n"
                        "or Pcbnew elements" ) );

    btn = AddBitmapButton( ID_TO_PCB_CALCULATOR, KiBitmap( icon_pcbcalculator_xpm ) );
    btn->SetToolTip( _( "Pcb calculator - Calculator for components, track width, etc." ) );

    btn = AddBitmapButton( ID_TO_PL_EDITOR, KiBitmap( icon_pagelayout_editor_xpm ) );
    btn->SetToolTip( _( "Pl editor - Worksheet layout editor" ) );
}


/**
 * Function AddBitmapButton
 * add a  Bitmap Button (fast launch button) to the buttons panel
 * @param aId = the button id
 * @param aBitmap = the wxBitmap used to create the button
 */
wxBitmapButton* LAUNCHER_PANEL::AddBitmapButton( wxWindowID aId, const wxBitmap& aBitmap  )
{
    wxPoint buttPos = m_buttonLastPosition;
    wxSize  buttSize;
    int     btn_margin = 10;     // extra margin around the bitmap.

    buttSize.x = aBitmap.GetWidth() + btn_margin;
    buttSize.y = aBitmap.GetHeight() + btn_margin;

    if( m_bitmapButtons_maxHeigth < buttSize.y )
        m_bitmapButtons_maxHeigth = buttSize.y;

    wxBitmapButton* btn = new wxBitmapButton( this, aId, aBitmap, buttPos, buttSize );
    m_buttonLastPosition.x += buttSize.x + m_buttonSeparation;

    return btn;
}
