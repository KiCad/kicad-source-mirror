/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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

#include <widgets/widget_save_restore.h>

#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/notebook.h>
#include <wx/radiobox.h>
#include <wx/radiobut.h>
#include <wx/textctrl.h>

#include <widgets/unit_binder.h>


void WIDGET_SAVE_RESTORE::Add( wxRadioBox& ctrl, long& dest )
{
    m_ctrls.emplace_back( WIDGET_CTRL_TYPE_T::RADIOBOX, ctrl, dest );
}


void WIDGET_SAVE_RESTORE::Add( wxRadioButton& ctrl, bool& dest )
{
    m_ctrls.emplace_back( WIDGET_CTRL_TYPE_T::RADIOBUTTON, ctrl, dest );
}


void WIDGET_SAVE_RESTORE::Add( wxCheckBox& ctrl, bool& dest )
{
    m_ctrls.emplace_back( WIDGET_CTRL_TYPE_T::CHECKBOX, ctrl, dest );
}


void WIDGET_SAVE_RESTORE::Add( wxTextCtrl& ctrl, wxString& dest )
{
    m_ctrls.emplace_back( WIDGET_CTRL_TYPE_T::TEXT, ctrl, dest );
}


void WIDGET_SAVE_RESTORE::Add( wxTextCtrl& ctrl, long& dest )
{
    m_ctrls.emplace_back( WIDGET_CTRL_TYPE_T::TEXT_INTEGER, ctrl, dest );
}


void WIDGET_SAVE_RESTORE::Add( wxTextCtrl& ctrl, double& dest )
{
    m_ctrls.emplace_back( WIDGET_CTRL_TYPE_T::TEXT_DOUBLE, ctrl, dest );
}


void WIDGET_SAVE_RESTORE::Add( UNIT_BINDER& ctrl, long& dest )
{
    m_ctrls.emplace_back( WIDGET_CTRL_TYPE_T::UNIT_BINDER, ctrl, dest );
}


void WIDGET_SAVE_RESTORE::Add( UNIT_BINDER& ctrl, EDA_ANGLE& dest )
{
    m_ctrls.emplace_back( WIDGET_CTRL_TYPE_T::UNIT_BINDER_ANGLE, ctrl, dest );
}

void WIDGET_SAVE_RESTORE::Add( wxChoice& ctrl, long& dest )
{
    m_ctrls.emplace_back( WIDGET_CTRL_TYPE_T::CHOICE, ctrl, dest );
}


void WIDGET_SAVE_RESTORE::Add( wxNotebook& ctrl, long& dest )
{
    m_ctrls.emplace_back( WIDGET_CTRL_TYPE_T::TAB, ctrl, dest );
}


void WIDGET_SAVE_RESTORE::ReadConfigFromControls()
{
    for( WIDGET_SAVE_RESTORE::WIDGET_CTRL_T& ctrl : m_ctrls )
    {
        switch( ctrl.m_type )
        {
        case WIDGET_CTRL_TYPE_T::CHECKBOX:
            *ctrl.m_dest.m_bool = ctrl.m_control.m_checkbox->GetValue();
            break;

        case WIDGET_CTRL_TYPE_T::RADIOBUTTON:
            *ctrl.m_dest.m_bool = ctrl.m_control.m_radiobutton->GetValue();
            break;

        case WIDGET_CTRL_TYPE_T::TEXT:
            *ctrl.m_dest.m_str = ctrl.m_control.m_textctrl->GetValue();
            break;

        case WIDGET_CTRL_TYPE_T::TEXT_INTEGER:
            ctrl.m_control.m_textctrl->GetValue().ToLong( ctrl.m_dest.m_long );
            break;

        case WIDGET_CTRL_TYPE_T::TEXT_DOUBLE:
            ctrl.m_control.m_textctrl->GetValue().ToDouble( ctrl.m_dest.m_double );
            break;

        case WIDGET_CTRL_TYPE_T::UNIT_BINDER:
            *ctrl.m_dest.m_long = ctrl.m_control.m_unit_binder->GetValue();
            break;

        case WIDGET_CTRL_TYPE_T::UNIT_BINDER_ANGLE:
            *ctrl.m_dest.m_angle = ctrl.m_control.m_unit_binder->GetAngleValue();
            break;

        case WIDGET_CTRL_TYPE_T::CHOICE:
            *ctrl.m_dest.m_long = ctrl.m_control.m_choice->GetSelection();
            break;

        case WIDGET_CTRL_TYPE_T::RADIOBOX:
            *ctrl.m_dest.m_long = ctrl.m_control.m_radiobox->GetSelection();
            break;

        case WIDGET_CTRL_TYPE_T::TAB:
            *ctrl.m_dest.m_long = ctrl.m_control.m_notebook->GetSelection();
            break;
        }
    }

    m_valid = true;
}


void WIDGET_SAVE_RESTORE::RestoreConfigToControls()
{
    if( !m_valid )
        return;

    for( WIDGET_SAVE_RESTORE::WIDGET_CTRL_T& ctrl : m_ctrls )
    {
        switch( ctrl.m_type )
        {
        case WIDGET_CTRL_TYPE_T::CHECKBOX:
            ctrl.m_control.m_checkbox->SetValue( *ctrl.m_dest.m_bool );
            break;

        case WIDGET_CTRL_TYPE_T::RADIOBUTTON:
            ctrl.m_control.m_radiobutton->SetValue( *ctrl.m_dest.m_bool );
            break;

        case WIDGET_CTRL_TYPE_T::TEXT:
            ctrl.m_control.m_textctrl->SetValue( *ctrl.m_dest.m_str );
            break;

        case WIDGET_CTRL_TYPE_T::TEXT_INTEGER:
            ctrl.m_control.m_textctrl->SetValue( wxString::Format( "%ld", *ctrl.m_dest.m_long ) );
            break;

        case WIDGET_CTRL_TYPE_T::TEXT_DOUBLE:
            ctrl.m_control.m_textctrl->SetValue( wxString::Format( "%f", *ctrl.m_dest.m_double ) );
            break;

        case WIDGET_CTRL_TYPE_T::UNIT_BINDER:
            ctrl.m_control.m_unit_binder->SetValue( *ctrl.m_dest.m_long );
            break;

        case WIDGET_CTRL_TYPE_T::UNIT_BINDER_ANGLE:
            ctrl.m_control.m_unit_binder->SetAngleValue( *ctrl.m_dest.m_angle );
            break;

        case WIDGET_CTRL_TYPE_T::CHOICE:
            ctrl.m_control.m_choice->SetSelection( *ctrl.m_dest.m_long );
            break;

        case WIDGET_CTRL_TYPE_T::RADIOBOX:
            ctrl.m_control.m_radiobox->SetSelection( *ctrl.m_dest.m_long );
            break;

        case WIDGET_CTRL_TYPE_T::TAB:
            ctrl.m_control.m_notebook->SetSelection( *ctrl.m_dest.m_long );
            break;
        }
    }
}
