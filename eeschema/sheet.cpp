/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2004-2018 KiCad Developers, see change_log.txt for contributors.
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
 * @file sheet.cpp
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

        SCH_SCREEN* screen = aSheet->GetScreen();

        if( screen )
            screen->UpdateSymbolLinks( true );
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


/* Move selected sheet with the cursor.
 * Callback function used by m_mouseCaptureCallback.
 * Note also now this function is called only when resizing the sheet
 * But the (very small code) relative to sheet move is still present here
 */
static void resizeSheetWithMouseCursor( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aPosition,
                                        bool aErase )
{
    BASE_SCREEN*   screen = aPanel->GetScreen();
    SCH_SHEET*     sheet = dynamic_cast<SCH_SHEET*>( screen->GetCurItem() );

    if( sheet == nullptr )  // Be sure we are using the right object
        return;

    wxPoint pos = sheet->GetPosition();

    int width  = aPanel->GetParent()->GetCrossHairPosition().x - pos.x;
    int height = aPanel->GetParent()->GetCrossHairPosition().y - pos.y;

    // If the sheet doesn't have any pins, clamp the minimum size to the default values.
    width = ( width < MIN_SHEET_WIDTH ) ? MIN_SHEET_WIDTH : width;
    height = ( height < MIN_SHEET_HEIGHT ) ? MIN_SHEET_HEIGHT : height;

    if( sheet->HasPins() )
    {
        int gridSizeX = KiROUND( screen->GetGridSize().x );
        int gridSizeY = KiROUND( screen->GetGridSize().y );

        // If the sheet has pins, use the pin positions to clamp the minimum width and height.
        height = ( height < sheet->GetMinHeight() + gridSizeY ) ?
                 sheet->GetMinHeight() + gridSizeY : height;
        width = ( width < sheet->GetMinWidth() + gridSizeX ) ?
                sheet->GetMinWidth() + gridSizeX : width;
    }

    wxPoint grid = aPanel->GetParent()->GetNearestGridPosition(
                    wxPoint( pos.x + width, pos.y + height ) );
    sheet->Resize( wxSize( grid.x - pos.x, grid.y - pos.y ) );

    auto panel = static_cast<SCH_DRAW_PANEL*>( aPanel );
    auto view = panel->GetView();

    view->Hide( sheet );
    view->ClearPreview();
    view->AddToPreview( sheet->Clone() );
}


//  Complete sheet move.
static void ExitSheet( EDA_DRAW_PANEL* aPanel, wxDC* aDC )
{
    SCH_SCREEN* screen = (SCH_SCREEN*) aPanel->GetScreen();
    SCH_ITEM*   item = screen->GetCurItem();

    SCH_EDIT_FRAME* parent = (SCH_EDIT_FRAME*) aPanel->GetParent();

    if( (item == NULL) || (item->Type() != SCH_SHEET_T) || (parent == NULL) )
        return;

    parent->SetRepeatItem( NULL );

    if( item->IsNew() )
    {
        delete item;
    }
    else if( item->IsMoving() || item->IsResized() )
    {
        parent->RemoveFromScreen( item );
        delete item;

        item = parent->GetUndoItem();

        wxCHECK_RET( item != NULL, wxT( "Cannot restore undefined last sheet item." ) );

        parent->AddToScreen( item );

        // the owner of item is no more parent, this is the draw list of screen:
        parent->SetUndoItem( NULL );

        item->ClearFlags();
    }
    else
    {
        item->ClearFlags();
    }

    auto panel = static_cast<SCH_DRAW_PANEL*>( aPanel );
    auto view = panel->GetView();
    view->ClearPreview();

    screen->SetCurItem( NULL );
}


// Create hierarchy sheet.
SCH_SHEET* SCH_EDIT_FRAME::CreateSheet( wxDC* aDC )
{
    SetRepeatItem( NULL );

    SCH_SHEET* sheet = new SCH_SHEET( GetCrossHairPosition() );

    sheet->SetFlags( IS_NEW | IS_RESIZED );
    sheet->SetTimeStamp( GetNewTimeStamp() );
    sheet->SetParent( GetScreen() );
    sheet->SetScreen( NULL );

    // need to check if this is being added to the GetDrawItems().
    // also need to update the hierarchy, if we are adding
    // a sheet to a screen that already has multiple instances (!)
    GetScreen()->SetCurItem( sheet );
    m_canvas->SetMouseCapture( resizeSheetWithMouseCursor, ExitSheet );
    m_canvas->CallMouseCapture( aDC, wxDefaultPosition, false );
    m_canvas->CrossHairOff( aDC );

    SetCrossHairPosition( sheet->GetResizePosition() );

    m_canvas->MoveCursorToCrossHair();
    m_canvas->CrossHairOn( aDC );

    return sheet;
}


void SCH_EDIT_FRAME::ReSizeSheet( SCH_SHEET* aSheet, wxDC* aDC )
{
    if( aSheet == NULL || aSheet->IsNew() )
        return;

    wxCHECK_RET( aSheet->Type() == SCH_SHEET_T,
                 wxString::Format( wxT( "Cannot perform sheet resize on %s object." ),
                                   GetChars( aSheet->GetClass() ) ) );

    m_canvas->CrossHairOff( aDC );
    SetCrossHairPosition( aSheet->GetResizePosition() );
    m_canvas->MoveCursorToCrossHair();
    m_canvas->CrossHairOn( aDC );

    SetUndoItem( aSheet );
    aSheet->SetFlags( IS_RESIZED );

    std::vector<DANGLING_END_ITEM> emptySet;
    aSheet->UpdateDanglingState( emptySet );

    m_canvas->SetMouseCapture( resizeSheetWithMouseCursor, ExitSheet );
    m_canvas->CallMouseCapture( aDC, wxDefaultPosition, true );

    if( aSheet->IsNew() )    // not already in edit, save a copy for undo/redo
        SetUndoItem( aSheet );
}


void SCH_EDIT_FRAME::RotateHierarchicalSheet( SCH_SHEET* aSheet, bool aRotCCW )
{
    if( aSheet == NULL )
        return;

    // Save old sheet in undo list if not already in edit, or moving.
    if( aSheet->GetFlags() == 0 )
        SaveCopyInUndoList( aSheet, UR_CHANGED );

    // Rotate the sheet on itself. Sheets do not have a anchor point.
    // Rotation is made around it center
    wxPoint rotPoint = aSheet->GetBoundingBox().Centre();

    // Keep this rotation point on the grid, otherwise all items of this sheet
    // will be moved off grid
    rotPoint = GetNearestGridPosition( rotPoint );

    // rotate CCW, or CW. to rotate CW, rotate 3 times
    aSheet->Rotate( rotPoint );

    if( !aRotCCW )
    {
        aSheet->Rotate( rotPoint );
        aSheet->Rotate( rotPoint );
    }

    GetCanvas()->GetView()->Update( aSheet );
    OnModify();
}


void SCH_EDIT_FRAME::MirrorSheet( SCH_SHEET* aSheet, bool aFromXaxis )
{
    if( aSheet == NULL )
        return;

    // Save old sheet in undo list if not already in edit, or moving.
    if( aSheet->GetFlags() == 0 )
        SaveCopyInUndoList( aSheet, UR_CHANGED );

    // Mirror the sheet on itself. Sheets do not have a anchor point.
    // Mirroring is made around it center
    wxPoint mirrorPoint = aSheet->GetBoundingBox().Centre();

    if( aFromXaxis )    // mirror relative to Horizontal axis
        aSheet->MirrorX( mirrorPoint.y );
    else                // Mirror relative to vertical axis
        aSheet->MirrorY( mirrorPoint.x );

    GetCanvas()->GetView()->Update( aSheet );
    OnModify();
}
