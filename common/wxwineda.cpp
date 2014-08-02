/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
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
 * @file wxwineda.cpp
 */

#include <fctsys.h>
#include <wxstruct.h>
#include <dialog_helpers.h>
#include <base_units.h>
#include <macros.h>


/*******************************************************/
/* Class to edit a graphic + text size in INCHES or MM */
/*******************************************************/
EDA_GRAPHIC_TEXT_CTRL::EDA_GRAPHIC_TEXT_CTRL( wxWindow*       parent,
                                              const wxString& Title,
                                              const wxString& TextToEdit,
                                              int             textsize,
                                              EDA_UNITS_T     user_unit,
                                              wxBoxSizer*     BoxSizer,
                                              int             framelen )
{
    m_UserUnit = user_unit;
    m_Title = NULL;

    m_Title = new wxStaticText( parent, -1, Title );

    BoxSizer->Add( m_Title, 0, wxGROW | wxLEFT | wxRIGHT | wxTOP, 5 );

    m_FrameText = new wxTextCtrl( parent, -1, TextToEdit );

    BoxSizer->Add( m_FrameText, 0, wxGROW | wxLEFT | wxRIGHT | wxBOTTOM, 5 );

    if( !Title.IsEmpty() )
    {
        wxString      msg;
        msg.Printf( _( "Size%s" ), GetChars( ReturnUnitSymbol( m_UserUnit ) ) );
        wxStaticText* text = new wxStaticText( parent, -1, msg );

        BoxSizer->Add( text, 0, wxGROW | wxLEFT | wxRIGHT, 5 );
    }

    wxString value = FormatSize( m_UserUnit, textsize );

    m_FrameSize = new wxTextCtrl( parent, -1, value, wxDefaultPosition, wxSize( 70, -1 ) );

    BoxSizer->Add( m_FrameSize, 0, wxGROW | wxLEFT | wxRIGHT | wxBOTTOM, 5 );
}


EDA_GRAPHIC_TEXT_CTRL::~EDA_GRAPHIC_TEXT_CTRL()
{
    /* no, these are deleted by the BoxSizer
    delete m_FrameText;
    delete m_Title;
    */
}


wxString EDA_GRAPHIC_TEXT_CTRL::FormatSize( EDA_UNITS_T aUnit, int textSize )
{
    // Limiting the size of the text of reasonable values.
    if( textSize < 10 )
        textSize = 10;

    if( textSize > 3000 )
        textSize = 3000;

    return StringFromValue( aUnit, textSize );
}


void EDA_GRAPHIC_TEXT_CTRL::SetTitle( const wxString& title )
{
    m_Title->SetLabel( title );
}


void EDA_GRAPHIC_TEXT_CTRL::SetValue( const wxString& value )
{
    m_FrameText->SetValue( value );
}


void EDA_GRAPHIC_TEXT_CTRL::SetValue( int textSize )
{
    wxString value = FormatSize( m_UserUnit, textSize );
    m_FrameSize->SetValue( value );
}


const wxString EDA_GRAPHIC_TEXT_CTRL::GetText() const
{
    wxString text = m_FrameText->GetValue();
    return text;
}


int EDA_GRAPHIC_TEXT_CTRL::ParseSize( const wxString& sizeText, EDA_UNITS_T aUnit )
{
    int    textsize;

    textsize = ValueFromString( aUnit, sizeText );

    // Limit to reasonable size
    if( textsize < 10 )
        textsize = 10;

    if( textsize > 3000 )
        textsize = 3000;

    return textsize;
}


int EDA_GRAPHIC_TEXT_CTRL::GetTextSize()
{
    return ParseSize( m_FrameSize->GetValue(), m_UserUnit );
}


void EDA_GRAPHIC_TEXT_CTRL::Enable( bool state )
{
    m_FrameText->Enable( state );
}


/********************************************************/
/* Class to display and edit a coordinated INCHES or MM */
/********************************************************/
EDA_POSITION_CTRL::EDA_POSITION_CTRL( wxWindow*       parent,
                                      const wxString& title,
                                      const wxPoint&  pos_to_edit,
                                      EDA_UNITS_T     user_unit,
                                      wxBoxSizer*     BoxSizer )
{
    wxString text;

    m_UserUnit = user_unit;

    if( title.IsEmpty() )
        text = _( "Pos " );
    else
        text = title;

    text   += _( "X" ) + ReturnUnitSymbol( m_UserUnit );
    m_TextX = new wxStaticText( parent, -1, text );

    BoxSizer->Add( m_TextX, 0, wxGROW | wxLEFT | wxRIGHT | wxTOP, 5 );
    m_FramePosX = new wxTextCtrl( parent, -1, wxEmptyString, wxDefaultPosition );

    BoxSizer->Add( m_FramePosX, 0, wxGROW | wxLEFT | wxRIGHT | wxBOTTOM, 5 );


    if( title.IsEmpty() )
        text = _( "Pos " );
    else
        text = title;
    text   += _( "Y" ) + ReturnUnitSymbol( m_UserUnit );

    m_TextY = new wxStaticText( parent, -1, text );

    BoxSizer->Add( m_TextY, 0, wxGROW | wxLEFT | wxRIGHT | wxTOP, 5 );

    m_FramePosY = new wxTextCtrl( parent, -1, wxEmptyString );

    BoxSizer->Add( m_FramePosY, 0, wxGROW | wxLEFT | wxRIGHT | wxBOTTOM, 5 );

    SetValue( pos_to_edit.x, pos_to_edit.y );
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
    wxPoint coord;

    coord.x = ValueFromString( m_UserUnit, m_FramePosX->GetValue() );
    coord.y = ValueFromString( m_UserUnit, m_FramePosY->GetValue() );

    return coord;
}


void EDA_POSITION_CTRL::Enable( bool x_win_on, bool y_win_on )
{
    m_FramePosX->Enable( x_win_on );
    m_FramePosY->Enable( y_win_on );
}


void EDA_POSITION_CTRL::SetValue( int x_value, int y_value )
{
    wxString msg;

    m_Pos_To_Edit.x = x_value;
    m_Pos_To_Edit.y = y_value;

    msg = StringFromValue( m_UserUnit, m_Pos_To_Edit.x );
    m_FramePosX->Clear();
    m_FramePosX->SetValue( msg );

    msg = StringFromValue( m_UserUnit, m_Pos_To_Edit.y );
    m_FramePosY->Clear();
    m_FramePosY->SetValue( msg );
}


/*******************/
/* EDA_SIZE_CTRL */
/*******************/
EDA_SIZE_CTRL::EDA_SIZE_CTRL( wxWindow* parent, const wxString& title,
                              const wxSize& size_to_edit, EDA_UNITS_T aUnit,
                              wxBoxSizer* aBoxSizer ) :
    EDA_POSITION_CTRL( parent, title, wxPoint( size_to_edit.x, size_to_edit.y ),
                       aUnit, aBoxSizer )
{
}


wxSize EDA_SIZE_CTRL::GetValue()
{
    wxPoint pos = EDA_POSITION_CTRL::GetValue();
    wxSize  size;

    size.x = pos.x;
    size.y = pos.y;
    return size;
}


/**************************************************************/
/* Class to display and edit a dimension INCHES, MM, or other */
/**************************************************************/
EDA_VALUE_CTRL::EDA_VALUE_CTRL( wxWindow* parent, const wxString& title,
                                int value, EDA_UNITS_T user_unit, wxBoxSizer* BoxSizer )
{
    wxString label = title;

    m_UserUnit = user_unit;
    m_Value = value;
    label  += ReturnUnitSymbol( m_UserUnit );

    m_Text = new wxStaticText( parent, -1, label );

    BoxSizer->Add( m_Text, 0, wxGROW | wxLEFT | wxRIGHT | wxTOP, 5 );

    wxString stringvalue = StringFromValue( m_UserUnit, m_Value );
    m_ValueCtrl = new wxTextCtrl( parent, -1, stringvalue );

    BoxSizer->Add( m_ValueCtrl,
                   0,
                   wxGROW | wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT | wxBOTTOM,
                   5 );
}


EDA_VALUE_CTRL::~EDA_VALUE_CTRL()
{
    delete m_ValueCtrl;
    delete m_Text;
}


int EDA_VALUE_CTRL::GetValue()
{
    int      coord;
    wxString txtvalue = m_ValueCtrl->GetValue();

    coord = ValueFromString( m_UserUnit, txtvalue );
    return coord;
}


void EDA_VALUE_CTRL::SetValue( int new_value )
{
    wxString buffer;

    m_Value = new_value;

    buffer = StringFromValue( m_UserUnit, m_Value );
    m_ValueCtrl->SetValue( buffer );
}


void EDA_VALUE_CTRL::Enable( bool enbl )
{
    m_ValueCtrl->Enable( enbl );
    m_Text->Enable( enbl );
}
