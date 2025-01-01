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

#ifndef LEGACYFILEDLG_NETLIST_OPTIONS_H_
#define LEGACYFILEDLG_NETLIST_OPTIONS_H_

#include <wx/checkbox.h>
#include <wx/panel.h>
#include <wx/sizer.h>

/**
 * Helper widget to add controls to a wxFileDialog to set netlist configuration options.
 */
class LEGACYFILEDLG_NETLIST_OPTIONS : public wxPanel
{
public:
    LEGACYFILEDLG_NETLIST_OPTIONS( wxWindow* aParent ) : wxPanel( aParent )
    {
        m_cbOmitExtras = new wxCheckBox( this, wxID_ANY, _( "Omit extra information" ) );
        m_cbOmitNets = new wxCheckBox( this, wxID_ANY, _( "Omit nets" ) );
        m_cbOmitFpUuids =
                new wxCheckBox( this, wxID_ANY, _( "Do not prefix path with footprint UUID." ) );

        wxBoxSizer* sizer = new wxBoxSizer( wxHORIZONTAL );
        sizer->Add( m_cbOmitExtras, 0, wxALL, 5 );
        sizer->Add( m_cbOmitNets, 0, wxALL, 5 );
        sizer->Add( m_cbOmitFpUuids, 0, wxALL, 5 );

        SetSizerAndFit( sizer );
    }

    int GetNetlistOptions() const
    {
        int options = 0;

        if( m_cbOmitExtras->GetValue() )
            options |= CTL_OMIT_EXTRA;

        if( m_cbOmitNets->GetValue() )
            options |= CTL_OMIT_NETS;

        if( m_cbOmitFpUuids->GetValue() )
            options |= CTL_OMIT_FP_UUID;

        return options;
    }

    static wxWindow* Create( wxWindow* aParent )
    {
        return new LEGACYFILEDLG_NETLIST_OPTIONS( aParent );
    }

protected:
    wxCheckBox* m_cbOmitExtras;
    wxCheckBox* m_cbOmitNets;
    wxCheckBox* m_cbOmitFpUuids;
};

#endif