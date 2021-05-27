/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2013 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2013 CERN (www.cern.ch)
 * Copyright (C) 1992-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <advanced_config.h>
#include <class_library.h>
#include <confirm.h>
#include <connection_graph.h>
#include <dialog_migrate_buses.h>
#include <dialog_symbol_remap.h>
#include <eeschema_settings.h>
#include <gestfich.h>
#include <id.h>
#include <kiface_i.h>
#include <kiplatform/app.h>
#include <pgm_base.h>
#include <profile.h>
#include <project/project_file.h>
#include <project_rescue.h>
#include <wx_html_report_box.h>
#include <dialog_HTML_reporter_base.h>
#include <reporter.h>
#include <richio.h>
#include <sch_edit_frame.h>
#include <sch_plugins/legacy/sch_legacy_plugin.h>
#include <sch_file_versions.h>
#include <sch_sheet.h>
#include <sch_sheet_path.h>
#include <schematic.h>
#include <settings/settings_manager.h>
#include <tool/actions.h>
#include <tool/tool_manager.h>
#include <tools/sch_editor_control.h>
#include <trace_helpers.h>
#include <widgets/infobar.h>
#include <wildcards_and_files_ext.h>
#include <drawing_sheet/ds_data_model.h>
#include <wx/ffile.h>
#include <tools/ee_inspection_tool.h>
#include <paths.h>

bool SCH_EDIT_FRAME::SaveEEFile( SCH_SHEET* aSheet, bool aSaveUnderNewName )
{
    wxString msg;
    wxFileName schematicFileName;
    wxFileName oldFileName;
    bool success;

    if( aSheet == nullptr )
        aSheet = GetCurrentSheet().Last();

    SCH_SCREEN* screen = aSheet->GetScreen();

    wxCHECK( screen, false );

    // If no name exists in the window yet - save as new.
    if( screen->GetFileName().IsEmpty() )
        aSaveUnderNewName = true;

    // Construct the name of the file to be saved
    schematicFileName = Prj().AbsolutePath( screen->GetFileName() );
    oldFileName = schematicFileName;

    if( aSaveUnderNewName )
    {
        wxFileName savePath( Prj().GetProjectFullName() );

        if( !savePath.IsOk() || !savePath.IsDirWritable() )
        {
            savePath = GetMruPath();

            if( !savePath.IsOk() || !savePath.IsDirWritable() )
                savePath = PATHS::GetDefaultUserProjectsPath();
        }

        wxFileDialog dlg( this, _( "Schematic Files" ), savePath.GetPath(),
                          schematicFileName.GetFullName(), KiCadSchematicFileWildcard(),
                          wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

        if( dlg.ShowModal() == wxID_CANCEL )
            return false;

        schematicFileName = dlg.GetPath();

        if( schematicFileName.GetExt().IsEmpty() )
            schematicFileName.SetExt( KiCadSchematicFileExtension );
    }

    if( !IsWritable( schematicFileName ) )
        return false;

    // This is a new schematic file so make sure it has a unique ID.
    if( aSaveUnderNewName && schematicFileName != oldFileName )
        screen->AssignNewUuid();

    wxFileName tempFile( schematicFileName );
    tempFile.SetName( wxT( "." ) + tempFile.GetName() );
    tempFile.SetExt( tempFile.GetExt() + wxT( "$" ) );

    // Save
    wxLogTrace( traceAutoSave, "Saving file " + schematicFileName.GetFullPath() );

    SCH_IO_MGR::SCH_FILE_T pluginType = SCH_IO_MGR::GuessPluginTypeFromSchPath(
            schematicFileName.GetFullPath() );
    SCH_PLUGIN::SCH_PLUGIN_RELEASER pi( SCH_IO_MGR::FindPlugin( pluginType ) );

    try
    {
        pi->Save( tempFile.GetFullPath(), aSheet, &Schematic() );
        success = true;
    }
    catch( const IO_ERROR& ioe )
    {
        msg.Printf( _( "Error saving schematic file \"%s\".\n%s" ),
                    schematicFileName.GetFullPath(), ioe.What() );
        DisplayError( this, msg );

        msg.Printf( _( "Failed to create temporary file \"%s\"" ), tempFile.GetFullPath() );
        SetMsgPanel( wxEmptyString, msg );

        // In case we started a file but didn't fully write it, clean up
        wxRemoveFile( tempFile.GetFullPath() );

        success = false;
    }

    if( success )
    {
        // Replace the original with the temporary file we just wrote
        success = wxRenameFile( tempFile.GetFullPath(), schematicFileName.GetFullPath() );

        if( !success )
        {
            msg.Printf( _( "Error saving schematic file \"%s\".\n"
                           "Failed to rename temporary file %s" ),
                        schematicFileName.GetFullPath(), tempFile.GetFullPath() );
            DisplayError( this, msg );

            msg.Printf( _( "Failed to rename temporary file \"%s\"" ), tempFile.GetFullPath() );
            SetMsgPanel( wxEmptyString, msg );
        }
    }

    if( success )
    {
        // Delete auto save file.
        wxFileName autoSaveFileName = schematicFileName;
        autoSaveFileName.SetName( GetAutoSaveFilePrefix() + schematicFileName.GetName() );

        if( autoSaveFileName.FileExists() )
        {
            wxLogTrace( traceAutoSave,
                        wxT( "Removing auto save file <" ) + autoSaveFileName.GetFullPath() +
                        wxT( ">" ) );

            wxRemoveFile( autoSaveFileName.GetFullPath() );
        }

        // Update the screen and frame info and reset the lock file.
        if( aSaveUnderNewName )
        {
            screen->SetFileName( schematicFileName.GetFullPath() );
            aSheet->SetFileName( schematicFileName.GetFullPath() );
            LockFile( schematicFileName.GetFullPath() );

            UpdateFileHistory( schematicFileName.GetFullPath() );
        }

        screen->ClrSave();
        screen->ClrModify();
        UpdateTitle();

        msg.Printf( _( "File \"%s\" saved." ),  screen->GetFileName() );
        SetStatusText( msg, 0 );
    }
    else
    {
        DisplayError( this, _( "File write operation failed." ) );
    }

    return success;
}


void SCH_EDIT_FRAME::Save_File( bool doSaveAs )
{
    if( doSaveAs )
    {
        if( SaveEEFile( nullptr, true ) )
        {
            SCH_SCREEN* screen = GetScreen();

            wxCHECK( screen, /* void */ );

            wxFileName fn = screen->GetFileName();

            if( fn.GetExt() == LegacySchematicFileExtension )
                CreateArchiveLibraryCacheFile( true );

            // If we are saving under a new name, and don't have a real project yet, create one
            fn.SetExt( ProjectFileExtension );

            if( fn.IsDirWritable() && !fn.FileExists() )
            {
                Prj().SetReadOnly( false );
                GetSettingsManager()->SaveProjectAs( fn.GetFullPath() );
            }
        }
    }
    else
    {
        SaveEEFile( nullptr );
    }

    UpdateTitle();
}


bool SCH_EDIT_FRAME::OpenProjectFiles( const std::vector<wxString>& aFileSet, int aCtl )
{
    // implement the pseudo code from KIWAY_PLAYER.h:
    wxString msg;

    auto cfg = dynamic_cast<EESCHEMA_SETTINGS*>( Kiface().KifaceSettings() );

    // This is for python:
    if( aFileSet.size() != 1 )
    {
        msg.Printf( "Eeschema:%s() takes only a single filename.", __WXFUNCTION__ );
        DisplayError( this, msg );
        return false;
    }

    wxString fullFileName( aFileSet[0] );

    // We insist on caller sending us an absolute path, if it does not, we say it's a bug.
    wxASSERT_MSG( wxFileName( fullFileName ).IsAbsolute(), wxT( "Path is not absolute!" ) );

    if( !LockFile( fullFileName ) )
    {
        msg.Printf( _( "Schematic file \"%s\" is already open." ), fullFileName );
        DisplayError( this, msg );
        return false;
    }

    if( !AskToSaveChanges() )
        return false;

#ifdef PROFILE
    PROF_COUNTER openFiles( "OpenProjectFile" );
#endif

    wxFileName pro = fullFileName;
    pro.SetExt( ProjectFileExtension );

    bool is_new = !wxFileName::IsFileReadable( fullFileName );

    // If its a non-existent schematic and caller thinks it exists
    if( is_new && !( aCtl & KICTL_CREATE ) )
    {
        // notify user that fullFileName does not exist, ask if user wants to create it.
        msg.Printf( _( "Schematic \"%s\" does not exist.  Do you wish to create it?" ),
                    fullFileName );

        if( !IsOK( this, msg ) )
            return false;
    }

    // Loading a complex project and build data can be time
    // consumming, so display a busy cursor
    wxBusyCursor dummy;

    // unload current project file before loading new
    {
        SetScreen( nullptr );
        m_toolManager->GetTool<EE_INSPECTION_TOOL>()->Reset( TOOL_BASE::MODEL_RELOAD );
        CreateScreens();
    }

    SetStatusText( wxEmptyString );
    m_infoBar->Dismiss();

    SCH_IO_MGR::SCH_FILE_T schFileType = SCH_IO_MGR::GuessPluginTypeFromSchPath( fullFileName );

    // PROJECT::SetProjectFullName() is an impactful function.  It should only be
    // called under carefully considered circumstances.

    // The calling code should know not to ask me here to change projects unless
    // it knows what consequences that will have on other KIFACEs running and using
    // this same PROJECT.  It can be very harmful if that calling code is stupid.

    // NOTE: The calling code should never call this in hosted (non-standalone) mode with a
    // different project than what has been loaded by the manager frame.  This will crash.

    bool differentProject = pro.GetFullPath() != Prj().GetProjectFullName();

    if( differentProject )
    {
        if( !Prj().IsNullProject() )
            GetSettingsManager()->SaveProject();

        Schematic().SetProject( nullptr );
        GetSettingsManager()->UnloadProject( &Prj(), false );

        GetSettingsManager()->LoadProject( pro.GetFullPath() );

        wxFileName legacyPro( pro );
        legacyPro.SetExt( LegacyProjectFileExtension );

        // Do not allow saving a project if one doesn't exist.  This normally happens if we are
        // standalone and opening a schematic that has been moved from its project folder.
        if( !pro.Exists() && !legacyPro.Exists() && !( aCtl & KICTL_CREATE ) )
            Prj().SetReadOnly();

        CreateScreens();
    }

    if( schFileType == SCH_IO_MGR::SCH_LEGACY )
    {
        // Don't reload the symbol libraries if we are just launching Eeschema from KiCad again.
        // They are already saved in the kiface project object.
        if( differentProject || !Prj().GetElem( PROJECT::ELEM_SCH_PART_LIBS ) )
        {
            // load the libraries here, not in SCH_SCREEN::Draw() which is a context
            // that will not tolerate DisplayError() dialog since we're already in an
            // event handler in there.
            // And when a schematic file is loaded, we need these libs to initialize
            // some parameters (links to PART LIB, dangling ends ...)
            Prj().SetElem( PROJECT::ELEM_SCH_PART_LIBS, nullptr );
            Prj().SchLibs();
        }
    }
    else
    {
        // No legacy symbol libraries including the cache are loaded with the new file format.
        Prj().SetElem( PROJECT::ELEM_SCH_PART_LIBS, nullptr );
    }

    // Load the symbol library table, this will be used forever more.
    Prj().SetElem( PROJECT::ELEM_SYMBOL_LIB_TABLE, nullptr );
    Prj().SchSymbolLibTable();

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

    if( is_new )
    {
        // mark new, unsaved file as modified.
        GetScreen()->SetModify();
        GetScreen()->SetFileName( fullFileName );
    }
    else
    {
        // This will rename the file if there is an autosave and the user want to recover.
		CheckForAutoSaveFile( fullFileName );

        SetScreen( nullptr );

        SCH_PLUGIN* plugin = SCH_IO_MGR::FindPlugin( schFileType );
        SCH_PLUGIN::SCH_PLUGIN_RELEASER pi( plugin );

        bool failedLoad = false;
        try
        {
            Schematic().SetRoot( pi->Load( fullFileName, &Schematic() ) );

            if( !pi->GetError().IsEmpty() )
            {
                DisplayErrorMessage( this,
                                     _( "The entire schematic could not be loaded.  Errors "
                                        "occurred attempting to load \nhierarchical sheet "
                                        "schematics." ),
                                     pi->GetError() );
            }
        }
        catch( const IO_ERROR& ioe )
        {
            msg.Printf( _( "Error loading schematic file \"%s\"" ),
                        fullFileName);
            DisplayErrorMessage( this, msg, ioe.What() );

            failedLoad = true;
        }
        catch( const std::bad_alloc& )
        {
            msg.Printf( _( "Memory exhausted loading schematic file \"%s\"" ), fullFileName );
            DisplayErrorMessage( this, msg );

            failedLoad = true;
        }

        if( failedLoad )
        {
            // Do not leave g_RootSheet == NULL because it is expected to be
            // a valid sheet. Therefore create a dummy empty root sheet and screen.
            CreateScreens();
            m_toolManager->RunAction( ACTIONS::zoomFitScreen, true );

            msg.Printf( _( "Failed to load \"%s\"" ), fullFileName );
            SetMsgPanel( wxEmptyString, msg );

            return false;
        }

        // It's possible the schematic parser fixed errors due to bugs so warn the user
        // that the schematic has been fixed (modified).
        SCH_SHEET_LIST sheetList = Schematic().GetSheets();

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
            // Convert old projects over to use symbol library table.
            if( schematic.HasNoFullyDefinedLibIds() )
            {
                DIALOG_SYMBOL_REMAP dlgRemap( this );

                dlgRemap.ShowQuasiModal();
            }
            else
            {
                // Double check to ensure no legacy library list entries have been
                // added to the projec file symbol library list.
                wxString paths;
                wxArrayString libNames;

                PART_LIBS::LibNamesAndPaths( &Prj(), false, &paths, &libNames );

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
                    PART_LIBS::LibNamesAndPaths( &Prj(), true, &paths, &libNames );
                }

                if( !cfg || !cfg->m_RescueNeverShow )
                {
                    SCH_EDITOR_CONTROL* editor = m_toolManager->GetTool<SCH_EDITOR_CONTROL>();
                    editor->RescueSymbolLibTableProject( false );
                }
            }

            // Ensure there is only one legacy library loaded and that it is the cache library.
            PART_LIBS* legacyLibs = Schematic().Prj().SchLibs();

            if( legacyLibs->GetLibraryCount() == 0 )
            {
                wxString extMsg;
                wxFileName cacheFn = pro;

                cacheFn.SetName( cacheFn.GetName() + "-cache" );
                cacheFn.SetExt( LegacySymbolLibFileExtension );

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
            sheetList.UpdateSymbolInstances( Schematic().RootScreen()->GetSymbolInstances() );
            sheetList.UpdateSheetInstances( Schematic().RootScreen()->GetSheetInstances() );
        }

        Schematic().ConnectionGraph()->Reset();

        SetScreen( GetCurrentSheet().LastScreen() );

        // Migrate conflicting bus definitions
        // TODO(JE) This should only run once based on schematic file version
        if( Schematic().ConnectionGraph()->GetBusesNeedingMigration().size() > 0 )
        {
            DIALOG_MIGRATE_BUSES dlg( this );
            dlg.ShowQuasiModal();
            RecalculateConnections( NO_CLEANUP );
            OnModify();
        }

        RecalculateConnections( GLOBAL_CLEANUP );
        ClearUndoRedoList();
    }

    // Load any exclusions from the project file
    ResolveERCExclusions();

    initScreenZoom();
    SetSheetNumberAndCount();

    RecomputeIntersheetRefs();
    GetCurrentSheet().UpdateAllScreenReferences();

    // re-create junctions if needed. Eeschema optimizes wires by merging
    // colinear segments. If a schematic is saved without a valid
    // cache library or missing installed libraries, this can cause connectivity errors
    // unless junctions are added.
    if( schFileType == SCH_IO_MGR::SCH_LEGACY )
        FixupJunctions();

    SyncView();
    GetScreen()->ClearDrawingState();

    UpdateTitle();

    wxFileName fn = Prj().AbsolutePath( GetScreen()->GetFileName() );

    if( fn.FileExists() && !fn.IsFileWritable() )
    {
        m_infoBar->RemoveAllButtons();
        m_infoBar->AddCloseButton();
        m_infoBar->ShowMessage( _( "Schematic file is read only." ), wxICON_WARNING );
    }

#ifdef PROFILE
    openFiles.Show();
#endif

    return true;
}


bool SCH_EDIT_FRAME::AppendSchematic()
{
    wxString    fullFileName;
    SCH_SCREEN* screen = GetScreen();

    if( !screen )
    {
        wxLogError( wxT( "Document not ready, cannot import" ) );
        return false;
    }

    // open file chooser dialog
    wxString path = wxPathOnly( Prj().GetProjectFullName() );

    wxFileDialog dlg( this, _( "Append Schematic" ), path, wxEmptyString,
                      KiCadSchematicFileWildcard(), wxFD_OPEN | wxFD_FILE_MUST_EXIST );

    if( dlg.ShowModal() == wxID_CANCEL )
        return false;

    fullFileName = dlg.GetPath();

    if( !LoadSheetFromFile( GetCurrentSheet().Last(), &GetCurrentSheet(), fullFileName ) )
        return false;

    initScreenZoom();
    SetSheetNumberAndCount();

    SyncView();
    OnModify();
    HardRedraw();   // Full reinit of the current screen and the display.

    return true;
}


void SCH_EDIT_FRAME::OnAppendProject( wxCommandEvent& event )
{
    if( GetScreen() && GetScreen()->IsModified() )
    {
        wxString msg = _( "This operation cannot be undone.\n\n"
                          "Do you want to save the current document before proceeding?" );

        if( IsOK( this, msg ) )
            SaveProject();
    }

    AppendSchematic();
}


void SCH_EDIT_FRAME::OnImportProject( wxCommandEvent& aEvent )
{
    if( !AskToSaveChanges() )
        return;

    // Set the project location if none is set or if we are running in standalone mode
    bool     setProject = Prj().GetProjectFullName().IsEmpty() || Kiface().IsSingle();
    wxString path = wxPathOnly( Prj().GetProjectFullName() );

    std::list<std::pair<const wxString, const SCH_IO_MGR::SCH_FILE_T>> loaders;

    // Import Altium schematic files.
    loaders.emplace_back( AltiumSchematicFileWildcard(), SCH_IO_MGR::SCH_ALTIUM );

    // Import CADSTAR Schematic Archive files.
    loaders.emplace_back( CadstarSchematicArchiveFileWildcard(), SCH_IO_MGR::SCH_CADSTAR_ARCHIVE );

    // Import Eagle schematic files.
    loaders.emplace_back( EagleSchematicFileWildcard(),  SCH_IO_MGR::SCH_EAGLE );

    wxString fileFilters;
    wxString allWildcards;

    for( auto& loader : loaders )
    {
        if( !fileFilters.IsEmpty() )
            fileFilters += wxChar( '|' );

        fileFilters += wxGetTranslation( loader.first );

        SCH_PLUGIN::SCH_PLUGIN_RELEASER plugin( SCH_IO_MGR::FindPlugin( loader.second ) );
        wxCHECK( plugin, /*void*/ );
        allWildcards += "*." + formatWildcardExt( plugin->GetFileExtension() ) + ";";
    }

    fileFilters = _( "All supported formats|" ) + allWildcards + "|" + fileFilters;

    wxFileDialog dlg( this, _( "Import Schematic" ), path, wxEmptyString, fileFilters,
                      wxFD_OPEN | wxFD_FILE_MUST_EXIST ); // TODO

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    if( setProject )
    {
        Schematic().SetProject( nullptr );
        GetSettingsManager()->UnloadProject( &Prj(), false );

        Schematic().Reset();

        wxFileName projectFn( dlg.GetPath() );
        projectFn.SetExt( ProjectFileExtension );
        GetSettingsManager()->LoadProject( projectFn.GetFullPath() );

        Schematic().SetProject( &Prj() );
    }

    wxFileName fn = dlg.GetPath();

    SCH_IO_MGR::SCH_FILE_T pluginType = SCH_IO_MGR::SCH_FILE_T::SCH_FILE_UNKNOWN;

    for( auto& loader : loaders )
    {
        if( fn.GetExt().CmpNoCase( SCH_IO_MGR::GetFileExtension( loader.second ) ) == 0 )
        {
            pluginType = loader.second;
            break;
        }
    }

    if( pluginType == SCH_IO_MGR::SCH_FILE_T::SCH_FILE_UNKNOWN )
    {
        wxLogError( wxString::Format( "unexpected file extension: %s", fn.GetExt() ) );
        return;
    }

    importFile( dlg.GetPath(), pluginType );
}


bool SCH_EDIT_FRAME::SaveProject()
{
    wxString msg;
    SCH_SCREEN* screen;
    SCH_SCREENS screens( Schematic().Root() );
    bool success = true;
    bool updateFileType = false;

    // I want to see it in the debugger, show me the string!  Can't do that with wxFileName.
    wxString    fileName = Prj().AbsolutePath( Schematic().Root().GetFileName() );
    wxFileName  fn = fileName;

    // If this a new schematic without a project and we are in the stand alone mode.  All new
    // sheets that are not loaded from an existing file will have to be saved to a new path
    // along with the root sheet.
    if( Prj().GetProjectFullName().IsEmpty() )
    {
        // This should only be possible in stand alone mode.
        wxCHECK( Kiface().IsSingle(), false );

        wxFileDialog dlg( this, _( "Schematic Files" ), fn.GetPath(), fn.GetFullName(),
                          KiCadSchematicFileWildcard(), wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

        if( dlg.ShowModal() == wxID_CANCEL )
            return false;

        wxFileName newFileName = dlg.GetPath();

        if( newFileName.GetExt().IsEmpty() )
            newFileName.SetExt( KiCadSchematicFileExtension );

        if( !newFileName.DirExists() && !newFileName.Mkdir() )
        {
            msg.Printf( _( "Cannot create folder \"%s\"." ), newFileName.GetPath() );

            wxMessageDialog dlgBadPath( this, msg, _( "Error" ),
                                        wxOK | wxICON_EXCLAMATION | wxCENTER );

            dlgBadPath.ShowModal();
            return false;
        }

        if( !newFileName.IsDirWritable() )
        {
            msg.Printf( _( "You do not have write permissions to folder \"%s\"." ),
                        newFileName.GetPath() );

            wxMessageDialog dlgBadPerms( this, msg, _( "Error" ),
                                         wxOK | wxICON_EXCLAMATION | wxCENTER );

            dlgBadPerms.ShowModal();
            return false;
        }

        Schematic().Root().SetFileName( newFileName.GetFullName() );
        Schematic().RootScreen()->SetFileName( newFileName.GetFullPath() );

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
                        wxT( "Changing schematic file name path from '%s' to '%s'." ),
                        screen->GetFileName(), tmp.GetFullPath() );

            if( !tmp.DirExists() && !tmp.Mkdir() )
            {
                msg.Printf( _( "Cannot create folder \"%s\"." ), newFileName.GetPath() );

                wxMessageDialog dlgBadFilePath( this, msg, _( "Error" ),
                                                wxOK | wxICON_EXCLAMATION | wxCENTER );

                dlgBadFilePath.ShowModal();
                return false;
            }

            screen->SetFileName( tmp.GetFullPath() );
        }

        // Attempt to make sheet file name paths relative to the new root schematic path.
        SCH_SHEET_LIST sheets = Schematic().GetSheets();

        for( SCH_SHEET_PATH& sheet : sheets )
        {
            if( sheet.Last()->IsRootSheet() )
                continue;

            sheet.MakeFilePathRelativeToParentSheet();
        }
    }

    // Warn user on potential file overwrite.  This can happen on shared sheets.
    wxArrayString overwrittenFiles;

    for( size_t i = 0; i < screens.GetCount(); i++ )
    {
        screen = screens.GetScreen( i );

        wxCHECK2( screen, continue );

        // Convert legacy schematics file name extensions for the new format.
        wxFileName tmpFn = screen->GetFileName();

        if( !tmpFn.IsOk() )
            continue;

        if( tmpFn.GetExt() == KiCadSchematicFileExtension )
            continue;

        tmpFn.SetExt( KiCadSchematicFileExtension );

        if( tmpFn.FileExists() )
            overwrittenFiles.Add( tmpFn.GetFullPath() );
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
        wxFileName tmpFn = screen->GetFileName();

        if( tmpFn.IsOk() && tmpFn.GetExt() != KiCadSchematicFileExtension )
        {
            updateFileType = true;
            tmpFn.SetExt( KiCadSchematicFileExtension );

            for( auto item : screen->Items().OfType( SCH_SHEET_T ) )
            {
                SCH_SHEET* sheet = static_cast<SCH_SHEET*>( item );
                wxFileName sheetFileName = sheet->GetFileName();

                if( !sheetFileName.IsOk() || sheetFileName.GetExt() == KiCadSchematicFileExtension )
                    continue;

                sheetFileName.SetExt( KiCadSchematicFileExtension );
                sheet->SetFileName( sheetFileName.GetFullPath() );
                UpdateItem( sheet );
            }

            screen->SetFileName( tmpFn.GetFullPath() );
        }

        std::vector<SCH_SHEET_PATH>& sheets = screen->GetClientSheetPaths();

        if( sheets.size() == 1 )
            screen->SetVirtualPageNumber( 1 );
        else
            screen->SetVirtualPageNumber( 0 );  // multiple uses; no way to store the real sheet #

        success &= SaveEEFile( screens.GetSheet( i ) );
    }

    if( updateFileType )
        UpdateFileHistory( Schematic().RootScreen()->GetFileName() );

    // Save the sheet name map to the project file
    std::vector<FILE_INFO_PAIR>& sheets = Prj().GetProjectFile().GetSheets();
    sheets.clear();

    for( SCH_SHEET_PATH& sheetPath : Schematic().GetSheets() )
    {
        SCH_SHEET* sheet = sheetPath.Last();

        wxCHECK2( sheet, continue );

        // Use the schematic UUID for the root sheet.
        if( sheet->IsRootSheet() )
        {
            screen = sheet->GetScreen();

            wxCHECK2( screen, continue );

            sheets.emplace_back( std::make_pair( screen->GetUuid(), sheet->GetName() ) );
        }
        else
        {
            sheets.emplace_back( std::make_pair( sheet->m_Uuid, sheet->GetName() ) );
        }
    }

    if( !Prj().IsNullProject() )
        Pgm().GetSettingsManager().SaveProject();

    if( !Kiface().IsSingle() )
    {
        WX_STRING_REPORTER backupReporter( &msg );

        if( !GetSettingsManager()->TriggerBackupIfNeeded( backupReporter ) )
            SetStatusText( msg, 0 );
    }

    UpdateTitle();

    m_infoBar->DismissOutdatedSave();

    return success;
}


bool SCH_EDIT_FRAME::doAutoSave()
{
    wxFileName  tmpFileName = Schematic().Root().GetFileName();
    wxFileName  fn = tmpFileName;
    wxFileName  tmp;
    SCH_SCREENS screens( Schematic().Root() );

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
        if( !screens.GetScreen( i )->IsSave() )
            continue;

        tmpFileName = fn = screens.GetScreen( i )->GetFileName();

        // Auto save file name is the normal file name prefixed with GetAutoSavePrefix().
        fn.SetName( GetAutoSaveFilePrefix() + fn.GetName() );

        screens.GetScreen( i )->SetFileName( fn.GetFullPath() );

        if( SaveEEFile( screens.GetSheet( i ), false ) )
            screens.GetScreen( i )->SetModify();
        else
            autoSaveOk = false;

        screens.GetScreen( i )->SetFileName( tmpFileName.GetFullPath() );
    }

    if( autoSaveOk )
    {
        m_autoSaveState = false;

        if( !Kiface().IsSingle() &&
            GetSettingsManager()->GetCommonSettings()->m_Backup.backup_on_autosave )
        {
            GetSettingsManager()->TriggerBackupIfNeeded( NULL_REPORTER::GetInstance() );
        }
    }

    SetTitle( title );

    return autoSaveOk;
}


bool SCH_EDIT_FRAME::importFile( const wxString& aFileName, int aFileType )
{
    wxFileName             newfilename;
    SCH_SHEET_LIST         sheetList = Schematic().GetSheets();
    SCH_IO_MGR::SCH_FILE_T fileType = (SCH_IO_MGR::SCH_FILE_T) aFileType;

    switch( fileType )
    {
    case SCH_IO_MGR::SCH_ALTIUM:
    case SCH_IO_MGR::SCH_CADSTAR_ARCHIVE:
    case SCH_IO_MGR::SCH_EAGLE:
        // We insist on caller sending us an absolute path, if it does not, we say it's a bug.
        wxASSERT_MSG( wxFileName( aFileName ).IsAbsolute(),
                      wxT( "Import schematic caller didn't send full filename" ) );

        if( !LockFile( aFileName ) )
        {
            wxString msg = wxString::Format( _( "Schematic file \"%s\" is already open." ),
                                             aFileName );
            DisplayError( this, msg );
            return false;
        }

        try
        {
            SCH_PLUGIN::SCH_PLUGIN_RELEASER pi( SCH_IO_MGR::FindPlugin( fileType ) );
            DIALOG_HTML_REPORTER*           reporter = new DIALOG_HTML_REPORTER( this );

            pi->SetReporter( reporter->m_Reporter );
            Schematic().SetRoot( pi->Load( aFileName, &Schematic() ) );

            if( reporter->m_Reporter->HasMessage() )
                reporter->ShowModal();

            pi->SetReporter( &WXLOG_REPORTER::GetInstance() );
            delete reporter;

            // Non-KiCad schematics do not use a drawing-sheet (or if they do, it works differently
            // to KiCad), so set it to an empty one
            DS_DATA_MODEL& drawingSheet = DS_DATA_MODEL::GetTheInstance();
            drawingSheet.SetEmptyLayout();

            BASE_SCREEN::m_PageLayoutDescrFileName = "empty.kicad_wks";
            wxFileName layoutfn( Prj().GetProjectPath(), BASE_SCREEN::m_PageLayoutDescrFileName );
            wxFFile layoutfile;

            if( layoutfile.Open( layoutfn.GetFullPath(), "wb" ) )
            {
                layoutfile.Write( DS_DATA_MODEL::EmptyLayout() );
                layoutfile.Close();
            }

            newfilename.SetPath( Prj().GetProjectPath() );
            newfilename.SetName( Prj().GetProjectName() );
            newfilename.SetExt( KiCadSchematicFileExtension );

            SetScreen( GetCurrentSheet().LastScreen() );

            Schematic().Root().SetFileName( newfilename.GetFullPath() );
            GetScreen()->SetFileName( newfilename.GetFullPath() );
            GetScreen()->SetModify();

            // Only fix junctions for CADSTAR importer for now as it may cause issues with
            // other importers
            if( fileType == SCH_IO_MGR::SCH_CADSTAR_ARCHIVE )
                FixupJunctions();

            // Only perform the dangling end test on root sheet.
            GetScreen()->TestDanglingEnds();

            ClearUndoRedoList();

            initScreenZoom();
            SetSheetNumberAndCount();
            SyncView();
            UpdateTitle();
        }
        catch( const IO_ERROR& ioe )
        {
            // Do not leave g_RootSheet == NULL because it is expected to be
            // a valid sheet. Therefore create a dummy empty root sheet and screen.
            CreateScreens();
            m_toolManager->RunAction( ACTIONS::zoomFitScreen, true );

            wxString msg;
            msg.Printf( _( "Error loading schematic \"%s\".\n%s" ), aFileName, ioe.What() );
            DisplayError( this, msg );

            msg.Printf( _( "Failed to load \"%s\"" ), aFileName );
            SetMsgPanel( wxEmptyString, msg );

            return false;
        }

        return true;

    default:
        return false;
    }
}


bool SCH_EDIT_FRAME::AskToSaveChanges()
{
    SCH_SCREENS screenList( Schematic().Root() );

    // Save any currently open and modified project files.
    for( SCH_SCREEN* screen = screenList.GetFirst(); screen; screen = screenList.GetNext() )
    {
        if( screen->IsModify() )
        {
            if( !HandleUnsavedChanges( this, _( "The current schematic has been modified.  "
                                                "Save changes?" ),
                                       [&]()->bool { return SaveProject(); } ) )
            {
                return false;
            }
        }
    }

    return true;
}
