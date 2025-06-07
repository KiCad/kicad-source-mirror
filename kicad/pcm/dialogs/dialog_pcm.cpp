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

// kicad_curl_easy.h **must be** included before any wxWidgets header to avoid conflicts
// at least on Windows/msys2
#include "kicad_curl/kicad_curl_easy.h"

#include "bitmaps.h"
#include "dialog_manage_repositories.h"
#include "dialog_pcm.h"
#include <eda_base_frame.h>
#include "grid_tricks.h"
#include "ki_exception.h"
#include "pcm_task_manager.h"
#include "pgm_base.h"
#include "settings/kicad_settings.h"
#include "settings/settings_manager.h"
#include "thread"
#include "widgets/wx_grid.h"

#include <fstream>
#include <launch_ext.h>
#include <sstream>
#include <vector>
#include <wx/filedlg.h>
#include <wx/msgdlg.h>


#define GRID_CELL_MARGIN 4

// Notes: These strings are static, so wxGetTranslation must be called to display the
// transalted text
static std::vector<std::pair<PCM_PACKAGE_TYPE, wxString>> PACKAGE_TYPE_LIST = {
    { PT_PLUGIN,     _( "Plugins (%d)" ) },
    { PT_FAB,        _( "Fabrication plugins (%d)" ) },
    { PT_LIBRARY,    _( "Libraries (%d)" ) },
    { PT_DATASOURCE, _( "Data sources (%d)" ) },
    { PT_COLORTHEME, _( "Color themes (%d)" ) },
};


DIALOG_PCM::DIALOG_PCM( EDA_BASE_FRAME* parent, std::shared_ptr<PLUGIN_CONTENT_MANAGER> pcm ) :
        DIALOG_PCM_BASE( parent ),
        m_parentFrame( parent ),
        m_pcm( pcm )
{
    // correct the min size from wxfb with fromdip
    SetMinSize( FromDIP( GetMinSize() ) );

    SetDoubleBuffered( true );

    m_defaultBitmap = KiBitmap( BITMAPS::icon_pcm );

    m_pcm->SetDialogWindow( this );
    m_pcm->StopBackgroundUpdate();

    m_gridPendingActions->PushEventHandler( new GRID_TRICKS( m_gridPendingActions ) );

    m_discardActionButton->SetBitmap( KiBitmapBundle( BITMAPS::small_trash ) );
    m_panelPending->Layout();

    m_actionCallback = [this]( const PACKAGE_VIEW_DATA& aData, PCM_PACKAGE_ACTION aAction,
                               const wxString& aVersion )
    {
        if( aAction == PPA_UPDATE && m_pcm->IsPackagePinned( aData.package.identifier ) )
        {
            if( wxMessageBox( wxString::Format( _( "Are you sure you want to update pinned package "
                                                   "from version %s to %s?" ),
                                                aData.current_version, aVersion ),
                              _( "Confirm update" ), wxICON_QUESTION | wxYES_NO, this )
                    == wxNO )
            {
                return;
            }
        }

        m_gridPendingActions->Freeze();

        PCM_PACKAGE_STATE new_state;

        m_gridPendingActions->AppendRows();
        int row = m_gridPendingActions->GetNumberRows() - 1;

        m_gridPendingActions->SetCellValue( row, PENDING_COL_NAME, aData.package.name );
        m_gridPendingActions->SetCellValue( row, PENDING_COL_REPOSITORY, aData.repository_name );

        switch( aAction )
        {
        default:
        case PPA_INSTALL:
            m_gridPendingActions->SetCellValue( row, PENDING_COL_ACTION, _( "Install" ) );
            m_gridPendingActions->SetCellValue( row, PENDING_COL_VERSION, aVersion );
            new_state = PPS_PENDING_INSTALL;
            break;

        case PPA_UPDATE:
            m_gridPendingActions->SetCellValue( row, PENDING_COL_ACTION, _( "Update" ) );
            m_gridPendingActions->SetCellValue( row, PENDING_COL_VERSION,
                                                wxString::Format( wxT( "%s \u279C %s" ),
                                                                  aData.current_version,
                                                                  aVersion ) );
            new_state = PPS_PENDING_UPDATE;
            break;

        case PPA_UNINSTALL:
            m_gridPendingActions->SetCellValue( row, PENDING_COL_ACTION, _( "Uninstall" ) );
            m_gridPendingActions->SetCellValue( row, PENDING_COL_VERSION,
                                                m_pcm->GetInstalledPackageVersion( aData.package.identifier ) );
            new_state = PPS_PENDING_UNINSTALL;
            break;
        }

        m_pendingActions.emplace_back( aAction, aData.repository_id, aData.package, aVersion );

        m_gridPendingActions->Thaw();

        updatePendingActionsTab();

        updatePackageState( aData.package.identifier, new_state );
    };

    m_pinCallback =
            [this]( const wxString& aPackageId, const PCM_PACKAGE_STATE aState, const bool aPinned )
            {
                m_pcm->SetPinned( aPackageId, aPinned );

                updatePackageState( aPackageId, aState );
            };

    m_installedPanel = new PANEL_PACKAGES_VIEW( m_panelInstalledHolder, m_pcm, m_actionCallback, m_pinCallback );
    m_panelInstalledHolder->GetSizer()->Add( m_installedPanel, 1, wxEXPAND );
    m_panelInstalledHolder->Layout();

    for( const std::pair<PCM_PACKAGE_TYPE, wxString>& entry : PACKAGE_TYPE_LIST )
    {
        PANEL_PACKAGES_VIEW* panel = new PANEL_PACKAGES_VIEW( m_contentNotebook, m_pcm, m_actionCallback,
                                                              m_pinCallback );
        wxString             label = wxGetTranslation( entry.second );
        m_contentNotebook->AddPage( panel, wxString::Format( label, 0 ) );
        m_repositoryContentPanels.insert( { entry.first, panel } );
    }

    m_dialogNotebook->SetPageText( 0, wxString::Format( _( "Repository (%d)" ), 0 ) );

    setInstalledPackages();
    updatePendingActionsTab();

    m_dialogNotebook->SetSelection( 0 );

    SetupStandardButtons( { { wxID_OK,     _( "Close" ) },
                            { wxID_APPLY,  _( "Apply Pending Changes" ) },
                            { wxID_CANCEL, _( "Discard Pending Changes" ) } } );

    Bind( wxEVT_CLOSE_WINDOW, &DIALOG_PCM::OnCloseWindow, this );
    m_sdbSizer1Cancel->Bind( wxEVT_UPDATE_UI, &DIALOG_PCM::OnUpdateEventButtons, this );
    m_sdbSizer1Apply->Bind( wxEVT_UPDATE_UI, &DIALOG_PCM::OnUpdateEventButtons, this );

    setRepositoryListFromPcm();

    for( int col = 0; col < m_gridPendingActions->GetNumberCols(); col++ )
    {
        const wxString& heading = m_gridPendingActions->GetColLabelValue( col );
        int             headingWidth = GetTextExtent( heading ).x + 2 * GRID_CELL_MARGIN;

        // Set the minimal width to the column label size.
        m_gridPendingActions->SetColMinimalWidth( col, headingWidth );
    }

    // fix sizers now widgets are set.
    finishDialogSettings();
}


DIALOG_PCM::~DIALOG_PCM()
{
    Unbind( wxEVT_CLOSE_WINDOW, &DIALOG_PCM::OnCloseWindow, this );
    m_sdbSizer1Cancel->Unbind( wxEVT_UPDATE_UI, &DIALOG_PCM::OnUpdateEventButtons, this );
    m_sdbSizer1Apply->Unbind( wxEVT_UPDATE_UI, &DIALOG_PCM::OnUpdateEventButtons, this );

    m_pcm->SaveInstalledPackages();
    m_pcm->SetDialogWindow( nullptr );
    m_pcm->RunBackgroundUpdate();

    m_gridPendingActions->PopEventHandler( true );
}


void DIALOG_PCM::SetActivePackageType( PCM_PACKAGE_TYPE aType )
{
    for( size_t i = 0; i < PACKAGE_TYPE_LIST.size(); ++i )
    {
        if( PACKAGE_TYPE_LIST[i].first == aType )
        {
            m_contentNotebook->SetSelection( i );
            break;
        }
    }
}


void DIALOG_PCM::OnUpdateEventButtons( wxUpdateUIEvent& event )
{
    event.Enable( !m_pendingActions.empty() );
}


void DIALOG_PCM::OnCloseClicked( wxCommandEvent& event )
{
    if( m_pendingActions.size() == 0
        || wxMessageBox( _( "Are you sure you want to close the package manager "
                            "and discard pending changes?" ),
                         _( "Plugin and Content Manager" ), wxICON_QUESTION | wxYES_NO, this )
                   == wxYES )
    {
        EndModal( wxID_OK );
    }
}


void DIALOG_PCM::OnCloseWindow( wxCloseEvent& aEvent )
{
    wxCommandEvent dummy;

    OnCloseClicked( dummy );
}


void DIALOG_PCM::OnManageRepositoriesClicked( wxCommandEvent& event )
{
    DIALOG_MANAGE_REPOSITORIES* dialog = new DIALOG_MANAGE_REPOSITORIES( this, m_pcm );

    std::vector<std::pair<wxString, wxString>>            dialog_data;
    std::vector<std::tuple<wxString, wxString, wxString>> repo_list = m_pcm->GetRepositoryList();

    for( const auto& [id, url, name] : repo_list )
        dialog_data.push_back( std::make_pair( url, name ) );

    dialog->SetData( dialog_data );

    if( dialog->ShowModal() == wxID_SAVE )
    {
        dialog_data = dialog->GetData();
        m_pcm->SetRepositoryList( dialog_data );

        if( KICAD_SETTINGS* cfg = GetAppSettings<KICAD_SETTINGS>( "kicad" ) )
            cfg->m_PcmRepositories = std::move( dialog_data );

        setRepositoryListFromPcm();
    }

    dialog->Destroy();
}


void DIALOG_PCM::setRepositoryListFromPcm()
{
    std::vector<std::tuple<wxString, wxString, wxString>> repositories = m_pcm->GetRepositoryList();

    m_choiceRepository->Clear();

    for( const auto& [id, url, name] : repositories )
        m_choiceRepository->Append( url, new wxStringClientData( id ) );

    if( repositories.size() > 0 )
    {
        m_choiceRepository->SetSelection( 0 );
        m_selectedRepositoryId = std::get<0>( repositories[0] );
        setRepositoryData( m_selectedRepositoryId );
    }
    else
    {
        m_selectedRepositoryId = "";
    }
}


void DIALOG_PCM::OnRefreshClicked( wxCommandEvent& event )
{
    m_pcm->DiscardRepositoryCache( m_selectedRepositoryId );
    setRepositoryData( m_selectedRepositoryId );
}


void DIALOG_PCM::OnInstallFromFileClicked( wxCommandEvent& event )
{
    wxFileDialog open_file_dialog( this, _( "Install Package" ), wxEmptyString, wxEmptyString,
                                   wxT( "Zip files (*.zip)|*.zip" ), wxFD_OPEN | wxFD_FILE_MUST_EXIST );

    if( open_file_dialog.ShowModal() == wxID_CANCEL )
        return;

    PCM_TASK_MANAGER task_manager( m_pcm );
    task_manager.InstallFromFile( this, open_file_dialog.GetPath() );

    m_changed_package_types.merge( task_manager.GetChangedPackageTypes() );

    setInstalledPackages();

    if( !m_selectedRepositoryId.IsEmpty() )
        setRepositoryData( m_selectedRepositoryId );
}


void DIALOG_PCM::OnRepositoryChoice( wxCommandEvent& event )
{
    wxStringClientData* data = static_cast<wxStringClientData*>(
            m_choiceRepository->GetClientObject( m_choiceRepository->GetSelection() ) );

    m_selectedRepositoryId = data->GetData();

    setRepositoryData( m_selectedRepositoryId );
}


void DIALOG_PCM::setRepositoryData( const wxString& aRepositoryId )
{
    m_dialogNotebook->Freeze();

    if( m_pcm->CacheRepository( aRepositoryId ) )
    {
        for( const auto& [ packageType, packagesView ] : m_repositoryContentPanels )
            packagesView->ClearData();

        m_packageBitmaps = m_pcm->GetRepositoryPackageBitmaps( aRepositoryId );

        const std::vector<PCM_PACKAGE> packages = m_pcm->GetRepositoryPackages( aRepositoryId );

        std::unordered_map<PCM_PACKAGE_TYPE, std::vector<PACKAGE_VIEW_DATA>> data;

        for( const PCM_PACKAGE& pkg : packages )
        {
            PACKAGE_VIEW_DATA package_data( pkg );

            if( m_packageBitmaps.count( package_data.package.identifier ) > 0 )
                package_data.bitmap = &m_packageBitmaps.at( package_data.package.identifier );
            else
                package_data.bitmap = &m_defaultBitmap;

            package_data.state = m_pcm->GetPackageState( aRepositoryId, pkg.identifier );

            if( package_data.state == PPS_INSTALLED || package_data.state == PPS_UPDATE_AVAILABLE )
            {
                package_data.current_version = m_pcm->GetInstalledPackageVersion( pkg.identifier );
                package_data.pinned = m_pcm->IsPackagePinned( pkg.identifier );
            }

            if( package_data.state == PPS_UPDATE_AVAILABLE )
                package_data.update_version = m_pcm->GetPackageUpdateVersion( pkg );


            for( const PENDING_ACTION& action : m_pendingActions )
            {
                if( action.package.identifier != pkg.identifier )
                    continue;

                switch( action.action )
                {
                case PPA_INSTALL: package_data.state = PPS_PENDING_INSTALL; break;
                case PPA_UPDATE: package_data.state = PPS_PENDING_UPDATE; break;
                case PPA_UNINSTALL: package_data.state = PPS_PENDING_UNINSTALL; break;
                }

                break;
            }

            package_data.repository_id = aRepositoryId;
            package_data.repository_name = m_choiceRepository->GetStringSelection();

            // Fabrication plugins are displayed in a different tab although they are still plugins
            PCM_PACKAGE_TYPE type = pkg.category == PC_FAB ? PT_FAB : pkg.type;

            data[type].emplace_back( package_data );
        }

        for( size_t i = 0; i < PACKAGE_TYPE_LIST.size(); i++ )
        {
            PCM_PACKAGE_TYPE type = PACKAGE_TYPE_LIST[i].first;
            const wxString&  label = PACKAGE_TYPE_LIST[i].second;
            m_repositoryContentPanels[type]->SetData( data[type] );
            m_contentNotebook->SetPageText( i, wxString::Format( wxGetTranslation( label ),
                                                                 (int) data[type].size() ) );
        }

        m_dialogNotebook->SetPageText( 0, wxString::Format( _( "Repository (%d)" ),
                                                            (int) packages.size() ) );
    }

    m_dialogNotebook->Thaw();
}


void DIALOG_PCM::OnPendingActionsCellClicked( wxGridEvent& event )
{
    m_gridPendingActions->ClearSelection();
    m_gridPendingActions->SelectRow( event.GetRow() );
}


void DIALOG_PCM::updatePendingActionsTab()
{
    m_dialogNotebook->SetPageText( 2, wxString::Format( _( "Pending (%d)" ),
                                                        (int) m_pendingActions.size() ) );

    for( int col = 0; col < m_gridPendingActions->GetNumberCols(); col++ )
    {
        // Set the width to see the full contents
        m_gridPendingActions->SetColSize( col, m_gridPendingActions->GetVisibleWidth( col ) );
    }
}


void DIALOG_PCM::setInstalledPackages()
{
    m_installedPanel->ClearData();

    const std::vector<PCM_INSTALLATION_ENTRY> installed = m_pcm->GetInstalledPackages();
    std::vector<PACKAGE_VIEW_DATA>            package_list;

    m_installedBitmaps = m_pcm->GetInstalledPackageBitmaps();

    for( const PCM_INSTALLATION_ENTRY& entry : installed )
    {
        PACKAGE_VIEW_DATA package_data( entry );

        if( m_installedBitmaps.count( package_data.package.identifier ) > 0 )
            package_data.bitmap = &m_installedBitmaps.at( package_data.package.identifier );
        else
            package_data.bitmap = &m_defaultBitmap;

        package_data.state = m_pcm->GetPackageState( entry.repository_id,
                                                     entry.package.identifier );

        if( package_data.state == PPS_UPDATE_AVAILABLE )
            package_data.update_version = m_pcm->GetPackageUpdateVersion( entry.package );

        package_list.emplace_back( package_data );
    }

    m_installedPanel->SetData( package_list );

    m_dialogNotebook->SetPageText( 1, wxString::Format( _( "Installed (%d)" ),
                                                        (int) package_list.size() ) );
}


void DIALOG_PCM::OnApplyChangesClicked( wxCommandEvent& event )
{
    if( m_pendingActions.size() == 0 )
        return;

    m_sdbSizer1OK->Disable();
    m_sdbSizer1Apply->Disable();
    m_sdbSizer1Cancel->Disable();

    PCM_TASK_MANAGER task_manager( m_pcm );

    for( const PENDING_ACTION& action : m_pendingActions )
    {
        if( action.action == PPA_UNINSTALL )
        {
            task_manager.Uninstall( action.package );
        }
        else
        {
            bool isUpdate = action.action == PPA_UPDATE;
            task_manager.DownloadAndInstall( action.package, action.version, action.repository_id,
                                             isUpdate );
        }
    }

    task_manager.RunQueue( this );

    m_changed_package_types.merge( task_manager.GetChangedPackageTypes() );

    m_sdbSizer1OK->Enable();
    m_sdbSizer1Apply->Enable();
    m_sdbSizer1Cancel->Enable();

    setInstalledPackages();
    wxCommandEvent dummy;
    OnDiscardChangesClicked( dummy );

    if( !m_selectedRepositoryId.IsEmpty() )
        setRepositoryData( m_selectedRepositoryId );
}


void DIALOG_PCM::OnDiscardChangesClicked( wxCommandEvent& event )
{
    m_gridPendingActions->Freeze();

    for( int i = m_pendingActions.size() - 1; i >= 0; i-- )
        discardAction( i );

    updatePendingActionsTab();
    m_gridPendingActions->Thaw();
}


void DIALOG_PCM::OnDiscardActionClicked( wxCommandEvent& event )
{
    wxArrayInt rows = m_gridPendingActions->GetSelectedRows();

    std::sort( rows.begin(), rows.end(),
               []( const int& a, const int& b )
               {
                   return a > b;
               } );

    m_gridPendingActions->Freeze();

    for( int row : rows )
        discardAction( row );

    updatePendingActionsTab();
    m_gridPendingActions->Thaw();
}


void DIALOG_PCM::discardAction( int aIndex )
{
    m_gridPendingActions->DeleteRows( aIndex );

    PENDING_ACTION action = m_pendingActions[aIndex];

    PCM_PACKAGE_STATE state = m_pcm->GetPackageState( action.repository_id,
                                                      action.package.identifier );

    updatePackageState( action.package.identifier, state );

    m_pendingActions.erase( m_pendingActions.begin() + aIndex );
}


void DIALOG_PCM::updatePackageState( const wxString& aPackageId, const PCM_PACKAGE_STATE aState )
{
    bool pinned = m_pcm->IsPackagePinned( aPackageId );

    m_installedPanel->SetPackageState( aPackageId, aState, pinned );

    for( const auto& [ packageType, packagesView ] : m_repositoryContentPanels )
        packagesView->SetPackageState( aPackageId, aState, pinned );
}


void DIALOG_PCM::OnOpenPackageDirClicked( wxCommandEvent& event )
{
    LaunchExternal( m_pcm->Get3rdPartyPath() );
}
