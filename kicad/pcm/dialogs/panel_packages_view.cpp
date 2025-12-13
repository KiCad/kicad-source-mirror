/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 Andrew Lutsenko, anlutsenko at gmail dot com
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

#include "panel_packages_view.h"
#include <grid_tricks.h>
#include <widgets/html_window.h>
#include <pgm_base.h>
#include <kiplatform/ui.h>
#include <settings/common_settings.h>
#include <settings/kicad_settings.h>
#include <settings/settings_manager.h>
#include <string_utils.h>
#include <widgets/wx_panel.h>
#include <widgets/wx_splitter_window.h>

#include <cmath>
#include <fstream>
#include <wx/filedlg.h>
#include <wx/font.h>
#include <wx/tokenzr.h>


#define GRID_CELL_MARGIN 4


std::unordered_map<PCM_PACKAGE_VERSION_STATUS, wxString> PANEL_PACKAGES_VIEW::STATUS_ENUM_TO_STR = {
    { PVS_INVALID, wxS( "invalid" ) },
    { PVS_STABLE, wxS( "stable" ) },
    { PVS_TESTING, wxS( "testing" ) },
    { PVS_DEVELOPMENT, wxS( "development" ) },
    { PVS_DEPRECATED, wxS( "deprecated" ) }
};


PANEL_PACKAGES_VIEW::PANEL_PACKAGES_VIEW( wxWindow*                               parent,
                                          std::shared_ptr<PLUGIN_CONTENT_MANAGER> aPcm,
                                          const ActionCallback&                   aActionCallback,
                                          const PinCallback&                      aPinCallback ) :
        PANEL_PACKAGES_VIEW_BASE( parent ),
        m_actionCallback( aActionCallback ),
        m_pinCallback( aPinCallback ),
        m_pcm( aPcm )
{
    // Replace wxFormBuilder's sash initializer with one which will respect m_initialSashPos.
    m_splitter1->Disconnect( wxEVT_IDLE,
                             wxIdleEventHandler( PANEL_PACKAGES_VIEW_BASE::m_splitter1OnIdle ),
                             NULL, this );
    m_splitter1->Connect( wxEVT_IDLE, wxIdleEventHandler( PANEL_PACKAGES_VIEW::SetSashOnIdle ),
                          NULL, this );

    m_splitter1->SetPaneMinimums( FromDIP( 350 ), FromDIP( 450 ) );

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
        m_gridVersions->SetColSize( col, m_gridVersions->GetVisibleWidth( col ) );
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
    m_splitter1->Disconnect( wxEVT_IDLE, wxIdleEventHandler( PANEL_PACKAGES_VIEW::SetSashOnIdle ), nullptr, this );

    COMMON_SETTINGS* cfg = Pgm().GetCommonSettings();
    cfg->m_PackageManager.sash_pos = m_splitter1->GetSashPosition();

    m_gridVersions->PopEventHandler( true );
}


void PANEL_PACKAGES_VIEW::ClearData()
{
    unsetPackageDetails();

    m_currentSelected = nullptr;
    m_updateablePackages.clear();
    m_packagePanels.clear();
    m_packageInitialOrder.clear();
    m_packageListWindow->GetSizer()->Clear( true ); // Delete panels
    m_packageListWindow->GetSizer()->FitInside( m_packageListWindow );
    m_packageListWindow->Layout();
}


void PANEL_PACKAGES_VIEW::SetData( const std::vector<PACKAGE_VIEW_DATA>& aPackageData )
{
    ClearData();

    for( const PACKAGE_VIEW_DATA& data : aPackageData )
    {
        PANEL_PACKAGE* package_panel = new PANEL_PACKAGE( m_packageListWindow, m_actionCallback,
                                                          m_pinCallback, data );

        package_panel->SetSelectCallback(
                [package_panel, this] ()
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

        if( data.state == PPS_UPDATE_AVAILABLE && !data.pinned )
            m_updateablePackages.insert( data.package.identifier );
    }

    updatePackageList();
    updateCommonState();
}


void PANEL_PACKAGES_VIEW::setPackageDetails( const PACKAGE_VIEW_DATA& aPackageData )
{
    const PCM_PACKAGE& package = aPackageData.package;

    // Details
    wxString details;

    details << wxT( "<h5>" ) + package.name + wxT( "</h5>" );

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
                        if( c == ' ' || c == '\n')
                        {
                            result += wxString::Format( wxT( "<a href='%s'>%s</a>" ), url, url );
                            inURL = false;

                            if( c == '\n' )
                                result += wxT( "</p><p>" );
                            else
                                result += c;
                        }
                        else
                        {
                            url += c;
                        }
                    }
                    else if( text.Mid( i, 5 ) == wxT( "http:" )
                                || text.Mid( i, 6 ) == wxT( "https:" ) )
                    {
                        url = c;
                        inURL = true;
                    }
                    else if( c == '\n' )
                    {
                        result += wxT( "</p><p>" );
                    }
                    else
                    {
                        result += c;
                    }
                }

                if( inURL )
                    result += wxString::Format( wxT( "<a href='%s'>%s</a>" ), url, url );

                return result;
            };

    wxString desc = package.description_full;
    details << wxT( "<p>" ) + format_desc( desc ) + wxT( "</p>" );

    details << wxT( "<p><b>" ) + _( "Metadata" ) + wxT( "</b></p>" );
    details << wxT( "<ul>" );
    details << wxT( "<li>" ) + _( "Package identifier: " ) + package.identifier + wxT( "</li>" );
    details << wxT( "<li>" ) + _( "License: " ) + package.license + wxT( "</li>" );

    if( package.tags.size() > 0 )
    {
        wxString tags_str;

        for( const std::string& tag : package.tags )
        {
            if( !tags_str.IsEmpty() )
                tags_str += ", ";

            tags_str += tag;
        }

        details << wxT( "<li>" ) + _( "Tags: " ) + tags_str + wxT( "</li>" );
    }

    auto format_entry =
            []( const std::pair<const std::string, wxString>& entry ) -> wxString
            {
                wxString name = entry.first;
                wxString url = EscapeHTML( entry.second );

                if( name == wxT( "email" ) )
                    return wxString::Format( wxT( "<a href='mailto:%s'>%s</a>" ), url, url );
                else if( url.StartsWith( wxT( "http:" ) ) || url.StartsWith( wxT( "https:" ) ) )
                    return wxString::Format( wxT( "<a href='%s'>%s</a>" ), url, url );
                else
                    return entry.second;
            };

    auto write_contact =
            [&]( const wxString& type, const PCM_CONTACT& contact )
            {
                details << wxT( "<li>" ) + type + wxT( ": " ) + contact.name + wxT( "<ul>" );

                for( const std::pair<const std::string, wxString>& entry : contact.contact )
                {
                    details << wxT( "<li>" );
                    details << entry.first << wxT( ": " ) + format_entry( entry );
                    details << wxT( "</li>" );
                }

                details << wxT( "</ul>" );
            };

    write_contact( _( "Author" ), package.author );

    if( package.maintainer )
        write_contact( _( "Maintainer" ), *package.maintainer );

    if( package.resources.size() > 0 )
    {
        details << wxT( "<li>" ) + _( "Resources" ) + wxT( "<ul>" );

        for( const std::pair<const std::string, wxString>& entry : package.resources )
        {
            details << wxT( "<li>" );
            details << entry.first << wxT( ": " );
            details << format_entry( entry ) + wxT( "</li>" );
        }

        details << wxT( "</ul>" );
    }

    details << wxT( "</ul>" );

    m_infoText->SetPage( details );

    wxSizeEvent dummy;
    OnSizeInfoBox( dummy );

    // Versions table
    m_gridVersions->Freeze();
    m_gridVersions->ClearRows();

    int      row = 0;
    wxString current_version;

    if( aPackageData.state == PPS_INSTALLED || aPackageData.state == PPS_UPDATE_AVAILABLE )
        current_version = m_pcm->GetInstalledPackageVersion( package.identifier );

    wxFont bold_font = m_gridVersions->GetDefaultCellFont().Bold();

    for( const PACKAGE_VERSION& version : package.versions )
    {
        if( !version.compatible && !m_showAllVersions->IsChecked() )
            continue;

        m_gridVersions->InsertRows( row );

        m_gridVersions->SetCellValue( row, COL_VERSION, version.version );
        m_gridVersions->SetCellValue( row, COL_DOWNLOAD_SIZE, toHumanReadableSize( version.download_size ) );
        m_gridVersions->SetCellValue( row, COL_INSTALL_SIZE, toHumanReadableSize( version.install_size ) );
        m_gridVersions->SetCellValue( row, COL_COMPATIBILITY, version.compatible ? wxT( "\u2714" ) : wxEmptyString );
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
        m_gridVersions->SetColSize( col, m_gridVersions->GetVisibleWidth( col ) );
    }

    // Autoselect preferred or installed version
    if( m_gridVersions->GetNumberRows() >= 1 )
    {
        wxString version = m_currentSelected->GetPackageData().current_version;

        if( version.IsEmpty() )
            version = m_currentSelected->GetPreferredVersion();

        if( !version.IsEmpty() )
        {
            for( int i = 0; i < m_gridVersions->GetNumberRows(); i++ )
            {
                if( m_gridVersions->GetCellValue( i, COL_VERSION ) == version )
                {
                    m_gridVersions->SelectRow( i );
                    m_gridVersions->SetGridCursor( i, COL_VERSION );
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
    m_gridVersions->ClearRows();
    m_gridVersions->Thaw();
}


wxString PANEL_PACKAGES_VIEW::toHumanReadableSize( const std::optional<uint64_t> size ) const
{
    if( !size )
        return wxT( "-" );

    uint64_t b = *size;

    if( b >= 1024 * 1024 )
        return wxString::Format( wxT( "%.1f MB" ), b / 1000.0 / 1000.0 );

    if( b >= 1024 )
        return wxString::Format( wxT( "%lld kB" ), b / 1000 );

    return wxString::Format( wxT( "%lld B" ), b );
}


bool PANEL_PACKAGES_VIEW::canDownload() const
{
    if( !m_currentSelected )
        return false;

    return m_gridVersions->GetNumberRows() == 1 || m_gridVersions->GetSelectedRows().size() == 1;
}


bool PANEL_PACKAGES_VIEW::canRunAction() const
{
    if( !m_currentSelected )
        return false;

    const PACKAGE_VIEW_DATA& packageData = m_currentSelected->GetPackageData();

    switch( packageData.state )
    {
    case PPS_PENDING_INSTALL:
    case PPS_PENDING_UNINSTALL:
    case PPS_PENDING_UPDATE:
        return false;

    default:
        break;
    }

    return m_gridVersions->GetNumberRows() == 1 || m_gridVersions->GetSelectedRows().size() == 1;
}


void PANEL_PACKAGES_VIEW::SetPackageState( const wxString& aPackageId, const PCM_PACKAGE_STATE aState,
                                           const bool aPinned )
{
    auto it = m_packagePanels.find( aPackageId );

    if( it != m_packagePanels.end() )
    {
        it->second->SetState( aState, aPinned );

        if( m_currentSelected && m_currentSelected == it->second )
        {
            wxMouseEvent dummy;
            m_currentSelected->OnClick( dummy );
        }

        if( aState == PPS_UPDATE_AVAILABLE && !aPinned )
            m_updateablePackages.insert( aPackageId );
        else
            m_updateablePackages.erase( aPackageId );

        updateCommonState();
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
        wxMessageBox( _( "Package download url is not specified" ), _( "Error downloading package" ),
                      wxICON_INFORMATION | wxOK, wxGetTopLevelParent( this ) );
        return;
    }

    const wxString& url = *ver_it->download_url;
    KICAD_SETTINGS* cfg = GetAppSettings<KICAD_SETTINGS>( "kicad" );

    wxWindow* topLevelParent = wxGetTopLevelParent( this );
    wxFileDialog dialog( topLevelParent, _( "Save Package" ), cfg->m_PcmLastDownloadDir,
                         wxString::Format( wxT( "%s_v%s.zip" ), package.identifier, version ),
                         wxT( "ZIP files (*.zip)|*.zip" ), wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    if( dialog.ShowModal() == wxID_CANCEL )
        return;

    wxString path = dialog.GetPath();
    cfg->m_PcmLastDownloadDir = wxPathOnly( path );

    std::ofstream output( path.ToUTF8(), std::ios_base::binary );

    WX_PROGRESS_REPORTER reporter( this, _( "Download Package" ), 1, PR_CAN_ABORT );

    bool success = m_pcm->DownloadToStream( url, &output, &reporter, 0 );

    output.close();

    if( success )
    {
        if( ver_it->download_sha256 )
        {
            std::ifstream stream( path.ToUTF8(), std::ios_base::binary );

            bool matches = m_pcm->VerifyHash( stream, *ver_it->download_sha256 );

            stream.close();

            if( !matches && wxMessageBox( _( "Integrity of the downloaded package could not be verified, hash "
                                             "does not match. Are you sure you want to keep this file?" ),
                                          _( "Keep downloaded file" ), wxICON_EXCLAMATION | wxYES_NO,
                                          wxGetTopLevelParent( this ) )
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


void PANEL_PACKAGES_VIEW::OnVersionActionClicked( wxCommandEvent& event )
{
    if( !canRunAction() )
    {
        wxBell();
        return;
    }

    PCM_PACKAGE_ACTION action = getAction();

    if( action == PPA_UNINSTALL )
    {
        m_actionCallback( m_currentSelected->GetPackageData(), PPA_UNINSTALL,
                          m_currentSelected->GetPackageData().current_version );
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

    wxCHECK_RET( ver_it != package.versions.end(), "Could not find package version" );

    if( !ver_it->compatible && wxMessageBox( _( "This package version is incompatible with your KiCad version or "
                                                "platform. Are you sure you want to install it anyway?" ),
                                             _( "Install package" ), wxICON_EXCLAMATION | wxYES_NO,
                                             wxGetTopLevelParent( this ) )
                                   == wxNO )
    {
        return;
    }

    m_actionCallback( m_currentSelected->GetPackageData(), action, version );
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
        const PCM_PACKAGE& pkg = m_packagePanels[m_packageInitialOrder[index]]->GetPackageData().package;

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

            if( !m_currentSelected )
            {
                wxMouseEvent dummy;
                panel->OnClick( dummy );
            }
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

    if( canRunAction() )
    {
        m_buttonAction->Enable();

        PCM_PACKAGE_ACTION action = getAction();

        switch( action )
        {
        case PPA_INSTALL:   m_buttonAction->SetLabel( _( "Install" ) );   break;
        case PPA_UNINSTALL: m_buttonAction->SetLabel( _( "Uninstall" ) ); break;
        case PPA_UPDATE:    m_buttonAction->SetLabel( _( "Update" ) );    break;
        }
    }
    else
    {
        m_buttonAction->Disable();
        m_buttonAction->SetLabel( _( "Pending" ) );
    }
}


PCM_PACKAGE_ACTION PANEL_PACKAGES_VIEW::getAction() const
{
    wxASSERT_MSG( m_gridVersions->GetNumberRows() == 1 || m_gridVersions->GetSelectedRows().size() == 1,
                  wxT( "getAction() called with ambiguous version selection" ) );

    int selected_row = 0;

    if( m_gridVersions->GetSelectedRows().size() == 1 )
        selected_row = m_gridVersions->GetSelectedRows()[0];

    wxString                 version = m_gridVersions->GetCellValue( selected_row, COL_VERSION );
    const PACKAGE_VIEW_DATA& package = m_currentSelected->GetPackageData();

    switch( package.state )
    {
    case PPS_AVAILABLE:
    case PPS_UNAVAILABLE:
        return PPA_INSTALL; // Only action for not installed package is to install it
    case PPS_INSTALLED:
    case PPS_UPDATE_AVAILABLE:
        if( version == package.current_version )
            return PPA_UNINSTALL;
        else
            return PPA_UPDATE;
    default:
        return PPA_INSTALL; // For pending states return value does not matter as button will be disabled
    }
}


void PANEL_PACKAGES_VIEW::OnSizeInfoBox( wxSizeEvent& aEvent )
{
    wxSize infoSize = KIPLATFORM::UI::GetUnobscuredSize( m_infoText->GetParent() );
    infoSize.x -= 10;
    m_infoText->SetMinSize( infoSize );
    m_infoText->SetMaxSize( infoSize );
    m_infoText->SetSize( infoSize );
    m_infoText->Layout();

    infoSize.y = m_infoText->GetInternalRepresentation()->GetHeight() + 12;
    m_infoText->SetMinSize( infoSize );
    m_infoText->SetMaxSize( infoSize );
    m_infoText->SetSize( infoSize );
    m_infoText->Layout();

    Refresh();
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

    m_splitter1->Disconnect( wxEVT_IDLE, wxIdleEventHandler( PANEL_PACKAGES_VIEW::SetSashOnIdle ), nullptr, this );
}


void PANEL_PACKAGES_VIEW::updateCommonState()
{
    m_buttonUpdateAll->Enable( m_updateablePackages.size() > 0 );
}


void PANEL_PACKAGES_VIEW::OnUpdateAllClicked( wxCommandEvent& event )
{
    // The map will be modified by the callback so we copy the list here
    std::vector<wxString> packages;

    std::copy( m_updateablePackages.begin(), m_updateablePackages.end(), std::back_inserter( packages ) );

    for( const wxString& pkg_id : packages )
    {
        auto it = m_packagePanels.find( pkg_id );

        if( it != m_packagePanels.end() )
        {
            const PACKAGE_VIEW_DATA& data = it->second->GetPackageData();

            m_actionCallback( data, PPA_UPDATE, data.update_version );
        }
    }
}
