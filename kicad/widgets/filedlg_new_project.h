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

#ifndef FILEDLG_NEW_PROJECT_H_
#define FILEDLG_NEW_PROJECT_H_

#include <wx/filedlgcustomize.h>

class FILEDLG_NEW_PROJECT : public wxFileDialogCustomizeHook
{
public:
    FILEDLG_NEW_PROJECT(){};

    virtual void AddCustomControls( wxFileDialogCustomize& customizer ) override
    {
#ifdef __WXMAC__
        customizer.AddStaticText( wxT( "\n\n" ) );  // Increase height of static box
#endif

        m_cb = customizer.AddCheckBox( _( "Create a new folder for the project" ) );
        m_cb->SetValue( true );
    }

    virtual void TransferDataFromCustomControls() override
    {
        m_createNewDir = m_cb->GetValue();
    }

    ///< Gets the selected state of the create new directory checkbox
    bool GetCreateNewDir() const { return m_createNewDir; }

private:
    bool m_createNewDir = true;

    wxFileDialogCheckBox* m_cb = nullptr;

    wxDECLARE_NO_COPY_CLASS( FILEDLG_NEW_PROJECT );
};

#endif
