/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2018 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <eda_base_frame.h>
#include <dialog_helpers.h>
#include <base_units.h>
#include <macros.h>


/********************************************************/
/* Class to display and edit a coordinated INCHES or MM */
/********************************************************/
EDA_POSITION_CTRL::EDA_POSITION_CTRL( wxWindow* parent, const wxString& title, const wxPoint& aPos,
                                      EDA_UNITS_T user_unit, wxBoxSizer* BoxSizer )
{
    m_UserUnit = user_unit;

    m_TextX = new wxStaticText( parent, -1, title + _( " X:" ) );
    BoxSizer->Add( m_TextX, 0, wxGROW | wxLEFT | wxRIGHT | wxTOP, 5 );

    m_FramePosX = new wxTextCtrl( parent, -1, wxEmptyString, wxDefaultPosition );
    BoxSizer->Add( m_FramePosX, 0, wxGROW | wxLEFT | wxRIGHT | wxBOTTOM, 5 );

    m_TextY = new wxStaticText( parent, -1, title + _( " Y:" ) );
    BoxSizer->Add( m_TextY, 0, wxGROW | wxLEFT | wxRIGHT | wxTOP, 5 );

    m_FramePosY = new wxTextCtrl( parent, -1, wxEmptyString );
    BoxSizer->Add( m_FramePosY, 0, wxGROW | wxLEFT | wxRIGHT | wxBOTTOM, 5 );

    SetValue( aPos.x, aPos.y );
}


EDA_POSITION_CTRL::~EDA_POSITION_CTRL()
{
    delete m_TextX;
    delete m_TextY;
    delete m_FramePosX;
    delete m_FramePosY;
}


/* Returns (in internal units) to coordinate between (in user units)
 */
wxPoint EDA_POSITION_CTRL::GetValue()
{
    return wxPoint( ValueFromString( m_UserUnit, m_FramePosX->GetValue() ),
                    ValueFromString( m_UserUnit, m_FramePosY->GetValue() ) );
}


void EDA_POSITION_CTRL::Enable( bool x_win_on, bool y_win_on )
{
    m_FramePosX->Enable( x_win_on );
    m_FramePosY->Enable( y_win_on );
}


void EDA_POSITION_CTRL::SetValue( int x_value, int y_value )
{
    m_FramePosX->SetValue( StringFromValue( m_UserUnit, x_value, true ) );
    m_FramePosY->SetValue( StringFromValue( m_UserUnit, y_value, true ) );
}


/*******************/
/* EDA_SIZE_CTRL */
/*******************/
EDA_SIZE_CTRL::EDA_SIZE_CTRL( wxWindow* parent, const wxString& title, const wxSize& aSize,
                              EDA_UNITS_T aUnit, wxBoxSizer* aBoxSizer ) :
    EDA_POSITION_CTRL( parent, title, wxPoint( aSize.x, aSize.y ), aUnit, aBoxSizer )
{
}


wxSize EDA_SIZE_CTRL::GetValue()
{
    wxPoint pos = EDA_POSITION_CTRL::GetValue();

    return wxSize( pos.x, pos.y );
}


