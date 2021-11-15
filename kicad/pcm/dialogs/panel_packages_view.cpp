/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 Andrew Lutsenko, anlutsenko at gmail dot com
 * Copyright (C) 1992-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "panel_packages_view.h"
#include <grid_tricks.h>
#include <kicad_settings.h>
#include <pgm_base.h>
#include <settings/settings_manager.h>
#include <settings/common_settings.h>
#include <widgets/wx_splitter_window.h>
#include <widgets/wx_panel.h>
#include <string_utils.h>
#include <html_window.h>

#include <cmath>
#include <fstream>
#include <wx/filedlg.h>
#include <wx/font.h>
#include <wx/tokenzr.h>


#define GRID_CELL_MARGIN 4


std::unordered_map<PCM_PACKAGE_VERSION_STATUS, wxString> PANEL_PACKAGES_VIEW::STATUS_ENUM_TO_STR = {
    { PVS_INVALID, "invalid" },
    { PVS_STABLE, "stable" },
    { PVS_TESTING, "testing" },
    { PVS_DEVELOPMENT, "development" },
    { PVS_DEPRECATED, "deprecated" }
};


PANEL_PACKAGES_VIEW::PANEL_PACKAGES_VIEW( wxWindow*                               parent,
                                          std::shared_ptr<PLUGIN_CONTENT_MANAGER> aPcm ) :
        PANEL_PACKAGES_VIEW_BASE( parent ),
        m_pcm( aPcm )
{
    // Replace wxFormBuilder's sash initializer with one which will respect m_initialSashPos.
    m_splitter1->Disconnect( wxEVT_IDLE,
                             wxIdleEventHandler( PANEL_PACKAGES_VIEW_BASE::m_splitter1OnIdle ),
                             NULL, this );
    m_splitter1->Connect( wxEVT_IDLE, wxIdleEventHandler( PANEL_PACKAGES_VIEW::SetSashOnIdle ),
                          NULL, this );

    m_splitter1->SetPaneMinimums( 350, 450 );

#ifdef __WXGTK__
    // wxSearchCtrl vertical height is not calculated correctly on some GTK setups
    // See https://gitlab.com/kicad/code/kicad/-/issues/9019
    m_searchCtrl->SetMinSize( wxSize( -1, GetTextExtent( wxT( "qb" ) ).y + 10 ) );
#endif

    m_searchCtrl->Bind( wxEVT_TEXT, &PANEL_PACKAGES_VIEW::OnSearchTextChanged, this );
    m_searchCtrl->SetDescriptiveText( _( "Filter" ) );

    m_panelList->SetBorders( false, true, false, false );

    m_gridVersions->PushEventHandler( new GRID_TRICKS( m_gridVersions ) );

    for( int col = 0; col < m_gridVersions->GetNumberCols(); col++ )
    {
        const wxString& heading = m_gridVersions->GetColLabelValue( col );
        int             headingWidth = GetTextExtent( heading ).x + 2 * GRID_CELL_MARGIN;

        // Set the minimal width to the column label size.
        m_gridVersions->SetColMinimalWidth( col, headingWidth );
        m_gridVersions->SetColSize( col, m_gridVersions->GetVisibleWidth( col, true, true,
                                                                          false ) );
    }

    // Most likely should be changed to wxGridSelectNone once WxWidgets>=3.1.5 is mandatory.
    m_gridVersions->SetSelectionMode( WX_GRID::wxGridSelectRows );

    wxColor background = wxStaticText::GetClassDefaultAttributes().colBg;
    m_panelList->SetBackgroundColour( background );
    m_packageListWindow->SetBackgroundColour( background );
    m_infoScrollWindow->SetBackgroundColour( background );
    m_infoScrollWindow->EnableScrolling( false, true );

    ClearData();
}


PANEL_PACKAGES_VIEW::~PANEL_PACKAGES_VIEW()
{
    COMMON_SETTINGS* cfg = Pgm().GetCommonSettings();
    cfg->m_PackageManager.sash_pos = m_splitter1->GetSashPosition();

    m_gridVersions->PopEventHandler( true );
}


void PANEL_PACKAGES_VIEW::ClearData()
{
    unsetPackageDetails();

    m_currentSelected = nullptr;
    m_packagePanels.clear();
    m_packageInitialOrder.clear();
    m_packageListWindow->GetSizer()->Clear( true ); // Delete panels
    m_packageListWindow->GetSizer()->FitInside( m_packageListWindow );
    m_packageListWindow->Layout();
}


void PANEL_PACKAGES_VIEW::SetData( const std::vector<PACKAGE_VIEW_DATA>& aPackageData,
                                   ActionCallback                        aCallback )
{
    m_actionCallback = aCallback;

    ClearData();

    for( const PACKAGE_VIEW_DATA& data : aPackageData )
    {
        PANEL_PACKAGE* package_panel =
                new PANEL_PACKAGE( m_packageListWindow, m_actionCallback, data );

        package_panel->SetSelectCallback(
                [package_panel, this]()
                {
                    if( m_currentSelected && m_currentSelected != package_panel )
                        m_currentSelected->SetSelected( false );

                    package_panel->SetSelected( true );
                    m_currentSelected = package_panel;
                    setPackageDetails( package_panel->GetPackageData() );

                    Layout();
                } );

        m_packagePanels.insert( { data.package.identifier, package_panel } );
        m_packageInitialOrder.push_back( data.package.identifier );
    }

    updatePackageList();
}


void PANEL_PACKAGES_VIEW::setPackageDetails( const PACKAGE_VIEW_DATA& aPackageData )
{
    const PCM_PACKAGE& package = aPackageData.package;

    // Details
    wxString details;

    details << "<h5>" + package.name + "</h5>";

    auto format_desc =
            []( const wxString& text ) -> wxString
            {
                wxString result;
                bool     inURL = false;
                wxString url;

                for( unsigned i = 0; i < text.length(); ++i )
                {
                    wxUniChar c = text[i];

                    if( inURL )
                    {
                        if( c == ' ' )
                        {
                            result += wxString::Format( "<a href='%s'>%s</a>", url, url );
                            inURL = false;

                            result += c;
                        }
                        else
                        {
                            url += c;
                        }
                    }
                    else if( text.Mid( i, 5 ) == "http:" || text.Mid( i, 6 ) == "https:" )
                    {
                        url = c;
                        inURL = true;
                    }
                    else if( c == '\n' )
                    {
                        result += "</p><p>";
                    }
                    else
                    {
                        result += c;
                    }
                }

                return result;
            };

    wxString desc = package.description_full;
    details << "<p>" + format_desc( desc ) + "</p>";

    details << "<p><b>" + _( "Metadata" ) + "</b></p>";
    details << "<ul>";
    details << "<li>" + _( "Package identifier: " ) + package.identifier + "</li>";
    details << "<li>" + _( "License: " ) + package.license + "</li>";

    if( package.tags.size() > 0 )
    {
        wxString tags_str;

        for( const std::string& tag : package.tags )
        {
            if( !tags_str.IsEmpty() )
                tags_str += ", ";

            tags_str += tag;
        }

        details << "<li>" + _( "Tags: " ) + tags_str + "</li>";
    }

    auto format_entry =
            []( const std::pair<const std::string, wxString>& entry ) -> wxString
            {
                wxString name = entry.first;
                wxString url = EscapeHTML( entry.second );

                if( name == "email" )
                    return wxString::Format( "<a href='mailto:%s'>%s</a>", url, url );
                else if( url.StartsWith( "http:" ) || url.StartsWith( "https:" ) )
                    return wxString::Format( "<a href='%s'>%s</a>", url, url );
                else
                    return entry.second;
            };

    auto write_contact =
            [&]( const wxString& type, const PCM_CONTACT& contact )
            {
                details << "<li>" + type + ": " + contact.name + "<ul>";

                for( const std::pair<const std::string, wxString>& entry : contact.contact )
                    details << "<li>" + entry.first + ": " + format_entry( entry ) + "</li>";

                details << "</ul>";
            };

    write_contact( _( "Author" ), package.author );

    if( package.maintainer )
        write_contact( _( "Maintainer" ), package.maintainer.get() );

    if( package.resources.size() > 0 )
    {
        details << "<li>" + _( "Resources" ) + "<ul>";

        for( const std::pair<const std::string, wxString>& entry : package.resources )
            details << "<li>" + entry.first + wxS( ": " ) + format_entry( entry ) + "</li>";

        details << "</ul>";
    }

    details << "</ul>";

    m_infoText->SetPage( details );

    wxSizeEvent dummy;
    OnSizeInfoBox( dummy );

    // Versions table
    m_gridVersions->Freeze();

    if( m_gridVersions->GetNumberRows() != 0 )
        m_gridVersions->DeleteRows( 0, m_gridVersions->GetNumberRows() );

    int      row = 0;
    wxString current_version;

    if( aPackageData.state == PPS_INSTALLED )
        current_version = m_pcm->GetInstalledPackageVersion( package.identifier );

    wxFont bold_font = m_gridVersions->GetDefaultCellFont().Bold();

    for( const PACKAGE_VERSION& version : package.versions )
    {
        if( !version.compatible && !m_showAllVersions->IsChecked() )
            continue;

        m_gridVersions->InsertRows( row );

        m_gridVersions->SetCellValue( row, COL_VERSION, version.version );
        m_gridVersions->SetCellValue( row, COL_DOWNLOAD_SIZE,
                                      toHumanReadableSize( version.download_size ) );
        m_gridVersions->SetCellValue( row, COL_INSTALL_SIZE,
                                      toHumanReadableSize( version.install_size ) );
        m_gridVersions->SetCellValue( row, COL_COMPATIBILITY,
                                      version.compatible ? wxT( "\u2714" ) : wxEmptyString );
        m_gridVersions->SetCellValue( row, COL_STATUS, STATUS_ENUM_TO_STR.at( version.status ) );

        m_gridVersions->SetCellAlignment( row, COL_COMPATIBILITY, wxALIGN_CENTER, wxALIGN_CENTER );

        if( current_version == version.version )
        {
            for( int col = 0; col < m_gridVersions->GetNumberCols(); col++ )
                m_gridVersions->SetCellFont( row, col, bold_font );
        }

        row++;
    }

    for( int col = 0; col < m_gridVersions->GetNumberCols(); col++ )
    {
        // Set the width to see the full contents
        m_gridVersions->SetColSize( col, m_gridVersions->GetVisibleWidth( col, true, true,
                                                                          false ) );
    }

    if( m_gridVersions->GetNumberRows() >= 1 )
    {
        wxString version = m_currentSelected->GetPreferredVersion();

        if( !version.IsEmpty() )
        {
            for( int i = 0; i < m_gridVersions->GetNumberRows(); i++ )
            {
                if( m_gridVersions->GetCellValue( i, COL_VERSION ) == version )
                {
                    std::cout << "auto select row: " << i << std::endl;
                    m_gridVersions->SelectRow( i );
                    break;
                }
            }
        }
        else
        {
            // Fall back to first row.
            m_gridVersions->SelectRow( 0 );
        }
    }

    m_gridVersions->Thaw();

    updateDetailsButtons();

    m_infoText->Show( true );
    m_sizerVersions->Show( true );
    m_sizerVersions->Layout();

    wxSize size = m_infoScrollWindow->GetTargetWindow()->GetBestVirtualSize();
    m_infoScrollWindow->SetVirtualSize( size );
}


void PANEL_PACKAGES_VIEW::unsetPackageDetails()
{
    m_infoText->SetPage( wxEmptyString );
    m_infoText->Show( false );
    m_sizerVersions->Show( false );

    wxSize size = m_infoScrollWindow->GetTargetWindow()->GetBestVirtualSize();
    m_infoScrollWindow->SetVirtualSize( size );

    // Clean up grid just so we don't keep stale info around (it's already been hidden).
    m_gridVersions->Freeze();

    if( m_gridVersions->GetNumberRows() > 0 )
        m_gridVersions->DeleteRows( 0, m_gridVersions->GetNumberRows() );

    m_gridVersions->Thaw();
}


wxString PANEL_PACKAGES_VIEW::toHumanReadableSize( const boost::optional<uint64_t> size ) const
{
    if( !size )
        return "-";

    uint64_t b = size.get();

    if( b >= 1024 * 1024 )
        return wxString::Format( "%.1f Mb", b / 1024.0 / 1024.0 );

    if( b >= 1024 )
        return wxString::Format( "%lld Kb", b / 1024 );

    return wxString::Format( "%lld b", b );
}


bool PANEL_PACKAGES_VIEW::canDownload() const
{
    if( !m_currentSelected )
        return false;

    return m_gridVersions->GetNumberRows() == 1 || m_gridVersions->GetSelectedRows().size() == 1;
}


bool PANEL_PACKAGES_VIEW::canInstall() const
{
    if( !m_currentSelected )
        return false;

    const PACKAGE_VIEW_DATA& packageData = m_currentSelected->GetPackageData();

    if( packageData.state != PPS_AVAILABLE && packageData.state != PPS_UNAVAILABLE )
        return false;

    return m_gridVersions->GetNumberRows() == 1 || m_gridVersions->GetSelectedRows().size() == 1;
}


void PANEL_PACKAGES_VIEW::SetPackageState( const wxString&         aPackageId,
                                           const PCM_PACKAGE_STATE aState ) const
{
    auto it = m_packagePanels.find( aPackageId );

    if( it != m_packagePanels.end() )
    {
        it->second->SetState( aState );

        if( m_currentSelected && m_currentSelected == it->second )
        {
            wxMouseEvent dummy;
            m_currentSelected->OnClick( dummy );
        }
    }
}


void PANEL_PACKAGES_VIEW::OnVersionsCellClicked( wxGridEvent& event )
{
    m_gridVersions->ClearSelection();
    m_gridVersions->SelectRow( event.GetRow() );

    updateDetailsButtons();
}


void PANEL_PACKAGES_VIEW::OnDownloadVersionClicked( wxCommandEvent& event )
{
    if( !canDownload() )
    {
        wxBell();
        return;
    }

    if( m_gridVersions->GetNumberRows() == 1 )
        m_gridVersions->SelectRow( 0 );

    const wxArrayInt selectedRows = m_gridVersions->GetSelectedRows();

    wxString           version = m_gridVersions->GetCellValue( selectedRows[0], COL_VERSION );
    const PCM_PACKAGE& package = m_currentSelected->GetPackageData().package;

    auto ver_it = std::find_if( package.versions.begin(), package.versions.end(),
                                [&]( const PACKAGE_VERSION& ver )
                                {
                                    return ver.version == version;
                                } );

    wxASSERT_MSG( ver_it != package.versions.end(), "Could not find package version" );

    if( !ver_it->download_url )
    {
        wxMessageBox( _( "Package download url is not specified" ),
                      _( "Error downloading package" ), wxICON_INFORMATION | wxOK, this );
        return;
    }

    const wxString& url = ver_it->download_url.get();

    SETTINGS_MANAGER& mgr = Pgm().GetSettingsManager();
    KICAD_SETTINGS*   app_settings = mgr.GetAppSettings<KICAD_SETTINGS>();

    wxFileDialog dialog( this, _( "Save package" ), app_settings->m_PcmLastDownloadDir,
                         wxString::Format( "%s_v%s.zip", package.identifier, version ),
                         "ZIP files (*.zip)|*.zip", wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    if( dialog.ShowModal() == wxID_CANCEL )
        return;

    wxString path = dialog.GetPath();
    app_settings->m_PcmLastDownloadDir = wxPathOnly( path );

    std::ofstream output( path.ToUTF8(), std::ios_base::binary );

    bool success = m_pcm->DownloadToStream( url, &output, _( "Downloading package" ), 0 );

    output.close();

    if( success )
    {
        if( ver_it->download_sha256 )
        {
            std::ifstream stream( path.ToUTF8(), std::ios_base::binary );

            bool matches = m_pcm->VerifyHash( stream, ver_it->download_sha256.get() );

            stream.close();

            if( !matches
                && wxMessageBox(
                           _( "Integrity of the downloaded package could not be verified, hash "
                              "does not match. Are you sure you want to keep this file?" ),
                           _( "Keep downloaded file" ), wxICON_EXCLAMATION | wxYES_NO, this )
                           == wxNO )
            {
                wxRemoveFile( path );
            }
        }
    }
    else
    {
        if( wxFileExists( path ) )
            wxRemoveFile( path );
    }
}


void PANEL_PACKAGES_VIEW::OnInstallVersionClicked( wxCommandEvent& event )
{
    if( !canInstall() )
    {
        wxBell();
        return;
    }

    if( m_gridVersions->GetNumberRows() == 1 )
        m_gridVersions->SelectRow( 0 );

    const wxArrayInt selectedRows = m_gridVersions->GetSelectedRows();

    wxString           version = m_gridVersions->GetCellValue( selectedRows[0], COL_VERSION );
    const PCM_PACKAGE& package = m_currentSelected->GetPackageData().package;

    auto ver_it = std::find_if( package.versions.begin(), package.versions.end(),
                                [&]( const PACKAGE_VERSION& ver )
                                {
                                    return ver.version == version;
                                } );

    wxASSERT_MSG( ver_it != package.versions.end(), "Could not find package version" );

    if( !ver_it->compatible
        && wxMessageBox( _( "This package version is incompatible with your kicad version or "
                            "platform. Are you sure you want to install it anyway?" ),
                         _( "Install package" ), wxICON_EXCLAMATION | wxYES_NO, this )
                   == wxNO )
    {
        return;
    }

    m_actionCallback( m_currentSelected->GetPackageData(), PPA_INSTALL, version );
}


void PANEL_PACKAGES_VIEW::OnShowAllVersionsClicked( wxCommandEvent& event )
{
    if( m_currentSelected )
    {
        wxMouseEvent dummy;
        m_currentSelected->OnClick( dummy );
    }

    updateDetailsButtons();
}


void PANEL_PACKAGES_VIEW::OnSearchTextChanged( wxCommandEvent& event )
{
    unsetPackageDetails();

    if( m_currentSelected )
        m_currentSelected->SetSelected( false );

    m_currentSelected = nullptr;

    updatePackageList();
}


void PANEL_PACKAGES_VIEW::updatePackageList()
{
    // Sort by descending rank, ascending index
    std::vector<std::pair<int, int>> package_ranks;

    const wxString search_term = m_searchCtrl->GetValue().Trim();

    for( size_t index = 0; index < m_packageInitialOrder.size(); index++ )
    {
        int                rank = 1;
        const PCM_PACKAGE& pkg =
                m_packagePanels[m_packageInitialOrder[index]]->GetPackageData().package;

        if( search_term.size() > 2 )
            rank = m_pcm->GetPackageSearchRank( pkg, search_term );

        // Packages with no versions are delisted and should not be shown
        if( pkg.versions.size() == 0 )
            rank = 0;

        package_ranks.emplace_back( rank, index );
    }

    std::sort( package_ranks.begin(), package_ranks.end(),
               []( const std::pair<int, int>& a, const std::pair<int, int>& b )
               {
                   return a.first > b.first || ( a.first == b.first && a.second < b.second );
               } );

    // Rearrange panels, hide ones with 0 rank
    wxSizer* sizer = m_packageListWindow->GetSizer();
    sizer->Clear( false ); // Don't delete panels

    for( const std::pair<int, int>& pair : package_ranks )
    {
        PANEL_PACKAGE* panel = m_packagePanels[m_packageInitialOrder[pair.second]];

        if( pair.first > 0 )
        {
            sizer->Add( panel, 0, wxEXPAND );
            panel->Show();
        }
        else
        {
            panel->Hide();
        }
    }

    m_packageListWindow->FitInside();
    m_packageListWindow->SetScrollRate( 0, 15 );
    m_packageListWindow->SendSizeEvent( wxSEND_EVENT_POST );
}


void PANEL_PACKAGES_VIEW::updateDetailsButtons()
{
    m_buttonDownload->Enable( canDownload() );
    m_buttonInstall->Enable( canInstall() );
}


void PANEL_PACKAGES_VIEW::OnSizeInfoBox( wxSizeEvent& aEvent )
{
    wxSize infoSize = m_infoText->GetParent()->GetClientSize();
    infoSize.x -= 8;
    m_infoText->SetMinSize( infoSize );
    m_infoText->SetMaxSize( infoSize );
    m_infoText->SetSize( infoSize );
    m_infoText->Layout();

    infoSize.y = m_infoText->GetInternalRepresentation()->GetHeight() + 12;
    m_infoText->SetMinSize( infoSize );
    m_infoText->SetMaxSize( infoSize );
    m_infoText->SetSize( infoSize );
    m_infoText->Layout();
}


void PANEL_PACKAGES_VIEW::OnURLClicked( wxHtmlLinkEvent& aEvent )
{
    const wxHtmlLinkInfo& info = aEvent.GetLinkInfo();
    ::wxLaunchDefaultBrowser( info.GetHref() );
}


void PANEL_PACKAGES_VIEW::OnInfoMouseWheel( wxMouseEvent& event )
{
    // Transfer scrolling from the info window to its parent scroll window
    m_infoScrollWindow->HandleOnMouseWheel( event );
}


void PANEL_PACKAGES_VIEW::SetSashOnIdle( wxIdleEvent& aEvent )
{
    COMMON_SETTINGS* cfg = Pgm().GetCommonSettings();
    m_splitter1->SetSashPosition( cfg->m_PackageManager.sash_pos );

    m_packageListWindow->FitInside();

	m_splitter1->Disconnect( wxEVT_IDLE, wxIdleEventHandler( PANEL_PACKAGES_VIEW::SetSashOnIdle ),
                             NULL, this );
}
