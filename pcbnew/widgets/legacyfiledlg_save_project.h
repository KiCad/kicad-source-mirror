/*
* This program source code file is part of KiCad, a free EDA CAD application.
*
* Copyright (C) 2022 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef LEGACYFILEDLG_SAVE_PROJ_H_
#define LEGACYFILEDLG_SAVE_PROJ_H_

#include <wx/checkbox.h>
#include <wx/panel.h>
#include <wx/sizer.h>

///< Helper widget to select whether a new project should be created for a file when saving
class LEGACYFILEDLG_SAVE_PROJECT : public wxPanel
{
public:
    LEGACYFILEDLG_SAVE_PROJECT( wxWindow* aParent ) : wxPanel( aParent )
    {
        m_cbCreateProject =
                new wxCheckBox( this, wxID_ANY, _( "Create a new project for this board" ) );
        m_cbCreateProject->SetValue( true );
        m_cbCreateProject->SetToolTip( _( "Creating a project will enable features such as "
                                          "design rules, net classes, and layer presets" ) );

        wxBoxSizer* sizer = new wxBoxSizer( wxHORIZONTAL );
        sizer->Add( m_cbCreateProject, 0, wxALL, 8 );

        SetSizerAndFit( sizer );
    }

    bool GetValue() const { return m_cbCreateProject->GetValue(); }

    static wxWindow* Create( wxWindow* aParent )
    {
        return new LEGACYFILEDLG_SAVE_PROJECT( aParent );
    }

protected:
    wxCheckBox* m_cbCreateProject;
};

#endif