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
#include <wx/clipbrd.h>
#include <sch_edit_frame.h>
#include <sch_legacy_plugin.h>
#include <sch_sheet.h>
#include <sch_sheet_path.h>
#include <sch_view.h>
#include <dialogs/dialog_sch_sheet_props.h>
#include <dialogs/dialog_sch_edit_sheet_pin.h>


void SCH_EDIT_FRAME::InitSheet( SCH_SHEET* aSheet, const wxString& aNewFilename )
{
    aSheet->SetScreen( new SCH_SCREEN( &Kiway() ) );
    aSheet->GetScreen()->SetModify();
    aSheet->GetScreen()->SetMaxUndoItems( m_UndoRedoCountMax );
    aSheet->GetScreen()->SetFileName( aNewFilename );
}


void SCH_EDIT_FRAME::LoadSheetFromFile( SCH_SHEET* aSheet, SCH_SHEET_PATH* aHierarchy,
                                        const wxString& aExistingFilename )
{
    SCH_PLUGIN::SCH_PLUGIN_RELEASER pi( SCH_IO_MGR::FindPlugin( SCH_IO_MGR::SCH_LEGACY ) );

    wxFileName fileName( aExistingFilename );

    if( !fileName.IsAbsolute() )
    {
        const SCH_SCREEN* currentScreen = aHierarchy->LastScreen();
        wxFileName currentSheetFileName = currentScreen->GetFileName();
        fileName.Normalize( wxPATH_NORM_ALL, currentSheetFileName.GetPath() );
    }

    wxString fullFilename = fileName.GetFullPath();

    try
    {
        pi->Load( fullFilename, &Kiway(), aSheet );

        if( !pi->GetError().IsEmpty() )
        {
            DisplayErrorMessage( this, _( "The entire schematic could not be loaded.\n"
                                          "Errors occurred loading hierarchical sheets." ),
                                 pi->GetError() );
        }
    }
    catch( const IO_ERROR& ioe )
    {
        wxString msg;

        msg.Printf( _( "Error occurred loading schematic file \"%s\"." ), fullFilename );
        DisplayErrorMessage( this, msg, ioe.What() );

        msg.Printf( _( "Failed to load schematic \"%s\"" ), fullFilename );
        AppendMsgPanel( wxEmptyString, msg, CYAN );

        return;
    }

    SCH_SCREEN* screen = aSheet->GetScreen();

    if( screen )
        screen->UpdateSymbolLinks( true );
}


bool SCH_EDIT_FRAME::EditSheet( SCH_SHEET* aSheet, SCH_SHEET_PATH* aHierarchy,
                                bool* aClearAnnotationNewItems )
{
    if( aSheet == NULL || aHierarchy == NULL )
        return false;

    SCH_SHEET_LIST hierarchy( g_RootSheet );       // This is the schematic sheet hierarchy.

    // Get the new texts
    DIALOG_SCH_SHEET_PROPS dlg( this, aSheet );

    if( dlg.ShowModal() == wxID_CANCEL )
        return false;

    wxFileName fileName = dlg.GetFileName();
    fileName.SetExt( SchematicFileExtension );

    wxString msg;
    bool loadFromFile = false;
    bool clearAnnotation = false;
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

    // Search for a schematic file having the same filename
    // already in use in the hierarchy or on disk, in order to reuse it.
    if( !g_RootSheet->SearchHierarchy( newFilename, &useScreen ) )
    {
        loadFromFile = wxFileExists( newFilename );
        wxLogDebug( "Sheet requested file \"%s\", %s",
                    newFilename,
                    ( loadFromFile ) ? "found" : "not found" );
    }

    // Inside Eeschema, filenames are stored using unix notation
    newFilename.Replace( wxT( "\\" ), wxT( "/" ) );

    if( aSheet->GetScreen() == NULL )              // New sheet.
    {
        if( useScreen || loadFromFile )            // Load from existing file.
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
        else                                                   // New file.
        {
            InitSheet( aSheet, newFilename );
        }
    }
    else                                                       // Existing sheet.
    {
        bool isUndoable = true;
        bool renameFile = false;
        wxString replaceMsg;
        wxString newMsg;
        wxString noUndoMsg;

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

            if( useScreen || loadFromFile )                    // Load from existing file.
            {
                clearAnnotation = true;

                msg.Printf( wxT( "%s\n\n%s" ), replaceMsg, noUndoMsg );

                if( !IsOK( this, msg ) )
                    return false;

                if( loadFromFile )
                    aSheet->SetScreen( NULL );
            }
            else                                               // Save to new file name.
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
    aSheet->SetFileName( userFileName.GetFullPath( wxPATH_UNIX ) );

    if( useScreen )
    {
        aSheet->SetScreen( useScreen );
    }
    else if( loadFromFile )
    {
        LoadSheetFromFile( aSheet, aHierarchy, newFilename );
    }

    aSheet->SetFileNameSize( dlg.GetFileNameTextSize() );
    aSheet->SetName( dlg.GetSheetName() );
    aSheet->SetSheetNameSize( dlg.GetSheetNameTextSize() );

    if( aSheet->GetName().IsEmpty() )
        aSheet->SetName( wxString::Format( wxT( "Sheet%8.8lX" ),
                                           (long unsigned) aSheet->GetTimeStamp() ) );

    // Make sure the sheet changes do not cause any recursion.
    SCH_SHEET_LIST sheetHierarchy( aSheet );

    // Make sure files have fully qualified path and file name.
    wxFileName destFn = aHierarchy->Last()->GetFileName();

    if( destFn.IsRelative() )
        destFn.MakeAbsolute( Prj().GetProjectPath() );

    if( hierarchy.TestForRecursion( sheetHierarchy, destFn.GetFullPath( wxPATH_UNIX ) ) )
    {
        msg.Printf( _( "The sheet changes cannot be made because the destination sheet already "
                       "has the sheet \"%s\" or one of it's subsheets as a parent somewhere in "
                       "the schematic hierarchy." ),
                    newFilename );
        DisplayError( this, msg );
        return false;
    }

    // Check to make sure the symbols have been remapped to the symbol library table.
    SCH_SCREENS newScreens( aSheet );

    if( newScreens.HasNoFullyDefinedLibIds() )
    {
        msg.Printf(_( "The schematic \"%s\" has not been remapped to the symbol\nlibrary table. "
                      " The project this schematic belongs to must first be remapped\nbefore it "
                      "can be imported into the current project." ), fileName.GetFullName() );

        DisplayInfoMessage( this, msg );
        return false;
    }

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


int SCH_EDIT_FRAME::EditSheetPin( SCH_SHEET_PIN* aSheetPin, bool aRedraw )
{
    if( aSheetPin == NULL )
        return wxID_CANCEL;

    DIALOG_SCH_EDIT_SHEET_PIN dlg( this, aSheetPin );

    if( dlg.ShowModal() == wxID_CANCEL )
        return wxID_CANCEL;

    if( aRedraw )
        RefreshItem( aSheetPin );

    return wxID_OK;
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
        int response = EditSheetPin( sheetPin, false );

        if( sheetPin->GetText().IsEmpty() || (response == wxID_CANCEL) )
        {
            delete sheetPin;
            return NULL;
        }
    }

    m_lastSheetPinType = sheetPin->GetShape();
    m_lastSheetPinTextSize = sheetPin->GetTextSize();

    sheetPin->SetPosition( GetCrossHairPosition() );

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
void SCH_EDIT_FRAME::DrawCurrentSheetToClipboard( wxCommandEvent& aEvt )
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


