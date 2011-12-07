/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2006 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2009-2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 2004-2011 KiCad Developers, see change_log.txt for contributors.
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

#include "fctsys.h"
#include "gr_basic.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "wxEeschemaStruct.h"

#include "general.h"
#include "sch_sheet.h"

#include "dialogs/dialog_sch_sheet_props.h"


bool SCH_EDIT_FRAME::EditSheet( SCH_SHEET* aSheet, wxDC* aDC )
{
    if( aSheet == NULL )
        return false;

    /* Get the new texts */
    DIALOG_SCH_SHEET_PROPS dlg( this );

    wxString units = GetUnitsLabel( g_UserUnit );
    dlg.SetFileName( aSheet->GetFileName() );
    dlg.SetFileNameTextSize( ReturnStringFromValue( g_UserUnit,
                                                    aSheet->m_FileNameSize,
                                                    m_InternalUnits ) );
    dlg.SetFileNameTextSizeUnits( units );
    dlg.SetSheetName( aSheet->m_SheetName );
    dlg.SetSheetNameTextSize( ReturnStringFromValue( g_UserUnit,
                                                     aSheet->m_SheetNameSize,
                                                     m_InternalUnits ) );
    dlg.SetSheetNameTextSizeUnits( units );

    /* This ugly hack fixes a bug in wxWidgets 2.8.7 and likely earlier
     * versions for the flex grid sizer in wxGTK that prevents the last
     * column from being sized correctly.  It doesn't cause any problems
     * on win32 so it doesn't need to wrapped in ugly #ifdef __WXGTK__
     * #endif.
     */
    dlg.Layout();
    dlg.Fit();
    dlg.SetMinSize( dlg.GetSize() );

    if( dlg.ShowModal() == wxID_CANCEL )
        return false;

    wxFileName fileName = dlg.GetFileName();
    fileName.SetExt( SchematicFileExtension );

    if( !fileName.IsOk() )
    {
        DisplayError( this, _( "File name is not valid!" ) );
        return false;
    }

    // Duplicate sheet names are not valid.
    const SCH_SHEET* sheet = GetScreen()->GetSheet( dlg.GetSheetName() );

    if( (sheet != NULL) && (sheet != aSheet) )
    {
        DisplayError( this, wxString::Format( _( "A sheet named \"%s\" already exists." ),
                                              GetChars( dlg.GetSheetName() ) ) );
        return false;
    }

    wxString msg;
    wxString tmp;
    bool loadFromFile = false;
    SCH_SCREEN* useScreen = NULL;

    if( !g_RootSheet->SearchHierarchy( fileName.GetFullPath(), &useScreen ) )
        loadFromFile = fileName.FileExists();

    if( aSheet->GetScreen() == NULL )                          // New sheet.
    {
        if( ( useScreen != NULL ) || loadFromFile )            // Load from existing file.
        {
            msg.Printf( _( "A file named \"%s\" already exists" ),
                        GetChars( fileName.GetFullName() ) );

            if( useScreen != NULL )
                msg += _( " in the current schematic hierarchy" );

            msg += _(".\n\nDo you want to create a sheet with the contents of this file?" );

            if( !IsOK( this, msg ) )
                return false;
        }
        else                                                   // New file.
        {
            aSheet->SetScreen( new SCH_SCREEN() );
            aSheet->GetScreen()->SetFileName( fileName.GetFullPath() );
        }
    }
    else                                                       // Existing sheet.
    {
        bool isUndoable = true;
        bool renameFile = false;

        if( fileName.GetFullName().CmpNoCase( aSheet->GetFileName() ) != 0 )
        {
            // Sheet file name changes cannot be undone.
            isUndoable = false;
            msg = _( "Changing the sheet file name cannot be undone.  " );

            if( ( useScreen != NULL ) || loadFromFile )        // Load from existing file.
            {
                tmp.Printf( _( "A file named \"%s\" already exists" ),
                            GetChars( fileName.GetFullName() ) );
                msg += tmp;

                if( useScreen != NULL )
                    msg += _( " in the current schematic hierarchy" );

                msg += _(".\n\nDo you want to replace the sheet with the contents of this file?" );

                if( !IsOK( this, msg ) )
                    return false;

                if( loadFromFile )
                    aSheet->SetScreen( NULL );
            }
            else                                               // Save to new file name.
            {
                if( aSheet->GetScreenCount() > 1 )
                {
                    msg += _( "This sheet uses shared data in a complex hierarchy.\n\n" );
                    msg += _( "Do you wish to convert it to a simple hierarchical sheet?" );

                    if( !IsOK( NULL, msg ) )
                        return false;
                }

                renameFile = true;
            }
        }

        aSheet->Draw( DrawPanel, aDC, wxPoint( 0, 0 ), g_XorMode );
        DrawPanel->m_IgnoreMouseEvents = true;

        if( isUndoable )
            SaveCopyInUndoList( aSheet, UR_CHANGED );

        if( renameFile )
        {
            aSheet->GetScreen()->SetFileName( fileName.GetFullName() );
            SaveEEFile( aSheet->GetScreen(), FILE_SAVE_AS );

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

    aSheet->SetFileName( fileName.GetFullPath() );

    if( useScreen )
        aSheet->SetScreen( useScreen );
    else if( loadFromFile )
        aSheet->Load( this );

    aSheet->m_FileNameSize = ReturnValueFromString( g_UserUnit,
                                                    dlg.GetFileNameTextSize(),
                                                    m_InternalUnits );
    aSheet->m_SheetName = dlg.GetSheetName();
    aSheet->m_SheetNameSize = ReturnValueFromString( g_UserUnit,
                                                     dlg.GetSheetNameTextSize(),
                                                     m_InternalUnits );

    if( aSheet->m_SheetName.IsEmpty() )
        aSheet->m_SheetName.Printf( wxT( "Sheet%8.8lX" ), aSheet->GetTimeStamp() );

    DrawPanel->MoveCursorToCrossHair();
    DrawPanel->m_IgnoreMouseEvents = false;
    aSheet->Draw( DrawPanel, aDC, wxPoint( 0, 0 ), GR_DEFAULT_DRAWMODE );
    OnModify();

    return true;
}


/* Move selected sheet with the cursor.
 * Callback function use by m_mouseCaptureCallback.
 */
static void MoveOrResizeSheet( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aPosition,
                               bool aErase )
{
    wxPoint        moveVector;
    BASE_SCREEN*   screen = aPanel->GetScreen();
    SCH_SHEET*     sheet = (SCH_SHEET*) screen->GetCurItem();

    if( aErase )
        sheet->Draw( aPanel, aDC, wxPoint( 0, 0 ), g_XorMode );

    if( sheet->IsResized() )
    {
        int width = screen->GetCrossHairPosition().x - sheet->m_Pos.x;
        int height = screen->GetCrossHairPosition().y - sheet->m_Pos.y;

        // If the sheet doesn't have any pins, clamp the minimum size to the default values.
        width = ( width < MIN_SHEET_WIDTH ) ? MIN_SHEET_WIDTH : width;
        height = ( height < MIN_SHEET_HEIGHT ) ? MIN_SHEET_HEIGHT : height;

        if( sheet->HasPins() )
        {
            int gridSizeX = wxRound( screen->GetGridSize().x );
            int gridSizeY = wxRound( screen->GetGridSize().y );

            // If the sheet has pins, use the pin positions to clamp the minimum height.
            height = ( height < sheet->GetMinHeight() + gridSizeY ) ?
                sheet->GetMinHeight() + gridSizeY : height;
            width = ( width < sheet->GetMinWidth() + gridSizeX ) ?
                sheet->GetMinWidth() + gridSizeX : width;
        }

        wxPoint grid = screen->GetNearestGridPosition( wxPoint( sheet->m_Pos.x + width,
                                                                sheet->m_Pos.y + height ) );
        sheet->Resize( wxSize( grid.x - sheet->m_Pos.x, grid.y - sheet->m_Pos.y ) );
    }
    else if( sheet->IsMoving() )
    {
        moveVector = screen->GetCrossHairPosition() - sheet->m_Pos;
        sheet->Move( moveVector );
    }

    sheet->Draw( aPanel, aDC, wxPoint( 0, 0 ), g_XorMode );
}


/*  Complete sheet move.  */
static void ExitSheet( EDA_DRAW_PANEL* aPanel, wxDC* aDC )
{
    SCH_SCREEN* screen = (SCH_SCREEN*) aPanel->GetScreen();
    SCH_ITEM* item = screen->GetCurItem();
    SCH_EDIT_FRAME* parent = ( SCH_EDIT_FRAME* ) aPanel->GetParent();

    if( (item == NULL) || (item->Type() != SCH_SHEET_T) || (parent == NULL) )
        return;

    parent->SetRepeatItem( NULL );

    item->Draw( aPanel, aDC, wxPoint( 0, 0 ), g_XorMode );

    if( item->IsNew() )
    {
        SAFE_DELETE( item );
    }
    else if( item->IsMoving() || item->IsResized() )
    {
        screen->RemoveFromDrawList( item );
        delete item;

        item = parent->GetUndoItem();

        wxCHECK_RET( item != NULL, wxT( "Cannot restore undefined last sheet item." ) );

        screen->AddToDrawList( item );
        // the owner of item is no more parent, this is the draw list of screen:
        parent->SetUndoItem( NULL );

        item->Draw( aPanel, aDC, wxPoint( 0, 0 ), GR_DEFAULT_DRAWMODE );
        item->ClearFlags();
    }
    else
    {
        item->ClearFlags();
    }

    screen->SetCurItem( NULL );
}


/* Create hierarchy sheet.  */
SCH_SHEET* SCH_EDIT_FRAME::CreateSheet( wxDC* aDC )
{
    m_itemToRepeat = NULL;

    SCH_SHEET* sheet = new SCH_SHEET( GetScreen()->GetCrossHairPosition() );

    sheet->SetFlags( IS_NEW | IS_RESIZED );
    sheet->m_TimeStamp = GetNewTimeStamp();
    sheet->SetParent( GetScreen() );
    sheet->SetScreen( NULL );

    // need to check if this is being added to the GetDrawItems().
    // also need to update the hierarchy, if we are adding
    // a sheet to a screen that already has multiple instances (!)
    GetScreen()->SetCurItem( sheet );
    DrawPanel->SetMouseCapture( MoveOrResizeSheet, ExitSheet );
    DrawPanel->m_mouseCaptureCallback( DrawPanel, aDC, wxDefaultPosition, false );
    DrawPanel->CrossHairOff( aDC );
    GetScreen()->SetCrossHairPosition( sheet->GetResizePosition() );
    DrawPanel->MoveCursorToCrossHair();
    DrawPanel->CrossHairOn( aDC );

    return sheet;
}


void SCH_EDIT_FRAME::ReSizeSheet( SCH_SHEET* aSheet, wxDC* aDC )
{
    if( aSheet == NULL || aSheet->IsNew() )
        return;

    wxCHECK_RET( aSheet->Type() == SCH_SHEET_T,
                 wxString::Format( wxT( "Cannot perform sheet resize on %s object." ),
                                   GetChars( aSheet->GetClass() ) ) );

    DrawPanel->CrossHairOff( aDC );
    GetScreen()->SetCrossHairPosition( aSheet->GetResizePosition() );
    DrawPanel->MoveCursorToCrossHair();
    DrawPanel->CrossHairOn( aDC );

    SetUndoItem( aSheet );
    aSheet->SetFlags( IS_RESIZED );

    DrawPanel->SetMouseCapture( MoveOrResizeSheet, ExitSheet );
    DrawPanel->m_mouseCaptureCallback( DrawPanel, aDC, wxDefaultPosition, true );

    if( aSheet->IsNew() )    // not already in edit, save a copy for undo/redo
        SetUndoItem( aSheet );
}


void SCH_EDIT_FRAME::StartMoveSheet( SCH_SHEET* aSheet, wxDC* aDC )
{
    if( ( aSheet == NULL ) || ( aSheet->Type() != SCH_SHEET_T ) )
        return;

    DrawPanel->CrossHairOff( aDC );
    GetScreen()->SetCrossHairPosition( aSheet->m_Pos );
    DrawPanel->MoveCursorToCrossHair();

    if( !aSheet->IsNew() )
        SetUndoItem( aSheet );

    aSheet->SetFlags( IS_MOVED );
    DrawPanel->SetMouseCapture( MoveOrResizeSheet, ExitSheet );
    DrawPanel->m_mouseCaptureCallback( DrawPanel, aDC, wxDefaultPosition, true );
    DrawPanel->CrossHairOn( aDC );
}
