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
#include <sch_edit_frame.h>
#include <sch_legacy_plugin.h>
#include <sch_sheet.h>
#include <sch_sheet_path.h>
#include <sch_view.h>
#include <dialogs/dialog_sch_sheet_props.h>
#include <dialogs/dialog_sch_edit_sheet_pin.h>


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

    SCH_PLUGIN::SCH_PLUGIN_RELEASER pi( SCH_IO_MGR::FindPlugin( SCH_IO_MGR::SCH_LEGACY ) );

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
            aSheet->SetScreen( new SCH_SCREEN( &Kiway() ) );
            aSheet->GetScreen()->SetModify();
            aSheet->GetScreen()->SetMaxUndoItems( m_UndoRedoCountMax );
            aSheet->GetScreen()->SetFileName( newFilename );
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

        m_canvas->SetIgnoreMouseEvents( true );

        if( isUndoable )
            SaveCopyInUndoList( aSheet, UR_CHANGED );

        if( renameFile )
        {
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
        try
        {
            aSheet = pi->Load( newFilename, &Kiway(), aSheet );

            if( !pi->GetError().IsEmpty() )
            {
                DisplayErrorMessage( this,
                                     _( "The entire schematic could not be loaded.\n"
                                        "Errors occurred loading hierarchical sheets." ),
                                     pi->GetError() );
            }
        }
        catch( const IO_ERROR& ioe )
        {
            msg.Printf( _( "Error occurred loading schematic file \"%s\"." ), newFilename );
            DisplayErrorMessage( this, msg, ioe.What() );

            msg.Printf( _( "Failed to load schematic \"%s\"" ), newFilename );
            AppendMsgPanel( wxEmptyString, msg, CYAN );

            return false;
        }
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

    m_canvas->MoveCursorToCrossHair();
    m_canvas->SetIgnoreMouseEvents( false );

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


SCH_SHEET_PIN* SCH_EDIT_FRAME::CreateSheetPin( SCH_SHEET* aSheet )
{
    wxString       line;
    SCH_SHEET_PIN* sheetPin;

    sheetPin = new SCH_SHEET_PIN( aSheet, wxPoint( 0, 0 ), line );
    sheetPin->SetFlags( IS_NEW );
    sheetPin->SetTextSize( GetLastSheetPinTextSize() );
    sheetPin->SetShape( m_lastSheetPinType );

    int response = EditSheetPin( sheetPin, false );

    if( sheetPin->GetText().IsEmpty() || (response == wxID_CANCEL) )
    {
        delete sheetPin;
        return NULL;
    }

    m_lastSheetPinType = sheetPin->GetShape();
    m_lastSheetPinTextSize = sheetPin->GetTextSize();

    sheetPin->SetPosition( GetCrossHairPosition() );

    return sheetPin;
}


SCH_SHEET_PIN* SCH_EDIT_FRAME::ImportSheetPin( SCH_SHEET* aSheet )
{
    EDA_ITEM*      item;
    SCH_SHEET_PIN* sheetPin;
    SCH_HIERLABEL* label = NULL;

    if( !aSheet->GetScreen() )
        return NULL;

    item = aSheet->GetScreen()->GetDrawItems();

    for( ; item != NULL; item = item->Next() )
    {
        if( item->Type() != SCH_HIER_LABEL_T )
            continue;

        label = (SCH_HIERLABEL*) item;

        /* A global label has been found: check if there a corresponding sheet label. */
        if( !aSheet->HasPin( label->GetText() ) )
            break;

        label = NULL;
    }

    if( label == NULL )
    {
        DisplayInfoMessage( this, _( "No new hierarchical labels found." ) );
        return NULL;
    }

    sheetPin = new SCH_SHEET_PIN( aSheet, wxPoint( 0, 0 ), label->GetText() );
    sheetPin->SetFlags( IS_NEW );
    sheetPin->SetTextSize( GetLastSheetPinTextSize() );
    m_lastSheetPinType = label->GetShape();
    sheetPin->SetShape( label->GetShape() );
    sheetPin->SetPosition( GetCrossHairPosition() );

    return sheetPin;
}
