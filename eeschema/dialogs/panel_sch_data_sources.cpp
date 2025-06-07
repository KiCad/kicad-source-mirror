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

#include <core/json_serializers.h>
#include "panel_sch_data_sources.h"

#include <algorithm>
#include <vector>

#include <dialogs/dialog_pcm.h>
#include <eda_base_frame.h>
#include <settings/settings_manager.h>
#include <settings/kicad_settings.h>
#include <widgets/ui_common.h>
#include <wx/button.h>
#include <wx/intl.h>
#include <wx/listbox.h>
#include <wx/sizer.h>
#include <wx/stattext.h>


PANEL_SCH_DATA_SOURCES::PANEL_SCH_DATA_SOURCES( wxWindow* aParent, EDA_BASE_FRAME* aFrame ) :
        RESETTABLE_PANEL( aParent ),
        m_frame( aFrame ),
        m_pcm( std::make_shared<PLUGIN_CONTENT_MANAGER>( []( int ) {} ) ),
        m_description( nullptr ),
        m_status( nullptr ),
        m_sourcesList( nullptr ),
        m_manageButton( nullptr )
{
    wxBoxSizer* topSizer = new wxBoxSizer( wxVERTICAL );

    m_description = new wxStaticText( this, wxID_ANY,
            _( "Install schematic data sources from the Plugin and Content Manager. Data sources extend KiCad by linking schematic items to external data providers." ) );
    m_description->Wrap( FromDIP( 480 ) );
    m_description->SetFont( KIUI::GetInfoFont( this ) );
    topSizer->Add( m_description, 0, wxBOTTOM | wxEXPAND, FromDIP( 12 ) );

    m_sourcesList = new wxListBox( this, wxID_ANY );
    m_sourcesList->SetMinSize( FromDIP( wxSize( -1, 160 ) ) );
    topSizer->Add( m_sourcesList, 1, wxBOTTOM | wxEXPAND, FromDIP( 12 ) );

    m_status = new wxStaticText( this, wxID_ANY, wxEmptyString );
    m_status->SetFont( KIUI::GetSmallInfoFont( this ).Italic() );
    topSizer->Add( m_status, 0, wxBOTTOM | wxEXPAND, FromDIP( 12 ) );

    m_manageButton = new wxButton( this, wxID_ANY, _( "Manage Data Sources..." ) );
    topSizer->Add( m_manageButton, 0, wxALIGN_RIGHT );

    SetSizer( topSizer );

    m_manageButton->Bind( wxEVT_BUTTON, &PANEL_SCH_DATA_SOURCES::OnManageDataSources, this );
}


bool PANEL_SCH_DATA_SOURCES::TransferDataToWindow()
{
    if( KICAD_SETTINGS* cfg = GetAppSettings<KICAD_SETTINGS>( "kicad" ) )
        m_pcm->SetRepositoryList( cfg->m_PcmRepositories );

    populateInstalledSources();

    return true;
}


bool PANEL_SCH_DATA_SOURCES::TransferDataFromWindow()
{
    return true;
}


void PANEL_SCH_DATA_SOURCES::ResetPanel()
{
    populateInstalledSources();
}


void PANEL_SCH_DATA_SOURCES::populateInstalledSources()
{
    m_sourcesList->Clear();

    std::vector<wxString> entries;

    for( const PCM_INSTALLATION_ENTRY& entry : m_pcm->GetInstalledPackages() )
    {
        PCM_PACKAGE_TYPE type = entry.package.category && entry.package.category.value() == PC_FAB
                                ? PT_FAB
                                : entry.package.type;

        if( type != PT_DATASOURCE )
            continue;

        wxString label = entry.package.name;

        if( !entry.current_version.IsEmpty() )
            label << wxS( " (" ) << entry.current_version << wxS( ")" );

        if( !entry.repository_name.IsEmpty() )
            label << wxS( " — " ) << entry.repository_name;

        entries.push_back( label );
    }

    if( entries.empty() )
    {
        m_status->SetLabel( _( "No data sources are currently installed." ) );
        return;
    }

    std::sort( entries.begin(), entries.end(),
            []( const wxString& a, const wxString& b )
            {
                return a.CmpNoCase( b ) < 0;
            } );

    for( const wxString& label : entries )
        m_sourcesList->Append( label );

    m_status->SetLabel( _( "Installed data sources are listed above." ) );
}


void PANEL_SCH_DATA_SOURCES::OnManageDataSources( wxCommandEvent& aEvent )
{
    EDA_BASE_FRAME* parentFrame = m_frame ? m_frame
                                          : dynamic_cast<EDA_BASE_FRAME*>( wxGetTopLevelParent( this ) );

    DIALOG_PCM dialog( parentFrame, m_pcm );
    dialog.SetActivePackageType( PT_DATASOURCE );
    dialog.ShowModal();

    populateInstalledSources();
}
