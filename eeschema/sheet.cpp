/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
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

#include <sch_draw_panel.h>
#include <confirm.h>
#include <kiface_base.h>
#include <project.h>
#include <math/vector2wx.h>
#include <wildcards_and_files_ext.h>
#include <tool/tool_manager.h>
#include <project_sch.h>
#include <sch_edit_frame.h>
#include <sch_io/kicad_legacy/sch_io_kicad_legacy.h>
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
#include <wx/log.h>
#include <wx/msgdlg.h>
#include <wx/richmsgdlg.h>

#include <advanced_config.h>
#include <libraries/symbol_library_manager_adapter.h>

#include "printing/sch_printout.h"


bool SCH_EDIT_FRAME::CheckSheetForRecursion( SCH_SHEET* aSheet, SCH_SHEET_PATH* aCurrentSheet )
{
    wxASSERT( aSheet && aCurrentSheet );

    wxString msg;
    SCH_SHEET_LIST schematicSheets = Schematic().Hierarchy();
    SCH_SHEET_LIST loadedSheets( aSheet );  // This is the schematicSheets of the loaded file.

    wxString destFilePath = aCurrentSheet->LastScreen()->GetFileName();

    if( destFilePath.IsEmpty() )
    {
        // If file is unsaved then there can't (yet) be any recursion.
        return false;
    }

    // SCH_SCREEN object file paths are expected to be absolute.  If this assert fires,
    // something is seriously broken.
    wxASSERT( wxFileName( destFilePath ).IsAbsolute() );

    if( schematicSheets.TestForRecursion( loadedSheets, destFilePath ) )
    {
        msg.Printf( _( "The sheet changes cannot be made because the destination sheet already "
                       "has the sheet '%s' or one of its subsheets as a parent somewhere in the "
                       "schematic hierarchy." ),
                    destFilePath );
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
        msg.Printf( _( "The schematic '%s' has not had its symbol library links remapped "
                       "to the symbol library table.  The project this schematic belongs to "
                       "must first be remapped before it can be imported into the current "
                       "project." ),
                    aSheet->GetScreen()->GetFileName() );
        DisplayInfoMessage( this, msg );
        return true;
    }

    return false;
}


void SCH_EDIT_FRAME::InitSheet( SCH_SHEET* aSheet, const wxString& aNewFilename )
{
    SCH_SCREEN* newScreen = new SCH_SCREEN( &Schematic() );
    aSheet->SetScreen( newScreen );
    aSheet->GetScreen()->SetContentModified();
    aSheet->GetScreen()->SetFileName( aNewFilename );

    EESCHEMA_SETTINGS* cfg = eeconfig();
    wxCHECK( cfg, /* void */ );

    if( cfg->m_PageSettings.export_paper )
        newScreen->SetPageSettings( GetScreen()->GetPageSettings() );

    const TITLE_BLOCK& tb1 = GetScreen()->GetTitleBlock();
    TITLE_BLOCK        tb2 = newScreen->GetTitleBlock();

    if( cfg->m_PageSettings.export_revision )
        tb2.SetRevision( tb1.GetRevision() );

    if( cfg->m_PageSettings.export_date )
        tb2.SetDate( tb1.GetDate() );

    if( cfg->m_PageSettings.export_title )
        tb2.SetTitle( tb1.GetTitle() );

    if( cfg->m_PageSettings.export_company )
        tb2.SetCompany( tb1.GetCompany() );

    if( cfg->m_PageSettings.export_comment1 )
        tb2.SetComment( 0, tb1.GetComment( 0 ) );

    if( cfg->m_PageSettings.export_comment2 )
        tb2.SetComment( 1, tb1.GetComment( 1 ) );

    if( cfg->m_PageSettings.export_comment3 )
        tb2.SetComment( 2, tb1.GetComment( 2 ) );

    if( cfg->m_PageSettings.export_comment4 )
        tb2.SetComment( 3, tb1.GetComment( 3 ) );

    if( cfg->m_PageSettings.export_comment5 )
        tb2.SetComment( 4, tb1.GetComment( 4 ) );

    if( cfg->m_PageSettings.export_comment6 )
        tb2.SetComment( 5, tb1.GetComment( 5 ) );

    if( cfg->m_PageSettings.export_comment7 )
        tb2.SetComment( 6, tb1.GetComment( 6 ) );

    if( cfg->m_PageSettings.export_comment8 )
        tb2.SetComment( 7, tb1.GetComment( 7 ) );

    if( cfg->m_PageSettings.export_comment9 )
        tb2.SetComment( 8, tb1.GetComment( 8 ) );

    newScreen->SetTitleBlock( tb2 );
}


bool SCH_EDIT_FRAME::LoadSheetFromFile( SCH_SHEET* aSheet, SCH_SHEET_PATH* aCurrentSheet,
                                        const wxString& aFileName, bool aSkipRecursionCheck,
                                        bool aSkipLibCheck )
{
    wxASSERT( aSheet && aCurrentSheet );

    wxString    msg;
    wxFileName  currentSheetFileName;
    bool        libTableChanged = false;

    SCH_IO_MGR::SCH_FILE_T schFileType = SCH_IO_MGR::GuessPluginTypeFromSchPath( aFileName );

    if( schFileType == SCH_IO_MGR::SCH_FILE_UNKNOWN )
        schFileType = SCH_IO_MGR::SCH_KICAD;

    IO_RELEASER<SCH_IO> pi( SCH_IO_MGR::FindPlugin( schFileType ) );
    std::unique_ptr< SCH_SHEET> tmpSheet = std::make_unique<SCH_SHEET>( &Schematic() );

    // This will cause the sheet UUID to be set to the UUID of the aSheet argument.  This is
    // required to ensure all of the sheet paths in any sub-sheets are correctly generated when
    // using the temporary SCH_SHEET object that the file is loaded into..
    const_cast<KIID&>( tmpSheet->m_Uuid ) = aSheet->m_Uuid;

    wxFileName fileName( aFileName );

    if( !fileName.IsAbsolute() && !fileName.MakeAbsolute() )
    {
        wxFAIL_MSG( wxString::Format( "Cannot make file name '%s' path absolute.", aFileName ) );
        return false;
    }

    wxString fullFilename = fileName.GetFullPath();

    try
    {
        if( aSheet->GetScreen() != nullptr )
        {
            tmpSheet.reset( pi->LoadSchematicFile( fullFilename, &Schematic() ) );
        }
        else
        {
            tmpSheet->SetFileName( fullFilename );
            pi->LoadSchematicFile( fullFilename, &Schematic(), tmpSheet.get() );
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
        msg.Printf( _( "Error loading schematic '%s'." ), fullFilename );
        DisplayErrorMessage( this, msg, ioe.What() );

        msg.Printf( _( "Failed to load '%s'." ), fullFilename );
        SetMsgPanel( wxEmptyString, msg );

        return false;
    }

    // If the loaded schematic is in a different folder from the current project and
    // it contains hierarchical sheets, the hierarchical sheet paths need to be updated.
    //
    // Additionally, we need to make all backing screens absolute paths be in the current project
    // path not the source path.
    if( fileName.GetPathWithSep() != Prj().GetProjectPath() )
    {
        SCH_SHEET_LIST loadedSheets( tmpSheet.get() );

        for( const SCH_SHEET_PATH& sheetPath : loadedSheets )
        {
            // Skip the loaded sheet since the user already determined if the file path should
            // be relative or absolute.
            if( sheetPath.size() == 1 )
                continue;

            wxString lastSheetPath = Prj().GetProjectPath();

            for( unsigned i = 1; i < sheetPath.size(); i++ )
            {
                SCH_SHEET* sheet = sheetPath.at( i );
                wxCHECK2( sheet, continue );

                SCH_SCREEN* screen = sheet->GetScreen();
                wxCHECK2( screen, continue );

                // Use the screen file name which should always be absolute.
                wxFileName loadedSheetFileName = screen->GetFileName();
                wxCHECK2( loadedSheetFileName.IsAbsolute(), continue );

                wxFileName tmp = loadedSheetFileName;
                wxString sheetFileName;

                if( tmp.MakeRelativeTo( lastSheetPath ) )
                    sheetFileName = tmp.GetFullPath();
                else
                    sheetFileName = loadedSheetFileName.GetFullPath();

                sheetFileName.Replace( wxT( "\\" ), wxT( "/" ) );
                sheet->SetFileName( sheetFileName );
                lastSheetPath = loadedSheetFileName.GetPath();
            }
        }
    }

    SCH_SHEET_LIST loadedSheets( tmpSheet.get() );
    Schematic().RefreshHierarchy();
    SCH_SHEET_LIST schematicSheets = Schematic().Hierarchy();

    // Make sure any new sheet changes do not cause any recursion issues.
    if( !aSkipRecursionCheck && CheckSheetForRecursion( tmpSheet.get(), aCurrentSheet ) )
        return false;

    if( checkForNoFullyDefinedLibIds( tmpSheet.get() ) )
        return false;

    // Make a valiant attempt to warn the user of all possible scenarios where there could
    // be broken symbol library links.
    wxArrayString    names;
    wxArrayString    newLibNames;
    SCH_SCREENS      newScreens( tmpSheet.get() );   // All screens associated with the import.
    SCH_SCREENS      prjScreens( &Schematic().Root() );

    newScreens.GetLibNicknames( names );

    wxMessageDialog::ButtonLabel okButtonLabel( _( "Continue Load" ) );
    wxMessageDialog::ButtonLabel cancelButtonLabel( _( "Cancel Load" ) );

    // Prior to schematic file format 20221002, all symbol instance data was saved in the root
    // sheet so loading a hierarchical sheet that is not the root sheet will have no symbol
    // instance data.  Give the user a chance to go back and save the project that contains this
    // hierarchical sheet so the symbol instance data will be correct on load.
    if( ( tmpSheet->GetScreen()->GetFileFormatVersionAtLoad() < 20221002 )
      && tmpSheet->GetScreen()->GetSymbolInstances().empty() )
    {
        msg = _( "There are hierarchical sheets in the loaded schematic file from an older "
                 "file version resulting in  missing symbol instance data.  This will "
                 "result in all of the symbols in the loaded schematic to use either the "
                 "default instance setting or fall back to the library symbol settings.  "
                 "Loading the project that uses this schematic file and saving to the "
                 "latest file version will resolve this issue.\n\n"
                 "Do you wish to continue?" );
        wxMessageDialog msgDlg7( this, msg, _( "Continue Load Schematic" ),
                                 wxOK | wxCANCEL | wxCANCEL_DEFAULT | wxCENTER | wxICON_QUESTION );
        msgDlg7.SetOKCancelLabels( okButtonLabel, cancelButtonLabel );

        if( msgDlg7.ShowModal() == wxID_CANCEL )
            return false;
    }

    if( !aSkipLibCheck && !prjScreens.HasSchematic( fullFilename ) )
    {
        if( fileName.GetPathWithSep() == Prj().GetProjectPath() )
        {
            // A schematic in the current project path that isn't part of the current project.
            // It's possible the user copied this schematic from another project so the library
            // links may not be available.  Even this is check is no guarantee that all symbol
            // library links are valid but it's better than nothing.
            for( const wxString& name : names )
            {
                if( !PROJECT_SCH::SymbolLibManager( &Prj() )->HasLibrary( name ) )
                    newLibNames.Add( name );
            }

            if( !newLibNames.IsEmpty() )
            {
                msg = _( "There are library names in the selected schematic that are missing "
                         "from the current project library table.  This may result in broken "
                         "symbol library references for the loaded schematic.\n\n"
                         "Do you wish to continue?" );
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

            for( const wxString& name : names )
            {
                if( !PROJECT_SCH::SymbolLibManager( &Prj() )->HasLibrary( name ) )
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
                    msg = _( "The selected file was created as part of a different project.  "
                             "Linking the file to this project may result in missing or "
                             "incorrect symbol library references.\n\n"
                             "Do you wish to continue?" );
                    wxMessageDialog msgDlg4( this, msg, _( "Continue Load Schematic" ),
                                             wxOK | wxCANCEL | wxCANCEL_DEFAULT | wxCENTER
                                                     | wxICON_QUESTION );
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
                        msg.Printf( _( "Error loading the symbol library table '%s'." ),
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
                    for( const wxString& newLibName : newLibNames )
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
                    msg = _( "There are symbol library names in the selected schematic that "
                             "are missing from the selected schematic project library table.  "
                             "This may result in broken symbol library references.\n\n"
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

                for( const wxString& duplicateLibName : duplicateLibNames )
                {
                    const SYMBOL_LIB_TABLE_ROW* thisRow = nullptr;
                    const SYMBOL_LIB_TABLE_ROW* otherRow = nullptr;

                    // TODO(JE) library tables
#if 0
                    if( PROJECT_SCH::SymbolLibManager( &Prj() )->HasLibrary( duplicateLibName ) )
                        thisRow = PROJECT_SCH::SymbolLibManager( &Prj() )->FindRow( duplicateLibName );
#endif

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
                             "may result in broken symbol library references.\n\n"
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
                      || PROJECT_SCH::SymbolLibManager( &Prj() )->HasLibrary( libName ) )
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

                    // TODO(JE) library tables
                    //PROJECT_SCH::SchSymbolLibTable( &Prj() )->InsertRow( newRow );
                    libTableChanged = true;
                }
            }
        }
    }

    SCH_SCREEN* newScreen = tmpSheet->GetScreen();
    wxCHECK_MSG( newScreen, false, "No screen defined for sheet." );

    // TODO(JE) library tables
#if 0
    if( libTableChanged )
    {
        PROJECT_SCH::SchSymbolLibTable( &Prj() )->Save( Prj().GetProjectPath() +
                                         SYMBOL_LIB_TABLE::GetSymbolLibTableFileName() );
    }
#endif

    // Make the best attempt to set the symbol instance data for the loaded schematic.
    if( newScreen->GetFileFormatVersionAtLoad() < 20221002 )
    {
        if( !newScreen->GetSymbolInstances().empty() )
        {
            // If the loaded schematic is a root sheet for another project, update the symbol
            // instances.
            loadedSheets.UpdateSymbolInstanceData( newScreen->GetSymbolInstances());
        }
    }

    newScreen->MigrateSimModels();

    // Attempt to create new symbol instances using the instance data loaded above.
    loadedSheets.AddNewSymbolInstances( *aCurrentSheet, Prj().GetProjectName() );

    // Add new sheet instance data.
    loadedSheets.AddNewSheetInstances( *aCurrentSheet, schematicSheets.GetLastVirtualPageNumber() );

    // It is finally safe to add or append the imported schematic.
    if( aSheet->GetScreen() == nullptr )
        aSheet->SetScreen( newScreen );
    else
        aSheet->GetScreen()->Append( newScreen );

    SCH_SCREENS allLoadedScreens( aSheet );
    allLoadedScreens.ReplaceDuplicateTimeStamps();

    return true;
}


bool SCH_EDIT_FRAME::EditSheetProperties( SCH_SHEET* aSheet, SCH_SHEET_PATH* aHierarchy,
                                          bool* aIsUndoable, bool* aClearAnnotationNewItems,
                                          bool* aUpdateHierarchyNavigator,
                                          wxString* aSourceSheetFilename )
{
    if( aSheet == nullptr || aHierarchy == nullptr )
        return false;

    // Get the new texts
    DIALOG_SHEET_PROPERTIES dlg( this, aSheet, aIsUndoable, aClearAnnotationNewItems,
                                 aUpdateHierarchyNavigator, aSourceSheetFilename );

    if( dlg.ShowModal() == wxID_CANCEL )
        return false;

    m_toolManager->ProcessEvent( EVENTS::SelectedItemsModified );

    return true;
}


void SCH_EDIT_FRAME::DrawCurrentSheetToClipboard()
{
    wxRect       drawArea;
    BASE_SCREEN* screen = GetScreen();

    drawArea.SetSize( ToWxSize( GetPageSizeIU() ) );

    // Calculate a reasonable dc size, in pixels, and the dc scale to fit
    // the drawings into the dc size
    // scale is the ratio resolution (in PPI) / internal units
    double ppi = 300;   // Use 300 pixels per inch to create bitmap images on start
    double inch2Iu = 1000.0 * schIUScale.IU_PER_MILS;
    double scale = ppi / inch2Iu;

    wxSize dcsize = drawArea.GetSize();

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
    VECTOR2I tmp_startvisu = screen->m_StartVisu;
    VECTOR2I old_org = screen->m_DrawOrg;
    screen->m_DrawOrg.x = screen->m_DrawOrg.y = 0;
    screen->m_StartVisu.x = screen->m_StartVisu.y = 0;

    wxMemoryDC dc;
    wxBitmap image( dcsize );
    dc.SelectObject( image );
    dc.Clear();

    GRResetPenAndBrush( &dc );
    GRForceBlackPen( false );
    dc.SetUserScale( scale, scale );

    SCH_RENDER_SETTINGS* cfg = GetRenderSettings();

    cfg->SetPrintDC( &dc );

    // Init the color of the layer actually used to print the drawing sheet:
    cfg->SetLayerColor( LAYER_DRAWINGSHEET, cfg->GetLayerColor( LAYER_SCHEMATIC_DRAWINGSHEET ) );

    cfg->SetDefaultFont( eeconfig()->m_Appearance.default_font );

    try
    {
        dc.SetUserScale( 1.0, 1.0 );
        SCH_PRINTOUT printout( this, wxEmptyString );
        // Ensure title block will be when printed on clipboard, regardless
        // the current Cairo print option
        EESCHEMA_SETTINGS* eecfg = eeconfig();
        bool print_tb_opt = eecfg->m_Printing.title_block;
        eecfg->m_Printing.title_block = true;
        bool success = printout.PrintPage( GetScreen(), cfg->GetPrintDC(), false );
        eecfg->m_Printing.title_block = print_tb_opt;

        if( !success )
            wxLogMessage( _( "Cannot create the schematic image") );
    }
    catch( ... )
    {
        wxLogMessage( "printout internal error" );
    }

    // Deselect Bitmap from DC before using the bitmap
    dc.SelectObject( wxNullBitmap );

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

    GRForceBlackPen( false );

    screen->m_StartVisu = tmp_startvisu;
    screen->m_DrawOrg   = old_org;
}


bool SCH_EDIT_FRAME::AllowCaseSensitiveFileNameClashes( const wxString& aOldName,
                                                        const wxString& aSchematicFileName )
{
    wxString       msg;
    SCH_SHEET_LIST sheets = Schematic().Hierarchy();
    wxFileName     fn = aSchematicFileName;

    wxCHECK( fn.IsAbsolute(), false );

    auto can_cause_issues = [&]() -> bool
    {
        wxFileName lhs;
        wxFileName rhs = aSchematicFileName;
        wxFileName old = aOldName;
        wxString   oldLower = old.GetFullName().Lower();
        wxString   rhsLower = rhs.GetFullName().Lower();
        wxString   lhsLower;

        size_t     count = 0;

        wxCHECK( rhs.IsAbsolute(), false );

        for( SCH_SHEET_PATH& sheet : sheets )
        {
            lhs = sheet.LastScreen()->GetFileName();

            if( lhs.GetPath() != rhs.GetPath() )
                continue;

            lhsLower = lhs.GetFullName().Lower();

            if( lhsLower == rhsLower && lhs.GetFullName() != rhs.GetFullName() )
                count++;
        }

        // If we are renaming a sheet that is only used once, then we are not going to cause
        // a case sensitivity issue.
        if( oldLower == rhsLower )
            return count > 1;

        return count > 0;
    };

    if( eeconfig()->m_Appearance.show_sheet_filename_case_sensitivity_dialog && can_cause_issues() )
    {
        msg.Printf( _( "The file name '%s' can cause issues with an existing file name\n"
                       "already defined in the schematic on systems that support case\n"
                       "insensitive file names.  This will cause issues if you copy this\n"
                       "project to an operating system that supports case insensitive file\n"
                       "names.\n\nDo you wish to continue?" ),
                    fn.GetName() );

        wxRichMessageDialog dlg( this, msg, _( "Warning" ),
                                 wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION );
        dlg.ShowCheckBox( _( "Do not show this message again." ) );
        dlg.SetYesNoLabels( wxMessageDialog::ButtonLabel( _( "Create New Sheet" ) ),
                            wxMessageDialog::ButtonLabel( _( "Cancel" ) ) );

        if( dlg.ShowModal() == wxID_NO )
            return false;

        eeconfig()->m_Appearance.show_sheet_filename_case_sensitivity_dialog =
                                                            !dlg.IsCheckBoxChecked();
    }

    return true;
}
