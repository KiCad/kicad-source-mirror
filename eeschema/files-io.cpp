/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2013 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2013 CERN (www.cern.ch)
 * Copyright (C) 1992-2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <fctsys.h>
#include <sch_draw_panel.h>
#include <confirm.h>
#include <gestfich.h>
#include <sch_edit_frame.h>
#include <pgm_base.h>
#include <kiface_i.h>
#include <richio.h>
#include <trace_helpers.h>
#include <tool/tool_manager.h>
#include <eeschema_id.h>
#include <class_library.h>
#include <lib_edit_frame.h>
#include <sch_sheet.h>
#include <sch_sheet_path.h>
#include <sch_component.h>
#include <wildcards_and_files_ext.h>
#include <project_rescue.h>
#include <eeschema_config.h>
#include <sch_legacy_plugin.h>
#include <sch_eagle_plugin.h>
#include <symbol_lib_table.h>
#include <dialog_symbol_remap.h>
#include <dialog_migrate_buses.h>
#include <ws_data_model.h>
#include <connection_graph.h>
#include <tool/actions.h>

bool SCH_EDIT_FRAME::SaveEEFile( SCH_SCREEN* aScreen, bool aSaveUnderNewName,
                                 bool aCreateBackupFile )
{
    wxString msg;
    wxFileName schematicFileName;
    bool success;

    if( aScreen == NULL )
        aScreen = GetScreen();

    // If no name exists in the window yet - save as new.
    if( aScreen->GetFileName().IsEmpty() )
        aSaveUnderNewName = true;

    // Construct the name of the file to be saved
    schematicFileName = Prj().AbsolutePath( aScreen->GetFileName() );

    if( aSaveUnderNewName )
    {
        wxFileDialog dlg( this, _( "Schematic Files" ), wxPathOnly( Prj().GetProjectFullName() ),
                          schematicFileName.GetFullName(), SchematicFileWildcard(),
                          wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

        if( dlg.ShowModal() == wxID_CANCEL )
            return false;

        schematicFileName = dlg.GetPath();

        if( schematicFileName.GetExt() != SchematicFileExtension )
            schematicFileName.SetExt( SchematicFileExtension );
    }

    if( !IsWritable( schematicFileName ) )
        return false;

    // Create backup if requested
    if( aCreateBackupFile && schematicFileName.FileExists() )
    {
        wxFileName backupFileName = schematicFileName;

        // Rename the old file to a '-bak' suffixed one:
        backupFileName.SetExt( schematicFileName.GetExt() + GetBackupSuffix()  );

        if( backupFileName.FileExists() )
            wxRemoveFile( backupFileName.GetFullPath() );

        if( !wxRenameFile( schematicFileName.GetFullPath(), backupFileName.GetFullPath() ) )
        {
            msg.Printf( _( "Could not save backup of file \"%s\"" ),
                        GetChars( schematicFileName.GetFullPath() ) );
            DisplayError( this, msg );
        }
    }

    // Save
    wxLogTrace( traceAutoSave,
                wxT( "Saving file <" ) + schematicFileName.GetFullPath() + wxT( ">" ) );

    SCH_PLUGIN::SCH_PLUGIN_RELEASER pi( SCH_IO_MGR::FindPlugin( SCH_IO_MGR::SCH_LEGACY ) );

    try
    {
        pi->Save( schematicFileName.GetFullPath(), aScreen, &Kiway() );
        success = true;
    }
    catch( const IO_ERROR& ioe )
    {
        msg.Printf( _( "Error saving schematic file \"%s\".\n%s" ),
                    GetChars( schematicFileName.GetFullPath() ), GetChars( ioe.What() ) );
        DisplayError( this, msg );

        msg.Printf( _( "Failed to save \"%s\"" ), GetChars( schematicFileName.GetFullPath() ) );
        AppendMsgPanel( wxEmptyString, msg, CYAN );

        success = false;
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
            aScreen->SetFileName( schematicFileName.GetFullPath() );
            LockFile( schematicFileName.GetFullPath() );
        }

        aScreen->ClrSave();
        aScreen->ClrModify();

        msg.Printf( _( "File %s saved" ), GetChars( aScreen->GetFileName() ) );
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
        if( SaveEEFile( NULL, true ) )
            CreateArchiveLibraryCacheFile( true );
    }
    else
    {
        SaveEEFile( NULL );
    }

    UpdateTitle();
}


bool SCH_EDIT_FRAME::OpenProjectFiles( const std::vector<wxString>& aFileSet, int aCtl )
{
    // implement the pseudo code from KIWAY_PLAYER.h:

    // This is for python:
    if( aFileSet.size() != 1 )
    {
        UTF8 msg = StrPrintf( "Eeschema:%s() takes only a single filename.", __func__ );
        DisplayError( this, msg );
        return false;
    }

    wxString    fullFileName( aFileSet[0] );

    // We insist on caller sending us an absolute path, if it does not, we say it's a bug.
    wxASSERT_MSG( wxFileName( fullFileName ).IsAbsolute(), wxT( "Path is not absolute!" ) );

    if( !LockFile( fullFileName ) )
    {
        wxString msg = wxString::Format( _( "Schematic file \"%s\" is already open." ),
                                         fullFileName );
        DisplayError( this, msg );
        return false;
    }

    if( !AskToSaveChanges() )
        return false;

    wxFileName pro = fullFileName;
    pro.SetExt( ProjectFileExtension );

    bool is_new = !wxFileName::IsFileReadable( fullFileName );

    // If its a non-existent schematic and caller thinks it exists
    if( is_new && !( aCtl & KICTL_CREATE ) )
    {
        // notify user that fullFileName does not exist, ask if user wants to create it.
        wxString ask = wxString::Format( _( "Schematic \"%s\" does not exist.  Do you wish to create it?" ),
                                         fullFileName );
        if( !IsOK( this, ask ) )
            return false;
    }

    // unload current project file before loading new
    {
        SetScreen( nullptr );
        delete g_RootSheet;
        if( g_CurrentSheet )
            g_CurrentSheet->clear();
        g_RootSheet = nullptr;

        CreateScreens();
    }

    GetScreen()->SetFileName( fullFileName );
    g_RootSheet->SetFileName( fullFileName );

    SetStatusText( wxEmptyString );
    ClearMsgPanel();

    // PROJECT::SetProjectFullName() is an impactful function.  It should only be
    // called under carefully considered circumstances.

    // The calling code should know not to ask me here to change projects unless
    // it knows what consequences that will have on other KIFACEs running and using
    // this same PROJECT.  It can be very harmful if that calling code is stupid.

    // Don't reload the symbol libraries if we are just launching Eeschema from KiCad again.
    // They are already saved in the kiface project object.
    if( pro.GetFullPath() != Prj().GetProjectFullName()
      || !Prj().GetElem( PROJECT::ELEM_SCH_PART_LIBS ) )
    {
        Prj().SetProjectFullName( pro.GetFullPath() );

        // load the libraries here, not in SCH_SCREEN::Draw() which is a context
        // that will not tolerate DisplayError() dialog since we're already in an
        // event handler in there.
        // And when a schematic file is loaded, we need these libs to initialize
        // some parameters (links to PART LIB, dangling ends ...)
        Prj().SetElem( PROJECT::ELEM_SCH_PART_LIBS, NULL );
        Prj().SchLibs();
    }

    LoadProjectFile();

    // Load the symbol library table, this will be used forever more.
    Prj().SetElem( PROJECT::ELEM_SYMBOL_LIB_TABLE, NULL );
    Prj().SchSymbolLibTable();

    if( is_new )
    {
        // mark new, unsaved file as modified.
        GetScreen()->SetModify();
    }
    else
    {
        SetScreen( nullptr );
        delete g_RootSheet;   // Delete the current project.
        g_RootSheet = NULL;   // Force CreateScreens() to build new empty project on load failure.

        SCH_PLUGIN::SCH_PLUGIN_RELEASER pi( SCH_IO_MGR::FindPlugin( SCH_IO_MGR::SCH_LEGACY ) );

        // This will rename the file if there is an autosave and the user want to recover
		CheckForAutoSaveFile( fullFileName );

        try
        {
            g_RootSheet = pi->Load( fullFileName, &Kiway() );

            g_CurrentSheet = new SCH_SHEET_PATH();
            g_CurrentSheet->clear();
            g_CurrentSheet->push_back( g_RootSheet );

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
            // Do not leave g_RootSheet == NULL because it is expected to be
            // a valid sheet. Therefore create a dummy empty root sheet and screen.
            CreateScreens();
            m_toolManager->RunAction( ACTIONS::zoomFitScreen, true );

            wxString msg;
            msg.Printf( _( "Error loading schematic file \"%s\".\n%s" ),
                        GetChars( fullFileName ), GetChars( ioe.What() ) );
            DisplayError( this, msg );

            msg.Printf( _( "Failed to load \"%s\"" ), GetChars( fullFileName ) );
            AppendMsgPanel( wxEmptyString, msg, CYAN );

            return false;
        }

        // It's possible the schematic parser fixed errors due to bugs so warn the user
        // that the schematic has been fixed (modified).
        SCH_SHEET_LIST sheetList( g_RootSheet );

        if( sheetList.IsModified() )
        {
            DisplayInfoMessage( this,
                                _( "An error was found when loading the schematic that has "
                                   "been automatically fixed.  Please save the schematic to "
                                   "repair the broken file or it may not be usable with other "
                                   "versions of KiCad." ) );
        }

        UpdateFileHistory( fullFileName );

        SCH_SCREENS schematic;

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
                if( m_showIllegalSymbolLibDialog )
                {
                    wxRichMessageDialog invalidLibDlg(
                            this,
                            _( "Illegal entry found in project file symbol library list." ),
                            _( "Project Load Warning" ),
                            wxOK | wxCENTER | wxICON_EXCLAMATION );
                    invalidLibDlg.SetExtendedMessage(
                            _( "Symbol libraries defined in the project file symbol library list "
                               "are no longer supported and will be\nremoved.  This may cause "
                               "broken symbol library links under certain conditions." ) );
                    invalidLibDlg.ShowCheckBox( _( "Do not show this dialog again." ) );
                    invalidLibDlg.ShowModal();
                    m_showIllegalSymbolLibDialog = !invalidLibDlg.IsCheckBoxChecked();
                }

                libNames.Clear();
                paths.Clear();
                PART_LIBS::LibNamesAndPaths( &Prj(), true, &paths, &libNames );
            }

            // Check to see whether some old library parts need to be rescued
            // Only do this if RescueNeverShow was not set.
            wxConfigBase *config = Kiface().KifaceSettings();
            bool rescueNeverShow = false;
            config->Read( RescueNeverShowEntry, &rescueNeverShow, false );

            if( !rescueNeverShow )
                RescueSymbolLibTableProject( false );
        }

        schematic.UpdateSymbolLinks();      // Update all symbol library links for all sheets.
        SetScreen( g_CurrentSheet->LastScreen() );

        // Ensure the schematic is fully segmented on first display
        NormalizeSchematicOnFirstLoad( true );
        GetScreen()->ClearUndoORRedoList( GetScreen()->m_UndoList, 1 );

        GetScreen()->TestDanglingEnds();    // Only perform the dangling end test on root sheet.

        // Migrate conflicting bus definitions
        // TODO(JE) This should only run once based on schematic file version
        if( g_ConnectionGraph->GetBusesNeedingMigration().size() > 0 )
        {
            DIALOG_MIGRATE_BUSES dlg( this );
            dlg.ShowQuasiModal();

            RecalculateConnections();
            OnModify();
        }

        GetScreen()->m_Initialized = true;
    }

    GetScreen()->SetGrid( ID_POPUP_GRID_LEVEL_1000 + m_LastGridSizeId );
    m_toolManager->RunAction( ACTIONS::zoomFitScreen, true );
    SetSheetNumberAndCount();

    // re-create junctions if needed. Eeschema optimizes wires by merging
    // colinear segments. If a schematic is saved without a valid
    // cache library or missing installed libraries, this can cause connectivity errors
    // unless junctions are added.
    FixupJunctions();

    SyncView();
    GetScreen()->ClearDrawingState();

    UpdateTitle();

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
                      SchematicFileWildcard(), wxFD_OPEN | wxFD_FILE_MUST_EXIST );

    if( dlg.ShowModal() == wxID_CANCEL )
        return false;

    fullFileName = dlg.GetPath();

    if( !LoadSheetFromFile( GetCurrentSheet().Last(), &GetCurrentSheet(), fullFileName ) )
        return false;

    SCH_SCREENS screens( GetCurrentSheet().Last() );
    screens.TestDanglingEnds();

    GetScreen()->SetGrid( ID_POPUP_GRID_LEVEL_1000 + m_LastGridSizeId );
    m_toolManager->RunAction( ACTIONS::zoomFitScreen, true );
    SetSheetNumberAndCount();

    SyncView();
    HardRedraw();   // Full reinit of the current screen and the display.
    OnModify();

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

    // Set the project location if none is set
    bool setProject = Prj().GetProjectFullName().IsEmpty();
    wxString path = wxPathOnly( Prj().GetProjectFullName() );

    wxFileDialog dlg( this, _( "Import Schematic" ), path, wxEmptyString,
                      EagleSchematicFileWildcard(), wxFD_OPEN | wxFD_FILE_MUST_EXIST );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    if( setProject )
    {
        wxFileName projectFn( dlg.GetPath() );
        projectFn.SetExt( ProjectFileExtension );
        Prj().SetProjectFullName( projectFn.GetFullPath() );
    }

    // For now there is only one import plugin
    importFile( dlg.GetPath(), SCH_IO_MGR::SCH_EAGLE );
}


bool SCH_EDIT_FRAME::SaveProject()
{
    SCH_SCREEN* screen;
    SCH_SCREENS screenList;
    bool success = true;

    // I want to see it in the debugger, show me the string!  Can't do that with wxFileName.
    wxString    fileName = Prj().AbsolutePath( g_RootSheet->GetFileName() );
    wxFileName  fn = fileName;

    if( !fn.IsDirWritable() )
    {
        wxString msg = wxString::Format( _( "Directory \"%s\" is not writable." ), fn.GetPath() );
        DisplayError( this, msg );
        return false;
    }

    for( screen = screenList.GetFirst(); screen; screen = screenList.GetNext() )
        success &= SaveEEFile( screen );

    CreateArchiveLibraryCacheFile();

    UpdateTitle();

    return success;
}


bool SCH_EDIT_FRAME::doAutoSave()
{
    wxFileName  tmpFileName = g_RootSheet->GetFileName();
    wxFileName  fn = tmpFileName;
    wxFileName  tmp;
    SCH_SCREENS screens;

    bool autoSaveOk = true;

    tmp.AssignDir( fn.GetPath() );

    if( !tmp.IsOk() )
        return false;

    if( !IsWritable( tmp ) )
        return false;

    for( SCH_SCREEN* screen = screens.GetFirst(); screen; screen = screens.GetNext() )
    {
        // Only create auto save files for the schematics that have been modified.
        if( !screen->IsSave() )
            continue;

        tmpFileName = fn = screen->GetFileName();

        // Auto save file name is the normal file name prefixed with GetAutoSavePrefix().
        fn.SetName( GetAutoSaveFilePrefix() + fn.GetName() );

        screen->SetFileName( fn.GetFullPath() );

        if( SaveEEFile( screen, false, NO_BACKUP_FILE ) )
            screen->SetModify();
        else
            autoSaveOk = false;

        screen->SetFileName( tmpFileName.GetFullPath() );
    }

    if( autoSaveOk )
        m_autoSaveState = false;

    return autoSaveOk;
}


bool SCH_EDIT_FRAME::importFile( const wxString& aFileName, int aFileType )
{
    wxString projectpath;
    wxFileName newfilename;
    SCH_SHEET_LIST sheetList( g_RootSheet );

    switch( (SCH_IO_MGR::SCH_FILE_T) aFileType )
    {
    case SCH_IO_MGR::SCH_EAGLE:
        // We insist on caller sending us an absolute path, if it does not, we say it's a bug.
        wxASSERT_MSG( wxFileName( aFileName ).IsAbsolute(),
                      wxT( "Import eagle schematic caller didn't send full filename" ) );

        if( !LockFile( aFileName ) )
        {
            wxString msg = wxString::Format( _( "Schematic file \"%s\" is already open." ),
                                             aFileName );
            DisplayError( this, msg );
            return false;
        }

        try
        {
            delete g_RootSheet;
            g_RootSheet = nullptr;
            SCH_PLUGIN::SCH_PLUGIN_RELEASER pi( SCH_IO_MGR::FindPlugin( SCH_IO_MGR::SCH_EAGLE ) );
            g_RootSheet = pi->Load( aFileName, &Kiway() );

            // Eagle sheets do not use a worksheet frame by default, so set it to an empty one
            WS_DATA_MODEL& pglayout = WS_DATA_MODEL::GetTheInstance();
            pglayout.SetEmptyLayout();

            BASE_SCREEN::m_PageLayoutDescrFileName = "empty.kicad_wks";
            wxFileName layoutfn( Kiway().Prj().GetProjectPath(),
                                 BASE_SCREEN::m_PageLayoutDescrFileName );
            wxFile layoutfile;

            if( layoutfile.Create( layoutfn.GetFullPath() ) )
            {
                layoutfile.Write( WS_DATA_MODEL::EmptyLayout() );
                layoutfile.Close();
            }

            projectpath = Kiway().Prj().GetProjectPath();
            newfilename.SetPath( Prj().GetProjectPath() );
            newfilename.SetName( Prj().GetProjectName() );
            newfilename.SetExt( SchematicFileExtension );

            g_CurrentSheet->clear();
            g_CurrentSheet->push_back( g_RootSheet );
            SetScreen( g_CurrentSheet->LastScreen() );

            g_RootSheet->SetFileName( newfilename.GetFullPath() );
            GetScreen()->SetFileName( newfilename.GetFullPath() );
            GetScreen()->SetModify();
            SaveProjectSettings( false );

            UpdateFileHistory( aFileName );
            SCH_SCREENS schematic;
            schematic.UpdateSymbolLinks();      // Update all symbol library links for all sheets.

            // Ensure the schematic is fully segmented on first display
            NormalizeSchematicOnFirstLoad( false );

            GetScreen()->m_Initialized = true;

            EE_TYPE_COLLECTOR components;
            SCH_SCREENS allScreens;

            for( SCH_SCREEN* screen = allScreens.GetFirst(); screen; screen = allScreens.GetNext() )
            {
                components.Collect( screen->GetDrawItems(), EE_COLLECTOR::ComponentsOnly );

                for( int cmpIdx = 0; cmpIdx < components.GetCount(); ++cmpIdx )
                {
                    std::vector<wxPoint> pts;
                    SCH_COMPONENT* cmp = static_cast<SCH_COMPONENT*>( components[cmpIdx] );

                    // Update footprint LIB_ID to point to the imported Eagle library
                    auto fpField = cmp->GetField( FOOTPRINT );

                    if( !fpField->GetText().IsEmpty() )
                    {
                        LIB_ID fpId;
                        fpId.Parse( fpField->GetText(), LIB_ID::ID_SCH, true );
                        fpId.SetLibNickname( newfilename.GetName() );
                        fpField->SetText( fpId.Format() );
                    }
                }
            }

            GetScreen()->ClearUndoORRedoList( GetScreen()->m_UndoList, 1 );
            // Only perform the dangling end test on root sheet.
            GetScreen()->TestDanglingEnds();
            RecalculateConnections();

            GetScreen()->SetGrid( ID_POPUP_GRID_LEVEL_1000 + m_LastGridSizeId );
            m_toolManager->RunAction( ACTIONS::zoomFitScreen, true );
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
            AppendMsgPanel( wxEmptyString, msg, CYAN );

            return false;
        }

        return true;

    default:
        return false;
    }
}


bool SCH_EDIT_FRAME::AskToSaveChanges()
{
    SCH_SCREENS screenList;

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
