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

#include <fctsys.h>
#include <common.h>

#include <dialog_text_entry.h>


WX_TEXT_ENTRY_DIALOG::WX_TEXT_ENTRY_DIALOG( wxWindow* aParent,
                                            const wxString& aFieldLabel,
                                            const wxString& aCaption,
                                            const wxString& aDefaultValue ) :
    WX_TEXT_ENTRY_DIALOG_BASE( aParent, wxID_ANY, aCaption, wxDefaultPosition, wxDefaultSize )
{
    m_label->SetLabel( aFieldLabel );
    m_textCtrl->SetValue( aDefaultValue );
    m_sdbSizer1OK->SetDefault();
}


void WX_TEXT_ENTRY_DIALOG::SetTextValidator( wxTextValidatorStyle style )
{
    SetTextValidator( wxTextValidator(style) );
}


void WX_TEXT_ENTRY_DIALOG::SetTextValidator( const wxTextValidator& validator )
{
    m_textCtrl->SetValidator( validator );
}


wxString WX_TEXT_ENTRY_DIALOG::GetValue()
{
    return m_textCtrl->GetValue();
}

