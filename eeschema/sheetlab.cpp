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


int SCH_EDIT_FRAME::m_lastSheetPinType = NET_INPUT;
wxSize SCH_EDIT_FRAME::m_lastSheetPinTextSize( DEFAULT_SIZE_TEXT, DEFAULT_SIZE_TEXT );
wxPoint SCH_EDIT_FRAME::m_lastSheetPinPosition;
int SCH_EDIT_FRAME::m_lastSheetPinEdge;


/* Called when aborting a move pinsheet label
 * delete a new pin sheet label, or restire its old position
 */
static void abortSheetPinMove( EDA_DRAW_PANEL* aPanel, wxDC* aDC )
{
    wxCHECK_RET( (aPanel != NULL) && (aDC != NULL), wxT( "Invalid panel or device context." ) );

    SCH_EDIT_FRAME* frame = (SCH_EDIT_FRAME*) aPanel->GetParent();
    SCH_SHEET_PIN* sheetPin = (SCH_SHEET_PIN*) aPanel->GetScreen()->GetCurItem();

    wxCHECK_RET( (frame != NULL) && (sheetPin != NULL) && (sheetPin->Type() == SCH_SHEET_PIN_T),
                 wxT( "Invalid frame or sheet pin." ) );

    sheetPin->Draw( aPanel, aDC, wxPoint( 0, 0 ), g_XorMode );

    if( sheetPin->IsNew() )
    {
        SAFE_DELETE( sheetPin );
    }
    else
    {
        // Restore edge position:
        sheetPin->m_Pos = frame->GetLastSheetPinPosition();
        sheetPin->SetEdge( frame->GetLastSheetPinEdge() );
        sheetPin->Draw( aPanel, aDC, wxPoint( 0, 0 ), GR_DEFAULT_DRAWMODE );
        sheetPin->ClearFlags();
    }

    aPanel->GetScreen()->SetCurItem( NULL );
}


static void moveSheetPin( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aPosition,
                          bool aErase )
{
    SCH_SHEET_PIN* sheetPin = (SCH_SHEET_PIN*) aPanel->GetScreen()->GetCurItem();

    if( sheetPin == NULL || sheetPin->Type() != SCH_SHEET_PIN_T )
        return;

    if( aErase )
        sheetPin->Draw( aPanel, aDC, wxPoint( 0, 0 ), g_XorMode );

    sheetPin->ConstraintOnEdge( aPanel->GetScreen()->GetCrossHairPosition() );
    sheetPin->Draw( aPanel, aDC, wxPoint( 0, 0 ), g_XorMode );
}


void SCH_EDIT_FRAME::MoveSheetPin( SCH_SHEET_PIN* aSheetPin, wxDC* aDC )
{
    wxCHECK_RET( (aSheetPin != NULL) && (aSheetPin->Type() == SCH_SHEET_PIN_T),
                 wxT( "Cannot move invalid schematic sheet pin object." ) );

    m_lastSheetPinTextSize = aSheetPin->m_Size;
    m_lastSheetPinType     = aSheetPin->m_Shape;
    m_lastSheetPinPosition = aSheetPin->m_Pos;
    m_lastSheetPinEdge     = aSheetPin->GetEdge();
    aSheetPin->SetFlags( IS_MOVED );

    DrawPanel->SetMouseCapture( moveSheetPin, abortSheetPinMove );
    moveSheetPin( DrawPanel, aDC, wxDefaultPosition, true );
}


int SCH_EDIT_FRAME::EditSheetPin( SCH_SHEET_PIN* aSheetPin, wxDC* aDC )
{
    if( aSheetPin == NULL )
        return wxID_CANCEL;

    DIALOG_SCH_EDIT_SHEET_PIN dlg( this );

    dlg.SetLabelName( aSheetPin->m_Text );
    dlg.SetTextHeight( ReturnStringFromValue( g_UserUnit, aSheetPin->m_Size.y, m_InternalUnits ) );
    dlg.SetTextHeightUnits( GetUnitsLabel( g_UserUnit ) );
    dlg.SetTextWidth( ReturnStringFromValue( g_UserUnit, aSheetPin->m_Size.x, m_InternalUnits ) );
    dlg.SetTextWidthUnits( GetUnitsLabel( g_UserUnit ) );
    dlg.SetConnectionType( aSheetPin->m_Shape );

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

    if( aDC )
        aSheetPin->Draw( DrawPanel, aDC, wxPoint( 0, 0 ), g_XorMode );

    aSheetPin->m_Text = dlg.GetLabelName();
    aSheetPin->m_Size.y = ReturnValueFromString( g_UserUnit, dlg.GetTextHeight(), m_InternalUnits );
    aSheetPin->m_Size.x = ReturnValueFromString( g_UserUnit, dlg.GetTextWidth(), m_InternalUnits );
    aSheetPin->m_Shape = dlg.GetConnectionType();

    if( aDC )
        aSheetPin->Draw( DrawPanel, aDC, wxPoint( 0, 0 ), GR_DEFAULT_DRAWMODE );

    return wxID_OK;
}


SCH_SHEET_PIN* SCH_EDIT_FRAME::CreateSheetPin( SCH_SHEET* aSheet, wxDC* aDC )
{
    wxString       line;
    SCH_SHEET_PIN* sheetPin;

    sheetPin = new SCH_SHEET_PIN( aSheet, wxPoint( 0, 0 ), line );
    sheetPin->SetFlags( IS_NEW );
    sheetPin->m_Size  = m_lastSheetPinTextSize;
    sheetPin->m_Shape = m_lastSheetPinType;

    int response = EditSheetPin( sheetPin, NULL );

    if( sheetPin->m_Text.IsEmpty() || (response == wxID_CANCEL) )
    {
        delete sheetPin;
        return NULL;
    }

    GetScreen()->SetCurItem( sheetPin );
    m_lastSheetPinType = sheetPin->m_Shape;
    m_lastSheetPinTextSize = sheetPin->m_Size;

    DrawPanel->SetMouseCapture( moveSheetPin, abortSheetPinMove );
    moveSheetPin( DrawPanel, aDC, wxDefaultPosition, false );

    OnModify();
    return sheetPin;
}


SCH_SHEET_PIN* SCH_EDIT_FRAME::ImportSheetPin( SCH_SHEET* aSheet, wxDC* aDC )
{
    EDA_ITEM*      item;
    SCH_SHEET_PIN* sheetPin;
    SCH_HIERLABEL* label = NULL;

    if( !aSheet->GetScreen() )
        return NULL;

    item = aSheet->GetScreen()->GetDrawItems();

    for( ; item != NULL; item = item->Next() )
    {
        if( item->Type() != SCH_HIERARCHICAL_LABEL_T )
            continue;

        label = (SCH_HIERLABEL*) item;

        /* A global label has been found: check if there a corresponding sheet label. */
        if( !aSheet->HasPin( label->m_Text ) )
            break;

        label = NULL;
    }

    if( label == NULL )
    {
        DisplayInfoMessage( this, _( "No new hierarchical labels found." ) );
        return NULL;
    }

    OnModify();
    SaveCopyInUndoList( aSheet, UR_CHANGED );

    sheetPin = new SCH_SHEET_PIN( aSheet, wxPoint( 0, 0 ), label->m_Text );
    sheetPin->SetFlags( IS_NEW );
    sheetPin->m_Size   = m_lastSheetPinTextSize;
    m_lastSheetPinType = sheetPin->m_Shape = label->m_Shape;

    GetScreen()->SetCurItem( sheetPin );
    DrawPanel->SetMouseCapture( moveSheetPin, abortSheetPinMove );
    moveSheetPin( DrawPanel, aDC, wxDefaultPosition, false );

    return sheetPin;
}
