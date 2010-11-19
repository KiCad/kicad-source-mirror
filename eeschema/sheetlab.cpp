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


static void ExitPinSheet( WinEDA_DrawPanel* Panel, wxDC* DC );
static void Move_PinSheet( WinEDA_DrawPanel* panel, wxDC* DC, bool erase );


static int     s_CurrentTypeLabel = NET_INPUT;
static wxSize  NetSheetTextSize( DEFAULT_SIZE_TEXT, DEFAULT_SIZE_TEXT );
static wxPoint s_InitialPosition;  // remember the initial value of the pin label when moving it
static int     s_InitialEdge;

/****************************************/
/* class WinEDA_PinSheetPropertiesFrame */
/****************************************/


class WinEDA_PinSheetPropertiesFrame : public wxDialog
{
private:

    WinEDA_SchematicFrame*  m_Parent;
    SCH_SHEET_PIN*          m_CurrentPinSheet;
    wxRadioBox*             m_PinSheetType;
    wxRadioBox*             m_PinSheetShape;
    WinEDA_GraphicTextCtrl* m_TextWin;

public: WinEDA_PinSheetPropertiesFrame( WinEDA_SchematicFrame* parent,
                                       SCH_SHEET_PIN* curr_pinsheet,
                                       const wxPoint& framepos = wxPoint( -1, -1 ) );
    ~WinEDA_PinSheetPropertiesFrame() { };

private:
    void OnOkClick( wxCommandEvent& event );
    void OnCancelClick( wxCommandEvent& event );

    DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE( WinEDA_PinSheetPropertiesFrame, wxDialog )
EVT_BUTTON( wxID_OK, WinEDA_PinSheetPropertiesFrame::OnOkClick )
EVT_BUTTON( wxID_CANCEL, WinEDA_PinSheetPropertiesFrame::OnCancelClick )
END_EVENT_TABLE()


WinEDA_PinSheetPropertiesFrame::WinEDA_PinSheetPropertiesFrame(
    WinEDA_SchematicFrame* parent,
    SCH_SHEET_PIN*         curr_pinsheet,
    const wxPoint&         framepos ) :
    wxDialog( parent, -1, _( "PinSheet Properties:" ), framepos,
              wxSize( 340, 220 ), DIALOG_STYLE )
{
    wxPoint   pos;
    wxString  number;
    wxButton* Button;

    m_Parent = parent;

    wxBoxSizer* MainBoxSizer = new wxBoxSizer( wxHORIZONTAL );
    SetSizer( MainBoxSizer );
    wxBoxSizer* LeftBoxSizer  = new wxBoxSizer( wxVERTICAL );
    wxBoxSizer* RightBoxSizer = new wxBoxSizer( wxVERTICAL );
    MainBoxSizer->Add( LeftBoxSizer, 0, wxGROW | wxALL, 5 );
    MainBoxSizer->Add( RightBoxSizer, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5 );

    m_CurrentPinSheet = curr_pinsheet;

    /* Create buttons: */
    Button = new wxButton( this, wxID_OK, _( "OK" ) );
    RightBoxSizer->Add( Button, 0, wxGROW | wxALL, 5 );

    Button = new wxButton( this, wxID_CANCEL, _( "Cancel" ) );
    RightBoxSizer->Add( Button, 0, wxGROW | wxALL, 5 );

    m_TextWin = new WinEDA_GraphicTextCtrl( this, _( "Text:" ),
                                            m_CurrentPinSheet->m_Text,
                                            m_CurrentPinSheet->m_Size.x,
                                            g_UserUnit, LeftBoxSizer, 200 );

    // Display shape selection :
    #define NBSHAPES 5
    wxString shape_list[NBSHAPES] =
    {
        _( "Input" ), _( "Output" ), _( "Bidi" ), _( "TriState" ),
        _( "Passive" )
    };
    m_PinSheetShape = new wxRadioBox( this, -1, _( "PinSheet Shape:" ),
                                      wxDefaultPosition, wxSize( -1, -1 ),
                                      NBSHAPES, shape_list, 1 );
    m_PinSheetShape->SetSelection( m_CurrentPinSheet->m_Shape );
    LeftBoxSizer->Add( m_PinSheetShape, 0, wxGROW | wxALL, 5 );

    m_TextWin->SetFocus();

    GetSizer()->Fit( this );
    GetSizer()->SetSizeHints( this );
    Centre();
}


void WinEDA_PinSheetPropertiesFrame::OnCancelClick( wxCommandEvent& WXUNUSED(
                                                       event) )
{
    EndModal( wxID_CANCEL );
}


void WinEDA_PinSheetPropertiesFrame::OnOkClick( wxCommandEvent& event )
{
    m_CurrentPinSheet->m_Text   = m_TextWin->GetText();
    m_CurrentPinSheet->m_Size.x = m_CurrentPinSheet->m_Size.y = m_TextWin->GetTextSize();

    m_CurrentPinSheet->m_Shape = m_PinSheetShape->GetSelection();
    EndModal( wxID_OK );
}


/* Called when aborting a move pinsheet label
 * delete a new pin sheet label, or restire its old position
 */
static void ExitPinSheet( WinEDA_DrawPanel* Panel, wxDC* DC )
{
    SCH_SHEET_PIN* SheetLabel = (SCH_SHEET_PIN*) Panel->GetScreen()->GetCurItem();

    if( SheetLabel == NULL )
        return;

    if( SheetLabel->m_Flags & IS_NEW )
    {
        RedrawOneStruct( Panel, DC, SheetLabel, g_XorMode );
        SAFE_DELETE( SheetLabel );
    }
    else
    {
        RedrawOneStruct( Panel, DC, SheetLabel, g_XorMode );
        SheetLabel->m_Pos = s_InitialPosition;

        // Restore edge position:
        SheetLabel->SetEdge( s_InitialEdge );
        RedrawOneStruct( Panel, DC, SheetLabel, GR_DEFAULT_DRAWMODE );
        SheetLabel->m_Flags = 0;
    }

    Panel->GetScreen()->SetCurItem( NULL );
    Panel->ManageCurseur = NULL;
    Panel->ForceCloseManageCurseur = NULL;
}


void SCH_SHEET_PIN::Place( WinEDA_SchematicFrame* frame, wxDC* DC )
{
    SCH_SHEET* Sheet = (SCH_SHEET*) GetParent();

    wxASSERT( Sheet != NULL && Sheet->Type() == DRAW_SHEET_STRUCT_TYPE );
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

    RedrawOneStruct( frame->DrawPanel, DC, Sheet, GR_DEFAULT_DRAWMODE );
    frame->DrawPanel->ManageCurseur = NULL;
    frame->DrawPanel->ForceCloseManageCurseur = NULL;
}


void WinEDA_SchematicFrame::StartMove_PinSheet( SCH_SHEET_PIN* SheetLabel,
                                                wxDC*          DC )
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


static void Move_PinSheet( WinEDA_DrawPanel* panel, wxDC* DC, bool erase )
{
    SCH_SHEET_PIN* SheetLabel = (SCH_SHEET_PIN*) panel->GetScreen()->GetCurItem();

    if( SheetLabel == NULL )
        return;

    if( erase )
        RedrawOneStruct( panel, DC, SheetLabel, g_XorMode );

    SheetLabel->ConstraintOnEdge( panel->GetScreen()->m_Curseur );

    RedrawOneStruct( panel, DC, SheetLabel, g_XorMode );
}


int WinEDA_SchematicFrame::Edit_PinSheet( SCH_SHEET_PIN* SheetLabel, wxDC* DC )
{
    if( SheetLabel == NULL )
        return wxID_CANCEL;

    if( DC )
        RedrawOneStruct( DrawPanel, DC, SheetLabel, g_XorMode );

    WinEDA_PinSheetPropertiesFrame* frame =
        new WinEDA_PinSheetPropertiesFrame( this, SheetLabel );

    int diag = frame->ShowModal();
    frame->Destroy();

    if( DC )
        RedrawOneStruct( DrawPanel, DC, SheetLabel, GR_DEFAULT_DRAWMODE );

    return diag;
}


/* Add a new sheet pin to the sheet at the current cursor position.
 */
SCH_SHEET_PIN* WinEDA_SchematicFrame::Create_PinSheet( SCH_SHEET* Sheet, wxDC* DC )
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
SCH_SHEET_PIN* WinEDA_SchematicFrame::Import_PinSheet( SCH_SHEET* Sheet, wxDC* DC )
{
    EDA_BaseStruct* DrawStruct;
    SCH_SHEET_PIN*  NewSheetLabel;
    SCH_HIERLABEL*  HLabel = NULL;

    if( !Sheet->m_AssociatedScreen )
        return NULL;

    DrawStruct = Sheet->m_AssociatedScreen->EEDrawList;
    HLabel     = NULL;

    for( ; DrawStruct != NULL; DrawStruct = DrawStruct->Next() )
    {
        if( DrawStruct->Type() != TYPE_SCH_HIERLABEL )
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
void WinEDA_SchematicFrame::DeleteSheetLabel( bool aRedraw, SCH_SHEET_PIN* aSheetLabelToDel )
{
    SCH_SHEET* parent = (SCH_SHEET*) aSheetLabelToDel->GetParent();

    wxASSERT( parent );
    wxASSERT( parent->Type() == DRAW_SHEET_STRUCT_TYPE );

#if 0 && defined(DEBUG)
    std::cout << "\n\nbefore deleting:\n" << std::flush;
    parent->Show( 0, std::cout );
    std::cout << "\n\n\n" << std::flush;
#endif

    parent->RemoveLabel( aSheetLabelToDel );

    if( aRedraw )
        DrawPanel->PostDirtyRect( parent->GetBoundingBox() );


#if 0 && defined(DEBUG)
    std::cout << "\n\nafter deleting:\n" << std::flush;
    parent->Show( 0, std::cout );
    std::cout << "~after deleting\n\n" << std::flush;
#endif
}
