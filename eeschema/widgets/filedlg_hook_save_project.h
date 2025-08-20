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

#ifndef FILEDLG_HOOK_SAVE_PROJECT_H
#define FILEDLG_HOOK_SAVE_PROJECT_H

#include <wx/filedlgcustomize.h>

class FILEDLG_HOOK_SAVE_PROJECT : public wxFileDialogCustomizeHook
{
public:
    FILEDLG_HOOK_SAVE_PROJECT(){};

    virtual void AddCustomControls( wxFileDialogCustomize& customizer ) override
    {
#ifdef __WXMAC__
        customizer.AddStaticText( wxT( "\n\n" ) );
#endif

        m_cb = customizer.AddCheckBox( _( "Create a new project for this schematic" ) );
        m_cb->SetValue( true );

        wxString choices[] = {
            _( "Do not copy subsheets" ),
            _( "Copy internal subsheets only" ),
            _( "Copy all subsheets" )
        };

        m_choice = customizer.AddChoice( 3, choices );
        m_choice->SetSelection( 1 ); // Default to copying internal subsheets only
        m_controlsAttached = true;
    }

    virtual void TransferDataFromCustomControls() override
    {
        m_createNewProject = m_cb->GetValue();
        m_copySubsheets = m_choice->GetSelection() > 0;
        m_includeExternal = m_choice->GetSelection() > 1;
    }

    ///< Gets the selected state of the create new project option
    bool GetCreateNewProject() const { return m_createNewProject; }

    ///< Gets the selected state of the copy subsheets option
    bool GetCopySubsheets() const { return m_copySubsheets; }

    ///< Gets the selected state of the include external sheets option
    bool GetIncludeExternSheets() const { return m_includeExternal; }

    ///< Gets if this hook has attached controls to a dialog box
    bool IsAttachedToDialog() const { return m_controlsAttached; }

private:
    bool m_createNewProject = true;
    bool m_copySubsheets = true;
    bool m_includeExternal = false;
    bool m_controlsAttached = false;

    wxFileDialogCheckBox* m_cb = nullptr;
    wxFileDialogChoice* m_choice = nullptr;

    wxDECLARE_NO_COPY_CLASS( FILEDLG_HOOK_SAVE_PROJECT );
};

#endif
