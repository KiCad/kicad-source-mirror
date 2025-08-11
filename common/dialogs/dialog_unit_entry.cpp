/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Roberto Fernandez Bautista <roberto.fer.bau@gmail.com>
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

#include <eda_draw_frame.h>
#include <dialogs/dialog_unit_entry.h>
#include <string_utils.h>


WX_UNIT_ENTRY_DIALOG::WX_UNIT_ENTRY_DIALOG( EDA_DRAW_FRAME* aParent, const wxString& aCaption,
                                            const wxString& aLabel, long long int aDefaultValue ) :
        WX_UNIT_ENTRY_DIALOG_BASE( ( wxWindow* ) aParent, wxID_ANY, aCaption ),
        m_unit_binder( aParent, m_label, m_textCtrl, m_unit_label, true )
{
    m_label->SetLabel( aLabel );
    m_unit_binder.SetValue( aDefaultValue );

    // DIALOG_SHIM needs a title- and label-specific hash_key so we don't save/restore state between
    // usage cases.
    m_hash_key = TO_UTF8( aCaption + aLabel );

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
                                        const VECTOR2I& aDefaultValue, bool aShowResetButt ) :
        WX_PT_ENTRY_DIALOG_BASE( ( wxWindow* ) aParent, wxID_ANY, aCaption ),
        m_unit_binder_x( aParent, m_labelX, m_textCtrlX, m_unitsX, true ),
        m_unit_binder_y( aParent, m_labelY, m_textCtrlY, m_unitsY, true )
{
    m_ButtonReset->Show( aShowResetButt );

    m_labelX->SetLabel( aLabelX );
    m_labelY->SetLabel( aLabelY );

    m_unit_binder_x.SetCoordType( ORIGIN_TRANSFORMS::ABS_X_COORD );
    m_unit_binder_y.SetCoordType( ORIGIN_TRANSFORMS::ABS_Y_COORD );

    m_unit_binder_x.SetValue( aDefaultValue.x );
    m_unit_binder_y.SetValue( aDefaultValue.y );

    // DIALOG_SHIM needs a title- and label-specific hash_key so we don't save/restore state between
    // usage cases.
    m_hash_key = TO_UTF8( aCaption + aLabelX + aLabelY );

    SetInitialFocus( m_textCtrlX );
    SetupStandardButtons();

	finishDialogSettings();
}


VECTOR2I WX_PT_ENTRY_DIALOG::GetValue()
{
    return VECTOR2I( m_unit_binder_x.GetIntValue(), m_unit_binder_y.GetIntValue() );
}

void WX_PT_ENTRY_DIALOG::ResetValues( wxCommandEvent& event )
{
    m_unit_binder_x.SetValue( 0 );
    m_unit_binder_y.SetValue( 0 );
}
