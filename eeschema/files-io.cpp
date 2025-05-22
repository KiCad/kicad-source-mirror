/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2013 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2013-2023 CERN (www.cern.ch)
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

#include <symbol_library.h>
#include <confirm.h>
#include <common.h>
#include <connection_graph.h>
#include <dialog_migrate_buses.h>
#include <dialog_symbol_remap.h>
#include <dialog_import_choose_project.h>
#include <eeschema_settings.h>
#include <id.h>
#include <kiface_base.h>
#include <kiplatform/app.h>
#include <lockfile.h>
#include <pgm_base.h>
#include <core/profile.h>
#include <project/project_file.h>
#include <project_rescue.h>
#include <project_sch.h>
#include <dialog_HTML_reporter_base.h>
#include <io/common/plugin_common_choose_project.h>
#include <reporter.h>
#include <richio.h>
#include <sch_bus_entry.h>
#include <sch_commit.h>
#include <sch_edit_frame.h>
#include <sch_io/kicad_legacy/sch_io_kicad_legacy.h>
#include <sch_file_versions.h>
#include <sch_line.h>
#include <sch_sheet.h>
#include <sch_sheet_path.h>
#include <schematic.h>
#include <settings/settings_manager.h>
#include <sim/simulator_frame.h>
#include <tool/actions.h>
#include <tool/tool_manager.h>
#include <tools/sch_editor_control.h>
#include <tools/sch_navigate_tool.h>
#include <trace_helpers.h>
#include <widgets/filedlg_import_non_kicad.h>
#include <widgets/wx_infobar.h>
#include <wildcards_and_files_ext.h>
#include <drawing_sheet/ds_data_model.h>
#include <wx/app.h>
#include <wx/ffile.h>
#include <wx/filedlg.h>
#include <wx/log.h>
#include <wx/richmsgdlg.h>
#include <wx/stdpaths.h>
#include <tools/sch_inspection_tool.h>
#include <tools/sch_selection_tool.h>
#include <paths.h>
#include <wx_filename.h>  // For ::ResolvePossibleSymlinks
#include <widgets/wx_progress_reporters.h>
#include <widgets/wx_html_report_box.h>

#include <kiplatform/io.h>

#include "widgets/filedlg_hook_save_project.h"

bool SCH_EDIT_FRAME::OpenProjectFiles( const std::vector<wxString>& aFileSet, int aCtl )
{
    // ensure the splash screen does not obscure any dialog at startup
    Pgm().HideSplash();

    // implement the pseudo code from KIWAY_PLAYER.h:
    wxString msg;

    EESCHEMA_SETTINGS* cfg = dynamic_cast<EESCHEMA_SETTINGS*>( Kiface().KifaceSettings() );

    // This is for python:
    if( aFileSet.size() != 1 )
    {
        msg.Printf( "Eeschema:%s() takes only a single filename.", __WXFUNCTION__ );
        DisplayError( this, msg );
        return false;
    }

    wxString   fullFileName( aFileSet[0] );
    wxFileName wx_filename( fullFileName );

    // We insist on caller sending us an absolute path, if it does not, we say it's a bug.
    wxASSERT_MSG( wx_filename.IsAbsolute(), wxS( "Path is not absolute!" ) );

    if( !LockFile( fullFileName ) )
    {
        msg.Printf( _( "Schematic '%s' is already open by '%s' at '%s'." ), fullFileName,
                m_file_checker->GetUsername(), m_file_checker->GetHostname() );

        if( !AskOverrideLock( this, msg ) )
            return false;

        m_file_checker->OverrideLock();
    }

    if( !AskToSaveChanges() )
        return false;

#ifdef PROFILE
    PROF_TIMER openFiles( "OpenProjectFile" );
#endif

    wxFileName pro = fullFileName;
    pro.SetExt( FILEEXT::ProjectFileExtension );

    bool is_new = !wxFileName::IsFileReadable( fullFileName );

    // If its a non-existent schematic and caller thinks it exists
    if( is_new && !( aCtl & KICTL_CREATE ) )
    {
        // notify user that fullFileName does not exist, ask if user wants to create it.
        msg.Printf( _( "Schematic '%s' does not exist.  Do you wish to create it?" ),
                    fullFileName );

        if( !IsOK( this, msg ) )
            return false;
    }

    wxCommandEvent e( EDA_EVT_SCHEMATIC_CHANGING );
    ProcessEventLocally( e );

    // unload current project file before loading new
    {
        ClearUndoRedoList();
        ClearRepeatItemsList();
        SetScreen( nullptr );
        m_toolManager->GetTool<SCH_INSPECTION_TOOL>()->Reset( TOOL_BASE::SUPERMODEL_RELOAD );
        CreateScreens();
    }

    SetStatusText( wxEmptyString );
    m_infoBar->Dismiss();

    WX_PROGRESS_REPORTER progressReporter( this, is_new ? _( "Creating Schematic" )
                                                        : _( "Loading Schematic" ), 1 );

    bool differentProject = pro.GetFullPath() != Prj().GetProjectFullName();

    if( differentProject )
    {
        if( !Prj().IsNullProject() )
        {
            SaveProjectLocalSettings();
            GetSettingsManager()->SaveProject();
        }

        Schematic().SetProject( nullptr );
        GetSettingsManager()->UnloadProject( &Prj(), false );

        GetSettingsManager()->LoadProject( pro.GetFullPath() );

        wxFileName legacyPro( pro );
        legacyPro.SetExt( FILEEXT::LegacyProjectFileExtension );

        // Do not allow saving a project if one doesn't exist.  This normally happens if we are
        // standalone and opening a schematic that has been moved from its project folder.
        if( !pro.Exists() && !legacyPro.Exists() && !( aCtl & KICTL_CREATE ) )
            Prj().SetReadOnly();

        CreateScreens();
    }

    SCH_IO_MGR::SCH_FILE_T schFileType = SCH_IO_MGR::GuessPluginTypeFromSchPath( fullFileName,
                                                                                 KICTL_KICAD_ONLY );

    if( schFileType == SCH_IO_MGR::SCH_LEGACY )
    {
        // Don't reload the symbol libraries if we are just launching Eeschema from KiCad again.
        // They are already saved in the kiface project object.
        if( differentProject || !Prj().GetElem( PROJECT::ELEM::SCH_SYMBOL_LIBS ) )
        {
            // load the libraries here, not in SCH_SCREEN::Draw() which is a context
            // that will not tolerate DisplayError() dialog since we're already in an
            // event handler in there.
            // And when a schematic file is loaded, we need these libs to initialize
            // some parameters (links to PART LIB, dangling ends ...)
            Prj().SetElem( PROJECT::ELEM::SCH_SYMBOL_LIBS, nullptr );
            PROJECT_SCH::SchLibs( &Prj() );
        }
    }
    else
    {
        // No legacy symbol libraries including the cache are loaded with the new file format.
        Prj().SetElem( PROJECT::ELEM::SCH_SYMBOL_LIBS, nullptr );
    }

    // Load the symbol library table, this will be used forever more.
    Prj().SetElem( PROJECT::ELEM::SYMBOL_LIB_TABLE, nullptr );
    PROJECT_SCH::SchSymbolLibTable( &Prj() );

    // Load project settings after schematic has been set up with the project link, since this will
    // update some of the needed schematic settings such as drawing defaults
    LoadProjectSettings();

    wxFileName rfn( GetCurrentFileName() );
    rfn.MakeRelativeTo( Prj().GetProjectPath() );
    LoadWindowState( rfn.GetFullPath() );

    KIPLATFORM::APP::SetShutdownBlockReason( this, _( "Schematic file changes are unsaved" ) );

    if( Kiface().IsSingle() )
    {
        KIPLATFORM::APP::RegisterApplicationRestart( fullFileName );
    }

    if( is_new || schFileType == SCH_IO_MGR::SCH_FILE_T::SCH_FILE_UNKNOWN )
    {
        // mark new, unsaved file as modified.
        GetScreen()->SetContentModified();
        GetScreen()->SetFileName( fullFileName );

        if( schFileType == SCH_IO_MGR::SCH_FILE_T::SCH_FILE_UNKNOWN )
        {
            msg.Printf( _( "'%s' is not a KiCad schematic file.\nUse File -> Import for "
                           "non-KiCad schematic files." ),
                        fullFileName );

            progressReporter.Hide();
            DisplayErrorMessage( this, msg );
        }
    }
    else
    {
        wxFileName autoSaveFn = fullFileName;

        autoSaveFn.SetName( getAutoSaveFileName() );
        autoSaveFn.ClearExt();

        if( ( aCtl & KICTL_REVERT ) )
        {
            DeleteAutoSaveFile( autoSaveFn );
        }
        else
        {
            // This will rename the file if there is an autosave and the user wants to recover
            CheckForAutoSaveFile( autoSaveFn );
        }

        SetScreen( nullptr );

        IO_RELEASER<SCH_IO> pi( SCH_IO_MGR::FindPlugin( schFileType ) );

        pi->SetProgressReporter( &progressReporter );

        bool failedLoad = false;

        try
        {
            {
                wxBusyCursor busy;
                Schematic().SetRoot( pi->LoadSchematicFile( fullFileName, &Schematic() ) );

                // Make ${SHEETNAME} work on the root sheet until we properly support
                // naming the root sheet
                Schematic().Root().SetName( _( "Root" ) );
                wxLogTrace( tracePathsAndFiles, wxS( "Loaded schematic with root sheet UUID %s" ),
                            Schematic().Root().m_Uuid.AsString() );
            }

            if( !pi->GetError().IsEmpty() )
            {
                DisplayErrorMessage( this, _( "The entire schematic could not be loaded.  Errors "
                                              "occurred attempting to load hierarchical sheets." ),
                                     pi->GetError() );
            }
        }
        catch( const FUTURE_FORMAT_ERROR& ffe )
        {
            msg.Printf( _( "Error loading schematic '%s'." ), fullFileName);
            progressReporter.Hide();
            DisplayErrorMessage( this, msg, ffe.Problem() );

            failedLoad = true;
        }
        catch( const IO_ERROR& ioe )
        {
            msg.Printf( _( "Error loading schematic '%s'." ), fullFileName);
            progressReporter.Hide();
            DisplayErrorMessage( this, msg, ioe.What() );

            failedLoad = true;
        }
        catch( const std::bad_alloc& )
        {
            msg.Printf( _( "Memory exhausted loading schematic '%s'." ), fullFileName );
            progressReporter.Hide();
            DisplayErrorMessage( this, msg, wxEmptyString );

            failedLoad = true;
        }

        // This fixes a focus issue after the progress reporter is done on GTK.  It shouldn't
        // cause any issues on macOS and Windows.  If it does, it will have to be conditionally
        // compiled.
        Raise();

        if( failedLoad )
        {
            // Do not leave g_RootSheet == NULL because it is expected to be
            // a valid sheet. Therefore create a dummy empty root sheet and screen.
            CreateScreens();
            m_toolManager->RunAction( ACTIONS::zoomFitScreen );

            msg.Printf( _( "Failed to load '%s'." ), fullFileName );
            SetMsgPanel( wxEmptyString, msg );

            return false;
        }

        // It's possible the schematic parser fixed errors due to bugs so warn the user
        // that the schematic has been fixed (modified).
        SCH_SHEET_LIST sheetList = Schematic().Hierarchy();

        if( sheetList.IsModified() )
        {
            DisplayInfoMessage( this,
                                _( "An error was found when loading the schematic that has "
                                   "been automatically fixed.  Please save the schematic to "
                                   "repair the broken file or it may not be usable with other "
                                   "versions of KiCad." ) );
        }

        if( sheetList.AllSheetPageNumbersEmpty() )
            sheetList.SetInitialPageNumbers();

        UpdateFileHistory( fullFileName );

        SCH_SCREENS schematic( Schematic().Root() );

        // LIB_ID checks and symbol rescue only apply to the legacy file formats.
        if( schFileType == SCH_IO_MGR::SCH_LEGACY )
        {
            // Convert any legacy bus-bus entries to just be bus wires
            for( SCH_SCREEN* screen = schematic.GetFirst(); screen; screen = schematic.GetNext() )
            {
                std::vector<SCH_ITEM*> deleted;

                for( SCH_ITEM* item : screen->Items() )
                {
                    if( item->Type() == SCH_BUS_BUS_ENTRY_T )
                    {
                        SCH_BUS_BUS_ENTRY* entry = static_cast<SCH_BUS_BUS_ENTRY*>( item );
                        std::unique_ptr<SCH_LINE> wire = std::make_unique<SCH_LINE>();

                        wire->SetLayer( LAYER_BUS );
                        wire->SetStartPoint( entry->GetPosition() );
                        wire->SetEndPoint( entry->GetEnd() );

                        screen->Append( wire.release() );
                        deleted.push_back( item );
                    }
                }

                for( SCH_ITEM* item : deleted )
                    screen->Remove( item );
            }


            // Convert old projects over to use symbol library table.
            if( schematic.HasNoFullyDefinedLibIds() )
            {
                DIALOG_SYMBOL_REMAP dlgRemap( this );

                dlgRemap.ShowQuasiModal();
            }
            else
            {
                // Double check to ensure no legacy library list entries have been
                // added to the project file symbol library list.
                wxString paths;
                wxArrayString libNames;

                SYMBOL_LIBS::GetLibNamesAndPaths( &Prj(), &paths, &libNames );

                if( !libNames.IsEmpty() )
                {
                    if( eeconfig()->m_Appearance.show_illegal_symbol_lib_dialog )
                    {
                        wxRichMessageDialog invalidLibDlg(
                                this,
                                _( "Illegal entry found in project file symbol library list." ),
                                _( "Project Load Warning" ),
                                wxOK | wxCENTER | wxICON_EXCLAMATION );
                        invalidLibDlg.ShowDetailedText(
                                _( "Symbol libraries defined in the project file symbol library "
                                   "list are no longer supported and will be removed.\n\n"
                                   "This may cause broken symbol library links under certain "
                                   "conditions." ) );
                        invalidLibDlg.ShowCheckBox( _( "Do not show this dialog again." ) );
                        invalidLibDlg.ShowModal();
                        eeconfig()->m_Appearance.show_illegal_symbol_lib_dialog =
                                !invalidLibDlg.IsCheckBoxChecked();
                    }

                    libNames.Clear();
                    paths.Clear();
                    SYMBOL_LIBS::SetLibNamesAndPaths( &Prj(), paths, libNames );
                }

                if( !cfg || !cfg->m_RescueNeverShow )
                {
                    SCH_EDITOR_CONTROL* editor = m_toolManager->GetTool<SCH_EDITOR_CONTROL>();
                    editor->RescueSymbolLibTableProject( false );
                }
            }

            // Ensure there is only one legacy library loaded and that it is the cache library.
            SYMBOL_LIBS* legacyLibs = PROJECT_SCH::SchLibs( &Schematic().Prj() );

            if( legacyLibs->GetLibraryCount() == 0 )
            {
                wxString extMsg;
                wxFileName cacheFn = pro;

                cacheFn.SetName( cacheFn.GetName() + "-cache" );
                cacheFn.SetExt( FILEEXT::LegacySymbolLibFileExtension );

                msg.Printf( _( "The project symbol library cache file '%s' was not found." ),
                            cacheFn.GetFullName() );
                extMsg = _( "This can result in a broken schematic under certain conditions.  "
                            "If the schematic does not have any missing symbols upon opening, "
                            "save it immediately before making any changes to prevent data "
                            "loss.  If there are missing symbols, either manual recovery of "
                            "the schematic or recovery of the symbol cache library file and "
                            "reloading the schematic is required." );

                wxMessageDialog dlgMissingCache( this, msg, _( "Warning" ),
                                                 wxOK | wxCANCEL | wxICON_EXCLAMATION | wxCENTER );
                dlgMissingCache.SetExtendedMessage( extMsg );
                dlgMissingCache.SetOKCancelLabels(
                        wxMessageDialog::ButtonLabel( _( "Load Without Cache File" ) ),
                        wxMessageDialog::ButtonLabel( _( "Abort" ) ) );

                if( dlgMissingCache.ShowModal() == wxID_CANCEL )
                {
                    Schematic().Reset();
                    CreateScreens();
                    return false;
                }
            }

            // Update all symbol library links for all sheets.
            schematic.UpdateSymbolLinks();

            m_infoBar->RemoveAllButtons();
            m_infoBar->AddCloseButton();
            m_infoBar->ShowMessage( _( "This file was created by an older version of KiCad. "
                                       "It will be converted to the new format when saved." ),
                                    wxICON_WARNING, WX_INFOBAR::MESSAGE_TYPE::OUTDATED_SAVE );

            // Legacy schematic can have duplicate time stamps so fix that before converting
            // to the s-expression format.
            schematic.ReplaceDuplicateTimeStamps();

            for( SCH_SCREEN* screen = schematic.GetFirst(); screen; screen = schematic.GetNext() )
                screen->FixLegacyPowerSymbolMismatches();

            // Allow the schematic to be saved to new file format without making any edits.
            OnModify();
        }
        else  // S-expression schematic.
        {
            if( schematic.GetFirst()->GetFileFormatVersionAtLoad() < SEXPR_SCHEMATIC_FILE_VERSION )
            {
                m_infoBar->RemoveAllButtons();
                m_infoBar->AddCloseButton();
                m_infoBar->ShowMessage( _( "This file was created by an older version of KiCad. "
                                           "It will be converted to the new format when saved." ),
                                        wxICON_WARNING, WX_INFOBAR::MESSAGE_TYPE::OUTDATED_SAVE );
            }

            for( SCH_SCREEN* screen = schematic.GetFirst(); screen; screen = schematic.GetNext() )
                screen->UpdateLocalLibSymbolLinks();

            // Restore all of the loaded symbol and sheet instances from the root sheet.
            if( Schematic().RootScreen()->GetFileFormatVersionAtLoad() < 20221002 )
                sheetList.UpdateSymbolInstanceData( Schematic().RootScreen()->GetSymbolInstances() );

            if( Schematic().RootScreen()->GetFileFormatVersionAtLoad() < 20221110 )
                sheetList.UpdateSheetInstanceData( Schematic().RootScreen()->GetSheetInstances());

            if( Schematic().RootScreen()->GetFileFormatVersionAtLoad() < 20230221 )
                for( SCH_SCREEN* screen = schematic.GetFirst(); screen;
                     screen = schematic.GetNext() )
                    screen->FixLegacyPowerSymbolMismatches();

            for( SCH_SCREEN* screen = schematic.GetFirst(); screen; screen = schematic.GetNext() )
                screen->MigrateSimModels();
        }

        // After the schematic is successfully loaded, we load the drawing sheet.
        // This allows us to use the drawing sheet embedded in the schematic (if any)
        // instead of the default one.
        LoadDrawingSheet();

        schematic.PruneOrphanedSymbolInstances( Prj().GetProjectName(), sheetList );
        schematic.PruneOrphanedSheetInstances( Prj().GetProjectName(), sheetList );
        sheetList.CheckForMissingSymbolInstances( Prj().GetProjectName() );

        Schematic().ConnectionGraph()->Reset();

        SetScreen( GetCurrentSheet().LastScreen() );

        // Migrate conflicting bus definitions
        // TODO(JE) This should only run once based on schematic file version
        if( Schematic().ConnectionGraph()->GetBusesNeedingMigration().size() > 0 )
        {
            DIALOG_MIGRATE_BUSES dlg( this );
            dlg.ShowQuasiModal();
            OnModify();
        }

        SCH_COMMIT dummy( this );

        RecalculateConnections( &dummy, GLOBAL_CLEANUP );

        if( schematic.HasSymbolFieldNamesWithWhiteSpace() )
        {
            m_infoBar->QueueShowMessage( _( "This schematic contains symbols that have leading "
                                            "and/or trailing white space field names." ),
                                         wxICON_WARNING );
        }
    }

    // Load any exclusions from the project file
    Schematic().ResolveERCExclusionsPostUpdate();

    initScreenZoom();
    SetSheetNumberAndCount();

    RecomputeIntersheetRefs();
    GetCurrentSheet().UpdateAllScreenReferences();

    // Re-create junctions if needed. Eeschema optimizes wires by merging
    // colinear segments. If a schematic is saved without a valid
    // cache library or missing installed libraries, this can cause connectivity errors
    // unless junctions are added.
    //
    // TODO: (RFB) This really needs to be put inside the Load() function of the SCH_IO_KICAD_LEGACY
    // I can't put it right now because of the extra code that is above to convert legacy bus-bus
    // entries to bus wires
    if( schFileType == SCH_IO_MGR::SCH_LEGACY )
        Schematic().FixupJunctions();

    SyncView();
    GetScreen()->ClearDrawingState();

    TestDanglingEnds();

    UpdateHierarchyNavigator( false, true );

    wxCommandEvent changedEvt( EDA_EVT_SCHEMATIC_CHANGED );
    ProcessEventLocally( changedEvt );

    for( wxEvtHandler* listener : m_schematicChangeListeners )
    {
        wxCHECK2( listener, continue );

        // Use the windows variant when handling event messages in case there is any special
        // event handler pre and/or post processing specific to windows.
        wxWindow* win = dynamic_cast<wxWindow*>( listener );

        if( win )
            win->HandleWindowEvent( e );
        else
            listener->SafelyProcessEvent( e );
    }

    updateTitle();
    m_toolManager->GetTool<SCH_NAVIGATE_TOOL>()->ResetHistory();

    wxFileName fn = Prj().AbsolutePath( GetScreen()->GetFileName() );

    if( fn.FileExists() && !fn.IsFileWritable() )
    {
        m_infoBar->RemoveAllButtons();
        m_infoBar->AddCloseButton();
        m_infoBar->ShowMessage( _( "Schematic is read only." ),
                                wxICON_WARNING, WX_INFOBAR::MESSAGE_TYPE::OUTDATED_SAVE );
    }

#ifdef PROFILE
    openFiles.Show();
#endif
    // Ensure all items are redrawn (especially the drawing-sheet items):
    if( GetCanvas() )
        GetCanvas()->DisplaySheet( GetCurrentSheet().LastScreen() );

    return true;
}


void SCH_EDIT_FRAME::OnImportProject( wxCommandEvent& aEvent )
{
    if( Schematic().RootScreen() && !Schematic().RootScreen()->Items().empty() )
    {
        wxString msg = _( "This operation replaces the contents of the current schematic, "
                          "which will be permanently lost.\n\n"
                          "Do you want to proceed?" );

        if( !IsOK( this, msg ) )
            return;
    }

    // Set the project location if none is set or if we are running in standalone mode
    bool     setProject = Prj().GetProjectFullName().IsEmpty() || Kiface().IsSingle();
    wxString path = wxPathOnly( Prj().GetProjectFullName() );

    wxString fileFiltersStr;
    wxString allWildcardsStr;

    for( const SCH_IO_MGR::SCH_FILE_T& fileType : SCH_IO_MGR::SCH_FILE_T_vector )
    {
        if( fileType == SCH_IO_MGR::SCH_KICAD || fileType == SCH_IO_MGR::SCH_LEGACY )
            continue; // this is "Import non-KiCad schematic"

        IO_RELEASER<SCH_IO> pi( SCH_IO_MGR::FindPlugin( fileType ) );

        if( !pi )
            continue;

        const IO_BASE::IO_FILE_DESC& desc = pi->GetSchematicFileDesc();

        if( desc.m_FileExtensions.empty() || !desc.m_CanRead )
            continue;

        if( !fileFiltersStr.IsEmpty() )
            fileFiltersStr += wxChar( '|' );

        fileFiltersStr += desc.FileFilter();

        for( const std::string& ext : desc.m_FileExtensions )
            allWildcardsStr << wxS( "*." ) << formatWildcardExt( ext ) << wxS( ";" );
    }

    fileFiltersStr = _( "All supported formats" ) + wxS( "|" ) + allWildcardsStr + wxS( "|" )
                     + fileFiltersStr;

    wxFileDialog dlg( this, _( "Import Schematic" ), path, wxEmptyString, fileFiltersStr,
                      wxFD_OPEN | wxFD_FILE_MUST_EXIST ); // TODO

    FILEDLG_IMPORT_NON_KICAD importOptions( eeconfig()->m_System.show_import_issues );
    dlg.SetCustomizeHook( importOptions );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    eeconfig()->m_System.show_import_issues = importOptions.GetShowIssues();

    // Don't leave dangling pointers to previously-opened document.
    m_toolManager->GetTool<SCH_SELECTION_TOOL>()->ClearSelection();
    ClearUndoRedoList();
    ClearRepeatItemsList();

    if( setProject )
    {
        Schematic().SetProject( nullptr );
        GetSettingsManager()->UnloadProject( &Prj(), false );

        // Clear view before destroying schematic as repaints depend on schematic being valid
        SetScreen( nullptr );

        Schematic().Reset();

        wxFileName projectFn( dlg.GetPath() );
        projectFn.SetExt( FILEEXT::ProjectFileExtension );
        GetSettingsManager()->LoadProject( projectFn.GetFullPath() );

        Schematic().SetProject( &Prj() );
    }

    wxFileName fn = dlg.GetPath();

    if( !fn.IsFileReadable() )
    {
        wxLogError( _( "Insufficient permissions to read file '%s'." ), fn.GetFullPath() );
        return;
    }

    SCH_IO_MGR::SCH_FILE_T pluginType = SCH_IO_MGR::SCH_FILE_T::SCH_FILE_UNKNOWN;

    for( const SCH_IO_MGR::SCH_FILE_T& fileType : SCH_IO_MGR::SCH_FILE_T_vector )
    {
        IO_RELEASER<SCH_IO> pi( SCH_IO_MGR::FindPlugin( fileType ) );

        if( !pi )
            continue;

        if( pi->CanReadSchematicFile( fn.GetFullPath() ) )
        {
            pluginType = fileType;
            break;
        }
    }

    if( pluginType == SCH_IO_MGR::SCH_FILE_T::SCH_FILE_UNKNOWN )
    {
        wxLogError( _( "No loader can read the specified file: '%s'." ), fn.GetFullPath() );
        CreateScreens();
        SetScreen( Schematic().RootScreen() );
        return;
    }

    importFile( dlg.GetPath(), pluginType );

    RefreshCanvas();
}


bool SCH_EDIT_FRAME::saveSchematicFile( SCH_SHEET* aSheet, const wxString& aSavePath )
{
    wxString msg;
    wxFileName schematicFileName;
    wxFileName oldFileName;
    bool success;

    SCH_SCREEN* screen = aSheet->GetScreen();

    wxCHECK( screen, false );

    // Cannot save to nowhere
    if( aSavePath.IsEmpty() )
        return false;

    // Construct the name of the file to be saved
    schematicFileName = Prj().AbsolutePath( aSavePath );
    oldFileName = schematicFileName;

    // Write through symlinks, don't replace them
    WX_FILENAME::ResolvePossibleSymlinks( schematicFileName );

    if( !schematicFileName.DirExists() )
    {
        if( !wxMkdir( schematicFileName.GetPath() ) )
        {
            msg.Printf( _( "Error saving schematic file '%s'.\n%s" ),
                        schematicFileName.GetFullPath(),
                        "Could not create directory: %s" + schematicFileName.GetPath() );
            DisplayError( this, msg );

            return false;
        }
    }

    if( !IsWritable( schematicFileName ) )
        return false;

    wxFileName projectFile( schematicFileName );

    projectFile.SetExt( FILEEXT::ProjectFileExtension );

    if( projectFile.FileExists() )
    {
        // Save various ERC settings, such as violation severities (which may have been edited
        // via the ERC dialog as well as the Schematic Setup dialog), ERC exclusions, etc.
        saveProjectSettings();
    }

    wxString tempFile = wxFileName::CreateTempFileName( wxS( "eeschema" ) );

    // Save
    wxLogTrace( traceAutoSave, wxS( "Saving file " ) + schematicFileName.GetFullPath() );

    if( m_infoBar->GetMessageType() == WX_INFOBAR::MESSAGE_TYPE::OUTDATED_SAVE )
        m_infoBar->Dismiss();

    SCH_IO_MGR::SCH_FILE_T pluginType = SCH_IO_MGR::GuessPluginTypeFromSchPath(
            schematicFileName.GetFullPath() );

    if( pluginType == SCH_IO_MGR::SCH_FILE_UNKNOWN )
        pluginType = SCH_IO_MGR::SCH_KICAD;

    IO_RELEASER<SCH_IO> pi( SCH_IO_MGR::FindPlugin( pluginType ) );

    try
    {
        pi->SaveSchematicFile( tempFile, aSheet, &Schematic() );
        success = true;
    }
    catch( const IO_ERROR& ioe )
    {
        msg.Printf( _( "Error saving schematic file '%s'.\n%s" ),
                    schematicFileName.GetFullPath(),
                    ioe.What() );
        DisplayError( this, msg );

        msg.Printf( _( "Failed to create temporary file '%s'." ),
                    tempFile );
        SetMsgPanel( wxEmptyString, msg );

        // In case we started a file but didn't fully write it, clean up
        wxRemoveFile( tempFile );

        success = false;
    }

    if( success )
    {
        // Preserve the permissions of the current file
        KIPLATFORM::IO::DuplicatePermissions( schematicFileName.GetFullPath(), tempFile );

        // Replace the original with the temporary file we just wrote
        success = wxRenameFile( tempFile, schematicFileName.GetFullPath() );

        if( !success )
        {
            msg.Printf( _( "Error saving schematic file '%s'.\n"
                           "Failed to rename temporary file '%s'." ),
                        schematicFileName.GetFullPath(),
                        tempFile );
            DisplayError( this, msg );

            msg.Printf( _( "Failed to rename temporary file '%s'." ),
                        tempFile );
            SetMsgPanel( wxEmptyString, msg );
        }
    }

    if( success )
    {
        // Delete auto save file.
        wxFileName autoSaveFileName = schematicFileName;
        autoSaveFileName.SetName( FILEEXT::AutoSaveFilePrefix + schematicFileName.GetName() );

        if( autoSaveFileName.FileExists() )
        {
            wxLogTrace( traceAutoSave,
                        wxS( "Removing auto save file <" ) + autoSaveFileName.GetFullPath() +
                        wxS( ">" ) );

            wxRemoveFile( autoSaveFileName.GetFullPath() );
        }

        screen->SetContentModified( false );

        msg.Printf( _( "File '%s' saved." ),  screen->GetFileName() );
        SetStatusText( msg, 0 );
    }
    else
    {
        DisplayError( this, _( "File write operation failed." ) );
    }

    return success;
}


bool SCH_EDIT_FRAME::SaveProject( bool aSaveAs )
{
    wxString msg;
    SCH_SCREEN* screen;
    SCH_SCREENS screens( Schematic().Root() );
    bool        saveCopy          = aSaveAs && !Kiface().IsSingle();
    bool        success           = true;
    bool        updateFileHistory = false;
    bool        createNewProject  = false;

    // I want to see it in the debugger, show me the string!  Can't do that with wxFileName.
    wxString    fileName = Prj().AbsolutePath( Schematic().Root().GetFileName() );
    wxFileName  fn = fileName;

    // Path to save each screen to: will be the stored filename by default, but is overwritten by
    // a Save As Copy operation.
    std::unordered_map<SCH_SCREEN*, wxString> filenameMap;

    // Handle "Save As" and saving a new project/schematic for the first time in standalone
    if( Prj().IsNullProject() || aSaveAs )
    {
        // Null project should only be possible in standalone mode.
        wxCHECK( Kiface().IsSingle() || aSaveAs, false );

        wxFileName newFileName;
        wxFileName savePath( Prj().GetProjectFullName() );

        if( !savePath.IsOk() || !savePath.IsDirWritable() )
        {
            savePath = GetMruPath();

            if( !savePath.IsOk() || !savePath.IsDirWritable() )
                savePath = PATHS::GetDefaultUserProjectsPath();
        }

        if( savePath.HasExt() )
            savePath.SetExt( FILEEXT::KiCadSchematicFileExtension );
        else
            savePath.SetName( wxEmptyString );

        wxFileDialog dlg( this, _( "Schematic Files" ), savePath.GetPath(), savePath.GetFullName(),
                          FILEEXT::KiCadSchematicFileWildcard(),
                          wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

        FILEDLG_HOOK_SAVE_PROJECT newProjectHook;

        // Add a "Create a project" checkbox in standalone mode and one isn't loaded
        if( Kiface().IsSingle() || aSaveAs )
        {
            dlg.SetCustomizeHook( newProjectHook );
        }

        if( dlg.ShowModal() == wxID_CANCEL )
            return false;

        newFileName = EnsureFileExtension( dlg.GetPath(), FILEEXT::KiCadSchematicFileExtension );

        if( ( !newFileName.DirExists() && !newFileName.Mkdir() ) ||
            !newFileName.IsDirWritable() )
        {
            msg.Printf( _( "Folder '%s' could not be created.\n\n"
                           "Make sure you have write permissions and try again." ),
                        newFileName.GetPath() );

            wxMessageDialog dlgBadPath( this, msg, _( "Error" ),
                                        wxOK | wxICON_EXCLAMATION | wxCENTER );

            dlgBadPath.ShowModal();
            return false;
        }

        if( newProjectHook.IsAttachedToDialog() )
            createNewProject = newProjectHook.GetCreateNewProject();

        if( !saveCopy )
        {
            Schematic().Root().SetFileName( newFileName.GetFullName() );
            Schematic().RootScreen()->SetFileName( newFileName.GetFullPath() );
            updateFileHistory = true;
        }
        else
        {
            filenameMap[Schematic().RootScreen()] = newFileName.GetFullPath();
        }

        // Set the base path to all new sheets.
        for( size_t i = 0; i < screens.GetCount(); i++ )
        {
            screen = screens.GetScreen( i );

            wxCHECK2( screen, continue );

            // The root screen file name has already been set.
            if( screen == Schematic().RootScreen() )
                continue;

            wxFileName tmp = screen->GetFileName();

            // Assume existing sheet files are being reused and do not save them to the new
            // path.  Maybe in the future, add a user option to copy schematic files to the
            // new project path.
            if( tmp.FileExists() )
                continue;

            if( tmp.GetPath().IsEmpty() )
            {
                tmp.SetPath( newFileName.GetPath() );
            }
            else if( tmp.GetPath() == fn.GetPath() )
            {
                tmp.SetPath( newFileName.GetPath() );
            }
            else if( tmp.GetPath().StartsWith( fn.GetPath() ) )
            {
                // NOTE: this hasn't been tested because the sheet properties dialog no longer
                //       allows adding a path specifier in the file name field.
                wxString newPath = newFileName.GetPath();
                newPath += tmp.GetPath().Right( fn.GetPath().Length() );
                tmp.SetPath( newPath );
            }

            wxLogTrace( tracePathsAndFiles,
                        wxS( "Moving schematic from '%s' to '%s'." ),
                        screen->GetFileName(),
                        tmp.GetFullPath() );

            if( !tmp.DirExists() && !tmp.Mkdir() )
            {
                msg.Printf( _( "Folder '%s' could not be created.\n\n"
                               "Make sure you have write permissions and try again." ),
                            newFileName.GetPath() );

                wxMessageDialog dlgBadFilePath( this, msg, _( "Error" ),
                                                wxOK | wxICON_EXCLAMATION | wxCENTER );

                dlgBadFilePath.ShowModal();
                return false;
            }

            if( saveCopy )
                filenameMap[screen] = tmp.GetFullPath();
            else
                screen->SetFileName( tmp.GetFullPath() );
        }

        // Attempt to make sheet file name paths relative to the new root schematic path.
        for( SCH_SHEET_PATH& sheet : Schematic().Hierarchy() )
        {
            if( !sheet.Last()->IsRootSheet() )
                sheet.MakeFilePathRelativeToParentSheet();
        }
    }
    else if( !fn.FileExists() )
    {
        // File doesn't exist yet; true if we just imported something
        updateFileHistory = true;
    }
    else if( screens.GetFirst()->GetFileFormatVersionAtLoad() < SEXPR_SCHEMATIC_FILE_VERSION )
    {
        // Allow the user to save un-edited files in new format
    }
    else if( !IsContentModified() )
    {
        return true;
    }

    if( filenameMap.empty() || !saveCopy )
    {
        for( size_t i = 0; i < screens.GetCount(); i++ )
            filenameMap[screens.GetScreen( i )] = screens.GetScreen( i )->GetFileName();
    }

    // Warn user on potential file overwrite.  This can happen on shared sheets.
    wxArrayString overwrittenFiles;
    wxArrayString lockedFiles;

    for( size_t i = 0; i < screens.GetCount(); i++ )
    {
        screen = screens.GetScreen( i );

        wxCHECK2( screen, continue );

        // Convert legacy schematics file name extensions for the new format.
        wxFileName tmpFn = filenameMap[screen];

        if( !tmpFn.IsOk() )
            continue;

        if( tmpFn.FileExists() && !tmpFn.IsFileWritable() )
            lockedFiles.Add( tmpFn.GetFullPath() );

        if( tmpFn.GetExt() == FILEEXT::KiCadSchematicFileExtension )
            continue;

        tmpFn.SetExt( FILEEXT::KiCadSchematicFileExtension );

        if( tmpFn.FileExists() )
            overwrittenFiles.Add( tmpFn.GetFullPath() );
    }

    if( !lockedFiles.IsEmpty() )
    {
        for( const wxString& lockedFile : lockedFiles )
        {
            if( msg.IsEmpty() )
                msg = lockedFile;
            else
                msg += "\n" + lockedFile;
        }

        wxRichMessageDialog dlg( this, wxString::Format( _( "Failed to save %s." ),
                                                         Schematic().Root().GetFileName() ),
                                 _( "Locked File Warning" ),
                                 wxOK | wxICON_WARNING | wxCENTER );
        dlg.SetExtendedMessage( _( "You do not have write permissions to:\n\n" ) + msg );

        dlg.ShowModal();
        return false;
    }

    if( !overwrittenFiles.IsEmpty() )
    {
        for( const wxString& overwrittenFile : overwrittenFiles )
        {
            if( msg.IsEmpty() )
                msg = overwrittenFile;
            else
                msg += "\n" + overwrittenFile;
        }

        wxRichMessageDialog dlg( this, _( "Saving will overwrite existing files." ),
                                 _( "Save Warning" ),
                                 wxOK | wxCANCEL | wxCANCEL_DEFAULT | wxCENTER |
                                 wxICON_EXCLAMATION );
        dlg.ShowDetailedText( _( "The following files will be overwritten:\n\n" ) + msg );
        dlg.SetOKCancelLabels( wxMessageDialog::ButtonLabel( _( "Overwrite Files" ) ),
                               wxMessageDialog::ButtonLabel( _( "Abort Project Save" ) ) );

        if( dlg.ShowModal() == wxID_CANCEL )
            return false;
    }

    screens.BuildClientSheetPathList();

    for( size_t i = 0; i < screens.GetCount(); i++ )
    {
        screen = screens.GetScreen( i );

        wxCHECK2( screen, continue );

        // Convert legacy schematics file name extensions for the new format.
        wxFileName tmpFn = filenameMap[screen];

        if( tmpFn.IsOk() && tmpFn.GetExt() != FILEEXT::KiCadSchematicFileExtension )
        {
            updateFileHistory = true;
            tmpFn.SetExt( FILEEXT::KiCadSchematicFileExtension );

            for( EDA_ITEM* item : screen->Items().OfType( SCH_SHEET_T ) )
            {
                SCH_SHEET* sheet = static_cast<SCH_SHEET*>( item );
                wxFileName sheetFileName = sheet->GetFileName();

                if( !sheetFileName.IsOk()
                    || sheetFileName.GetExt() == FILEEXT::KiCadSchematicFileExtension )
                    continue;

                sheetFileName.SetExt( FILEEXT::KiCadSchematicFileExtension );
                sheet->SetFileName( sheetFileName.GetFullPath() );
                UpdateItem( sheet );
            }

            filenameMap[screen] = tmpFn.GetFullPath();

            if( !saveCopy )
                screen->SetFileName( tmpFn.GetFullPath() );
        }

        // Do not save sheet symbols with no valid filename set
        if( !tmpFn.IsOk() )
            continue;

        std::vector<SCH_SHEET_PATH>& sheets = screen->GetClientSheetPaths();

        if( sheets.size() == 1 )
            screen->SetVirtualPageNumber( 1 );
        else
            screen->SetVirtualPageNumber( 0 );  // multiple uses; no way to store the real sheet #

        // This is a new schematic file so make sure it has a unique ID.
        if( !saveCopy && tmpFn.GetFullPath() != screen->GetFileName() )
            screen->AssignNewUuid();

        success &= saveSchematicFile( screens.GetSheet( i ), tmpFn.GetFullPath() );
    }

    if( success )
        m_autoSaveRequired = false;

    // One or more of the modified sheets did not save correctly so update the auto save file.
    if( !aSaveAs && !success )
        success &= updateAutoSaveFile();

    if( aSaveAs && success )
        LockFile( Schematic().RootScreen()->GetFileName() );

    if( updateFileHistory )
        UpdateFileHistory( Schematic().RootScreen()->GetFileName() );

    // Save the sheet name map to the project file
    std::vector<FILE_INFO_PAIR>& sheets = Prj().GetProjectFile().GetSheets();
    sheets.clear();

    for( SCH_SHEET_PATH& sheetPath : Schematic().Hierarchy() )
    {
        SCH_SHEET* sheet = sheetPath.Last();

        wxCHECK2( sheet, continue );

        // Use the schematic UUID for the root sheet.
        if( sheet->IsRootSheet() )
        {
            screen = sheet->GetScreen();

            wxCHECK2( screen, continue );

            // For the root sheet we use the canonical name ( "Root" ) because its name
            // cannot be modified by the user
            sheets.emplace_back( std::make_pair( screen->GetUuid(), wxT( "Root" ) ) );
        }
        else
        {
            sheets.emplace_back( std::make_pair( sheet->m_Uuid, sheet->GetName() ) );
        }
    }

    wxASSERT( filenameMap.count( Schematic().RootScreen() ) );
    wxFileName projectPath( filenameMap.at( Schematic().RootScreen() ) );
    projectPath.SetExt( FILEEXT::ProjectFileExtension );

    if( Prj().IsNullProject() || ( aSaveAs && !saveCopy ) )
    {
        Prj().SetReadOnly( !createNewProject );
        GetSettingsManager()->SaveProjectAs( projectPath.GetFullPath() );
    }
    else if( saveCopy && createNewProject )
    {
        GetSettingsManager()->SaveProjectCopy( projectPath.GetFullPath() );
    }
    else
    {
        SaveProjectLocalSettings();
        saveProjectSettings();
    }

    if( !Kiface().IsSingle() )
    {
        WX_STRING_REPORTER backupReporter;

        if( !GetSettingsManager()->TriggerBackupIfNeeded( backupReporter ) )
            SetStatusText( backupReporter.GetMessages(), 0 );
    }

    updateTitle();

    if( m_infoBar->GetMessageType() == WX_INFOBAR::MESSAGE_TYPE::OUTDATED_SAVE )
        m_infoBar->Dismiss();

    return success;
}


bool SCH_EDIT_FRAME::doAutoSave()
{
    wxFileName  tmpFileName = Schematic().Root().GetFileName();
    wxFileName  fn = tmpFileName;
    wxFileName  tmp;
    SCH_SCREENS screens( Schematic().Root() );

    // Don't run autosave if content has not been modified
    if( !IsContentModified() )
        return true;

    bool autoSaveOk = true;

    if( fn.GetPath().IsEmpty() )
        tmp.AssignDir( Prj().GetProjectPath() );
    else
        tmp.AssignDir( fn.GetPath() );

    if( !tmp.IsOk() )
        return false;

    if( !IsWritable( tmp ) )
        return false;

    wxString title = GetTitle();    // Save frame title, that can be modified by the save process

    for( size_t i = 0; i < screens.GetCount(); i++ )
    {
        // Only create auto save files for the schematics that have been modified.
        if( !screens.GetScreen( i )->IsContentModified() )
            continue;

        tmpFileName = fn = screens.GetScreen( i )->GetFileName();

        // Auto save file name is the normal file name prefixed with GetAutoSavePrefix().
        fn.SetName( FILEEXT::AutoSaveFilePrefix + fn.GetName() );

        if( saveSchematicFile( screens.GetSheet( i ), fn.GetFullPath() ) )
        {
            // This was only an auto-save, not a real save.  Reset the modified flag.
            screens.GetScreen( i )->SetContentModified();
        }
        else
        {
            autoSaveOk = false;
        }
    }

    if( autoSaveOk && updateAutoSaveFile() )
    {
        m_autoSaveRequired = false;
        m_autoSavePending = false;

        if( !Kiface().IsSingle()
                && GetSettingsManager()->GetCommonSettings()->m_Backup.backup_on_autosave )
        {
            GetSettingsManager()->TriggerBackupIfNeeded( NULL_REPORTER::GetInstance() );
        }
    }

    SetTitle( title );

    return autoSaveOk;
}


bool SCH_EDIT_FRAME::importFile( const wxString& aFileName, int aFileType,
                                 const std::map<std::string, UTF8>* aProperties )
{
    wxFileName             filename( aFileName );
    wxFileName             newfilename;
    SCH_IO_MGR::SCH_FILE_T fileType = (SCH_IO_MGR::SCH_FILE_T) aFileType;

    wxCommandEvent changingEvt( EDA_EVT_SCHEMATIC_CHANGING );
    ProcessEventLocally( changingEvt );

    switch( fileType )
    {
    case SCH_IO_MGR::SCH_ALTIUM:
    case SCH_IO_MGR::SCH_CADSTAR_ARCHIVE:
    case SCH_IO_MGR::SCH_EAGLE:
    case SCH_IO_MGR::SCH_LTSPICE:
    case SCH_IO_MGR::SCH_EASYEDA:
    case SCH_IO_MGR::SCH_EASYEDAPRO:
    {
        // We insist on caller sending us an absolute path, if it does not, we say it's a bug.
        // Unless we are passing the files in aproperties, in which case aFileName can be empty.
        wxCHECK_MSG( aFileName.IsEmpty() || filename.IsAbsolute(), false,
                     wxS( "Import schematic: path is not absolute!" ) );

        try
        {
            IO_RELEASER<SCH_IO>  pi( SCH_IO_MGR::FindPlugin( fileType ) );
            DIALOG_HTML_REPORTER errorReporter( this );
            WX_PROGRESS_REPORTER progressReporter( this, _( "Importing Schematic" ), 1 );

            if( PROJECT_CHOOSER_PLUGIN* c_pi = dynamic_cast<PROJECT_CHOOSER_PLUGIN*>( pi.get() ) )
            {
                c_pi->RegisterCallback( std::bind( DIALOG_IMPORT_CHOOSE_PROJECT::RunModal,
                                                   this, std::placeholders::_1 ) );
            }

            if( eeconfig()->m_System.show_import_issues )
                pi->SetReporter( errorReporter.m_Reporter );
            else
                pi->SetReporter( &NULL_REPORTER::GetInstance() );

            pi->SetProgressReporter( &progressReporter );

            SCH_SHEET* loadedSheet = pi->LoadSchematicFile( aFileName, &Schematic(), nullptr,
                                                            aProperties );

            if( loadedSheet )
            {
                Schematic().SetRoot( loadedSheet );

                if( errorReporter.m_Reporter->HasMessage() )
                {
                    errorReporter.m_Reporter->Flush(); // Build HTML messages
                    errorReporter.ShowModal();
                }

                // Non-KiCad schematics do not use a drawing-sheet (or if they do, it works
                // differently to KiCad), so set it to an empty one.
                DS_DATA_MODEL& drawingSheet = DS_DATA_MODEL::GetTheInstance();
                drawingSheet.SetEmptyLayout();
                BASE_SCREEN::m_DrawingSheetFileName = "empty.kicad_wks";

                newfilename.SetPath( Prj().GetProjectPath() );
                newfilename.SetName( Prj().GetProjectName() );
                newfilename.SetExt( FILEEXT::KiCadSchematicFileExtension );

                SetScreen( GetCurrentSheet().LastScreen() );

                Schematic().Root().SetFileName( newfilename.GetFullName() );
                GetScreen()->SetFileName( newfilename.GetFullPath() );
                GetScreen()->SetContentModified();

                RecalculateConnections( nullptr, GLOBAL_CLEANUP );

                // Only perform the dangling end test on root sheet.
                GetScreen()->TestDanglingEnds();
            }
            else
            {
                CreateScreens();
            }
        }
        catch( const IO_ERROR& ioe )
        {
            // Do not leave g_RootSheet == NULL because it is expected to be
            // a valid sheet. Therefore create a dummy empty root sheet and screen.
            CreateScreens();
            m_toolManager->RunAction( ACTIONS::zoomFitScreen );

            wxString msg = wxString::Format( _( "Error loading schematic '%s'." ), aFileName );
            DisplayErrorMessage( this, msg, ioe.What() );

            msg.Printf( _( "Failed to load '%s'." ), aFileName );
            SetMsgPanel( wxEmptyString, msg );
        }
        catch( const std::exception& exc )
        {
            CreateScreens();
            m_toolManager->RunAction( ACTIONS::zoomFitScreen );

            wxString msg = wxString::Format( _( "Unhandled exception occurred loading schematic "
                                                "'%s'." ), aFileName );
            DisplayErrorMessage( this, msg, exc.what() );

            msg.Printf( _( "Failed to load '%s'." ), aFileName );
            SetMsgPanel( wxEmptyString, msg );
        }

        ClearUndoRedoList();
        ClearRepeatItemsList();

        initScreenZoom();
        SetSheetNumberAndCount();
        SyncView();

        UpdateHierarchyNavigator( false, true );

        wxCommandEvent e( EDA_EVT_SCHEMATIC_CHANGED );
        ProcessEventLocally( e );

        for( wxEvtHandler* listener : m_schematicChangeListeners )
        {
            wxCHECK2( listener, continue );

            // Use the windows variant when handling event messages in case there is any
            // special event handler pre and/or post processing specific to windows.
            wxWindow* win = dynamic_cast<wxWindow*>( listener );

            if( win )
                win->HandleWindowEvent( e );
            else
                listener->SafelyProcessEvent( e );
        }

        updateTitle();
        break;
    }

    default:
        break;
    }

    return true;
}


bool SCH_EDIT_FRAME::AskToSaveChanges()
{
    SCH_SCREENS screenList( Schematic().Root() );

    // Save any currently open and modified project files.
    for( SCH_SCREEN* screen = screenList.GetFirst(); screen; screen = screenList.GetNext() )
    {
        SIMULATOR_FRAME* simFrame = (SIMULATOR_FRAME*) Kiway().Player( FRAME_SIMULATOR, false );

        // Simulator must be closed before loading another schematic, otherwise it may crash.
        // If there are any changes in the simulator the user will be prompted to save them.
        if( simFrame && !simFrame->Close() )
            return false;

        if( screen->IsContentModified() )
        {
            if( !HandleUnsavedChanges( this, _( "The current schematic has been modified.  "
                                                "Save changes?" ),
                                       [&]() -> bool
                                       {
                                           return SaveProject();
                                       } ) )
            {
                return false;
            }
        }
    }

    return true;
}


bool SCH_EDIT_FRAME::updateAutoSaveFile()
{
    wxFileName tmpFn = Prj().GetProjectFullName();
    wxFileName autoSaveFileName( tmpFn.GetPath(), getAutoSaveFileName() );

    wxLogTrace( traceAutoSave, "Creating auto save file %s", autoSaveFileName.GetFullPath() );

    wxCHECK( autoSaveFileName.IsDirWritable(), false );

    wxFileName fn;
    SCH_SCREENS screens( Schematic().Root() );
    std::vector< wxString > autoSavedFiles;

    for( size_t i = 0; i < screens.GetCount(); i++ )
    {
        // Only create auto save files for the schematics that have been modified.
        if( !screens.GetScreen( i )->IsContentModified() )
            continue;

        fn = screens.GetScreen( i )->GetFileName();

        // Auto save file name is the normal file name prefixed with GetAutoSavePrefix().
        fn.SetName( FILEEXT::AutoSaveFilePrefix + fn.GetName() );
        autoSavedFiles.emplace_back( fn.GetFullPath() );
    }

    wxTextFile autoSaveFile( autoSaveFileName.GetFullPath() );

    if( autoSaveFileName.FileExists() && !wxRemoveFile( autoSaveFileName.GetFullPath() ) )
    {
        wxLogTrace( traceAutoSave, "Error removing auto save file %s",
                    autoSaveFileName.GetFullPath() );

        return false;
    }

    // No modified sheet files to save.
    if( autoSavedFiles.empty() )
        return true;

    if( !autoSaveFile.Create() )
        return false;

    for( const wxString& fileName : autoSavedFiles )
    {
        wxLogTrace( traceAutoSave, "Adding auto save file %s to %s",
                    fileName, autoSaveFileName.GetName() );
        autoSaveFile.AddLine( fileName );
    }

    if( !autoSaveFile.Write() )
        return false;

    wxLogTrace( traceAutoSave, "Auto save file '%s' written", autoSaveFileName.GetFullName() );

    return true;
}


void removeFile( const wxString& aFilename, wxArrayString& aUnremoved )
{
    wxLogTrace( traceAutoSave, wxS( "Removing auto save file " ) + aFilename );

    if( wxFileExists( aFilename ) && !wxRemoveFile( aFilename ) )
        aUnremoved.Add( aFilename );
};


void SCH_EDIT_FRAME::CheckForAutoSaveFile( const wxFileName& aFileName )
{
    if( !Pgm().IsGUI() )
        return;

    wxCHECK_RET( aFileName.IsOk(), wxS( "Invalid file name!" ) );

    wxLogTrace( traceAutoSave,
                wxS( "Checking for auto save file " ) + aFileName.GetFullPath() );

    if( !aFileName.FileExists() )
        return;

    wxString msg = _(
            "Well this is potentially embarrassing!\n"
            "It appears that the last time you were editing one or more of the schematic files\n"
            "were not saved properly.  Do you wish to restore the last saved edits you made?" );

    int response = wxMessageBox( msg, Pgm().App().GetAppDisplayName(), wxYES_NO | wxICON_QUESTION,
                                 this );

    wxTextFile fileList( aFileName.GetFullPath() );

    if( !fileList.Open() )
    {
        msg.Printf( _( "The file '%s' could not be opened.\n"
                       "Manual recovery of automatically saved files is required." ),
                    aFileName.GetFullPath() );

        wxMessageBox( msg, Pgm().App().GetAppDisplayName(), wxOK | wxICON_EXCLAMATION, this );
        return;
    }

    if( response == wxYES )
    {
        wxArrayString unrecoveredFiles;

        for( wxString fn = fileList.GetFirstLine(); !fileList.Eof(); fn = fileList.GetNextLine() )
        {
            wxFileName recoveredFn = fn;
            wxString tmp = recoveredFn.GetName();

            // Strip "_autosave-" prefix from the auto save file name.
            tmp.Replace( FILEEXT::AutoSaveFilePrefix, wxS( "" ), false );
            recoveredFn.SetName( tmp );

            wxFileName backupFn = recoveredFn;

            backupFn.SetExt( backupFn.GetExt() + FILEEXT::BackupFileSuffix );

            wxLogTrace( traceAutoSave, wxS( "Recovering auto save file:\n"
                                            "  Original file:  '%s'\n"
                                            "  Backup file:    '%s'\n"
                                            "  Auto save file: '%s'" ),
                        recoveredFn.GetFullPath(), backupFn.GetFullPath(), fn );

            if( !wxFileExists( fn ) )
            {
                unrecoveredFiles.Add( recoveredFn.GetFullPath() );
            }
            // Attempt to back up the last schematic file before overwriting it with the auto
            // save file.
            else if( recoveredFn.Exists()
                    && !wxCopyFile( recoveredFn.GetFullPath(), backupFn.GetFullPath() ) )
            {
                unrecoveredFiles.Add( recoveredFn.GetFullPath() );
            }
            // Attempt to replace last saved file with auto save file
            else if( !wxRenameFile( fn, recoveredFn.GetFullPath() ) )
            {
                unrecoveredFiles.Add( recoveredFn.GetFullPath() );
            }
        }

        if( !unrecoveredFiles.IsEmpty() )
        {
            msg = _( "The following automatically saved file(s) could not be restored\n" );

            for( size_t i = 0; i < unrecoveredFiles.GetCount(); i++ )
                msg += unrecoveredFiles[i] + wxS( "\n" );

            msg += _( "Manual recovery will be required to restore the file(s) above." );
            wxMessageBox( msg, Pgm().App().GetAppDisplayName(), wxOK | wxICON_EXCLAMATION,
                          this );
        }

        wxArrayString unremovedFiles;
        removeFile( aFileName.GetFullPath(), unremovedFiles );

        if( !unremovedFiles.IsEmpty() )
        {
            msg.Printf( _( "The autosave file '%s' could not be removed.\n"
                           "Manual removal will be required." ),
                        unremovedFiles[0] );

            wxMessageBox( msg, Pgm().App().GetAppDisplayName(), wxOK | wxICON_EXCLAMATION, this );
        }
    }
    else
    {
        DeleteAutoSaveFile( aFileName );
    }
}


void SCH_EDIT_FRAME::DeleteAutoSaveFile( const wxFileName& aFileName )
{
    if( !Pgm().IsGUI() )
        return;

    wxCHECK_RET( aFileName.IsOk(), wxS( "Invalid file name!" ) );

    if( !aFileName.FileExists() )
        return;

    wxTextFile    fileList( aFileName.GetFullPath() );
    wxArrayString unremovedFiles;

    for( wxString fn = fileList.GetFirstLine(); !fileList.Eof(); fn = fileList.GetNextLine() )
        removeFile( fn, unremovedFiles );

    removeFile( aFileName.GetFullPath(), unremovedFiles );

    if( !unremovedFiles.IsEmpty() )
    {
        wxString msg = _( "The following automatically saved file(s) could not be removed\n" );

        for( size_t i = 0; i < unremovedFiles.GetCount(); i++ )
            msg += unremovedFiles[i] + wxS( "\n" );

        msg += _( "Manual removal will be required for the file(s) above." );
        wxMessageBox( msg, Pgm().App().GetAppDisplayName(), wxOK | wxICON_EXCLAMATION, this );
    }
}


const wxString& SCH_EDIT_FRAME::getAutoSaveFileName() const
{
    static wxString autoSaveFileName( wxS( "#auto_saved_files#" ) );

    return autoSaveFileName;
}
