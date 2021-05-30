/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2004-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <sch_draw_panel.h>
#include <confirm.h>
#include <kiface_i.h>
#include <project.h>
#include <wildcards_and_files_ext.h>
#include <tool/tool_manager.h>
#include <sch_edit_frame.h>
#include <sch_plugins/legacy/sch_legacy_plugin.h>
#include <sch_sheet.h>
#include <sch_sheet_path.h>
#include <sch_view.h>
#include <sch_painter.h>
#include <schematic.h>
#include <symbol_lib_table.h>
#include <dialogs/dialog_sheet_properties.h>
#include <tool/actions.h>

#include <wx/clipbrd.h>
#include <wx/dcmemory.h>


bool SCH_EDIT_FRAME::CheckSheetForRecursion( SCH_SHEET* aSheet, SCH_SHEET_PATH* aHierarchy )
{
    wxASSERT( aSheet && aHierarchy );

    wxString msg;
    SCH_SHEET_LIST hierarchy = Schematic().GetSheets();  // The full schematic sheet hierarchy.
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
    aSheet->SetScreen( new SCH_SCREEN( &Schematic() ) );
    aSheet->GetScreen()->SetContentModified();
    aSheet->GetScreen()->SetFileName( aNewFilename );
}


bool SCH_EDIT_FRAME::LoadSheetFromFile( SCH_SHEET* aSheet, SCH_SHEET_PATH* aHierarchy,
                                        const wxString& aFileName )
{
    wxASSERT( aSheet && aHierarchy );

    wxString    msg;
    wxString    topLevelSheetPath;
    wxFileName  tmp;
    wxFileName  currentSheetFileName;
    bool        libTableChanged = false;
    SCH_IO_MGR::SCH_FILE_T schFileType = SCH_IO_MGR::GuessPluginTypeFromSchPath( aFileName );
    SCH_PLUGIN::SCH_PLUGIN_RELEASER pi( SCH_IO_MGR::FindPlugin( schFileType ) );
    std::unique_ptr< SCH_SHEET> newSheet = std::make_unique<SCH_SHEET>( &Schematic() );

    // This will cause the sheet UUID to be set to the loaded schematic UUID.  This is required
    // to ensure all of the sheet paths in any subsheets are correctly generated.
    const_cast<KIID&>( newSheet->m_Uuid ) = KIID( 0 );

    wxFileName fileName( aFileName );

    if( !fileName.IsAbsolute() && !fileName.MakeAbsolute() )
    {
        wxFAIL_MSG( wxString::Format( "Cannot make file name \"%s\" path absolute.", aFileName ) );
        return false;
    }

    wxString fullFilename = fileName.GetFullPath();

    try
    {
        if( aSheet->GetScreen() != nullptr )
        {
            newSheet.reset( pi->Load( fullFilename, &Schematic() ) );
        }
        else
        {
            newSheet->SetFileName( fullFilename );
            pi->Load( fullFilename, &Schematic(), newSheet.get() );
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
        SetMsgPanel( wxEmptyString, msg );

        return false;
    }

    tmp = fileName;

    // If the loaded schematic is in a different folder from the current project and
    // it contains hierarchical sheets, the hierarchical sheet paths need to be updated.
    if( fileName.GetPathWithSep() != Prj().GetProjectPath() && newSheet->CountSheets() )
    {
        // Give the user the option to choose relative path if possible.
        if( tmp.MakeRelativeTo( Prj().GetProjectPath() ) )
            topLevelSheetPath = tmp.GetPathWithSep();
        else
            topLevelSheetPath = fileName.GetPathWithSep();

        if( wxFileName::GetPathSeparator() == '\\' )
            topLevelSheetPath.Replace( "\\", "/" );
    }

    // Make sure any new sheet changes do not cause any recursion issues.
    SCH_SHEET_LIST hierarchy = Schematic().GetSheets(); // This is the schematic sheet hierarchy.
    SCH_SHEET_LIST sheetHierarchy( newSheet.get() );    // This is the hierarchy of the loaded file.

    if( CheckSheetForRecursion( newSheet.get(), aHierarchy )
          || checkForNoFullyDefinedLibIds( newSheet.get() ) )
    {
        return false;
    }

    // Make a valiant attempt to warn the user of all possible scenarios where there could
    // be broken symbol library links.
    wxArrayString    names;
    wxArrayString    newLibNames;
    SCH_SCREENS      newScreens( newSheet.get() );   // All screens associated with the import.
    SCH_SCREENS      prjScreens( &Schematic().Root() );

    newScreens.GetLibNicknames( names );

    wxMessageDialog::ButtonLabel okButtonLabel( _( "Continue Load" ) );
    wxMessageDialog::ButtonLabel cancelButtonLabel( _( "Cancel Load" ) );

    if( fileName.GetPathWithSep() == Prj().GetProjectPath()
      && !prjScreens.HasSchematic( fullFilename ) )
    {
        // A schematic in the current project path that isn't part of the current project.
        // It's possible the user copied this schematic from another project so the library
        // links may not be available.  Even this is check is no guarantee that all symbol
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
    else if( fileName.GetPathWithSep() != Prj().GetProjectPath() )
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
                    msg.Printf( _( "An error occurred loading the symbol library table \"%s\"." ),
                                symLibTableFn.GetFullPath() );
                    DisplayErrorMessage( nullptr, msg, ioe.What() );
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
            for( const wxString& libName : newLibNames )
            {
                if( !table.HasLibrary( libName )
                  || Prj().SchSymbolLibTable()->HasLibrary( libName ) )
                {
                    continue;
                }

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

                wxCHECK( row, false );

                SYMBOL_LIB_TABLE_ROW* newRow = new SYMBOL_LIB_TABLE_ROW( libName, uri,
                                                                         row->GetType(),
                                                                         row->GetOptions(),
                                                                         row->GetDescr() );

                Prj().SchSymbolLibTable()->InsertRow( newRow );
                libTableChanged = true;
            }
        }
    }

    SCH_SCREEN* newScreen = newSheet->GetScreen();
    wxCHECK_MSG( newScreen, false, "No screen defined for sheet." );

    if( libTableChanged )
    {
        Prj().SchSymbolLibTable()->Save( Prj().GetProjectPath() +
                                         SYMBOL_LIB_TABLE::GetSymbolLibTableFileName() );
    }

    // It is finally safe to add or append the imported schematic.
    if( aSheet->GetScreen() == nullptr )
        aSheet->SetScreen( newScreen );
    else
        aSheet->GetScreen()->Append( newScreen );

    SCH_SCREENS allScreens( Schematic().Root() );
    allScreens.ReplaceDuplicateTimeStamps();

    return true;
}


bool SCH_EDIT_FRAME::EditSheetProperties( SCH_SHEET* aSheet, SCH_SHEET_PATH* aHierarchy,
                                          bool* aClearAnnotationNewItems )
{
    if( aSheet == nullptr || aHierarchy == nullptr )
        return false;

    // Get the new texts
    DIALOG_SHEET_PROPERTIES dlg( this, aSheet, aClearAnnotationNewItems );

    if( dlg.ShowModal() == wxID_CANCEL )
        return false;

    return true;
}


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
    double scale = ppi / inch2Iu;

    wxSize dcsize = DrawArea.GetSize();

    int maxdim = std::max( dcsize.x, dcsize.y );

    // the max size in pixels of the bitmap used to build the sheet copy
    const int maxbitmapsize = 5600;

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
    wxPoint old_org       = screen->m_DrawOrg;
    screen->m_DrawOrg.x   = screen->m_DrawOrg.y = 0;
    screen->m_StartVisu.x = screen->m_StartVisu.y = 0;

    wxMemoryDC dc;
    wxBitmap image( dcsize );
    dc.SelectObject( image );
    dc.Clear();

    GRResetPenAndBrush( &dc );
    GRForceBlackPen( false );
    dc.SetUserScale( scale, scale );

    GetRenderSettings()->SetPrintDC( &dc );
    // Init the color of the layer actually used to print the drawing sheet:
    GetRenderSettings()->SetLayerColor( LAYER_DRAWINGSHEET,
                GetRenderSettings()->GetLayerColor( LAYER_SCHEMATIC_DRAWINGSHEET ) );

    PrintPage( GetRenderSettings() );

    {
        wxLogNull doNotLog; // disable logging of failed clipboard actions

        if( wxTheClipboard->Open() )
        {
            // This data objects are held by the clipboard, so do not delete them in the app.
            wxBitmapDataObject* clipbrd_data = new wxBitmapDataObject( image );
            wxTheClipboard->SetData( clipbrd_data );
            wxTheClipboard->Flush(); // Allow data to be available after closing KiCad
            wxTheClipboard->Close();
        }
    }

    // Deselect Bitmap from DC in order to delete the MemoryDC
    dc.SelectObject( wxNullBitmap );

    GRForceBlackPen( false );

    screen->m_StartVisu = tmp_startvisu;
    screen->m_DrawOrg   = old_org;
}


bool SCH_EDIT_FRAME::AllowCaseSensitiveFileNameClashes( const wxString& aSchematicFileName )
{
    wxString msg;
    SCH_SCREENS screens( Schematic().Root() );
    wxFileName fn = aSchematicFileName;

    wxCHECK( fn.IsAbsolute(), false );

    if( eeconfig()->m_Appearance.show_sheet_filename_case_sensitivity_dialog
      && screens.CanCauseCaseSensitivityIssue( aSchematicFileName ) )
    {
        msg.Printf( _( "The file name \"%s\" can cause issues with an existing file name\n"
                       "already defined in the schematic on systems that support case\n"
                       "insensitive file names.  This will cause issues if you copy this\n"
                       "project to an operating system that supports case insensitive file\n"
                       "names.\n\nDo you wish to continue?" ),
                    fn.GetName() );

        wxRichMessageDialog dlg( this, msg, _( "Warning" ),
                                 wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION );
        dlg.ShowCheckBox( _( "Do not show this message again." ) );
        dlg.SetYesNoLabels( wxMessageDialog::ButtonLabel( _( "Create New Sheet" ) ),
                            wxMessageDialog::ButtonLabel( _( "Discard New Sheet" ) ) );

        if( dlg.ShowModal() == wxID_NO )
            return false;

        eeconfig()->m_Appearance.show_sheet_filename_case_sensitivity_dialog =
                                                            !dlg.IsCheckBoxChecked();
    }

    return true;
}
