/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010 Wayne Stambaugh <stambaughw@verizon.net>
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

#include <dialog_sch_edit_sheet_pin.h>


static wxString sheetPinTypes[] =
{
    _( "Input" ),
    _( "Output" ),
    _( "Bidirectional" ),
    _( "Tri-state" ),
    _( "Passive" )
};


#define SHEET_PIN_TYPE_CNT   ( sizeof( sheetPinTypes ) / sizeof( wxString ) )


DIALOG_SCH_EDIT_SHEET_PIN::DIALOG_SCH_EDIT_SHEET_PIN( wxWindow* parent ) :
    DIALOG_SCH_EDIT_SHEET_PIN_BASE( parent )
{
    for( size_t i = 0;  i < SHEET_PIN_TYPE_CNT;  i++ )
        m_choiceConnectionType->Append( sheetPinTypes[ i ] );

    m_choiceConnectionType->SetSelection( 0 );
    m_textName->SetFocus();
    m_sdbSizerOK->SetDefault();

    // Set invalid label characters list:
    wxTextValidator* validator = static_cast<wxTextValidator*>( m_textName->GetValidator() );
    validator->SetCharExcludes( " /" );

    // Now all widgets have the size fixed, call FinishDialogSettings
    FinishDialogSettings();

    // On some windows manager (Unity, XFCE), this dialog is
    // not always raised, depending on this dialog is run.
    // Force it to be raised
    Raise();
}



void DIALOG_SCH_EDIT_SHEET_PIN::onOKButton( wxCommandEvent& event )
{
    // Disable wxWidgets message if a pin name has not allowed chars
    // (It happens only when editing a old sheet pin name that can contains not allowed chars)
    wxTextValidator* validator = static_cast<wxTextValidator*>( m_textName->GetValidator() );
    validator->SetCharExcludes( "" );

    event.Skip();
}
