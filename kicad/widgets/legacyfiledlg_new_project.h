/*
* This program source code file is part of KiCad, a free EDA CAD application.
*
* Copyright (C) 2022-2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef LEGACY_DIR_CHECKBOX_H_
#define LEGACY_DIR_CHECKBOX_H_

#include <wx/checkbox.h>
#include <wx/panel.h>
#include <wx/sizer.h>

///< Helper widget to select whether a new directory should be created for a project.
class LEGACYFILEDLG_NEW_PROJECT : public wxPanel
{
public:
    LEGACYFILEDLG_NEW_PROJECT( wxWindow* aParent ) : wxPanel( aParent )
    {
        m_cbCreateDir =
                new wxCheckBox( this, wxID_ANY, _( "Create a new folder for the project" ) );
        m_cbCreateDir->SetValue( true );

        wxBoxSizer* sizer = new wxBoxSizer( wxHORIZONTAL );
        sizer->Add( m_cbCreateDir, 0, wxALL, 8 );

        SetSizerAndFit( sizer );
    }

    bool CreateNewDir() const { return m_cbCreateDir->GetValue(); }

    static wxWindow* Create( wxWindow* aParent )
    {
        return new LEGACYFILEDLG_NEW_PROJECT( aParent );
    }

protected:
    wxCheckBox* m_cbCreateDir;
};

#endif
