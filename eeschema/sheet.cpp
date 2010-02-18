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
#include "class_drawpanel.h"
#include "confirm.h"
#include "gestfich.h"

#include "program.h"
#include "general.h"
#include "protos.h"

#include "dialog_sch_sheet_props.h"


static int     s_PreviousSheetWidth;
static int     s_PreviousSheetHeight;
static wxPoint s_OldPos;  /* Former position for cancellation or move ReSize */


/* Routine to edit the SheetName and the FileName for the sheet "Sheet" */
bool WinEDA_SchematicFrame::EditSheet( SCH_SHEET* aSheet, wxDC* aDC )
{
    bool edit = true;

    if( aSheet == NULL )
        return false;

    /* Get the new texts */
    RedrawOneStruct( DrawPanel, aDC, aSheet, g_XorMode );

    DrawPanel->m_IgnoreMouseEvents = true;

    DIALOG_SCH_SHEET_PROPS dlg( this );

    wxString units = GetUnitsLabel( g_UnitMetric );
    dlg.SetFileName( aSheet->GetFileName() );
    dlg.SetFileNameTextSize( ReturnStringFromValue( g_UnitMetric,
                                                    aSheet->m_FileNameSize,
                                                    m_InternalUnits ) );
    dlg.SetFileNameTextSizeUnits( units );
    dlg.SetSheetName( aSheet->m_SheetName );
    dlg.SetSheetNameTextSize( ReturnStringFromValue( g_UnitMetric,
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

    if( dlg.ShowModal() == wxID_OK )
    {
        wxFileName fileName;
        wxString   msg;

        fileName = dlg.GetFileName();

        if( !fileName.IsOk() )
        {
            DisplayError( this, _( "File name is not valid! Aborted" ) );
            edit = false;
        }
        else
        {
            fileName.SetExt( SchematicFileExtension );

            /* m_CurrentSheet->m_AssociatedScreen must be a valid screen, and the
             * sheet must have a valid associated filename,
             * so we must call m_CurrentSheet->ChangeFileName to set a filename,
             * AND always when a new sheet is created ( when
             * m_CurrentSheet->m_AssociatedScreen is null ),
             * to create or set an Associated Screen
             */
            if( ( fileName.GetFullPath() != aSheet->GetFileName() )
                || ( aSheet->m_AssociatedScreen == NULL ) )
            {
                msg = _( "Changing the sheet file name can change all the schematic \
structures and cannot be undone.\nOk to continue renaming?" );

                if( aSheet->m_AssociatedScreen == NULL || IsOK( NULL, msg ) )
                {
                    // do not prompt on a new sheet. in fact, we should not allow a
                    // sheet to be created without a valid associated filename to be
                    // read from.
                    GetScreen()->ClearUndoRedoList();

                    // set filename and the associated screen
                    aSheet->ChangeFileName( this, fileName.GetFullPath() );
                }
            }

            aSheet->m_FileNameSize = ReturnValueFromString( g_UnitMetric,
                                                            dlg.GetFileNameTextSize(),
                                                            m_InternalUnits );

            aSheet->m_SheetName = dlg.GetSheetName();
            aSheet->m_SheetNameSize = ReturnValueFromString( g_UnitMetric,
                                                             dlg.GetSheetNameTextSize(),
                                                             m_InternalUnits );

            if( aSheet->m_SheetName.IsEmpty() )
            {
                aSheet->m_SheetName.Printf( wxT( "Sheet%8.8lX" ), GetTimeStamp() );
            }
        }
    }
    else
    {
        edit = false;
    }

    DrawPanel->MouseToCursorSchema();
    DrawPanel->m_IgnoreMouseEvents = false;

    RedrawOneStruct( DrawPanel, aDC, aSheet, GR_DEFAULT_DRAWMODE );

    return edit;
}


/* Move selected sheet with the cursor.
 * Callback function use by ManageCurseur.
 */
static void MoveSheet( WinEDA_DrawPanel* aPanel, wxDC* aDC, bool aErase )
{
    wxPoint        moveVector;
    SCH_SHEET_PIN* sheetLabel;
    BASE_SCREEN*   screen = aPanel->GetScreen();
    SCH_SHEET*     sheet = (SCH_SHEET*) screen->GetCurItem();

    if( aErase )
        RedrawOneStruct( aPanel, aDC, sheet, g_XorMode );

    if( sheet->m_Flags & IS_RESIZED )
    {
        sheet->m_Size.x = MAX( s_PreviousSheetWidth, screen->m_Curseur.x - sheet->m_Pos.x );
        sheet->m_Size.y = MAX( s_PreviousSheetHeight, screen->m_Curseur.y - sheet->m_Pos.y );
        sheetLabel = sheet->m_Label;

        while( sheetLabel )
        {
            if( sheetLabel->m_Edge )
                sheetLabel->m_Pos.x = sheet->m_Pos.x + sheet->m_Size.x;
            sheetLabel = sheetLabel->Next();
        }
    }
    else             /* Move Sheet */
    {
        moveVector = screen->m_Curseur - sheet->m_Pos;
        sheet->Move( moveVector );
    }

    RedrawOneStruct( aPanel, aDC, sheet, g_XorMode );
}


/*  Complete sheet move.  */
static void ExitSheet( WinEDA_DrawPanel* aPanel, wxDC* aDC )
{
    SCH_SCREEN* screen = (SCH_SCREEN*) aPanel->GetScreen();
    SCH_SHEET*  sheet  = (SCH_SHEET*) screen->GetCurItem();

    if( sheet == NULL )
        return;

    if( sheet->m_Flags & IS_NEW )
    {
        RedrawOneStruct( aPanel, aDC, sheet, g_XorMode );
        SAFE_DELETE( sheet );
    }
    else if( sheet->m_Flags & IS_RESIZED )
    {
        /* Resize in progress, cancel move. */
        RedrawOneStruct( aPanel, aDC, sheet, g_XorMode );
        sheet->m_Size.x = s_OldPos.x;
        sheet->m_Size.y = s_OldPos.y;
        RedrawOneStruct( aPanel, aDC, sheet, GR_DEFAULT_DRAWMODE );
        sheet->m_Flags = 0;
    }
    else if( sheet->m_Flags & IS_MOVED )
    {
        wxPoint curspos = screen->m_Curseur;
        aPanel->GetScreen()->m_Curseur = s_OldPos;
        MoveSheet( aPanel, aDC, true );
        RedrawOneStruct( aPanel, aDC, sheet, GR_DEFAULT_DRAWMODE );
        sheet->m_Flags = 0;
        screen->m_Curseur = curspos;
    }
    else
    {
        sheet->m_Flags = 0;
    }

    screen->SetCurItem( NULL );
    aPanel->ManageCurseur = NULL;
    aPanel->ForceCloseManageCurseur = NULL;
}


#define SHEET_MIN_WIDTH  500
#define SHEET_MIN_HEIGHT 150


/* Create hierarchy sheet.  */
SCH_SHEET* WinEDA_SchematicFrame::CreateSheet( wxDC* aDC )
{
    g_ItemToRepeat = NULL;

    SCH_SHEET* sheet = new SCH_SHEET( GetScreen()->m_Curseur );

    sheet->m_Flags     = IS_NEW | IS_RESIZED;
    sheet->m_TimeStamp = GetTimeStamp();
    sheet->SetParent( GetScreen() );
    sheet->m_AssociatedScreen = NULL;
    s_PreviousSheetWidth = SHEET_MIN_WIDTH;
    s_PreviousSheetHeight = SHEET_MIN_HEIGHT;

    // need to check if this is being added to the EEDrawList.
    // also need to update the hierarchy, if we are adding
    // a sheet to a screen that already has multiple instances (!)
    GetScreen()->SetCurItem( sheet );

    DrawPanel->ManageCurseur = MoveSheet;
    DrawPanel->ForceCloseManageCurseur = ExitSheet;

    DrawPanel->ManageCurseur( DrawPanel, aDC, false );

    return sheet;
}


void WinEDA_SchematicFrame::ReSizeSheet( SCH_SHEET* aSheet, wxDC* aDC )
{
    SCH_SHEET_PIN* sheetLabel;

    if( aSheet == NULL || aSheet->m_Flags & IS_NEW )
        return;

    if( aSheet->Type() != DRAW_SHEET_STRUCT_TYPE )
    {
        DisplayError( this,
                      wxT( "WinEDA_SchematicFrame::ReSizeSheet: Bad SructType" ) );
        return;
    }

    OnModify( );
    aSheet->m_Flags |= IS_RESIZED;

    s_OldPos.x = aSheet->m_Size.x;
    s_OldPos.y = aSheet->m_Size.y;

    s_PreviousSheetWidth = SHEET_MIN_WIDTH;
    s_PreviousSheetHeight = SHEET_MIN_HEIGHT;
    sheetLabel   = aSheet->m_Label;

    while( sheetLabel )
    {
        s_PreviousSheetWidth = MAX( s_PreviousSheetWidth,
                                    (int) ( ( sheetLabel->GetLength() + 1 ) *
                                            sheetLabel->m_Size.x ) );
        s_PreviousSheetHeight = MAX( s_PreviousSheetHeight,
                                     sheetLabel->m_Pos.y - aSheet->m_Pos.y );
        sheetLabel   = sheetLabel->Next();
    }

    DrawPanel->ManageCurseur = MoveSheet;
    DrawPanel->ForceCloseManageCurseur = ExitSheet;
    DrawPanel->ManageCurseur( DrawPanel, aDC, true );
}


void WinEDA_SchematicFrame::StartMoveSheet( SCH_SHEET* aSheet, wxDC* aDC )
{
    if( ( aSheet == NULL ) || ( aSheet->Type() != DRAW_SHEET_STRUCT_TYPE ) )
        return;

    DrawPanel->CursorOff( aDC );
    GetScreen()->m_Curseur = aSheet->m_Pos;
    DrawPanel->MouseToCursorSchema();

    s_OldPos = aSheet->m_Pos;
    aSheet->m_Flags |= IS_MOVED;
    DrawPanel->ManageCurseur = MoveSheet;
    DrawPanel->ForceCloseManageCurseur = ExitSheet;
    DrawPanel->ManageCurseur( DrawPanel, aDC, true );
    DrawPanel->CursorOn( aDC );
}
