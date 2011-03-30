/////////////////////////////////////////////////////////////////////////////
// Name:        sheet.cpp
// Purpose:
// Author:      jean-pierre Charras
// Modified by: Wayne Stambaugh
// Created:     08/02/2006 18:37:02
// RCS-ID:
// Copyright:   License GNU
// License:
/////////////////////////////////////////////////////////////////////////////

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "macros.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "gestfich.h"
#include "wxEeschemaStruct.h"
#include "class_sch_screen.h"

#include "general.h"
#include "protos.h"
#include "sch_sheet.h"

#include "dialogs/dialog_sch_sheet_props.h"

#include <boost/foreach.hpp>


static int     s_PreviousSheetWidth;
static int     s_PreviousSheetHeight;
static wxPoint s_OldPos;  /* Former position for cancellation or move ReSize */


/**
 * Function EditSheet
 * is used to edit an existing sheet or add a new sheet to the schematic.
 * <p>
 * When \a aSheet is a new sheet:
 * <ul>
 * <li>and the file name already exists in the schematic hierarchy, the screen associated with
 * the sheet found in the hierarchy is associated with \a aSheet.</li>
 * <li>and the file name already exists on the system, then \a aSheet is loaded with the
 * existing file.</li>
 * <li>and the file name does not exist in the schematic hierarchy or on the file system, then
 * a new screen is created and associated with \a aSheet.</li>
 * </ul> </p> <p>
 * When \a aSheet is an existing sheet:
 * <ul>
 * <li>and the file name already exists in the schematic hierarchy, the current associated screen
 * is replace by the one found in the hierarchy.</li>
 * <li>and the file name already exists on the system, the current associated screen file name
 * is changed and the file is loaded.</li>
 * <li>and the file name does not exist in the schematic hierarchy or on the file system, the
 * current associated screen file name is changed and saved to disk.</li>
 * </ul> </p>
 */
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
        aSheet->m_SheetName.Printf( wxT( "Sheet%8.8lX" ), GetTimeStamp() );

    DrawPanel->MoveCursorToCrossHair();
    DrawPanel->m_IgnoreMouseEvents = false;
    aSheet->Draw( DrawPanel, aDC, wxPoint( 0, 0 ), GR_DEFAULT_DRAWMODE );

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

    if( sheet->m_Flags & IS_RESIZED )
    {
        wxSize newSize( MAX( s_PreviousSheetWidth, screen->GetCrossHairPosition().x - sheet->m_Pos.x ),
                        MAX( s_PreviousSheetHeight, screen->GetCrossHairPosition().y - sheet->m_Pos.y ) );
        sheet->Resize( newSize );
    }
    else             /* Move Sheet */
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
    SCH_SHEET*  sheet  = (SCH_SHEET*) screen->GetCurItem();

    if( sheet == NULL )
        return;

    if( sheet->IsNew() )
    {
        sheet->Draw( aPanel, aDC, wxPoint( 0, 0 ), g_XorMode );
        SAFE_DELETE( sheet );
    }
    else if( (sheet->m_Flags & (IS_RESIZED|IS_MOVED)) )
    {
        wxPoint curspos = screen->GetCrossHairPosition();
        aPanel->GetScreen()->SetCrossHairPosition( s_OldPos );
        MoveOrResizeSheet( aPanel, aDC, wxDefaultPosition, true );
        sheet->Draw( aPanel, aDC, wxPoint( 0, 0 ), GR_DEFAULT_DRAWMODE );
        sheet->m_Flags = 0;
        screen->SetCrossHairPosition( curspos );
    }
    else
    {
        sheet->m_Flags = 0;
    }

    screen->SetCurItem( NULL );
    SAFE_DELETE( g_ItemToUndoCopy );
}


#define SHEET_MIN_WIDTH  500
#define SHEET_MIN_HEIGHT 150


/* Create hierarchy sheet.  */
SCH_SHEET* SCH_EDIT_FRAME::CreateSheet( wxDC* aDC )
{
    m_itemToRepeat = NULL;
    SAFE_DELETE( g_ItemToUndoCopy );

    SCH_SHEET* sheet = new SCH_SHEET( GetScreen()->GetCrossHairPosition() );

    sheet->m_Flags     = IS_NEW | IS_RESIZED;
    sheet->m_TimeStamp = GetTimeStamp();
    sheet->SetParent( GetScreen() );
    sheet->SetScreen( NULL );
    s_PreviousSheetWidth = SHEET_MIN_WIDTH;
    s_PreviousSheetHeight = SHEET_MIN_HEIGHT;

    // need to check if this is being added to the GetDrawItems().
    // also need to update the hierarchy, if we are adding
    // a sheet to a screen that already has multiple instances (!)
    GetScreen()->SetCurItem( sheet );
    DrawPanel->SetMouseCapture( MoveOrResizeSheet, ExitSheet );
    DrawPanel->m_mouseCaptureCallback( DrawPanel, aDC, wxDefaultPosition, false );

    return sheet;
}


void SCH_EDIT_FRAME::ReSizeSheet( SCH_SHEET* aSheet, wxDC* aDC )
{
    if( aSheet == NULL || aSheet->IsNew() )
        return;

    if( aSheet->Type() != SCH_SHEET_T )
    {
        DisplayError( this, wxT( "SCH_EDIT_FRAME::ReSizeSheet: Bad SructType" ) );
        return;
    }

    OnModify( );
    aSheet->m_Flags |= IS_RESIZED;

    s_OldPos = aSheet->m_Pos + aSheet->m_Size;

    s_PreviousSheetWidth = SHEET_MIN_WIDTH;
    s_PreviousSheetHeight = SHEET_MIN_HEIGHT;

    BOOST_FOREACH( SCH_SHEET_PIN sheetPin, aSheet->GetPins() )
    {
        s_PreviousSheetWidth = MAX( s_PreviousSheetWidth,
                                    ( sheetPin.GetLength() + 1 ) * sheetPin.m_Size.x );
        s_PreviousSheetHeight = MAX( s_PreviousSheetHeight,
                                     sheetPin.m_Pos.y - aSheet->m_Pos.y );
    }

    DrawPanel->SetMouseCapture( MoveOrResizeSheet, ExitSheet );
    DrawPanel->m_mouseCaptureCallback( DrawPanel, aDC, wxDefaultPosition, true );

    if( aSheet->IsNew() )    // not already in edit, save a copy for undo/redo
    {
        delete g_ItemToUndoCopy;
        g_ItemToUndoCopy = DuplicateStruct( aSheet, true );
    }
}


void SCH_EDIT_FRAME::StartMoveSheet( SCH_SHEET* aSheet, wxDC* aDC )
{
    if( ( aSheet == NULL ) || ( aSheet->Type() != SCH_SHEET_T ) )
        return;

    DrawPanel->CrossHairOff( aDC );
    GetScreen()->SetCrossHairPosition( aSheet->m_Pos );
    DrawPanel->MoveCursorToCrossHair();

    s_OldPos = aSheet->m_Pos;
    aSheet->m_Flags |= IS_MOVED;
    DrawPanel->SetMouseCapture( MoveOrResizeSheet, ExitSheet );
    DrawPanel->m_mouseCaptureCallback( DrawPanel, aDC, wxDefaultPosition, true );
    DrawPanel->CrossHairOn( aDC );

    if( !aSheet->IsNew() )    // not already in edit, save a copy for undo/redo
    {
        delete g_ItemToUndoCopy;
        g_ItemToUndoCopy = DuplicateStruct( aSheet, true );
    }
}
