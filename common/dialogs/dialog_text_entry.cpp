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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <dialogs/dialog_text_entry.h>
#include <string_utils.h>


WX_TEXT_ENTRY_DIALOG::WX_TEXT_ENTRY_DIALOG( wxWindow* aParent, const wxString& aFieldLabel,
                                            const wxString& aCaption, const wxString& aDefaultValue,
                                            bool aExtraWidth ) :
        WX_TEXT_ENTRY_DIALOG_BASE( aParent, wxID_ANY, aCaption, wxDefaultPosition, wxDefaultSize )
{
    if( aFieldLabel.IsEmpty() )
        m_label->Hide();
    else
        m_label->SetLabel( aFieldLabel );

    m_textCtrl->SetValue( aDefaultValue );
    m_textCtrl->SetMinSize( FromDIP( aExtraWidth ? wxSize( 700, -1 ) : wxSize( 300, -1 ) ) );

    // DIALOG_SHIM needs a title- and label-specific hash_key so we don't save/restore state between
    // usage cases.
    m_hash_key = TO_UTF8( aCaption + aFieldLabel );

    // The text value is always supplied by the caller through aDefaultValue and is specific to the
    // item being edited.  Persisting and restoring it would clobber that value with whatever was last
    // entered, so opt the control out of DIALOG_SHIM state save/restore.
    OptOut( m_textCtrl );

    SetupStandardButtons();

    SetInitialFocus( m_textCtrl );

    this->Layout();
    m_mainSizer->Fit( this );
}


void WX_TEXT_ENTRY_DIALOG::SetTextValidator( const wxTextValidator& validator )
{
    m_textCtrl->SetValidator( validator );
}


wxString WX_TEXT_ENTRY_DIALOG::GetValue() const
{
    return m_textCtrl->GetValue();
}


