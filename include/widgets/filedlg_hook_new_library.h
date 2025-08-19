/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <wx/filedlgcustomize.h>


class FILEDLG_HOOK_NEW_LIBRARY : public wxFileDialogCustomizeHook
{
public:
    FILEDLG_HOOK_NEW_LIBRARY( bool aDefaultUseGlobalTable ) :
            m_useGlobalTable( aDefaultUseGlobalTable )
    {};

    virtual void AddCustomControls( wxFileDialogCustomize& customizer ) override
    {
        wxString padding;
#ifdef __WXMAC__
        padding = wxT( "                    " );
        customizer.AddStaticText( wxT( "\n\n" ) );
#endif

        // Radio buttons are only grouped if they are consecutive.  If we want padding, we need to add it to the
        // first radio button
        m_addGlobalTableEntry  = customizer.AddRadioButton( _( "Add new library to global library table" ) + padding );
        m_addProjectTableEntry = customizer.AddRadioButton( _( "Add new library to project library table" ) );

        if( m_useGlobalTable )
            m_addGlobalTableEntry->SetValue( true );
        else
            m_addProjectTableEntry->SetValue( true );
    }

    virtual void TransferDataFromCustomControls() override
    {
        m_useGlobalTable = m_addGlobalTableEntry->GetValue();
    }

    bool GetUseGlobalTable() const { return m_useGlobalTable; }

private:
    bool m_useGlobalTable;

    wxFileDialogRadioButton* m_addGlobalTableEntry = nullptr;
    wxFileDialogRadioButton* m_addProjectTableEntry = nullptr;

    wxDECLARE_NO_COPY_CLASS( FILEDLG_HOOK_NEW_LIBRARY );
};

