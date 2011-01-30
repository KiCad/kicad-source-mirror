/**********************************************************/
/*	sheetlab.cpp  create and edit the SCH_SHEET_PIN items */
/**********************************************************/

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "macros.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "class_sch_screen.h"
#include "wxEeschemaStruct.h"

#include "general.h"
#include "protos.h"
#include "sch_sheet.h"
#include "dialog_helpers.h"

#include "dialogs/dialog_sch_edit_sheet_pin.h"


static void ExitPinSheet( EDA_DRAW_PANEL* Panel, wxDC* DC );
static void Move_PinSheet( EDA_DRAW_PANEL* panel, wxDC* DC, bool erase );


static int     s_CurrentTypeLabel = NET_INPUT;
static wxSize  NetSheetTextSize( DEFAULT_SIZE_TEXT, DEFAULT_SIZE_TEXT );
static wxPoint s_InitialPosition;  // remember the initial value of the pin label when moving it
static int     s_InitialEdge;


/* Called when aborting a move pinsheet label
 * delete a new pin sheet label, or restire its old position
 */
static void ExitPinSheet( EDA_DRAW_PANEL* Panel, wxDC* DC )
{
    SCH_SHEET_PIN* SheetLabel = (SCH_SHEET_PIN*) Panel->GetScreen()->GetCurItem();

    if( SheetLabel == NULL )
        return;

    if( SheetLabel->m_Flags & IS_NEW )
    {
        SheetLabel->Draw( Panel, DC, wxPoint( 0, 0 ), g_XorMode );
        SAFE_DELETE( SheetLabel );
    }
    else
    {
        SheetLabel->Draw( Panel, DC, wxPoint( 0, 0 ), g_XorMode );
        SheetLabel->m_Pos = s_InitialPosition;

        // Restore edge position:
        SheetLabel->SetEdge( s_InitialEdge );
        SheetLabel->Draw( Panel, DC, wxPoint( 0, 0 ), GR_DEFAULT_DRAWMODE );
        SheetLabel->m_Flags = 0;
    }

    Panel->GetScreen()->SetCurItem( NULL );
    Panel->ManageCurseur = NULL;
    Panel->ForceCloseManageCurseur = NULL;
}


void SCH_SHEET_PIN::Place( SCH_EDIT_FRAME* frame, wxDC* DC )
{
    SCH_SHEET* Sheet = (SCH_SHEET*) GetParent();

    wxASSERT( Sheet != NULL && Sheet->Type() == SCH_SHEET_T );
    SAFE_DELETE( g_ItemToUndoCopy );

    int flags = m_Flags;
    m_Flags = 0;

    if( flags & IS_NEW )
    {
        frame->SaveCopyInUndoList( Sheet, UR_CHANGED );
        Sheet->AddLabel( this );
    }
    else    // pin sheet was existing and only moved
    {
        wxPoint tmp = m_Pos;
        m_Pos = s_InitialPosition;
        SetEdge( s_InitialEdge );
        frame->SaveCopyInUndoList( Sheet, UR_CHANGED );
        m_Pos = tmp;
    }

    ConstraintOnEdge( frame->GetScreen()->m_Curseur );

    Sheet->Draw( frame->DrawPanel, DC, wxPoint( 0, 0 ), GR_DEFAULT_DRAWMODE );
    frame->DrawPanel->ManageCurseur = NULL;
    frame->DrawPanel->ForceCloseManageCurseur = NULL;
}


void SCH_EDIT_FRAME::StartMove_PinSheet( SCH_SHEET_PIN* SheetLabel, wxDC* DC )
{
    NetSheetTextSize     = SheetLabel->m_Size;
    s_CurrentTypeLabel   = SheetLabel->m_Shape;
    SheetLabel->m_Flags |= IS_MOVED;
    s_InitialPosition    = SheetLabel->m_Pos;
    s_InitialEdge = SheetLabel->GetEdge();

    DrawPanel->ManageCurseur = Move_PinSheet;
    DrawPanel->ForceCloseManageCurseur = ExitPinSheet;
    DrawPanel->ManageCurseur( DrawPanel, DC, TRUE );
}


static void Move_PinSheet( EDA_DRAW_PANEL* panel, wxDC* DC, bool erase )
{
    SCH_SHEET_PIN* SheetLabel = (SCH_SHEET_PIN*) panel->GetScreen()->GetCurItem();

    if( SheetLabel == NULL )
        return;

    if( erase )
        SheetLabel->Draw( panel, DC, wxPoint( 0, 0 ), g_XorMode );

    SheetLabel->ConstraintOnEdge( panel->GetScreen()->m_Curseur );

    SheetLabel->Draw( panel, DC, wxPoint( 0, 0 ), g_XorMode );
}


int SCH_EDIT_FRAME::Edit_PinSheet( SCH_SHEET_PIN* aLabel, wxDC* aDC )
{
    if( aLabel == NULL )
        return wxID_CANCEL;

    if( aDC )
        aLabel->Draw( DrawPanel, aDC, wxPoint( 0, 0 ), g_XorMode );

    DIALOG_SCH_EDIT_SHEET_PIN dlg( this );

    dlg.SetLabelName( aLabel->m_Text );
    dlg.SetTextHeight( ReturnStringFromValue( g_UserUnit, aLabel->m_Size.y, m_InternalUnits ) );
    dlg.SetTextHeightUnits( GetUnitsLabel( g_UserUnit ) );
    dlg.SetTextWidth( ReturnStringFromValue( g_UserUnit, aLabel->m_Size.x, m_InternalUnits ) );
    dlg.SetTextWidthUnits( GetUnitsLabel( g_UserUnit ) );
    dlg.SetConnectionType( aLabel->m_Shape );

    /* This ugly hack fixes a bug in wxWidgets 2.8.7 and likely earlier versions for
     * the flex grid sizer in wxGTK that prevents the last column from being sized
     * correctly.  It doesn't cause any problems on win32 so it doesn't need to wrapped
     * in ugly #ifdef __WXGTK__ #endif.
     */
    dlg.Layout();
    dlg.Fit();
    dlg.SetMinSize( dlg.GetSize() );

    if( dlg.ShowModal() == wxID_CANCEL )
        return wxID_CANCEL;

    aLabel->m_Text = dlg.GetLabelName();
    aLabel->m_Size.y = ReturnValueFromString( g_UserUnit, dlg.GetTextHeight(), m_InternalUnits );
    aLabel->m_Size.x = ReturnValueFromString( g_UserUnit, dlg.GetTextWidth(), m_InternalUnits );
    aLabel->m_Shape = dlg.GetConnectionType();

    if( aDC )
        aLabel->Draw( DrawPanel, aDC, wxPoint( 0, 0 ), GR_DEFAULT_DRAWMODE );

    return wxID_OK;
}


/* Add a new sheet pin to the sheet at the current cursor position.
 */
SCH_SHEET_PIN* SCH_EDIT_FRAME::Create_PinSheet( SCH_SHEET* Sheet, wxDC* DC )
{
    wxString       Line, Text;
    SCH_SHEET_PIN* NewSheetLabel;

    NewSheetLabel = new SCH_SHEET_PIN( Sheet, wxPoint( 0, 0 ), Line );
    NewSheetLabel->m_Flags = IS_NEW;
    NewSheetLabel->m_Size  = NetSheetTextSize;
    NewSheetLabel->m_Shape = s_CurrentTypeLabel;

    int diag = Edit_PinSheet( NewSheetLabel, NULL );

    if( NewSheetLabel->m_Text.IsEmpty() || (diag == wxID_CANCEL) )
    {
        delete NewSheetLabel;
        return NULL;
    }

    GetScreen()->SetCurItem( NewSheetLabel );
    s_CurrentTypeLabel = NewSheetLabel->m_Shape;

    DrawPanel->ManageCurseur = Move_PinSheet;
    DrawPanel->ForceCloseManageCurseur = ExitPinSheet;
    DrawPanel->ManageCurseur( DrawPanel, DC, TRUE );

    OnModify();
    return NewSheetLabel;
}


/* Automatically create a sheet labels from global labels for each node in
 * the corresponding hierarchy.
 */
SCH_SHEET_PIN* SCH_EDIT_FRAME::Import_PinSheet( SCH_SHEET* Sheet, wxDC* DC )
{
    EDA_ITEM*      DrawStruct;
    SCH_SHEET_PIN* NewSheetLabel;
    SCH_HIERLABEL* HLabel = NULL;

    if( !Sheet->GetScreen() )
        return NULL;

    DrawStruct = Sheet->GetScreen()->GetDrawItems();
    HLabel     = NULL;

    for( ; DrawStruct != NULL; DrawStruct = DrawStruct->Next() )
    {
        if( DrawStruct->Type() != SCH_HIERARCHICAL_LABEL_T )
            continue;

        HLabel = (SCH_HIERLABEL*) DrawStruct;

        /* A global label has been found: check if there a corresponding sheet label. */
        if( !Sheet->HasLabel( HLabel->m_Text ) )
            break;

        HLabel = NULL;
    }

    if( HLabel == NULL )
    {
        DisplayInfoMessage( this, _( "No new hierarchical labels found" ) );
        return NULL;
    }

    OnModify();
    SAFE_DELETE( g_ItemToUndoCopy );
    SaveCopyInUndoList( Sheet, UR_CHANGED );

    NewSheetLabel = new SCH_SHEET_PIN( Sheet, wxPoint( 0, 0 ), HLabel->m_Text );
    NewSheetLabel->m_Flags = IS_NEW;
    NewSheetLabel->m_Size  = NetSheetTextSize;
    s_CurrentTypeLabel     = NewSheetLabel->m_Shape = HLabel->m_Shape;

    GetScreen()->SetCurItem( NewSheetLabel );
    DrawPanel->ManageCurseur = Move_PinSheet;
    DrawPanel->ForceCloseManageCurseur = ExitPinSheet;
    Move_PinSheet( DrawPanel, DC, FALSE );

    return NewSheetLabel;
}


/*
 * Remove sheet label.
 *
 * This sheet label can not be put in a pile "undelete" because it would not
 * Possible to link it back it's 'SCH_SHEET' parent.
 */
void SCH_EDIT_FRAME::DeleteSheetLabel( bool aRedraw, SCH_SHEET_PIN* aSheetLabelToDel )
{
    SCH_SHEET* parent = (SCH_SHEET*) aSheetLabelToDel->GetParent();

    wxASSERT( parent );
    wxASSERT( parent->Type() == SCH_SHEET_T );

#if 0 && defined(DEBUG)
    std::cout << "\n\nbefore deleting:\n" << std::flush;
    parent->Show( 0, std::cout );
    std::cout << "\n\n\n" << std::flush;
#endif

    parent->RemoveLabel( aSheetLabelToDel );

    if( aRedraw )
        DrawPanel->RefreshDrawingRect( parent->GetBoundingBox() );


#if 0 && defined(DEBUG)
    std::cout << "\n\nafter deleting:\n" << std::flush;
    parent->Show( 0, std::cout );
    std::cout << "~after deleting\n\n" << std::flush;
#endif
}
