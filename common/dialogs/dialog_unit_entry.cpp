/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Roberto Fernandez Bautista <roberto.fer.bau@gmail.com>
 * Copyright (C) 2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <eda_draw_frame.h>
#include <dialogs/dialog_unit_entry.h>


WX_UNIT_ENTRY_DIALOG::WX_UNIT_ENTRY_DIALOG( EDA_DRAW_FRAME* aParent, const wxString& aLabel,
                                            const wxString& aCaption,
                                            const long long& aDefaultValue ) :
        WX_UNIT_ENTRY_DIALOG_BASE( ( wxWindow* ) aParent, wxID_ANY, aCaption ),
        m_unit_binder( aParent, m_label, m_textCtrl, m_unit_label, true )
{
    m_label->SetLabel( aLabel );
    m_unit_binder.SetValue( aDefaultValue );

    SetupStandardButtons();
}


long long WX_UNIT_ENTRY_DIALOG::GetValue()
{
    return m_unit_binder.GetValue();
}
