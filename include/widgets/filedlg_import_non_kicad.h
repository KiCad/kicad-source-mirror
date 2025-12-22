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


class FILEDLG_IMPORT_NON_KICAD : public wxFileDialogCustomizeHook
{
public:
    FILEDLG_IMPORT_NON_KICAD( bool aDefaultShowIssues = true ) :
            m_showIssues( aDefaultShowIssues )
    {};

    virtual void AddCustomControls( wxFileDialogCustomize& customizer ) override
    {
#ifdef __WXMAC__
        customizer.AddStaticText( wxT( "\n\n" ) );  // Increase height of static box
#endif

        m_cb = customizer.AddCheckBox( _( "Show import issues" ) );
        m_cb->SetValue( m_showIssues );
    }

    virtual void TransferDataFromCustomControls() override
    {
        m_showIssues = m_cb->GetValue();
    }

    bool GetShowIssues() const { return m_showIssues; }

private:
    bool m_showIssues;

    wxFileDialogCheckBox* m_cb = nullptr;

    wxDECLARE_NO_COPY_CLASS( FILEDLG_IMPORT_NON_KICAD );
};
