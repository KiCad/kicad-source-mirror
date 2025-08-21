/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <dialogs/dialog_paste_special.h>


DIALOG_PASTE_SPECIAL::DIALOG_PASTE_SPECIAL( wxWindow* aParent, PASTE_MODE* aMode, const wxString& aDefaultRef ) :
    DIALOG_PASTE_SPECIAL_BASE( aParent ),
    m_mode( aMode )
{
    m_options->SetItemToolTip( static_cast<int>( PASTE_MODE::UNIQUE_ANNOTATIONS ),
                               _( "Finds the next available reference designator for any designators that already "
                                  "exist in the design." ) );

    m_options->SetItemToolTip( static_cast<int>( PASTE_MODE::KEEP_ANNOTATIONS ),
                                wxT( "" ) ); // Self explanatory

    m_options->SetItemToolTip( static_cast<int>( PASTE_MODE::REMOVE_ANNOTATIONS ),
                               wxString::Format( _( "Replaces reference designators with '%s'." ), aDefaultRef ) );

    m_options->SetFocus();

    // Now all widgets have the size fixed, call FinishDialogSettings
    finishDialogSettings();
}


bool DIALOG_PASTE_SPECIAL::TransferDataToWindow()
{
    switch( *m_mode )
    {
    case PASTE_MODE::UNIQUE_ANNOTATIONS: m_options->SetSelection( 0 ); break;
    case PASTE_MODE::KEEP_ANNOTATIONS:   m_options->SetSelection( 1 ); break;
    case PASTE_MODE::REMOVE_ANNOTATIONS: m_options->SetSelection( 2 ); break;
    }

    return true;
}


bool DIALOG_PASTE_SPECIAL::TransferDataFromWindow()
{
    switch( m_options->GetSelection() )
    {
    case 0: *m_mode = PASTE_MODE::UNIQUE_ANNOTATIONS; break;
    case 1: *m_mode = PASTE_MODE::KEEP_ANNOTATIONS;   break;
    case 2: *m_mode = PASTE_MODE::REMOVE_ANNOTATIONS; break;
    }

    return true;
}


void DIALOG_PASTE_SPECIAL::onRadioBoxEvent( wxCommandEvent& event )
{
    event.Skip();

    m_sdbSizerOK->SetFocus();
}
