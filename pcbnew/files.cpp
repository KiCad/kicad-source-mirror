/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2023 CERN (www.cern.ch)
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <string>
#include <vector>

#include <confirm.h>
#include <kidialog.h>
#include <core/arraydim.h>
#include <thread_pool.h>
#include <gestfich.h>
#include <local_history.h>
#include <pcb_edit_frame.h>
#include <board_design_settings.h>
#include <3d_viewer/eda_3d_viewer_frame.h>
#include <footprint_library_adapter.h>
#include <kiface_base.h>
#include <macros.h>
#include <trace_helpers.h>
#include <length_delay_calculation/length_delay_calculation.h>
#include <lockfile.h>
#include <wx/snglinst.h>
#include <netlist_reader/pcb_netlist.h>
#include <pcbnew_id.h>
#include <wildcards_and_files_ext.h>
#include <tool/tool_manager.h>
#include <board.h>
#include <kiplatform/app.h>
#include <widgets/appearance_controls.h>
#include <widgets/wx_infobar.h>
#include <widgets/wx_progress_reporters.h>
#include <settings/settings_manager.h>
#include <paths.h>
#include <pgm_base.h>
#include <project/project_file.h>
#include <project_pcb.h>
#include <project/project_local_settings.h>
#include <project/net_settings.h>
#include <io/common/plugin_common_choose_project.h>
#include <pcb_io/pcb_io_mgr.h>
#include <pcb_io/cadstar/pcb_io_cadstar_archive.h>
#include <pcb_io/kicad_sexpr/pcb_io_kicad_sexpr.h>
#include <dialogs/dialog_export_2581.h>
#include <dialogs/dialog_map_layers.h>
#include <dialogs/dialog_export_odbpp.h>
#include <jobs/job_export_pcb_odb.h>
#include <dialogs/dialog_import_choose_project.h>
#include <tools/pcb_actions.h>
#include <tools/board_editor_control.h>
#include "footprint_info_impl.h"
#include <board_commit.h>
#include <reporter.h>
#include <zone_filler.h>
#include <widgets/filedlg_import_non_kicad.h>
#include <widgets/kistatusbar.h>
#include <widgets/wx_html_report_box.h>
#include <wx_filename.h>  // For ::ResolvePossibleSymlinks()
#include <kiplatform/io.h>

#include <wx/stdpaths.h>
#include <wx/ffile.h>
#include <wx/filedlg.h>
#include <wx/txtstrm.h>
#include <wx/wfstream.h>
#include <wx/zipstrm.h>
#include <wx/dir.h>

#include "widgets/filedlg_hook_save_project.h"

//#define     USE_INSTRUMENTATION     1
#define     USE_INSTRUMENTATION     0


/**
 * Show a wxFileDialog asking for a #BOARD filename to open.
 *
 * @param aParent is a wxFrame passed to wxFileDialog.
 * @param aCtl is where to put the OpenProjectFiles() control bits.
 * @param aFileName on entry is a probable choice, on return is the chosen filename.
 * @param aKicadFilesOnly true to list KiCad pcb files plugins only, false to list import plugins.
 * @return  true if chosen, else false if user aborted.
 */
bool AskLoadBoardFileName( PCB_EDIT_FRAME* aParent, wxString* aFileName, int aCtl = 0 )
{
    std::vector<IO_BASE::IO_FILE_DESC> descriptions;

    for( const auto& plugin : PCB_IO_MGR::PLUGIN_REGISTRY::Instance()->AllPlugins() )
    {
        bool isKiCad = plugin.m_type == PCB_IO_MGR::KICAD_SEXP || plugin.m_type == PCB_IO_MGR::LEGACY;

        if( ( aCtl & KICTL_KICAD_ONLY ) && !isKiCad )
            continue;

        if( ( aCtl & KICTL_NONKICAD_ONLY ) && isKiCad )
            continue;

        IO_RELEASER<PCB_IO> pi( plugin.m_createFunc() );
        wxCHECK( pi, false );

        const IO_BASE::IO_FILE_DESC& desc = pi->GetBoardFileDesc();

        if( desc.m_FileExtensions.empty() || !desc.m_CanRead )
            continue;

        descriptions.emplace_back( desc );
    }

    wxString                 fileFiltersStr;
    std::vector<std::string> allExtensions;
    std::set<wxString>       allWildcardsSet;

    for( const IO_BASE::IO_FILE_DESC& desc : descriptions )
    {
        if( !fileFiltersStr.IsEmpty() )
            fileFiltersStr += wxChar( '|' );

        fileFiltersStr += desc.FileFilter();

        for( const std::string& ext : desc.m_FileExtensions )
        {
            allExtensions.emplace_back( ext );
            allWildcardsSet.insert( wxT( "*." ) + formatWildcardExt( ext ) + wxT( ";" ) );
        }
    }

    wxString allWildcardsStr;

    for( const wxString& wildcard : allWildcardsSet )
        allWildcardsStr << wildcard;

    if( aCtl & KICTL_KICAD_ONLY )
    {
        fileFiltersStr = _( "All KiCad Board Files" ) + AddFileExtListToFilter( allExtensions );
    }
    else
    {
        fileFiltersStr = _( "All supported formats" ) + wxT( "|" ) + allWildcardsStr + wxT( "|" )
                         + fileFiltersStr;
    }

    wxFileName fileName( *aFileName );
    wxString   path;
    wxString   name;

    if( fileName.FileExists() )
    {
        path = fileName.GetPath();
        name = fileName.GetFullName();
    }
    else
    {
        path = aParent->GetMruPath();

        if( path.IsEmpty() )
            path = PATHS::GetDefaultUserProjectsPath();
        // leave name empty
    }

    bool kicadFormat = ( aCtl & KICTL_KICAD_ONLY );

    wxFileDialog dlg( aParent, kicadFormat ? _( "Open Board File" ) : _( "Import Non KiCad Board File" ),
                      path, name, fileFiltersStr, wxFD_OPEN | wxFD_FILE_MUST_EXIST );

    FILEDLG_IMPORT_NON_KICAD importOptions( aParent->config()->m_System.show_import_issues );

    if( !kicadFormat )
        dlg.SetCustomizeHook( importOptions );

    if( dlg.ShowModal() == wxID_OK )
    {
        *aFileName = dlg.GetPath();
        aParent->SetMruPath( wxFileName( dlg.GetPath() ).GetPath() );

        if( !kicadFormat )
            aParent->config()->m_System.show_import_issues = importOptions.GetShowIssues();

        return true;
    }
    else
    {
        return false;
    }
}


/**
 * Put up a wxFileDialog asking for a BOARD filename to save.
 *
 * @param aParent is a wxFrame passed to wxFileDialog.
 * @param aFileName on entry is a probable choice, on return is the chosen full filename
 *                  (includes path).
 * @param aCreateProject will be filled with the state of the Create Project? checkbox if relevant.
 * @return true if chosen, else false if user aborted.
 */
bool AskSaveBoardFileName( PCB_EDIT_FRAME* aParent, wxString* aFileName, bool* aCreateProject )
{
    wxString   wildcard = FILEEXT::PcbFileWildcard();
    wxFileName  fn = *aFileName;

    fn.SetExt( FILEEXT::KiCadPcbFileExtension );

    wxFileDialog dlg( aParent, _( "Save Board File As" ), fn.GetPath(), fn.GetFullName(), wildcard,
                      wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

// Add a "Create a project" checkbox in standalone mode and one isn't loaded
    FILEDLG_HOOK_SAVE_PROJECT newProjectHook;

    if( Kiface().IsSingle() && aParent->Prj().IsNullProject() )
        dlg.SetCustomizeHook( newProjectHook );

    if( dlg.ShowModal() != wxID_OK )
        return false;

    *aFileName = dlg.GetPath();
    *aFileName = EnsureFileExtension( *aFileName, FILEEXT::KiCadPcbFileExtension );

    if( newProjectHook.IsAttachedToDialog() )
        *aCreateProject = newProjectHook.GetCreateNewProject();
    else if( !aParent->Prj().IsNullProject() )
        *aCreateProject = true;

    return true;
}


void PCB_EDIT_FRAME::OnFileHistory( wxCommandEvent& event )
{
    wxString filename = GetFileFromHistory( event.GetId(), _( "Printed circuit board" ) );

    if( !filename.IsEmpty() )
    {
        if( !wxFileName::IsFileReadable( filename ) )
        {
            if( !AskLoadBoardFileName( this, &filename, KICTL_KICAD_ONLY ) )
                return;
        }

        OpenProjectFiles( std::vector<wxString>( 1, filename ), KICTL_KICAD_ONLY );
    }
}


void PCB_EDIT_FRAME::OnClearFileHistory( wxCommandEvent& aEvent )
{
    ClearFileHistory();
}


int BOARD_EDITOR_CONTROL::Open( const TOOL_EVENT& aEvent )
{
    // Only standalone mode can directly load a new document
    if( !Kiface().IsSingle() )
        return false;

    int      open_ctl = KICTL_KICAD_ONLY;
    wxString fileName = m_frame->Prj().AbsolutePath( m_frame->GetBoard()->GetFileName() );

    if( AskLoadBoardFileName( m_frame, &fileName, open_ctl ) )
        m_frame->OpenProjectFiles( std::vector<wxString>( 1, fileName ), open_ctl );

    return 0;
}


int BOARD_EDITOR_CONTROL::OpenNonKicadBoard( const TOOL_EVENT& aEvent )
{
    // Note: we explicitly allow this even if not in standalone mode for now, even though it is dangerous.
    int      open_ctl = KICTL_NONKICAD_ONLY;
    wxString fileName; // = Prj().AbsolutePath( GetBoard()->GetFileName() );

    if( AskLoadBoardFileName( m_frame, &fileName, open_ctl ) )
           m_frame->OpenProjectFiles( std::vector<wxString>( 1, fileName ), open_ctl );

    return 0;
}


int BOARD_EDITOR_CONTROL::Revert( const TOOL_EVENT& aEvent )
{
    wxFileName fn = m_frame->Prj().AbsolutePath( m_frame->GetBoard()->GetFileName() );

    if( !IsOK( m_frame, wxString::Format( _( "Revert '%s' to last version saved?" ), fn.GetFullPath() ) ) )
        return false;

    m_frame->GetScreen()->SetContentModified( false );    // do not prompt the user for changes

    m_frame->ReleaseFile();

    m_frame->OpenProjectFiles( std::vector<wxString>( 1, fn.GetFullPath() ), KICTL_REVERT );

    return 0;
}


int BOARD_EDITOR_CONTROL::New( const TOOL_EVENT& aEvent )
{
    // Only standalone mode can directly load a new document
    if( !Kiface().IsSingle() )
        return false;

    if( m_frame->IsContentModified() )
    {
        wxFileName fileName = m_frame->GetBoard()->GetFileName();
        wxString   saveMsg = _( "Current board will be closed, save changes to '%s' before "
                                "continuing?" );

        if( !HandleUnsavedChanges( m_frame, wxString::Format( saveMsg, fileName.GetFullName() ),
                                   [&]()->bool
                                   {
                                       return m_frame->SaveBoard();
                                   } ) )
        {
            return false;
        }
    }
    else if( !m_frame->GetBoard()->IsEmpty() )
    {
        if( !IsOK( m_frame, _( "Current Board will be closed. Continue?" ) ) )
            return false;
    }

    m_frame->SaveProjectLocalSettings();

    m_frame->GetBoard()->ClearProject();
    m_frame->GetSettingsManager()->UnloadProject( &m_frame->Prj() );

    if( !m_frame->Clear_Pcb( false ) )
        return false;

    m_frame->LoadProjectSettings();
    m_frame->LoadDrawingSheet();

    m_frame->OnBoardLoaded();
    m_frame->OnModify();

    return 0;
}


bool PCB_EDIT_FRAME::SaveBoard( bool aSaveAs, bool aSaveCopy )
{
    if( !aSaveAs )
    {
        if( !GetBoard()->GetFileName().IsEmpty() )
        {
            if( SavePcbFile( Prj().AbsolutePath( GetBoard()->GetFileName() ) ) )
            {
                m_autoSaveRequired = false;
                return true;
            }

            return false;
        }
    }

    wxString orig_name;

    wxFileName::SplitPath( GetBoard()->GetFileName(), nullptr, nullptr, &orig_name, nullptr );

    if( orig_name.IsEmpty() )
        orig_name = NAMELESS_PROJECT;

    wxFileName savePath( Prj().GetProjectFullName() );

    if( !savePath.IsOk() || !savePath.IsDirWritable() )
    {
        savePath = GetMruPath();

        if( !savePath.IsOk() || !savePath.IsDirWritable() )
            savePath = PATHS::GetDefaultUserProjectsPath();
    }

    wxFileName  fn( savePath.GetPath(), orig_name, FILEEXT::KiCadPcbFileExtension );
    wxString    filename = fn.GetFullPath();
    bool        createProject = false;
    bool        success = false;

    if( AskSaveBoardFileName( this, &filename, &createProject ) )
    {
        if( aSaveCopy )
        {
            success = SavePcbCopy( EnsureFileExtension( filename, FILEEXT::KiCadPcbFileExtension ), createProject );
        }
        else
        {
            success = SavePcbFile( filename, aSaveAs, createProject );

            if( success )
                m_autoSaveRequired = false;
        }
    }

    return success;
}


int PCB_EDIT_FRAME::inferLegacyEdgeClearance( BOARD* aBoard, bool aShowUserMsg )
{
    PCB_LAYER_COLLECTOR collector;

    collector.SetLayerId( Edge_Cuts );
    collector.Collect( aBoard, GENERAL_COLLECTOR::AllBoardItems );

    int  edgeWidth = -1;
    bool mixed = false;

    for( int i = 0; i < collector.GetCount(); i++ )
    {
        if( collector[i]->Type() == PCB_SHAPE_T )
        {
            int itemWidth = static_cast<PCB_SHAPE*>( collector[i] )->GetWidth();

            if( edgeWidth != -1 && edgeWidth != itemWidth )
            {
                mixed = true;
                edgeWidth = std::max( edgeWidth, itemWidth );
            }
            else
            {
                edgeWidth = itemWidth;
            }
        }
    }

    if( mixed && aShowUserMsg )
    {
        // If they had different widths then we can't ensure that fills will be the same.
        DisplayInfoMessage( this,
                            _( "If the zones on this board are refilled the Copper Edge "
                               "Clearance setting will be used (see Board Setup > Design "
                               "Rules > Constraints).\n This may result in different fills "
                               "from previous KiCad versions which used the line thicknesses "
                               "of the board boundary on the Edge Cuts layer." ) );
    }

    return std::max( 0, edgeWidth / 2 );
}


bool PCB_EDIT_FRAME::OpenProjectFiles( const std::vector<wxString>& aFileSet, int aCtl )
{
    // This is for python:
    if( aFileSet.size() != 1 )
    {
        DisplayError( this, wxString::Format( "Pcbnew:%s() takes a single filename", __func__ ) );
        return false;
    }

    wxString   fullFileName( aFileSet[0] );
    wxFileName wx_filename( fullFileName );
    Kiway().LocalHistory().Init( wx_filename.GetPath() );
    wxString   msg;

    if( Kiface().IsSingle() )
        KIPLATFORM::APP::RegisterApplicationRestart( fullFileName );

    // We insist on caller sending us an absolute path, if it does not, we say it's a bug.
    wxASSERT_MSG( wx_filename.IsAbsolute(), wxT( "Path is not absolute!" ) );

    std::unique_ptr<LOCKFILE> lock = std::make_unique<LOCKFILE>( fullFileName );

    if( !lock->Valid() && lock->IsLockedByMe() )
    {
        // If we cannot acquire the lock but we appear to be the one who locked it, check to
        // see if there is another KiCad instance running.  If not, then we can override the
        // lock.  This could happen if KiCad crashed or was interrupted.

        if( !Pgm().SingleInstance()->IsAnotherRunning() )
            lock->OverrideLock();
    }

    if( !lock->Valid() )
    {
        msg.Printf( _( "PCB '%s' is already open by '%s' at '%s'." ),
                    wx_filename.GetFullName(),
                    lock->GetUsername(),
                    lock->GetHostname() );

        if( !AskOverrideLock( this, msg ) )
            return false;

        lock->OverrideLock();
    }

    if( IsContentModified() )
    {
        if( !HandleUnsavedChanges( this, _( "The current PCB has been modified.  Save changes?" ),
                                   [&]() -> bool
                                   {
                                       return SavePcbFile( GetBoard()->GetFileName() );
                                   } ) )
        {
            return false;
        }
    }

    wxFileName pro = fullFileName;
    pro.SetExt( FILEEXT::ProjectFileExtension );

    bool is_new = !wxFileName::IsFileReadable( fullFileName );

    wxString previousBoardFileName = GetBoard() ? GetBoard()->GetFileName() : wxString();

    // If its a non-existent PCB and caller thinks it exists
    if( is_new && !( aCtl & KICTL_CREATE ) )
    {
        // notify user that fullFileName does not exist, ask if user wants to create it.
        msg.Printf( _( "PCB '%s' does not exist. Do you wish to create it?" ), fullFileName );

        if( !IsOK( this, msg ) )
            return false;
    }

    // Get rid of any existing warnings about the old board
    GetInfoBar()->Dismiss();

    if( KISTATUSBAR* statusBar = dynamic_cast<KISTATUSBAR*>( GetStatusBar() ) )
        statusBar->ClearLoadWarningMessages();

    WX_PROGRESS_REPORTER progressReporter( this, is_new ? _( "Create PCB" ) : _( "Load PCB" ), 1,
                                           PR_CAN_ABORT );
    WX_STRING_REPORTER loadReporter;
    LOAD_INFO_REPORTER_SCOPE loadReporterScope( &loadReporter );

    // No save prompt (we already prompted above), and only reset to a new blank board if new
    Clear_Pcb( false, !is_new );

    PCB_IO_MGR::PCB_FILE_T pluginType = PCB_IO_MGR::KICAD_SEXP;

    if( !is_new )
        pluginType = PCB_IO_MGR::FindPluginTypeFromBoardPath( fullFileName, aCtl );

    if( pluginType == PCB_IO_MGR::FILE_TYPE_NONE )
    {
        progressReporter.Hide();
        DisplayErrorMessage( this, _( "File format is not supported" ), wxEmptyString );
        return false;
    }

    bool converted =  pluginType != PCB_IO_MGR::LEGACY && pluginType != PCB_IO_MGR::KICAD_SEXP;

    // Loading a project should only be done under carefully considered circumstances.

    // The calling code should know not to ask me here to change projects unless
    // it knows what consequences that will have on other KIFACEs running and using
    // this same PROJECT.  It can be very harmful if that calling code is stupid.
    SETTINGS_MANAGER* mgr = GetSettingsManager();
    bool              setProject;

    if( Kiface().IsSingle() || !( aCtl & KICTL_NONKICAD_ONLY ) )
        setProject = pro.GetFullPath() != mgr->Prj().GetProjectFullName();
    else
        setProject = Prj().GetProjectFullName().IsEmpty();

    if( setProject )
    {
        // calls SaveProject
        SaveProjectLocalSettings();

        GetBoard()->ClearProject();
        mgr->UnloadProject( &mgr->Prj() );

        mgr->LoadProject( pro.GetFullPath() );

        // Do not allow saving a project if one doesn't exist.  This normally happens if we are
        // opening a board that has been moved from its project folder.
        // For converted projects, we don't want to set the read-only flag because we want a
        // project to be saved for the new file in case things like netclasses got migrated.
        Prj().SetReadOnly( !pro.Exists() && !converted );
    }

    // Clear the cache footprint list which may be project specific
    GFootprintList.Clear();

    if( is_new )
    {
        // Link the existing blank board to the new project
        GetBoard()->SetProject( &Prj() );

        GetBoard()->SetFileName( fullFileName );

        OnModify();
    }
    else
    {
        BOARD*              loadedBoard = nullptr;   // it will be set to non-NULL if loaded OK
        IO_RELEASER<PCB_IO> pi( PCB_IO_MGR::FindPlugin( pluginType ) );

        if( LAYER_MAPPABLE_PLUGIN* mappable_pi = dynamic_cast<LAYER_MAPPABLE_PLUGIN*>( pi.get() ) )
        {
            mappable_pi->RegisterCallback( std::bind( DIALOG_MAP_LAYERS::RunModal,
                                                      this, std::placeholders::_1 ) );
        }

        if( PROJECT_CHOOSER_PLUGIN* chooser_pi = dynamic_cast<PROJECT_CHOOSER_PLUGIN*>( pi.get() ) )
        {
            chooser_pi->RegisterCallback( std::bind( DIALOG_IMPORT_CHOOSE_PROJECT::RunModal,
                                                     this,
                                                     std::placeholders::_1 ) );
        }

        bool failedLoad = false;

        try
        {
            if( pi == nullptr )
            {
                // There was no plugin found, e.g. due to invalid file extension, file header,...
                THROW_IO_ERROR( _( "File format is not supported" ) );
            }

            std::map<std::string, UTF8> props;

            if( m_importProperties )
                props.insert( m_importProperties->begin(), m_importProperties->end() );

            // PCB_IO_EAGLE can use this info to center the BOARD, but it does not yet.
            props["page_width"] = std::to_string( GetPageSizeIU().x );
            props["page_height"] = std::to_string( GetPageSizeIU().y );

            pi->SetQueryUserCallback(
                    [&]( wxString aTitle, int aIcon, wxString aMessage, wxString aAction ) -> bool
                    {
                        KIDIALOG dlg( nullptr, aMessage, aTitle, wxOK | wxCANCEL | aIcon );

                        if( !aAction.IsEmpty() )
                            dlg.SetOKLabel( aAction );

                        dlg.DoNotShowCheckbox( aMessage, 0 );

                        return dlg.ShowModal() == wxID_OK;
                    } );

#if USE_INSTRUMENTATION
            // measure the time to load a BOARD.
            int64_t startTime = GetRunningMicroSecs();
#endif
            // Use loadReporter for import issues - they will be shown in the status bar
            // warning icon instead of a modal dialog
            if( config()->m_System.show_import_issues )
                pi->SetReporter( &loadReporter );
            else
                pi->SetReporter( &NULL_REPORTER::GetInstance() );

            pi->SetProgressReporter( &progressReporter );
            loadedBoard = pi->LoadBoard( fullFileName, nullptr, &props, &Prj() );

#if USE_INSTRUMENTATION
            int64_t stopTime = GetRunningMicroSecs();
            printf( "PCB_IO::Load(): %u usecs\n", stopTime - startTime );
#endif
        }
        catch( const FUTURE_FORMAT_ERROR& ffe )
        {
            msg.Printf( _( "Error loading PCB '%s'." ), fullFileName );
            progressReporter.Hide();
            DisplayErrorMessage( this, msg, ffe.Problem() );

            failedLoad = true;
        }
        catch( const IO_ERROR& ioe )
        {
            if( ioe.Problem() != wxT( "CANCEL" ) )
            {
                msg.Printf( _( "Error loading PCB '%s'." ), fullFileName );
                progressReporter.Hide();
                DisplayErrorMessage( this, msg, ioe.What() );
            }

            failedLoad = true;
        }
        catch( const std::bad_alloc& )
        {
            msg.Printf( _( "Memory exhausted loading PCB '%s'" ), fullFileName );
            progressReporter.Hide();
            DisplayErrorMessage( this, msg, wxEmptyString );

            failedLoad = true;
        }

        if( failedLoad || !loadedBoard )
        {
            // We didn't create a new blank board above, so do that now
            Clear_Pcb( false );

            // Show any messages collected before the failure
            if( KISTATUSBAR* statusBar = dynamic_cast<KISTATUSBAR*>( GetStatusBar() ) )
                statusBar->SetLoadWarningMessages( loadReporter.GetMessages() );

            return false;
        }

        // This fixes a focus issue after the progress reporter is done on GTK.  It shouldn't
        // cause any issues on macOS and Windows.  If it does, it will have to be conditionally
        // compiled.
        Raise();

        // Skip (possibly expensive) connectivity build here; we build it below after load
        SetBoard( loadedBoard, false, &progressReporter );

        if( GFootprintList.GetCount() == 0 )
            GFootprintList.ReadCacheFromFile( Prj().GetProjectPath() + wxT( "fp-info-cache" ) );

        if( loadedBoard->m_LegacyDesignSettingsLoaded )
        {
            Prj().SetReadOnly( false );

            // Before we had a copper edge clearance setting, the edge line widths could be used
            // as a kludge to control them.  So if there's no setting then infer it from the
            // edge widths.
            if( !loadedBoard->m_LegacyCopperEdgeClearanceLoaded )
            {
                // Do not show the inferred edge clearance warning dialog when loading third
                // party boards.  For some reason the dialog completely hangs all of KiCad and
                // the imported board cannot be saved.
                int edgeClearance = inferLegacyEdgeClearance( loadedBoard, !converted );
                loadedBoard->GetDesignSettings().m_CopperEdgeClearance = edgeClearance;
            }

            // On save; design settings will be removed from the board
            loadedBoard->SetModified();
        }

        // Move legacy view settings to local project settings
        if( !loadedBoard->m_LegacyVisibleLayers.test( Rescue ) )
        {
            Prj().GetLocalSettings().m_VisibleLayers = loadedBoard->m_LegacyVisibleLayers;
            loadedBoard->SetModified();
        }

        if( !loadedBoard->m_LegacyVisibleItems.test( GAL_LAYER_INDEX( GAL_LAYER_ID_BITMASK_END ) ) )
        {
            Prj().GetLocalSettings().m_VisibleItems = loadedBoard->m_LegacyVisibleItems;
            loadedBoard->SetModified();
        }

        if( !loadedBoard->SynchronizeComponentClasses( std::unordered_set<wxString>() ) )
        {
            m_infoBar->RemoveAllButtons();
            m_infoBar->AddCloseButton();
            m_infoBar->ShowMessage( _( "Could not load component class assignment rules" ),
                                    wxICON_WARNING, WX_INFOBAR::MESSAGE_TYPE::GENERIC );
        }

        // we should not ask PCB_IOs to do these items:
        loadedBoard->BuildListOfNets();
        m_toolManager->RunAction( PCB_ACTIONS::repairBoard, true);
        m_toolManager->RunAction( PCB_ACTIONS::rehatchShapes );

        if( loadedBoard->IsModified() )
            OnModify();
        else
            GetScreen()->SetContentModified( false );

        if( ( pluginType == PCB_IO_MGR::LEGACY )
         || ( pluginType == PCB_IO_MGR::KICAD_SEXP
                && loadedBoard->GetFileFormatVersionAtLoad() < SEXPR_BOARD_FILE_VERSION
                && loadedBoard->GetGenerator().Lower() != wxT( "gerbview" ) ) )
        {
            m_infoBar->RemoveAllButtons();
            m_infoBar->AddCloseButton();
            m_infoBar->ShowMessage( _( "This file was created by an older version of KiCad. "
                                       "It will be converted to the new format when saved." ),
                                    wxICON_WARNING, WX_INFOBAR::MESSAGE_TYPE::OUTDATED_SAVE );
        }

        // TODO(JE) library tables -- I think this functionality should be deleted
#if 0

        // Import footprints into a project-specific library
        //==================================================
        // TODO: This should be refactored out of here into somewhere specific to the Project Import
        // E.g. KICAD_MANAGER_FRAME::ImportNonKiCadProject
        if( aCtl & KICTL_IMPORT_LIB )
        {
            wxFileName loadedBoardFn( fullFileName );
            wxString   libNickName = loadedBoardFn.GetName();

            // Extract a footprint library from the design and add it to the fp-lib-table
            // The footprints are saved in a new .pretty library.
            // If this library already exists, all previous footprints will be deleted
            std::vector<FOOTPRINT*> loadedFootprints = pi->GetImportedCachedLibraryFootprints();
            wxString                newLibPath = CreateNewProjectLibrary( _( "New Footprint Library" ),
                                                                          libNickName );

            // Only create the new library if CreateNewLibrary succeeded (note that this fails if
            // the library already exists and the user aborts after seeing the warning message
            // which prompts the user to continue with overwrite or abort)
            if( newLibPath.Length() > 0 )
            {
                IO_RELEASER<PCB_IO> piSexpr( PCB_IO_MGR::FindPlugin( PCB_IO_MGR::KICAD_SEXP ) );

                for( FOOTPRINT* footprint : loadedFootprints )
                {
                    try
                    {
                        if( !footprint->GetFPID().GetLibItemName().empty() ) // Handle old boards.
                        {
                            footprint->SetReference( "REF**" );
                            piSexpr->FootprintSave( newLibPath, footprint );
                            delete footprint;
                        }
                    }
                    catch( const IO_ERROR& ioe )
                    {
                        wxLogError( _( "Error saving footprint %s to project specific library." )
                                    + wxS( "\n%s" ),
                                    footprint->GetFPID().GetUniStringLibItemName(),
                                    ioe.What() );
                    }
                }

                FP_LIB_TABLE*   prjlibtable = PROJECT_PCB::PcbFootprintLibs( &Prj() );
                const wxString& project_env = PROJECT_VAR_NAME;
                wxString        rel_path, env_path;

                wxASSERT_MSG( wxGetEnv( project_env, &env_path ),
                              wxT( "There is no project variable?" ) );

                wxString result( newLibPath );

                if( result.Replace( env_path, wxT( "$(" ) + project_env + wxT( ")" ) ) )
                    rel_path = result;

                FP_LIB_TABLE_ROW* row = new FP_LIB_TABLE_ROW( libNickName, rel_path,
                                                              wxT( "KiCad" ), wxEmptyString );
                prjlibtable->InsertRow( row );

                wxString tblName = Prj().FootprintLibTblName();

                try
                {
                    PROJECT_PCB::PcbFootprintLibs( &Prj() )->Save( tblName );
                }
                catch( const IO_ERROR& ioe )
                {
                    wxLogError( _( "Error saving project specific footprint library table." )
                                + wxS( "\n%s" ),
                                ioe.What() );
                }

                // Update footprint LIB_IDs to point to the just imported library
                for( FOOTPRINT* footprint : GetBoard()->Footprints() )
                {
                    LIB_ID libId = footprint->GetFPID();

                    if( libId.GetLibItemName().empty() )
                        continue;

                    libId.SetLibNickname( libNickName );
                    footprint->SetFPID( libId );
                }
            }
        }
#endif
    }

    {
        wxString fname;

        if( !previousBoardFileName.IsEmpty() && ( aCtl & KICTL_NONKICAD_ONLY ) && !setProject )
        {
            fname = previousBoardFileName;
        }
        else
        {
            wxFileName fn;

            fn.SetPath( Prj().GetProjectPath() );
            fn.SetName( Prj().GetProjectName() );
            fn.SetExt( FILEEXT::KiCadPcbFileExtension );

            fname = fn.GetFullPath();

            fname.Replace( WIN_STRING_DIR_SEP, UNIX_STRING_DIR_SEP );
        }

        GetBoard()->SetFileName( fname );
    }

    // Lock the file newly opened:
    m_file_checker.reset( lock.release() );

    if( !converted )
        UpdateFileHistory( GetBoard()->GetFileName() );

    std::vector<ZONE*> toFill;

    // Rebuild list of nets (full ratsnest rebuild)
    GetBoard()->BuildConnectivity( &progressReporter );

    // Load project settings after setting up board; some of them depend on the nets list
    LoadProjectSettings();
    LoadDrawingSheet();

    // Resolve DRC exclusions after project settings are loaded
    ResolveDRCExclusions( true );

    // Initialise caches used by component classes
    GetBoard()->GetComponentClassManager().RebuildRequiredCaches();

    // Initialise time domain tuning caches
    GetBoard()->GetLengthCalculation()->SynchronizeTuningProfileProperties();

    // Syncs the UI (appearance panel, etc) with the loaded board and project
    OnBoardLoaded();

    // Refresh the 3D view, if any
    EDA_3D_VIEWER_FRAME* draw3DFrame = Get3DViewerFrame();

    if( draw3DFrame )
        draw3DFrame->NewDisplay();
#if 0 && defined(DEBUG)
    // Output the board object tree to stdout, but please run from command prompt:
    GetBoard()->Show( 0, std::cout );
#endif

    // from EDA_APPL which was first loaded BOARD only:
    {
        /* For an obscure reason the focus is lost after loading a board file
         * when starting up the process.
         * (seems due to the recreation of the layer manager after loading the file)
         * Give focus to main window and Drawpanel
         * must be done for these 2 windows (for an obscure reason ...)
         * Linux specific
         * This is more a workaround than a fix.
         */
        SetFocus();
        GetCanvas()->SetFocus();
    }

    if( !setProject )
    {
        // If we didn't reload the project, we still need to call ProjectChanged() to ensure
        // frame-specific initialization happens (like registering the autosave saver).
        // When running under the project manager, KIWAY::ProjectChanged() was called before
        // this frame existed, so we need to call our own ProjectChanged() now.
        ProjectChanged();
    }

    if( KISTATUSBAR* statusBar = dynamic_cast<KISTATUSBAR*>( GetStatusBar() ) )
        statusBar->SetLoadWarningMessages( loadReporter.GetMessages() );

    return true;
}


bool PCB_EDIT_FRAME::SavePcbFile( const wxString& aFileName, bool addToHistory,
                                  bool aChangeProject )
{
    // please, keep it simple.  prompting goes elsewhere.
    wxFileName pcbFileName = aFileName;

    if( pcbFileName.GetExt() == FILEEXT::LegacyPcbFileExtension )
        pcbFileName.SetExt( FILEEXT::KiCadPcbFileExtension );

    // Write through symlinks, don't replace them
    WX_FILENAME::ResolvePossibleSymlinks( pcbFileName );

    if( !IsWritable( pcbFileName ) )
    {
        wxString msg = wxString::Format( _( "Insufficient permissions to write file '%s'." ),
                                         pcbFileName.GetFullPath() );

        DisplayError( this, msg );
        return false;
    }

    // TODO: these will break if we ever go multi-board
    wxFileName projectFile( pcbFileName );
    wxFileName rulesFile( pcbFileName );
    wxString   msg;

    projectFile.SetExt( FILEEXT::ProjectFileExtension );
    rulesFile.SetExt( FILEEXT::DesignRulesFileExtension );

    if( projectFile.FileExists() )
    {
        GetSettingsManager()->SaveProject();
    }
    else if( aChangeProject )
    {
        Prj().SetReadOnly( false );
        GetSettingsManager()->SaveProjectAs( projectFile.GetFullPath() );
    }

    wxFileName currentRules( GetDesignRulesPath() );

    if( currentRules.FileExists() && !rulesFile.FileExists() && aChangeProject )
        KiCopyFile( currentRules.GetFullPath(), rulesFile.GetFullPath(), msg );

    if( !msg.IsEmpty() )
    {
        DisplayError( this, wxString::Format( _( "Error saving custom rules file '%s'." ),
                                              rulesFile.GetFullPath() ) );
    }

    if( projectFile.FileExists() )
    {
        // Save various DRC parameters, such as violation severities (which may have been
        // edited via the DRC dialog as well as the Board Setup dialog), DRC exclusions, etc.
        saveProjectSettings();

        GetBoard()->SynchronizeProperties();
        GetBoard()->SynchronizeNetsAndNetClasses( false );
    }

    wxString   upperTxt;
    wxString   lowerTxt;

    // On Windows, ensure the target file is writeable by clearing problematic attributes like
    // hidden or read-only. This can happen when files are synced via cloud services.
    if( pcbFileName.FileExists() )
        KIPLATFORM::IO::MakeWriteable( pcbFileName.GetFullPath() );

    try
    {
        IO_RELEASER<PCB_IO> pi( PCB_IO_MGR::FindPlugin( PCB_IO_MGR::KICAD_SEXP ) );

        pi->SaveBoard( pcbFileName.GetFullPath(), GetBoard(), nullptr );
    }
    catch( const IO_ERROR& ioe )
    {
        DisplayError( this, wxString::Format( _( "Error saving board file '%s'.\n%s" ),
                                              pcbFileName.GetFullPath(),
                                              ioe.What() ) );
        return false;
    }

    if( !Kiface().IsSingle() )
    {
        WX_STRING_REPORTER backupReporter;

        if( !GetSettingsManager()->TriggerBackupIfNeeded( backupReporter ) )
        {
            upperTxt = backupReporter.GetMessages();
            SetStatusText( upperTxt, 1 );
        }
    }

    GetBoard()->SetFileName( pcbFileName.GetFullPath() );

    // Update the lock in case it was a Save As
    LockFile( pcbFileName.GetFullPath() );

    // Put the saved file in File History if requested
    if( addToHistory )
        UpdateFileHistory( GetBoard()->GetFileName() );

    lowerTxt.Printf( _( "File '%s' saved." ), pcbFileName.GetFullPath() );

    SetStatusText( lowerTxt, 0 );

    // Get rid of the old version conversion warning, or any other dismissable warning :)
    if( m_infoBar->GetMessageType() == WX_INFOBAR::MESSAGE_TYPE::OUTDATED_SAVE )
        m_infoBar->Dismiss();

    if( m_infoBar->IsShownOnScreen() && m_infoBar->HasCloseButton() )
        m_infoBar->Dismiss();

    GetScreen()->SetContentModified( false );
    UpdateTitle();
    UpdateStatusBar();

    // Capture entire project state for PCB save events.
    Kiway().LocalHistory().CommitFullProjectSnapshot( pcbFileName.GetPath(), wxS( "PCB Save" ) );
    Kiway().LocalHistory().TagSave( pcbFileName.GetPath(), wxS( "pcb" ) );

    if( m_autoSaveTimer )
        m_autoSaveTimer->Stop();

    m_autoSavePending = false;
    m_autoSaveRequired = false;
    return true;
}


bool PCB_EDIT_FRAME::SavePcbCopy( const wxString& aFileName, bool aCreateProject, bool aHeadless )
{
    wxFileName pcbFileName( aFileName );

    if( !IsWritable( pcbFileName ) )
    {
        if( !aHeadless )
        {
            DisplayError( this, wxString::Format( _( "Insufficient permissions to write file '%s'." ),
                                                  pcbFileName.GetFullPath() ) );
        }
        return false;
    }

    // Save various DRC parameters, such as violation severities (which may have been
    // edited via the DRC dialog as well as the Board Setup dialog), DRC exclusions, etc.
    SaveProjectLocalSettings();

    GetBoard()->SynchronizeNetsAndNetClasses( false );

    // On Windows, ensure the target file is writeable by clearing problematic attributes like
    // hidden or read-only. This can happen when files are synced via cloud services.
    if( pcbFileName.FileExists() )
        KIPLATFORM::IO::MakeWriteable( pcbFileName.GetFullPath() );

    try
    {
        IO_RELEASER<PCB_IO> pi( PCB_IO_MGR::FindPlugin( PCB_IO_MGR::KICAD_SEXP ) );

        wxASSERT( pcbFileName.IsAbsolute() );

        pi->SaveBoard( pcbFileName.GetFullPath(), GetBoard(), nullptr );
    }
    catch( const IO_ERROR& ioe )
    {
        if( !aHeadless )
        {
            DisplayError( this, wxString::Format( _( "Error saving board file '%s'.\n%s" ),
                                                  pcbFileName.GetFullPath(),
                                                  ioe.What() ) );
        }

        return false;
    }

    wxFileName projectFile( pcbFileName );
    wxFileName rulesFile( pcbFileName );
    wxString   msg;

    projectFile.SetExt( FILEEXT::ProjectFileExtension );
    rulesFile.SetExt( FILEEXT::DesignRulesFileExtension );

    if( aCreateProject && !projectFile.FileExists() )
        GetSettingsManager()->SaveProjectCopy( projectFile.GetFullPath() );

    wxFileName currentRules( GetDesignRulesPath() );

    if( aCreateProject && currentRules.FileExists() && !rulesFile.FileExists() )
        KiCopyFile( currentRules.GetFullPath(), rulesFile.GetFullPath(), msg );

    if( !msg.IsEmpty() && !aHeadless )
    {
        DisplayError( this, wxString::Format( _( "Error saving custom rules file '%s'." ),
                                              rulesFile.GetFullPath() ) );
    }

    return true;
}


bool PCB_EDIT_FRAME::importFile( const wxString& aFileName, int aFileType,
                                 const std::map<std::string, UTF8>* aProperties )
{
    NULLER raiiNuller( (void*&) m_importProperties );

    m_importProperties = aProperties;

    switch( (PCB_IO_MGR::PCB_FILE_T) aFileType )
    {
    case PCB_IO_MGR::CADSTAR_PCB_ARCHIVE:
    case PCB_IO_MGR::EAGLE:
    case PCB_IO_MGR::EASYEDA:
    case PCB_IO_MGR::EASYEDAPRO:
        return OpenProjectFiles( std::vector<wxString>( 1, aFileName ), KICTL_NONKICAD_ONLY | KICTL_IMPORT_LIB );

    case PCB_IO_MGR::ALTIUM_DESIGNER:
    case PCB_IO_MGR::ALTIUM_CIRCUIT_MAKER:
    case PCB_IO_MGR::ALTIUM_CIRCUIT_STUDIO:
    case PCB_IO_MGR::SOLIDWORKS_PCB:
        return OpenProjectFiles( std::vector<wxString>( 1, aFileName ), KICTL_NONKICAD_ONLY );

    default:
        return false;
    }
}


int BOARD_EDITOR_CONTROL::GenIPC2581File( const TOOL_EVENT& aEvent )
{
    DIALOG_EXPORT_2581 dlg( m_frame );

    dlg.ShowModal();

    return 0;
}


int BOARD_EDITOR_CONTROL::GenerateODBPPFiles( const TOOL_EVENT& aEvent )
{
    DIALOG_EXPORT_ODBPP dlg( m_frame );

    if( dlg.ShowModal() != wxID_OK )
        return 0;

    JOB_EXPORT_PCB_ODB job;

    job.SetConfiguredOutputPath( dlg.GetOutputPath() );
    job.m_filename = m_frame->GetBoard()->GetFileName();
    job.m_compressionMode = static_cast<JOB_EXPORT_PCB_ODB::ODB_COMPRESSION>( dlg.GetCompressFormat() );

    job.m_precision = dlg.GetPrecision();
    job.m_units = dlg.GetUnitsString() == "mm" ? JOB_EXPORT_PCB_ODB::ODB_UNITS::MM
                                               : JOB_EXPORT_PCB_ODB::ODB_UNITS::INCH;

    WX_PROGRESS_REPORTER progressReporter( m_frame, _( "Generate ODB++ Files" ), 3, PR_CAN_ABORT );
    WX_STRING_REPORTER reporter;

    DIALOG_EXPORT_ODBPP::GenerateODBPPFiles( job, m_frame->GetBoard(), m_frame, &progressReporter, &reporter );

    if( reporter.HasMessage() )
        DisplayError( m_frame, reporter.GetMessages() );

    return 0;
}
