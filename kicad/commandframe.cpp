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

/**
 * @file commandframe.cpp
 * @brief Frame showing fast launch buttons and messages box
 */


#include <bitmaps.h>

#include "kicad.h"

// Amount of clearance between tool buttons
const int BUTTON_SEPARATION = 5;

// Buttons are larger than images by this amount
const int BUTTON_EXPANSION  = 5;

LAUNCHER_PANEL::LAUNCHER_PANEL( wxWindow* parent ) :
    wxPanel( parent, wxID_ANY )
{

    // Add bitmap buttons to launch KiCad utilities:
    CreateCommandToolbar();
}

int LAUNCHER_PANEL::GetPanelHeight() const
{
    return m_height + 2 * BUTTON_SEPARATION;
}

/**
 * Add application launcher buttons
 * e.g. Eeschema, CvPcb, Pcbnew, GerbView
 */
void LAUNCHER_PANEL::CreateCommandToolbar()
{
    m_buttonSizer = new wxBoxSizer( wxHORIZONTAL );

    AddButton( ID_TO_SCH,
               KiBitmap( icon_eeschema_xpm ),
               _( "Schematic layout editor" ) );

    AddButton( ID_TO_SCH_LIB_EDITOR,
               KiBitmap( icon_libedit_xpm ),
               _( "Schematic library editor" ) );

    AddButton( ID_TO_PCB,
               KiBitmap( icon_pcbnew_xpm ),
               _( "PCB layout editor" ) );

    AddButton( ID_TO_PCB_FP_EDITOR,
               KiBitmap( icon_modedit_xpm ),
               _( "PCB library editor" ) );

    AddButton( ID_TO_GERBVIEW,
               KiBitmap( icon_gerbview_xpm ),
               _( "Gerber viewer" ) );

    AddButton( ID_TO_BITMAP_CONVERTER,
               KiBitmap( icon_bitmap2component_xpm ),
               _( "Import bitmap\n"
                  "Convert bitmap images to schematic or PCB elements" ) );

    AddButton( ID_TO_PCB_CALCULATOR,
               KiBitmap( icon_pcbcalculator_xpm ),
               _( "Calculator tools" ) );

    AddButton( ID_TO_PL_EDITOR,
               KiBitmap( icon_pagelayout_editor_xpm ),
               _( "Worksheet layout editor" ) );

    // Add a stretchy spacer to make button bar fill the entire screen
    m_buttonSizer->AddStretchSpacer();

    SetSizer( m_buttonSizer );
}

/**
 * Add a single button to the toolbar
 * @param aId is the ID of the button
 * @param aBitmap is the image to be used
 * @param aToolTip is the button mouse-over tool tip
 */
void LAUNCHER_PANEL::AddButton( wxWindowID aId, const wxBitmap& aBitmap, wxString aToolTip )
{
    wxSize  buttSize( aBitmap.GetWidth() + BUTTON_EXPANSION,
                      aBitmap.GetHeight() + BUTTON_EXPANSION );

    if( m_height < buttSize.y )
        m_height = buttSize.y;

    auto btn = new wxBitmapButton( this, aId, aBitmap, wxDefaultPosition, buttSize );

    btn->SetToolTip( aToolTip );

    m_buttonSizer->Add( btn,
                        0,
                        wxALL | wxEXPAND,
                        BUTTON_SEPARATION );
}
