/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2013 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2013 CERN (www.cern.ch)
 * Copyright (C) 1992-2018 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file eeschema/files-io.cpp
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
#include <worksheet_shape_builder.h>


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

        // Rename the old file to a '.bak' one:
        backupFileName.SetExt( SchematicBackupFileExtension );

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
        autoSaveFileName.SetName( AUTOSAVE_PREFIX_FILENAME + schematicFileName.GetName() );

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


void SCH_EDIT_FRAME::Save_File( wxCommandEvent& event )
{
    int id = event.GetId();

    switch( id )
    {
    case ID_UPDATE_ONE_SHEET:
        SaveEEFile( NULL );
        break;

    case ID_SAVE_ONE_SHEET_UNDER_NEW_NAME:
        if( SaveEEFile( NULL, true ) )
        {
            CreateArchiveLibraryCacheFile( true );
        }
        break;
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
        g_RootSheet = NULL;

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

        try
        {
            g_RootSheet = pi->Load( fullFileName, &Kiway() );
            m_CurrentSheet->clear();
            m_CurrentSheet->push_back( g_RootSheet );

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
            Zoom_Automatique( false );

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
            // Check to see whether some old library parts need to be rescued
            // Only do this if RescueNeverShow was not set.
            wxConfigBase *config = Kiface().KifaceSettings();
            bool rescueNeverShow = false;
            config->Read( RescueNeverShowEntry, &rescueNeverShow, false );

            if( !rescueNeverShow )
                RescueSymbolLibTableProject( false );
        }

        schematic.UpdateSymbolLinks();      // Update all symbol library links for all sheets.
        SetScreen( m_CurrentSheet->LastScreen() );

        // Ensure the schematic is fully segmented on first display
        BreakSegmentsOnJunctions();
        SchematicCleanUp( true );
        GetScreen()->ClearUndoORRedoList( GetScreen()->m_UndoList, 1 );
        GetScreen()->TestDanglingEnds();    // Only perform the dangling end test on root sheet.
        GetScreen()->m_Initialized = true;
    }

    GetScreen()->SetGrid( ID_POPUP_GRID_LEVEL_1000 + m_LastGridSizeId );
    Zoom_Automatique( false );
    SetSheetNumberAndCount();
    SyncView();
    GetScreen()->ClearDrawingState();

    return true;
}


bool SCH_EDIT_FRAME::AppendSchematic()
{
    wxString    msg;
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

    wxFileName fn = fullFileName;

    if( fn.IsRelative() )
    {
        fn.MakeAbsolute();
        fullFileName = fn.GetFullPath();
    }

    wxString cache_name = PART_LIBS::CacheName( fullFileName );

    if( !!cache_name )
    {
        PART_LIBS*  libs = Prj().SchLibs();

        try
        {
            if( PART_LIB* lib = libs->AddLibrary( cache_name ) )
                lib->SetCache();
        }
        catch( const IO_ERROR& ioe )
        {
            DisplayError( this, ioe.What() );
        }
    }

    wxLogDebug( wxT( "Importing schematic " ) + fullFileName );

    // Load the schematic into a temporary sheet.
    SCH_PLUGIN::SCH_PLUGIN_RELEASER pi( SCH_IO_MGR::FindPlugin( SCH_IO_MGR::SCH_LEGACY ) );
    std::unique_ptr< SCH_SHEET> newSheet( new SCH_SHEET );

    newSheet->SetFileName( fullFileName );

    try
    {
        pi->Load( fullFileName, &Kiway(), newSheet.get() );

        if( !pi->GetError().IsEmpty() )
        {
            DisplayErrorMessage( this,
                                 _( "The entire schematic could not be loaded.  Errors "
                                    "occurred attempting to load hierarchical sheet "
                                    "schematics." ),
                                 pi->GetError() );
        }
    }
    catch( const IO_ERROR& ioe )
    {
        msg.Printf( _( "Error occurred loading schematic file \"%s\"." ), fullFileName );
        DisplayErrorMessage( this, msg, ioe.What() );

        msg.Printf( _( "Failed to load schematic \"%s\"" ), fullFileName );
        AppendMsgPanel( wxEmptyString, msg, CYAN );

        return false;
    }

    // Make sure any new sheet changes do not cause any recursion issues.
    SCH_SHEET_LIST hierarchy( g_RootSheet );          // This is the schematic sheet hierarchy.
    SCH_SHEET_LIST sheetHierarchy( newSheet.get() );  // This is the hierarchy of the import.

    wxFileName destFile = screen->GetFileName();

    if( destFile.IsRelative() )
        destFile.MakeAbsolute( Prj().GetProjectPath() );

    if( hierarchy.TestForRecursion( sheetHierarchy, destFile.GetFullPath( wxPATH_UNIX ) ) )
    {
        msg.Printf( _( "The sheet changes cannot be made because the destination sheet already "
                       "has the sheet \"%s\" or one of it's subsheets as a parent somewhere in "
                       "the schematic hierarchy." ),
                    destFile.GetFullPath() );
        DisplayError( this, msg );
        return false;
    }

    wxArrayString names;

    // Make sure the imported schematic has been remapped to the symbol library table.
    SCH_SCREENS newScreens( newSheet.get() );         // All screens associated with the import.

    if( newScreens.HasNoFullyDefinedLibIds() )
    {
        DisplayInfoMessage( this,
                            "This schematic has not been remapped to the symbol library\n"
                            "table.  The project this schematic belongs to must first be\n"
                            "remapped before it can be imported into the current project." );
        return false;
    }
    else
    {
        // If there are symbol libraries in the imported schematic that are not in the
        // symbol library table of this project, there could be a lot of broken symbol
        // library links.  Attempt to add the missing libraries to the project symbol
        // library table.
        newScreens.GetLibNicknames( names );
        wxArrayString newLibNames;

        for( const auto& name : names )
        {
            if( !Prj().SchSymbolLibTable()->HasLibrary( name ) )
                newLibNames.Add( name );
        }

        wxFileName symLibTableFn( fn.GetPath(), SYMBOL_LIB_TABLE::GetSymbolLibTableFileName() );

        if( !newLibNames.IsEmpty() && symLibTableFn.Exists() && symLibTableFn.IsFileReadable() )
        {
            SYMBOL_LIB_TABLE table;

            try
            {
                table.Load( symLibTableFn.GetFullPath() );
            }
            catch( const IO_ERROR& ioe )
            {
                msg.Printf( _( "An error occurred loading the symbol library table \"%s\"." ),
                            symLibTableFn.GetFullPath() );
                DisplayErrorMessage( NULL, msg, ioe.What() );
            }

            if( !table.IsEmpty() )
            {
                for( const auto& libName : newLibNames )
                {
                    if( !table.HasLibrary( libName ) )
                        continue;

                    // Don't expand environment variable because KIPRJMOD will not be correct
                    // for a different project.
                    wxString uri = table.GetFullURI( libName, false );
                    wxFileName newLib;

                    if( uri.Contains( "${KIPRJMOD}" ) )
                    {
                        newLib.SetPath( fn.GetPath() );
                        newLib.SetFullName( uri.AfterLast( '}' ) );
                        uri = newLib.GetFullPath();
                    }
                    else if( uri.Contains( "$(KIPRJMOD)" ) )
                    {
                        newLib.SetPath( fn.GetPath() );
                        newLib.SetFullName( uri.AfterLast( ')' ) );
                        uri = newLib.GetFullPath();
                    }
                    else
                    {
                        uri = table.GetFullURI( libName );
                    }

                    // Add the library from the imported project to the current project
                    // symbol library table.
                    const SYMBOL_LIB_TABLE_ROW* row = table.FindRow( libName );

                    wxCHECK2_MSG( row, continue, "Library '" + libName +
                                  "' missing from symbol library table '" +
                                  symLibTableFn.GetFullPath() + "'." );

                    wxString newLibName = libName;
                    int libNameCnt = 1;

                    // Rename the imported symbol library if it already exists.
                    while( Prj().SchSymbolLibTable()->HasLibrary( newLibName ) )
                        newLibName = wxString::Format( "%s%d", libName, libNameCnt );

                    auto newRow = new SYMBOL_LIB_TABLE_ROW( newLibName, uri, row->GetType(),
                                                            row->GetOptions(), row->GetDescr() );
                    Prj().SchSymbolLibTable()->InsertRow( newRow );

                    if( libName != newLibName )
                        newScreens.ChangeSymbolLibNickname( libName, newLibName );
                }
            }
        }
    }

    newScreens.ClearAnnotation();

    // Check for duplicate sheet names in the current page.
    wxArrayString duplicateSheetNames;
    SCH_TYPE_COLLECTOR sheets;

    sheets.Collect( screen->GetDrawItems(), SCH_COLLECTOR::SheetsOnly );

    for( int i = 0;  i < sheets.GetCount();  ++i )
    {
        if( newSheet->GetScreen()->GetSheet( ( ( SCH_SHEET* ) sheets[i] )->GetName() ) )
            duplicateSheetNames.Add( ( ( SCH_SHEET* ) sheets[i] )->GetName() );
    }

    if( !duplicateSheetNames.IsEmpty() )
    {
        msg.Printf( "Duplicate sheet names exist on the current page.  Do you want to "
                    "automatically rename the duplicate sheet names?" );
        if( !IsOK( this, msg ) )
            return false;
    }

    SCH_SCREEN* newScreen = newSheet->GetScreen();
    wxCHECK_MSG( newScreen, false, "No screen defined for imported sheet." );

    for( const auto& duplicateName : duplicateSheetNames )
    {
        SCH_SHEET* renamedSheet = newScreen->GetSheet( duplicateName );

        wxCHECK2_MSG( renamedSheet, continue,
                      "Sheet " + duplicateName + " not found in imported schematic." );

        timestamp_t newtimestamp = GetNewTimeStamp();
        renamedSheet->SetTimeStamp( newtimestamp );
        renamedSheet->SetName( wxString::Format( "Sheet%8.8lX", (unsigned long) newtimestamp ) );
    }

    // It is finally safe to add the imported schematic.
    screen->Append( newScreen );

    SCH_SCREENS allScreens;
    allScreens.ReplaceDuplicateTimeStamps();

    SCH_SCREENS screens( GetCurrentSheet().Last() );
    screens.UpdateSymbolLinks( true );

    // Clear all annotation in the imported schematic to prevent clashes with existing annotation.
    // Must be done after updating the symbol links as we need to know about multi-unit parts.
    // screens.ClearAnnotation();

    screens.TestDanglingEnds();

    GetScreen()->SetGrid( ID_POPUP_GRID_LEVEL_1000 + m_LastGridSizeId );
    Zoom_Automatique( false );
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


void SCH_EDIT_FRAME::OnSaveProject( wxCommandEvent& aEvent )
{
    SaveProject();
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

        // Auto save file name is the normal file name prefixed with AUTOSAVE_PREFIX_FILENAME.
        fn.SetName( AUTOSAVE_PREFIX_FILENAME + fn.GetName() );

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
            WORKSHEET_LAYOUT& pglayout = WORKSHEET_LAYOUT::GetTheInstance();
            pglayout.SetEmptyLayout();

            BASE_SCREEN::m_PageLayoutDescrFileName = "empty.kicad_wks";
            wxFileName layoutfn( Kiway().Prj().GetProjectPath(),
                                 BASE_SCREEN::m_PageLayoutDescrFileName );
            wxFile layoutfile;

            if( layoutfile.Create( layoutfn.GetFullPath() ) )
            {
                layoutfile.Write( WORKSHEET_LAYOUT::EmptyLayout() );
                layoutfile.Close();
            }

            projectpath = Kiway().Prj().GetProjectPath();
            newfilename.SetPath( Prj().GetProjectPath() );
            newfilename.SetName( Prj().GetProjectName() );
            newfilename.SetExt( SchematicFileExtension );

            m_CurrentSheet->clear();
            m_CurrentSheet->push_back( g_RootSheet );
            SetScreen( m_CurrentSheet->LastScreen() );

            g_RootSheet->SetFileName( newfilename.GetFullPath() );
            GetScreen()->SetFileName( newfilename.GetFullPath() );
            GetScreen()->SetModify();
            SaveProjectSettings( false );

            UpdateFileHistory( aFileName );
            SCH_SCREENS schematic;
            schematic.UpdateSymbolLinks();      // Update all symbol library links for all sheets.

            // Ensure the schematic is fully segmented on first display
            BreakSegmentsOnJunctions();
            SchematicCleanUp( true );
            GetScreen()->m_Initialized = true;

            SCH_TYPE_COLLECTOR components;
            SCH_SCREENS allScreens;

            for( SCH_SCREEN* screen = allScreens.GetFirst(); screen; screen = allScreens.GetNext() )
            {
                components.Collect( screen->GetDrawItems(), SCH_COLLECTOR::ComponentsOnly );

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

                    // Add junction dots where necessary
                    cmp->GetConnectionPoints( pts );

                    for( auto i = pts.begin(); i != pts.end(); ++i )
                    {
                        if( GetScreen()->IsJunctionNeeded( *i, true ) )
                            AddJunction( *i, true );
                    }
                }
            }

            GetScreen()->ClearUndoORRedoList( GetScreen()->m_UndoList, 1 );
            // Only perform the dangling end test on root sheet.
            GetScreen()->TestDanglingEnds();

            GetScreen()->SetGrid( ID_POPUP_GRID_LEVEL_1000 + m_LastGridSizeId );
            Zoom_Automatique( false );
            SetSheetNumberAndCount();
            SyncView();
            UpdateTitle();
        }
        catch( const IO_ERROR& ioe )
        {
            // Do not leave g_RootSheet == NULL because it is expected to be
            // a valid sheet. Therefore create a dummy empty root sheet and screen.
            CreateScreens();
            Zoom_Automatique( false );

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
            if( !HandleUnsavedChanges( this, _( "The current schematic has been modified.  Save changes?" ),
                                       [&]()->bool { return SaveProject(); } ) )
            {
                return false;
            }
        }
    }

    return true;
}
