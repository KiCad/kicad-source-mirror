/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Roberto Fernandez Bautista <roberto.fer.bau@gmail.com>
 * Copyright (C) 2020-2023 KiCad Developers, see AUTHORS.txt for contributors.
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


WX_UNIT_ENTRY_DIALOG::WX_UNIT_ENTRY_DIALOG( EDA_DRAW_FRAME* aParent, const wxString& aCaption,
                                            const wxString& aLabel, long long int aDefaultValue ) :
        WX_UNIT_ENTRY_DIALOG_BASE( ( wxWindow* ) aParent, wxID_ANY, aCaption ),
        m_unit_binder( aParent, m_label, m_textCtrl, m_unit_label, true )
{
    m_label->SetLabel( aLabel );
    m_unit_binder.SetValue( aDefaultValue );

    SetInitialFocus( m_textCtrl );
    SetupStandardButtons();

	Layout();
	bSizerMain->Fit( this );
}


int WX_UNIT_ENTRY_DIALOG::GetValue()
{
    return m_unit_binder.GetIntValue();
}


WX_PT_ENTRY_DIALOG::WX_PT_ENTRY_DIALOG( EDA_DRAW_FRAME* aParent, const wxString& aCaption,
                                        const wxString& aLabelX, const wxString& aLabelY,
                                        const VECTOR2I& aDefaultValue ) :
        WX_PT_ENTRY_DIALOG_BASE( ( wxWindow* ) aParent, wxID_ANY, aCaption ),
        m_unit_binder_x( aParent, m_labelX, m_textCtrlX, m_unitsX, true ),
        m_unit_binder_y( aParent, m_labelY, m_textCtrlY, m_unitsY, true )
{
    m_labelX->SetLabel( aLabelX );
    m_labelY->SetLabel( aLabelY );
    m_unit_binder_x.SetValue( aDefaultValue.x );
    m_unit_binder_y.SetValue( aDefaultValue.y );

    SetInitialFocus( m_textCtrlX );
    SetupStandardButtons();

	Layout();
	bSizerMain->Fit( this );
}


VECTOR2I WX_PT_ENTRY_DIALOG::GetValue()
{
    return VECTOR2I( m_unit_binder_x.GetIntValue(), m_unit_binder_y.GetIntValue() );
}
