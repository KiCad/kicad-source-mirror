/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2023 CERN (www.cern.ch)
 * Copyright (C) 2016-2024 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <confirm.h>
#include <core/arraydim.h>
#include <core/thread_pool.h>
#include <dialog_HTML_reporter_base.h>
#include <gestfich.h>
#include <pcb_edit_frame.h>
#include <board_design_settings.h>
#include <3d_viewer/eda_3d_viewer_frame.h>
#include <fp_lib_table.h>
#include <kiface_base.h>
#include <macros.h>
#include <trace_helpers.h>
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
#include <string_utf8_map.h>
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
#include <dialogs/dialog_imported_layers.h>
#include <dialogs/dialog_import_choose_project.h>
#include <tools/pcb_actions.h>
#include "footprint_info_impl.h"
#include <board_commit.h>
#include <zone_filler.h>
#include <widgets/filedlg_import_non_kicad.h>
#include <widgets/wx_html_report_box.h>
#include <wx_filename.h>  // For ::ResolvePossibleSymlinks()

#include <kiplatform/io.h>

#include <wx/stdpaths.h>
#include <wx/ffile.h>
#include <wx/filedlg.h>
#include <wx/txtstrm.h>
#include <wx/wfstream.h>
#include <wx/zipstrm.h>

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

        if( desc.m_FileExtensions.empty() )
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

    wxFileDialog dlg( aParent,
                      kicadFormat ? _( "Open Board File" ) : _( "Import Non KiCad Board File" ),
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
    wxString fn = GetFileFromHistory( event.GetId(), _( "Printed circuit board" ) );

    if( !!fn )
    {
        if( !wxFileName::IsFileReadable( fn ) )
        {
            if( !AskLoadBoardFileName( this, &fn, KICTL_KICAD_ONLY ) )
                return;
        }

        OpenProjectFiles( std::vector<wxString>( 1, fn ), KICTL_KICAD_ONLY );
    }
}


void PCB_EDIT_FRAME::OnClearFileHistory( wxCommandEvent& aEvent )
{
    ClearFileHistory();
}


void PCB_EDIT_FRAME::Files_io( wxCommandEvent& event )
{
    int        id = event.GetId();
    Files_io_from_id( id );
}


bool PCB_EDIT_FRAME::Files_io_from_id( int id )
{
    wxString   msg;

    switch( id )
    {
    case ID_LOAD_FILE:
    {
        // Only standalone mode can directly load a new document
        if( !Kiface().IsSingle() )
            return false;

        int      open_ctl = KICTL_KICAD_ONLY;
        wxString fileName = Prj().AbsolutePath( GetBoard()->GetFileName() );

        return AskLoadBoardFileName( this, &fileName, open_ctl )
               && OpenProjectFiles( std::vector<wxString>( 1, fileName ), open_ctl );
    }

    case ID_IMPORT_NON_KICAD_BOARD:
    {
        // Note: we explicitly allow this even if not in standalone mode for now, even though it is dangerous.
        int      open_ctl = KICTL_NONKICAD_ONLY;
        wxString fileName; // = Prj().AbsolutePath( GetBoard()->GetFileName() );

        return AskLoadBoardFileName( this, &fileName, open_ctl )
               && OpenProjectFiles( std::vector<wxString>( 1, fileName ), open_ctl );
    }

    case ID_MENU_RECOVER_BOARD_AUTOSAVE:
    {
        wxFileName currfn = Prj().AbsolutePath( GetBoard()->GetFileName() );
        wxFileName fn = currfn;

        wxString rec_name = GetAutoSaveFilePrefix() + fn.GetName();
        fn.SetName( rec_name );

        if( !fn.FileExists() )
        {
            msg.Printf( _( "Recovery file '%s' not found." ), fn.GetFullPath() );
            DisplayInfoMessage( this, msg );
            return false;
        }

        msg.Printf( _( "OK to load recovery file '%s'?" ), fn.GetFullPath() );

        if( !IsOK( this, msg ) )
            return false;

        GetScreen()->SetContentModified( false );    // do not prompt the user for changes

        if( OpenProjectFiles( std::vector<wxString>( 1, fn.GetFullPath() ) ) )
        {
            // Re-set the name since name or extension was changed
            GetBoard()->SetFileName( currfn.GetFullPath() );
            UpdateTitle();
            return true;
        }

        return false;
    }

    case ID_REVERT_BOARD:
    {
        wxFileName fn = Prj().AbsolutePath( GetBoard()->GetFileName() );

        msg.Printf( _( "Revert '%s' to last version saved?" ), fn.GetFullPath() );

        if( !IsOK( this, msg ) )
            return false;

        GetScreen()->SetContentModified( false );    // do not prompt the user for changes

        ReleaseFile();

        return OpenProjectFiles( std::vector<wxString>( 1, fn.GetFullPath() ), KICTL_REVERT );
    }

    case ID_NEW_BOARD:
    {
        // Only standalone mode can directly load a new document
        if( !Kiface().IsSingle() )
            return false;

        if( IsContentModified() )
        {
            wxFileName fileName = GetBoard()->GetFileName();
            wxString   saveMsg = _( "Current board will be closed, save changes to '%s' before "
                                    "continuing?" );

            if( !HandleUnsavedChanges( this, wxString::Format( saveMsg, fileName.GetFullName() ),
                                       [&]()->bool
                                       {
                                           return Files_io_from_id( ID_SAVE_BOARD );
                                       } ) )
            {
                return false;
            }
        }
        else if( !GetBoard()->IsEmpty() )
        {
            if( !IsOK( this, _( "Current Board will be closed. Continue?" ) ) )
                return false;
        }

        SaveProjectLocalSettings();

        GetBoard()->ClearProject();

        SETTINGS_MANAGER* mgr = GetSettingsManager();
        mgr->UnloadProject( &mgr->Prj() );

        if( !Clear_Pcb( false ) )
            return false;

        LoadProjectSettings();

        onBoardLoaded();

        OnModify();
        return true;
    }

    case ID_SAVE_BOARD:
        if( !GetBoard()->GetFileName().IsEmpty() )
        {
            if( SavePcbFile( Prj().AbsolutePath( GetBoard()->GetFileName() ) ) )
            {
                m_autoSaveRequired = false;
                return true;
            }

            return false;
        }

        KI_FALLTHROUGH;

    case ID_COPY_BOARD_AS:
    case ID_SAVE_BOARD_AS:
    {
        bool     addToHistory = ( id == ID_SAVE_BOARD_AS );
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
            if( id == ID_COPY_BOARD_AS )
            {
                success = SavePcbCopy( filename, createProject );
            }
            else
            {
                success = SavePcbFile( filename, addToHistory, createProject );

                if( success )
                    m_autoSaveRequired = false;
            }
        }

        return success;
    }

    default:
        return false;
    }
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
                               "Rules > Constraints).\nThis may result in different fills "
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
        UTF8 msg = StrPrintf( "Pcbnew:%s() takes a single filename", __func__ );
        DisplayError( this, msg );
        return false;
    }

    wxString   fullFileName( aFileSet[0] );
    wxFileName wx_filename( fullFileName );
    wxString   msg;

    if( Kiface().IsSingle() )
        KIPLATFORM::APP::RegisterApplicationRestart( fullFileName );

    // We insist on caller sending us an absolute path, if it does not, we say it's a bug.
    wxASSERT_MSG( wx_filename.IsAbsolute(), wxT( "Path is not absolute!" ) );

    std::unique_ptr<LOCKFILE> lock = std::make_unique<LOCKFILE>( fullFileName );

    if( !lock->Valid() && lock->IsLockedByMe() )
    {
        // If we cannot acquire the lock but we appear to be the one who
        // locked it, check to see if there is another KiCad instance running.
        // If there is not, then we can override the lock.  This could happen if
        // KiCad crashed or was interrupted

        if( !Pgm().SingleInstance()->IsAnotherRunning() )
            lock->OverrideLock();
    }

    if( !lock->Valid() )
    {
        msg.Printf( _( "PCB '%s' is already open by '%s' at '%s'." ), wx_filename.GetFullName(),
                lock->GetUsername(), lock->GetHostname() );

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

    // If its a non-existent schematic and caller thinks it exists
    if( is_new && !( aCtl & KICTL_CREATE ) )
    {
        // notify user that fullFileName does not exist, ask if user wants to create it.
        msg.Printf( _( "PCB '%s' does not exist. Do you wish to create it?" ), fullFileName );

        if( !IsOK( this, msg ) )
            return false;
    }

    // Get rid of any existing warnings about the old board
    GetInfoBar()->Dismiss();

    WX_PROGRESS_REPORTER progressReporter( this, is_new ? _( "Creating PCB" )
                                                        : _( "Loading PCB" ), 1 );

    // No save prompt (we already prompted above), and only reset to a new blank board if new
    Clear_Pcb( false, !is_new );

    PCB_IO_MGR::PCB_FILE_T pluginType = PCB_IO_MGR::KICAD_SEXP;

    if( !is_new )
        pluginType = PCB_IO_MGR::FindPluginTypeFromBoardPath( fullFileName, aCtl );

    if( pluginType == PCB_IO_MGR::FILE_TYPE_NONE )
        return false;

    bool converted =  pluginType != PCB_IO_MGR::LEGACY && pluginType != PCB_IO_MGR::KICAD_SEXP;

    // Loading a project should only be done under carefully considered circumstances.

    // The calling code should know not to ask me here to change projects unless
    // it knows what consequences that will have on other KIFACEs running and using
    // this same PROJECT.  It can be very harmful if that calling code is stupid.
    SETTINGS_MANAGER* mgr = GetSettingsManager();

    if( pro.GetFullPath() != mgr->Prj().GetProjectFullName() )
    {
        // calls SaveProject
        SaveProjectLocalSettings();

        GetBoard()->ClearProject();
        mgr->UnloadProject( &mgr->Prj() );

        mgr->LoadProject( pro.GetFullPath() );

        // Do not allow saving a project if one doesn't exist.  This normally happens if we are
        // standalone and opening a board that has been moved from its project folder.
        // For converted projects, we don't want to set the read-only flag because we want a project
        // to be saved for the new file in case things like netclasses got migrated.
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
        IO_RELEASER<PCB_IO> pi( PCB_IO_MGR::PluginFind( pluginType ) );

        LAYER_REMAPPABLE_PLUGIN* layerRemappableIO = dynamic_cast<LAYER_REMAPPABLE_PLUGIN*>( pi.get() );

        if( layerRemappableIO )
        {
            layerRemappableIO->RegisterLayerMappingCallback(
                    std::bind( DIALOG_IMPORTED_LAYERS::GetMapModal, this, std::placeholders::_1 ) );
        }

        PROJECT_CHOOSER_PLUGIN* projectChooserIO = dynamic_cast<PROJECT_CHOOSER_PLUGIN*>( pi.get() );

        if( projectChooserIO )
        {
            projectChooserIO->RegisterChooseProjectCallback(
                    std::bind( DIALOG_IMPORT_CHOOSE_PROJECT::GetSelectionsModal, this,
                               std::placeholders::_1 ) );
        }

        if( ( aCtl & KICTL_REVERT ) )
        {
            DeleteAutoSaveFile( fullFileName );
        }
        else
        {
            // This will rename the file if there is an autosave and the user wants to recover
            CheckForAutoSaveFile( fullFileName );
        }

        DIALOG_HTML_REPORTER errorReporter( this );
        bool failedLoad = false;

        try
        {
            if( pi == nullptr )
            {
                // There was no plugin found, e.g. due to invalid file extension, file header,...
                THROW_IO_ERROR( _( "File format is not supported" ) );
            }

            STRING_UTF8_MAP props;

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
            if( config()->m_System.show_import_issues )
                pi->SetReporter( errorReporter.m_Reporter );
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

            return false;
        }

        // This fixes a focus issue after the progress reporter is done on GTK.  It shouldn't
        // cause any issues on macOS and Windows.  If it does, it will have to be conditionally
        // compiled.
        Raise();

        if( errorReporter.m_Reporter->HasMessage() )
        {
            errorReporter.m_Reporter->Flush(); // Build HTML messages
            errorReporter.ShowModal();
        }

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

        // we should not ask PCB_IOs to do these items:
        loadedBoard->BuildListOfNets();
        ResolveDRCExclusions( true );
        m_toolManager->RunAction( PCB_ACTIONS::repairBoard, true);

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
            wxString                newLibPath = CreateNewProjectLibrary( libNickName );

            // Only create the new library if CreateNewLibrary succeeded (note that this fails if
            // the library already exists and the user aborts after seeing the warning message
            // which prompts the user to continue with overwrite or abort)
            if( newLibPath.Length() > 0 )
            {
                IO_RELEASER<PCB_IO> piSexpr( PCB_IO_MGR::PluginFind( PCB_IO_MGR::KICAD_SEXP ) );

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
    }

    {
        wxFileName fn = fullFileName;

        if( converted )
            fn.SetExt( FILEEXT::PcbFileExtension );

        wxString fname = fn.GetFullPath();

        fname.Replace( WIN_STRING_DIR_SEP, UNIX_STRING_DIR_SEP );

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

    // Syncs the UI (appearance panel, etc) with the loaded board and project
    onBoardLoaded();

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

    wxString   tempFile = wxFileName::CreateTempFileName( wxS( "pcbnew" ) );
    wxString   upperTxt;
    wxString   lowerTxt;

    try
    {
        IO_RELEASER<PCB_IO> pi( PCB_IO_MGR::PluginFind( PCB_IO_MGR::KICAD_SEXP ) );

        pi->SaveBoard( tempFile, GetBoard(), nullptr );
    }
    catch( const IO_ERROR& ioe )
    {
        DisplayError( this, wxString::Format( _( "Error saving board file '%s'.\n%s" ),
                                              pcbFileName.GetFullPath(),
                                              ioe.What() ) );

        lowerTxt.Printf( _( "Failed to create temporary file '%s'." ), tempFile );

        SetMsgPanel( upperTxt, lowerTxt );

        // In case we started a file but didn't fully write it, clean up
        wxRemoveFile( tempFile );

        return false;
    }

    // Preserve the permissions of the current file
    KIPLATFORM::IO::DuplicatePermissions( pcbFileName.GetFullPath(), tempFile );

    // If save succeeded, replace the original with what we just wrote
    if( !wxRenameFile( tempFile, pcbFileName.GetFullPath() ) )
    {
        DisplayError( this, wxString::Format( _( "Error saving board file '%s'.\n"
                                                 "Failed to rename temporary file '%s." ),
                                              pcbFileName.GetFullPath(),
                                              tempFile ) );

        lowerTxt.Printf( _( "Failed to rename temporary file '%s'." ),
                         tempFile );

        SetMsgPanel( upperTxt, lowerTxt );

        return false;
    }

    if( !Kiface().IsSingle() )
    {
        WX_STRING_REPORTER backupReporter;

        if( !GetSettingsManager()->TriggerBackupIfNeeded( backupReporter ) )
            upperTxt = backupReporter.GetMessages();
    }

    GetBoard()->SetFileName( pcbFileName.GetFullPath() );

    // Update the lock in case it was a Save As
    LockFile( pcbFileName.GetFullPath() );

    // Put the saved file in File History if requested
    if( addToHistory )
        UpdateFileHistory( GetBoard()->GetFileName() );

    // Delete auto save file on successful save.
    wxFileName autoSaveFileName = pcbFileName;

    autoSaveFileName.SetName( GetAutoSaveFilePrefix() + pcbFileName.GetName() );

    if( autoSaveFileName.FileExists() )
        wxRemoveFile( autoSaveFileName.GetFullPath() );

    lowerTxt.Printf( _( "File '%s' saved." ), pcbFileName.GetFullPath() );

    SetStatusText( lowerTxt, 0 );

    // Get rid of the old version conversion warning, or any other dismissable warning :)
    if( m_infoBar->GetMessageType() == WX_INFOBAR::MESSAGE_TYPE::OUTDATED_SAVE )
        m_infoBar->Dismiss();

    if( m_infoBar->IsShownOnScreen() && m_infoBar->HasCloseButton() )
        m_infoBar->Dismiss();

    GetScreen()->SetContentModified( false );
    UpdateTitle();
    return true;
}


bool PCB_EDIT_FRAME::SavePcbCopy( const wxString& aFileName, bool aCreateProject )
{
    wxFileName pcbFileName( EnsureFileExtension( aFileName, FILEEXT::KiCadPcbFileExtension ) );

    if( !IsWritable( pcbFileName ) )
    {
        DisplayError( this, wxString::Format( _( "Insufficient permissions to write file '%s'." ),
                                              pcbFileName.GetFullPath() ) );
        return false;
    }

    // Save various DRC parameters, such as violation severities (which may have been
    // edited via the DRC dialog as well as the Board Setup dialog), DRC exclusions, etc.
    SaveProjectLocalSettings();

    GetBoard()->SynchronizeNetsAndNetClasses( false );

    try
    {
        IO_RELEASER<PCB_IO> pi( PCB_IO_MGR::PluginFind( PCB_IO_MGR::KICAD_SEXP ) );

        wxASSERT( pcbFileName.IsAbsolute() );

        pi->SaveBoard( pcbFileName.GetFullPath(), GetBoard(), nullptr );
    }
    catch( const IO_ERROR& ioe )
    {
        DisplayError( this, wxString::Format( _( "Error saving board file '%s'.\n%s" ),
                                              pcbFileName.GetFullPath(),
                                              ioe.What() ) );

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

    if( !msg.IsEmpty() )
    {
        DisplayError( this, wxString::Format( _( "Error saving custom rules file '%s'." ),
                                              rulesFile.GetFullPath() ) );
    }

    DisplayInfoMessage( this, wxString::Format( _( "Board copied to:\n%s" ),
                                                pcbFileName.GetFullPath() ) );

    return true;
}


bool PCB_EDIT_FRAME::doAutoSave()
{
    wxFileName tmpFileName;

    // Don't run autosave if content has not been modified
    if( !IsContentModified() )
        return true;

    wxString title = GetTitle();    // Save frame title, that can be modified by the save process

    if( GetBoard()->GetFileName().IsEmpty() )
    {
        tmpFileName = wxFileName( PATHS::GetDefaultUserProjectsPath(), NAMELESS_PROJECT,
                                  FILEEXT::KiCadPcbFileExtension );
        GetBoard()->SetFileName( tmpFileName.GetFullPath() );
    }
    else
    {
        tmpFileName = Prj().AbsolutePath( GetBoard()->GetFileName() );
    }

    wxFileName autoSaveFileName = tmpFileName;

    // Auto save file name is the board file name prepended with autosaveFilePrefix string.
    autoSaveFileName.SetName( GetAutoSaveFilePrefix() + autoSaveFileName.GetName() );

    if( !autoSaveFileName.IsOk() )
        return false;

    // If the board file path is not writable, try writing to a platform specific temp file
    // path.  If that path isn't writable, give up.
    if( !autoSaveFileName.IsDirWritable() )
    {
        autoSaveFileName.SetPath( wxFileName::GetTempDir() );

        if( !autoSaveFileName.IsOk() || !autoSaveFileName.IsDirWritable() )
            return false;
    }

    wxLogTrace( traceAutoSave,
                wxT( "Creating auto save file <" ) + autoSaveFileName.GetFullPath() + wxT( ">" ) );

    if( SavePcbFile( autoSaveFileName.GetFullPath(), false, false ) )
    {
        GetScreen()->SetContentModified();
        GetBoard()->SetFileName( tmpFileName.GetFullPath() );
        UpdateTitle();
        m_autoSaveRequired = false;
        m_autoSavePending = false;

        if( !Kiface().IsSingle() &&
            GetSettingsManager()->GetCommonSettings()->m_Backup.backup_on_autosave )
        {
            GetSettingsManager()->TriggerBackupIfNeeded( NULL_REPORTER::GetInstance() );
        }

        SetTitle( title );      // Restore initial frame title

        return true;
    }

    GetBoard()->SetFileName( tmpFileName.GetFullPath() );

    SetTitle( title );      // Restore initial frame title

    return false;
}


bool PCB_EDIT_FRAME::importFile( const wxString& aFileName, int aFileType,
                                 const STRING_UTF8_MAP* aProperties )
{
    m_importProperties = aProperties;

    switch( (PCB_IO_MGR::PCB_FILE_T) aFileType )
    {
    case PCB_IO_MGR::CADSTAR_PCB_ARCHIVE:
    case PCB_IO_MGR::EAGLE:
    case PCB_IO_MGR::EASYEDA:
    case PCB_IO_MGR::EASYEDAPRO:
        return OpenProjectFiles( std::vector<wxString>( 1, aFileName ),
                                 KICTL_NONKICAD_ONLY | KICTL_IMPORT_LIB );

    default: break;
    }

    m_importProperties = nullptr;

    return false;
}


void PCB_EDIT_FRAME::GenIPC2581File( wxCommandEvent& event )
{
    DIALOG_EXPORT_2581 dlg( this );

    if( dlg.ShowModal() != wxID_OK )
        return;

    wxFileName pcbFileName = dlg.GetOutputPath();

    // Write through symlinks, don't replace them
    WX_FILENAME::ResolvePossibleSymlinks( pcbFileName );

    if( pcbFileName.GetName().empty() )
    {
        DisplayError( this, _( "The board must be saved before generating IPC2581 file." ) );
        return;
    }

    if( !IsWritable( pcbFileName ) )
    {
        wxString msg = wxString::Format( _( "Insufficient permissions to write file '%s'." ),
                                         pcbFileName.GetFullPath() );

        DisplayError( this, msg );
        return;
    }

    wxString   tempFile = wxFileName::CreateTempFileName( wxS( "pcbnew_ipc" ) );
    wxString   upperTxt;
    wxString   lowerTxt;
    WX_PROGRESS_REPORTER reporter( this, _( "Generating IPC2581 file" ), 5 );
    STRING_UTF8_MAP props;

    props["units"] = dlg.GetUnitsString();
    props["sigfig"] = dlg.GetPrecision();
    props["version"] = dlg.GetVersion();
    props["OEMRef"] = dlg.GetOEM();
    props["mpn"] = dlg.GetMPN();
    props["mfg"] = dlg.GetMfg();
    props["dist"] = dlg.GetDist();
    props["distpn"] = dlg.GetDistPN();

    auto saveFile = [&]() -> bool
    {
        try
        {
            IO_RELEASER<PCB_IO> pi( PCB_IO_MGR::PluginFind( PCB_IO_MGR::IPC2581 ) );
            pi->SetProgressReporter( &reporter );
            pi->SaveBoard( tempFile, GetBoard(), &props );
            return true;
        }
        catch( const IO_ERROR& ioe )
        {
            DisplayError( this, wxString::Format( _( "Error generating IPC2581 file '%s'.\n%s" ),
                                                  pcbFileName.GetFullPath(), ioe.What() ) );

            lowerTxt.Printf( _( "Failed to create temporary file '%s'." ), tempFile );

            SetMsgPanel( upperTxt, lowerTxt );

            // In case we started a file but didn't fully write it, clean up
            wxRemoveFile( tempFile );

            return false;
        }
    };

    thread_pool& tp = GetKiCadThreadPool();
    auto ret = tp.submit( saveFile );


    std::future_status status = ret.wait_for( std::chrono::milliseconds( 250 ) );

    while( status != std::future_status::ready )
    {
        reporter.KeepRefreshing();
        status = ret.wait_for( std::chrono::milliseconds( 250 ) );
    }

    try
    {
        if( !ret.get() )
            return;
    }
    catch(const std::exception& e)
    {
        wxLogError( "Exception in IPC2581 generation: %s", e.what() );
        GetScreen()->SetContentModified( false );
        return;
    }

    // Preserve the permissions of the current file
    KIPLATFORM::IO::DuplicatePermissions( pcbFileName.GetFullPath(), tempFile );

    if( dlg.GetCompress() )
    {
        wxFileName tempfn = pcbFileName;
        tempfn.SetExt( FILEEXT::Ipc2581FileExtension );
        wxFileName zipfn = tempFile;
        zipfn.SetExt( "zip" );

        wxFFileOutputStream fnout( zipfn.GetFullPath() );
        wxZipOutputStream zip( fnout );
        wxFFileInputStream fnin( tempFile );

        zip.PutNextEntry( tempfn.GetFullName() );
        fnin.Read( zip );
        zip.Close();
        fnout.Close();

        wxRemoveFile( tempFile );
        tempFile = zipfn.GetFullPath();
    }

    // If save succeeded, replace the original with what we just wrote
    if( !wxRenameFile( tempFile, pcbFileName.GetFullPath() ) )
    {
        DisplayError( this, wxString::Format( _( "Error generating IPC2581 file '%s'.\n"
                                                 "Failed to rename temporary file '%s." ),
                                              pcbFileName.GetFullPath(),
                                              tempFile ) );

        lowerTxt.Printf( _( "Failed to rename temporary file '%s'." ),
                         tempFile );

        SetMsgPanel( upperTxt, lowerTxt );
    }

    GetScreen()->SetContentModified( false );
}
