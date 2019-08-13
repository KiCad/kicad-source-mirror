/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2004-2019 KiCad Developers, see change_log.txt for contributors.
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
#include <kiface_i.h>
#include <project.h>
#include <wildcards_and_files_ext.h>
#include <tool/tool_manager.h>
#include <wx/clipbrd.h>
#include <sch_edit_frame.h>
#include <sch_legacy_plugin.h>
#include <sch_sheet.h>
#include <sch_sheet_path.h>
#include <sch_view.h>
#include <symbol_lib_table.h>
#include <dialogs/dialog_sch_sheet_props.h>
#include <dialogs/dialog_sch_edit_sheet_pin.h>
#include <tool/actions.h>


bool SCH_EDIT_FRAME::checkSheetForRecursion( SCH_SHEET* aSheet, SCH_SHEET_PATH* aHierarchy )
{
    wxASSERT( aSheet && aHierarchy );

    wxString msg;
    SCH_SHEET_LIST hierarchy( g_RootSheet );  // This is the full schematic sheet hierarchy.
    SCH_SHEET_LIST sheetHierarchy( aSheet );  // This is the hierarchy of the loaded file.

    wxFileName destFile = aHierarchy->LastScreen()->GetFileName();

    // SCH_SCREEN object file paths are expected to be absolute.  If this assert fires,
    // something is seriously broken.
    wxASSERT( destFile.IsAbsolute() );

    if( hierarchy.TestForRecursion( sheetHierarchy, destFile.GetFullPath() ) )
    {
        msg.Printf( _( "The sheet changes cannot be made because the destination sheet already "
                       "has the sheet \"%s\" or one of it's subsheets as a parent somewhere in "
                       "the schematic hierarchy." ),
                    destFile.GetFullPath() );
        DisplayError( this, msg );
        return true;
    }

    return false;
}


bool SCH_EDIT_FRAME::checkForNoFullyDefinedLibIds( SCH_SHEET* aSheet )
{
    wxASSERT( aSheet && aSheet->GetScreen() );

    wxString msg;
    SCH_SCREENS newScreens( aSheet );

    if( newScreens.HasNoFullyDefinedLibIds() )
    {
        msg.Printf( _( "The schematic \"%s\" has not had it's symbol library links remapped "
                       "to the symbol library table.  The project this schematic belongs to "
                       "must first be remapped before it can be imported into the current "
                       "project." ), aSheet->GetScreen()->GetFileName() );
        DisplayInfoMessage( this, msg );
        return true;
    }

    return false;
}


void SCH_EDIT_FRAME::InitSheet( SCH_SHEET* aSheet, const wxString& aNewFilename )
{
    aSheet->SetScreen( new SCH_SCREEN( &Kiway() ) );
    aSheet->GetScreen()->SetModify();
    aSheet->GetScreen()->SetMaxUndoItems( m_UndoRedoCountMax );
    aSheet->GetScreen()->SetFileName( aNewFilename );
}


bool SCH_EDIT_FRAME::LoadSheetFromFile( SCH_SHEET* aSheet, SCH_SHEET_PATH* aHierarchy,
                                        const wxString& aFileName )
{
    wxASSERT( aSheet && aHierarchy );

    int         i;
    wxString    msg;
    wxString    topLevelSheetPath;
    wxFileName  tmp;
    wxFileName  currentSheetFileName;
    bool        libTableChanged = false;
    SCH_SCREEN* currentScreen = aHierarchy->LastScreen();
    SCH_PLUGIN::SCH_PLUGIN_RELEASER pi( SCH_IO_MGR::FindPlugin( SCH_IO_MGR::SCH_LEGACY ) );
    std::unique_ptr< SCH_SHEET> newSheet( new SCH_SHEET );

    wxFileName fileName( aFileName );

    if( !fileName.IsAbsolute() )
    {
        wxCHECK_MSG( fileName.MakeAbsolute(), false,
                     wxString::Format( "Cannot make file name \"%s\" path absolute.",
                                       aFileName ) );
    }

    wxString fullFilename = fileName.GetFullPath();

    try
    {
        if( aSheet->GetScreen() != nullptr )
        {
            newSheet.reset( pi->Load( fullFilename, &Kiway() ) );
        }
        else
        {
            newSheet->SetFileName( fullFilename );
            pi->Load( fullFilename, &Kiway(), newSheet.get() );
        }

        if( !pi->GetError().IsEmpty() )
        {
            msg = _( "The entire schematic could not be loaded.  Errors occurred attempting "
                     "to load hierarchical sheet schematics." );

            wxMessageDialog msgDlg1( this, msg, _( "Schematic Load Error" ),
                                     wxOK | wxCANCEL | wxCANCEL_DEFAULT |
                                     wxCENTER | wxICON_QUESTION );
            msgDlg1.SetOKLabel( wxMessageDialog::ButtonLabel( _( "Use partial schematic" ) ) );
            msgDlg1.SetExtendedMessage( pi->GetError() );

            if( msgDlg1.ShowModal() == wxID_CANCEL )
                return false;
        }
    }
    catch( const IO_ERROR& ioe )
    {
        msg.Printf( _( "Error occurred loading schematic file \"%s\"." ), fullFilename );
        DisplayErrorMessage( this, msg, ioe.What() );

        msg.Printf( _( "Failed to load schematic \"%s\"" ), fullFilename );
        AppendMsgPanel( wxEmptyString, msg, CYAN );

        return false;
    }

    tmp = fileName;

    // If the loaded schematic is in a different folder from the current project and
    // it contains hierarchical sheets, the hierarchical sheet paths need to be updated.
    if( fileName.GetPath( wxPATH_GET_SEPARATOR ) != Prj().GetProjectPath()
      && newSheet->CountSheets() )
    {
        // Give the user the option to choose relative path if possible.
        if( tmp.MakeRelativeTo( Prj().GetProjectPath() ) )
        {
            wxMessageDialog msgDlg2(
                    this,
                    "Do you want to use a relative path to the loaded "
                    "schematic?", "Select Path Type",
                    wxYES_NO | wxCANCEL | wxYES_DEFAULT | wxICON_QUESTION | wxCENTER );
            msgDlg2.SetYesNoLabels( wxMessageDialog::ButtonLabel( "Use Relative Path" ),
                                    wxMessageDialog::ButtonLabel( "Use Absolute Path" ) );
            int rsp = msgDlg2.ShowModal();

            if( rsp == wxID_CANCEL )
            {
                return false;
            }
            else if( rsp == wxID_NO )
            {
                topLevelSheetPath = fileName.GetPathWithSep();
            }
            else
            {
                topLevelSheetPath = tmp.GetPathWithSep();
            }
        }
        else
        {
            topLevelSheetPath = tmp.GetPathWithSep();
        }

        if( wxFileName::GetPathSeparator() == '\\' )
            topLevelSheetPath.Replace( "\\", "/" );
    }

    // Make sure any new sheet changes do not cause any recursion issues.
    SCH_SHEET_LIST hierarchy( g_RootSheet );          // This is the schematic sheet hierarchy.
    SCH_SHEET_LIST sheetHierarchy( newSheet.get() );  // This is the hierarchy of the loaded file.

    if( checkSheetForRecursion( newSheet.get(), aHierarchy )
      || checkForNoFullyDefinedLibIds( newSheet.get() ) )
        return false;

    // Make a valiant attempt to warn the user of all possible scenarios where there could
    // be broken symbol library links.
    wxArrayString    names;
    wxArrayString    newLibNames;
    SCH_SCREENS      newScreens( newSheet.get() );   // All screens associated with the import.
    SCH_SCREENS      prjScreens( g_RootSheet );

    newScreens.GetLibNicknames( names );

    wxMessageDialog::ButtonLabel okButtonLabel( _( "Continue Load" ) );
    wxMessageDialog::ButtonLabel cancelButtonLabel( _( "Cancel Load" ) );

    if( fileName.GetPath( wxPATH_GET_SEPARATOR ) == Prj().GetProjectPath()
      && !prjScreens.HasSchematic( fullFilename ) )
    {
        // A schematic in the current project path that isn't part of the current project.
        // It's possible the user copied this schematic from another project so the library
        // links may not be avaible.  Even this is check is no guarantee that all symbol
        // library links are valid but it's better than nothing.
        for( const auto& name : names )
        {
            if( !Prj().SchSymbolLibTable()->HasLibrary( name ) )
                newLibNames.Add( name );
        }

        if( !newLibNames.IsEmpty() )
        {
            msg = _( "There are library names in the loaded schematic that are missing "
                     "from the project library table.  This may result in broken symbol "
                     "library links for the loaded schematic.  Do you wish to continue?" );
            wxMessageDialog msgDlg3( this, msg, _( "Continue Load Schematic" ),
                                     wxOK | wxCANCEL | wxCANCEL_DEFAULT |
                                     wxCENTER | wxICON_QUESTION );
            msgDlg3.SetOKCancelLabels( okButtonLabel, cancelButtonLabel );

            if( msgDlg3.ShowModal() == wxID_CANCEL )
                return false;
        }
    }
    else if( fileName.GetPath( wxPATH_GET_SEPARATOR ) != Prj().GetProjectPath() )
    {
        // A schematic loaded from a path other than the current project path.

        // If there are symbol libraries in the imported schematic that are not in the
        // symbol library table of this project, there could be a lot of broken symbol
        // library links.  Attempt to add the missing libraries to the project symbol
        // library table.
        wxArrayString    duplicateLibNames;

        for( const auto& name : names )
        {
            if( !Prj().SchSymbolLibTable()->HasLibrary( name ) )
                newLibNames.Add( name );
            else
                duplicateLibNames.Add( name );
        }

        SYMBOL_LIB_TABLE table;
        wxFileName symLibTableFn( fileName.GetPath(),
                                  SYMBOL_LIB_TABLE::GetSymbolLibTableFileName() );

        // If there are any new or duplicate libraries, check to see if it's possible that
        // there could be any missing libraries that would cause broken symbol library links.
        if( !newLibNames.IsEmpty() || !duplicateLibNames.IsEmpty() )
        {
            if( !symLibTableFn.Exists() || !symLibTableFn.IsFileReadable() )
            {
                msg.Printf( _( "The project library table \"%s\" does not exist or cannot "
                               "be read.  This may result in broken symbol links for the "
                               "schematic.  Do you wish to continue?" ),
                            fileName.GetFullPath() );
                wxMessageDialog msgDlg4( this, msg, _( "Continue Load Schematic" ),
                                         wxOK | wxCANCEL | wxCANCEL_DEFAULT |
                                         wxCENTER | wxICON_QUESTION );
                msgDlg4.SetOKCancelLabels( okButtonLabel, cancelButtonLabel );

                if( msgDlg4.ShowModal() == wxID_CANCEL )
                    return false;
            }
            else
            {
                try
                {
                    table.Load( symLibTableFn.GetFullPath() );
                }
                catch( const IO_ERROR& ioe )
                {
                    msg.Printf( _( "An error occurred loading the symbol library table "
                                   "\"%s\"." ),
                                symLibTableFn.GetFullPath() );
                    DisplayErrorMessage( NULL, msg, ioe.What() );
                    return false;
                }
            }
        }

        // Check to see if any of the symbol libraries found in the appended schematic do
        // not exist in the current project are missing from the appended project symbol
        // library table.
        if( !newLibNames.IsEmpty() )
        {
            bool missingLibNames = table.IsEmpty();

            if( !missingLibNames )
            {
                for( const auto& newLibName : newLibNames )
                {
                    if( !table.HasLibrary( newLibName ) )
                    {
                        missingLibNames = true;
                        break;
                    }
                }
            }

            if( missingLibNames )
            {
                msg = _( "There are library names in the loaded schematic that are missing "
                         "from the loaded schematic project library table.  This may result "
                         "in broken symbol library links for the schematic.  "
                         "Do you wish to continue?" );
                wxMessageDialog msgDlg5( this, msg, _( "Continue Load Schematic" ),
                                         wxOK | wxCANCEL | wxCANCEL_DEFAULT |
                                         wxCENTER | wxICON_QUESTION );
                msgDlg5.SetOKCancelLabels( okButtonLabel, cancelButtonLabel );

                if( msgDlg5.ShowModal() == wxID_CANCEL )
                    return false;
            }
        }

        // The library name already exists in the current project.  Check to see if the
        // duplicate name is the same library in the current project.  If it's not, it's
        // most likely that the symbol library links will be broken.
        if( !duplicateLibNames.IsEmpty() && !table.IsEmpty() )
        {
            bool libNameConflict = false;

            for( const auto& duplicateLibName : duplicateLibNames )
            {
                const SYMBOL_LIB_TABLE_ROW* thisRow = nullptr;
                const SYMBOL_LIB_TABLE_ROW* otherRow = nullptr;

                if( Prj().SchSymbolLibTable()->HasLibrary( duplicateLibName ) )
                    thisRow = Prj().SchSymbolLibTable()->FindRow( duplicateLibName );

                if( table.HasLibrary( duplicateLibName ) )
                    otherRow = table.FindRow( duplicateLibName );

                // It's in the global library table so there is no conflict.
                if( thisRow && !otherRow )
                    continue;

                if( !thisRow || !otherRow )
                    continue;

                wxFileName otherUriFileName;
                wxString thisURI = thisRow->GetFullURI( true );
                wxString otherURI = otherRow->GetFullURI( false);

                if( otherURI.Contains( "${KIPRJMOD}" ) || otherURI.Contains( "$(KIPRJMOD)" ) )
                {
                    // Cannot use relative paths here, "${KIPRJMOD}../path-to-cache-lib" does
                    // not expand to a valid symbol library path.
                    otherUriFileName.SetPath( fileName.GetPath() );
                    otherUriFileName.SetFullName( otherURI.AfterLast( '}' ) );
                    otherURI = otherUriFileName.GetFullPath();
                }

                if( thisURI != otherURI )
                {
                    libNameConflict = true;
                    break;
                }
            }

            if( libNameConflict )
            {
                msg = _( "A duplicate library name that references a different library exists "
                         "in the current library table.  This conflict cannot be resolved and "
                         "may result in broken symbol library links for the schematic.  "
                         "Do you wish to continue?" );
                wxMessageDialog msgDlg6( this, msg, _( "Continue Load Schematic" ),
                                         wxOK | wxCANCEL | wxCANCEL_DEFAULT |
                                         wxCENTER | wxICON_QUESTION );
                msgDlg6.SetOKCancelLabels( okButtonLabel, cancelButtonLabel );

                if( msgDlg6.ShowModal() == wxID_CANCEL )
                    return false;
            }
        }

        // All (most?) of the possible broken symbol library link cases are covered.  Map the
        // new appended schematic project symbol library table entries to the current project
        // symbol library table.
        if( !newLibNames.IsEmpty() && !table.IsEmpty() )
        {
            for( const auto& libName : newLibNames )
            {
                if( !table.HasLibrary( libName )
                  || Prj().SchSymbolLibTable()->HasLibrary( libName ) )
                    continue;

                // Don't expand environment variable because KIPRJMOD will not be correct
                // for a different project.
                wxString uri = table.GetFullURI( libName, false );
                wxFileName newLib;

                if( uri.Contains( "${KIPRJMOD}" ) || uri.Contains( "$(KIPRJMOD)" ) )
                {
                    // Cannot use relative paths here, "${KIPRJMOD}../path-to-cache-lib" does
                    // not expand to a valid symbol library path.
                    newLib.SetPath( fileName.GetPath() );
                    newLib.SetFullName( uri.AfterLast( '}' ) );
                    uri = newLib.GetFullPath();
                }
                else
                {
                    uri = table.GetFullURI( libName );
                }

                // Add the library from the imported project to the current project
                // symbol library table.
                const SYMBOL_LIB_TABLE_ROW* row = table.FindRow( libName );

                auto newRow = new SYMBOL_LIB_TABLE_ROW( libName, uri, row->GetType(),
                                                        row->GetOptions(), row->GetDescr() );

                Prj().SchSymbolLibTable()->InsertRow( newRow );
                libTableChanged = true;
            }
        }
    }

    // Check for duplicate sheet names in the current page.
    wxArrayString duplicateSheetNames;
    EE_TYPE_COLLECTOR sheets;

    sheets.Collect( currentScreen->GetDrawItems(), EE_COLLECTOR::SheetsOnly );

    for( i = 0;  i < sheets.GetCount();  ++i )
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

    // Rename all duplicate sheet names.
    SCH_SCREEN* newScreen = newSheet->GetScreen();
    wxCHECK_MSG( newScreen, false, "No screen defined for sheet." );

    for( const auto& duplicateName : duplicateSheetNames )
    {
        SCH_SHEET* renamedSheet = newScreen->GetSheet( duplicateName );

        wxCHECK2_MSG( renamedSheet, continue,
                      "Sheet " + duplicateName + " not found in imported schematic." );

        timestamp_t newtimestamp = GetNewTimeStamp();
        renamedSheet->SetTimeStamp( newtimestamp );
        renamedSheet->SetName( wxString::Format( "Sheet%8.8lX", (unsigned long) newtimestamp ) );
    }

    // Set all sheets loaded into the correct sheet file paths.
    EE_TYPE_COLLECTOR newTopLevelSheets;

    newTopLevelSheets.Collect( newSheet->GetScreen()->GetDrawItems(), EE_COLLECTOR::SheetsOnly );

    for( i = 0;  i < newTopLevelSheets.GetCount();  ++i )
    {
        SCH_SHEET* tmpSheet = dynamic_cast< SCH_SHEET* >( newTopLevelSheets[i] );
        wxCHECK2( tmpSheet != nullptr, continue );
        tmpSheet->SetFileName( topLevelSheetPath + tmpSheet->GetFileName() );
    }

    if( libTableChanged )
        Prj().SchSymbolLibTable()->Save( Prj().GetProjectPath() +
                                         SYMBOL_LIB_TABLE::GetSymbolLibTableFileName() );

    // It is finally safe to add or append the imported schematic.
    if( aSheet->GetScreen() == nullptr )
        aSheet->SetScreen( newScreen );
    else
        aSheet->GetScreen()->Append( newScreen );

    SCH_SCREENS allScreens;
    allScreens.ReplaceDuplicateTimeStamps();

    SCH_SCREENS screens( aSheet );
    screens.UpdateSymbolLinks( true );

    return true;
}


bool SCH_EDIT_FRAME::EditSheet( SCH_SHEET* aSheet, SCH_SHEET_PATH* aHierarchy,
                                bool* aClearAnnotationNewItems )
{
    if( aSheet == NULL || aHierarchy == NULL )
        return false;

    // Get the new texts
    DIALOG_SCH_SHEET_PROPS dlg( this, aSheet );

    if( dlg.ShowModal() == wxID_CANCEL )
        return false;

    wxFileName fileName = dlg.GetFileName();
    fileName.SetExt( SchematicFileExtension );

    wxString msg;
    bool renameFile = false;
    bool loadFromFile = false;
    bool clearAnnotation = false;
    bool restoreSheet = false;
    bool isExistingSheet = false;
    SCH_SCREEN* useScreen = NULL;

    // Relative file names are relative to the path of the current sheet.  This allows for
    // nesting of schematic files in subfolders.
    if( !fileName.IsAbsolute() )
    {
        const SCH_SCREEN* currentScreen = aHierarchy->LastScreen();

        wxCHECK_MSG( currentScreen, false, "Invalid sheet path object." );

        wxFileName currentSheetFileName = currentScreen->GetFileName();

        wxCHECK_MSG( fileName.Normalize( wxPATH_NORM_ALL, currentSheetFileName.GetPath() ), false,
                     "Cannot normalize new sheet schematic file path." );
    }

    wxString newFilename = fileName.GetFullPath();

    // Search for a schematic file having the same filename already in use in the hierarchy
    // or on disk, in order to reuse it.
    if( !g_RootSheet->SearchHierarchy( newFilename, &useScreen ) )
    {
        loadFromFile = wxFileExists( newFilename );
        wxLogDebug( "Sheet requested file \"%s\", %s",
                    newFilename,
                    ( loadFromFile ) ? "found" : "not found" );
    }

    // Inside Eeschema, filenames are stored using unix notation
    newFilename.Replace( wxT( "\\" ), wxT( "/" ) );

    if( aSheet->GetScreen() == NULL )                 // New sheet.
    {
        if( useScreen || loadFromFile )               // Load from existing file.
        {
            clearAnnotation = true;

            wxString existsMsg;
            wxString linkMsg;
            existsMsg.Printf( _( "\"%s\" already exists." ), fileName.GetFullName() );
            linkMsg.Printf( _( "Link \"%s\" to this file?" ), dlg.GetSheetName() );
            msg.Printf( wxT( "%s\n\n%s" ), existsMsg, linkMsg );

            if( !IsOK( this, msg ) )
                return false;

        }
        else                                          // New file.
        {
            InitSheet( aSheet, newFilename );
        }
    }
    else                                              // Existing sheet.
    {
        bool isUndoable = true;
        wxString replaceMsg;
        wxString newMsg;
        wxString noUndoMsg;

        isExistingSheet = true;

        // Changing the filename of a sheet can modify the full hierarchy structure
        // and can be not always undoable.
        // So prepare messages for user notifications:
        replaceMsg.Printf( _( "Change \"%s\" link from \"%s\" to \"%s\"?" ),
                dlg.GetSheetName(), aSheet->GetFileName(), fileName.GetFullName() );
        newMsg.Printf( _( "Create new file \"%s\" with contents of \"%s\"?" ),
                fileName.GetFullName(), aSheet->GetFileName() );
        noUndoMsg = _( "This action cannot be undone." );

        // We are always using here a case insensitive comparison
        // to avoid issues under Windows, although under Unix
        // filenames are case sensitive.
        // But many users create schematic under both Unix and Windows
        // **
        // N.B. 1: aSheet->GetFileName() will return a relative path
        //         aSheet->GetScreen()->GetFileName() returns a full path
        //
        // N.B. 2: newFilename uses the unix notation for separator.
        //         so we must use it also to compare the old filename to the new filename
        wxString oldFilename = aSheet->GetScreen()->GetFileName();
        oldFilename.Replace( wxT( "\\" ), wxT( "/" ) );

        if( newFilename.CmpNoCase( oldFilename ) != 0 )
        {
            // Sheet file name changes cannot be undone.
            isUndoable = false;

            if( useScreen || loadFromFile )           // Load from existing file.
            {
                clearAnnotation = true;

                msg.Printf( wxT( "%s\n\n%s" ), replaceMsg, noUndoMsg );

                if( !IsOK( this, msg ) )
                    return false;

                if( loadFromFile )
                    aSheet->SetScreen( NULL );
            }
            else                                      // Save to new file name.
            {
                if( aSheet->GetScreenCount() > 1 )
                {
                    msg.Printf( wxT( "%s\n\n%s" ), newMsg, noUndoMsg );

                    if( !IsOK( this, msg ) )
                        return false;
                }

                renameFile = true;
            }
        }

        if( isUndoable )
            SaveCopyInUndoList( aSheet, UR_CHANGED );

        if( renameFile )
        {
            SCH_PLUGIN::SCH_PLUGIN_RELEASER pi( SCH_IO_MGR::FindPlugin( SCH_IO_MGR::SCH_LEGACY ) );

            // If the the associated screen is shared by more than one sheet, do not
            // change the filename of the corresponding screen here.
            // (a new screen will be created later)
            // if it is not shared, update the filename
            if( aSheet->GetScreenCount() <= 1 )
                aSheet->GetScreen()->SetFileName( newFilename );

            try
            {
                pi->Save( newFilename, aSheet->GetScreen(), &Kiway() );
            }
            catch( const IO_ERROR& ioe )
            {
                msg.Printf( _( "Error occurred saving schematic file \"%s\"." ), newFilename );
                DisplayErrorMessage( this, msg, ioe.What() );

                msg.Printf( _( "Failed to save schematic \"%s\"" ), newFilename );
                AppendMsgPanel( wxEmptyString, msg, CYAN );

                return false;
            }

            // If the the associated screen is shared by more than one sheet, remove the
            // screen and reload the file to a new screen.  Failure to do this will trash
            // the screen reference counting in complex hierarchies.
            if( aSheet->GetScreenCount() > 1 )
            {
                aSheet->SetScreen( NULL );
                loadFromFile = true;
            }
        }
    }

    wxFileName userFileName = dlg.GetFileName();
    userFileName.SetExt( SchematicFileExtension );

    if( useScreen )
    {
        // Create a temporary sheet for recursion testing to prevent a possible recursion error.
        std::unique_ptr< SCH_SHEET> tmpSheet( new SCH_SHEET );
        tmpSheet->SetName( dlg.GetSheetName() );
        tmpSheet->SetFileName( userFileName.GetFullPath() );
        tmpSheet->SetScreen( useScreen );

        // No need to check for valid library IDs if we are using an existing screen.
        if( checkSheetForRecursion( tmpSheet.get(), aHierarchy ) )
        {
            if( restoreSheet )
                aHierarchy->LastScreen()->Append( aSheet );

            return false;
        }

        // It's safe to set the sheet screen now.
        aSheet->SetScreen( useScreen );
    }
    else if( loadFromFile )
    {
        if( isExistingSheet )
        {
            // Temporarily remove the sheet from the current schematic page so that recursion
            // and symbol library link tests can be performed with the modified sheet settings.
            restoreSheet = true;
            aHierarchy->LastScreen()->Remove( aSheet );
        }

        if( !LoadSheetFromFile( aSheet, aHierarchy, newFilename ) )
        {
            if( restoreSheet )
                aHierarchy->LastScreen()->Append( aSheet );

            return false;
        }

        if( restoreSheet )
            aHierarchy->LastScreen()->Append( aSheet );
    }

    wxString tmpFn = userFileName.GetFullPath();

    if( wxFileName::GetPathSeparator() == '\\' )
        tmpFn.Replace( "\\", "/" );

    aSheet->SetFileName( tmpFn );
    aSheet->SetFileNameSize( dlg.GetFileNameTextSize() );
    aSheet->SetName( dlg.GetSheetName() );
    aSheet->SetSheetNameSize( dlg.GetSheetNameTextSize() );

    if( aSheet->GetName().IsEmpty() )
        aSheet->SetName( wxString::Format( wxT( "Sheet%8.8lX" ),
                                           (long unsigned) aSheet->GetTimeStamp() ) );

    if( aClearAnnotationNewItems )
        *aClearAnnotationNewItems = clearAnnotation;

    GetCanvas()->GetView()->Update( aSheet );

    OnModify();

    return true;
}


PINSHEETLABEL_SHAPE SCH_EDIT_FRAME::m_lastSheetPinType = NET_INPUT;
wxSize SCH_EDIT_FRAME::m_lastSheetPinTextSize( -1, -1 );
wxPoint SCH_EDIT_FRAME::m_lastSheetPinPosition;

const wxSize &SCH_EDIT_FRAME::GetLastSheetPinTextSize()
{
    // Delayed initialization (need the preferences to be loaded)
    if( m_lastSheetPinTextSize.x == -1 )
    {
        m_lastSheetPinTextSize.x = GetDefaultTextSize();
        m_lastSheetPinTextSize.y = GetDefaultTextSize();
    }
    return m_lastSheetPinTextSize;
}


SCH_SHEET_PIN* SCH_EDIT_FRAME::CreateSheetPin( SCH_SHEET* aSheet, SCH_HIERLABEL* aLabel )
{
    wxString       text;
    SCH_SHEET_PIN* sheetPin;

    if( aLabel )
    {
        text = aLabel->GetText();
        m_lastSheetPinType = aLabel->GetShape();
    }

    sheetPin = new SCH_SHEET_PIN( aSheet, wxPoint( 0, 0 ), text );
    sheetPin->SetFlags( IS_NEW );
    sheetPin->SetTextSize( GetLastSheetPinTextSize() );
    sheetPin->SetShape( m_lastSheetPinType );

    if( !aLabel )
    {
        DIALOG_SCH_EDIT_SHEET_PIN dlg( this, sheetPin );

        if( dlg.ShowModal() != wxID_OK || sheetPin->GetText().IsEmpty()  )
        {
            delete sheetPin;
            return nullptr;
        }
    }

    m_lastSheetPinType = sheetPin->GetShape();
    m_lastSheetPinTextSize = sheetPin->GetTextSize();

    sheetPin->SetPosition( (wxPoint) GetCanvas()->GetViewControls()->GetCursorPosition() );

    return sheetPin;
}


SCH_HIERLABEL* SCH_EDIT_FRAME::ImportHierLabel( SCH_SHEET* aSheet )
{
    if( !aSheet->GetScreen() )
        return NULL;

    for( EDA_ITEM* item = aSheet->GetScreen()->GetDrawItems(); item != NULL; item = item->Next() )
    {
        if( item->Type() != SCH_HIER_LABEL_T )
            continue;

        SCH_HIERLABEL* label = (SCH_HIERLABEL*) item;

        /* A global label has been found: check if there a corresponding sheet label. */
        if( !aSheet->HasPin( label->GetText() ) )
            return label;
    }

    return nullptr;
}


/*
 * Copy the current page or block to the clipboard, to export drawings to other applications
 * (word processing ...) This is not suitable for copy command within Eeschema or Pcbnew.
 */
void SCH_EDIT_FRAME::DrawCurrentSheetToClipboard()
{
    wxRect  DrawArea;
    BASE_SCREEN* screen = GetScreen();

    DrawArea.SetSize( GetPageSizeIU() );

    // Calculate a reasonable dc size, in pixels, and the dc scale to fit
    // the drawings into the dc size
    // scale is the ratio resolution (in PPI) / internal units
    double ppi = 300;   // Use 300 pixels per inch to create bitmap images on start
    double inch2Iu = 1000.0 * IU_PER_MILS;
    double  scale = ppi / inch2Iu;

    wxSize dcsize = DrawArea.GetSize();

    int maxdim = std::max( dcsize.x, dcsize.y );

    // the max size in pixels of the bitmap used to byuild the sheet copy
    const int maxbitmapsize = 3000;

    while( int( maxdim * scale ) > maxbitmapsize )
    {
        ppi = ppi / 1.5;
        scale = ppi / inch2Iu;
    }

    dcsize.x *= scale;
    dcsize.y *= scale;

    // Set draw offset, zoom... to values needed to draw in the memory DC
    // after saving initial values:
    wxPoint tmp_startvisu = screen->m_StartVisu;
    double tmpzoom = screen->GetZoom();
    wxPoint old_org = screen->m_DrawOrg;
    screen->m_DrawOrg.x   = screen->m_DrawOrg.y = 0;
    screen->m_StartVisu.x = screen->m_StartVisu.y = 0;

    screen->SetZoom( 1 );   // we use zoom = 1 in draw functions.

    wxMemoryDC dc;
    wxBitmap image( dcsize );
    dc.SelectObject( image );

    GRResetPenAndBrush( &dc );
    GRForceBlackPen( false );
    screen->m_IsPrinting = true;
    dc.SetUserScale( scale, scale );

    dc.Clear();
    PrintPage( &dc );
    screen->m_IsPrinting = false;

    if( wxTheClipboard->Open() )
    {
        // This data objects are held by the clipboard, so do not delete them in the app.
        wxBitmapDataObject* clipbrd_data = new wxBitmapDataObject( image );
        wxTheClipboard->SetData( clipbrd_data );
        wxTheClipboard->Close();
    }

    // Deselect Bitmap from DC in order to delete the MemoryDC
    dc.SelectObject( wxNullBitmap );

    GRForceBlackPen( false );

    screen->m_StartVisu = tmp_startvisu;
    screen->m_DrawOrg   = old_org;
    screen->SetZoom( tmpzoom );
}


