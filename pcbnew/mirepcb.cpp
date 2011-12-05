/**
 * @file mirepcb.cpp
 * @brief Functions to edit targets (class MIRE).
 */

#include "fctsys.h"
#include "class_drawpanel.h"
#include "wxPcbStruct.h"
#include "dialog_helpers.h"
#include "gr_basic.h"

#include "class_board.h"
#include "class_mire.h"

#include "pcbnew.h"
#include "protos.h"


/* Routines Locales */
static void AbortMoveAndEditTarget( EDA_DRAW_PANEL* Panel, wxDC* DC );
static void ShowTargetShapeWhileMovingMouse( EDA_DRAW_PANEL* aPanel,
                                             wxDC*           aDC,
                                             const wxPoint&  aPosition,
                                             bool            aErase );

/* Local variables : */
static int     MireDefaultSize = 5000;
static PCB_TARGET s_TargetCopy( NULL ); /* Used to store "old" values of the
                                         * current item parameters before
                                         * edition (used in undo/redo or
                                         * cancel operations)
                                         */

/************************************/
/* class TARGET_PROPERTIES_DIALOG_EDITOR */
/************************************/

class TARGET_PROPERTIES_DIALOG_EDITOR : public wxDialog
{
private:

    PCB_EDIT_FRAME*   m_Parent;
    wxDC*             m_DC;
    PCB_TARGET*       m_Target;
    EDA_VALUE_CTRL*   m_MireWidthCtrl;
    EDA_VALUE_CTRL*   m_MireSizeCtrl;
    wxRadioBox*       m_MireShape;

public:
    TARGET_PROPERTIES_DIALOG_EDITOR( PCB_EDIT_FRAME* parent, PCB_TARGET* Mire, wxDC* DC );
    ~TARGET_PROPERTIES_DIALOG_EDITOR() { }

private:
    void OnOkClick( wxCommandEvent& event );
    void OnCancelClick( wxCommandEvent& event );

    DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE( TARGET_PROPERTIES_DIALOG_EDITOR, wxDialog )
    EVT_BUTTON( wxID_OK, TARGET_PROPERTIES_DIALOG_EDITOR::OnOkClick )
    EVT_BUTTON( wxID_CANCEL, TARGET_PROPERTIES_DIALOG_EDITOR::OnCancelClick )
END_EVENT_TABLE()


void PCB_EDIT_FRAME::ShowTargetOptionsDialog( PCB_TARGET* aTarget, wxDC* DC )
{
    TARGET_PROPERTIES_DIALOG_EDITOR* frame =
        new TARGET_PROPERTIES_DIALOG_EDITOR( this, aTarget, DC );

    frame->ShowModal();
    frame->Destroy();
}


TARGET_PROPERTIES_DIALOG_EDITOR::TARGET_PROPERTIES_DIALOG_EDITOR( PCB_EDIT_FRAME* parent,
                                                                  PCB_TARGET* aTarget, wxDC* DC ) :
    wxDialog( parent, wxID_ANY, wxString( _( "Target Properties" ) ) )
{
    wxString  number;
    wxButton* Button;

    m_Parent = parent;
    m_DC     = DC;
    Centre();

    m_Target = aTarget;

    wxBoxSizer* MainBoxSizer = new wxBoxSizer( wxHORIZONTAL );
    SetSizer( MainBoxSizer );
    wxBoxSizer* LeftBoxSizer  = new wxBoxSizer( wxVERTICAL );
    wxBoxSizer* RightBoxSizer = new wxBoxSizer( wxVERTICAL );
    MainBoxSizer->Add( LeftBoxSizer, 0, wxGROW | wxALL, 5 );
    MainBoxSizer->Add( RightBoxSizer, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5 );

    /* Create of the command buttons. */
    Button = new wxButton( this, wxID_OK, _( "OK" ) );
    RightBoxSizer->Add( Button, 0, wxGROW | wxALL, 5 );

    Button = new wxButton( this, wxID_CANCEL, _( "Cancel" ) );
    RightBoxSizer->Add( Button, 0, wxGROW | wxALL, 5 );

    // Size:
    m_MireSizeCtrl = new EDA_VALUE_CTRL( this, _( "Size" ),
                                         m_Target->m_Size,
                                         g_UserUnit, LeftBoxSizer,
                                         m_Parent->m_InternalUnits );

    // Width:
    m_MireWidthCtrl = new EDA_VALUE_CTRL( this, _( "Width" ),
                                          m_Target->m_Width,
                                          g_UserUnit, LeftBoxSizer,
                                          m_Parent->m_InternalUnits );

    // Shape
    wxString shape_list[2] = { _( "shape +" ), _( "shape X" ) };
    m_MireShape = new wxRadioBox( this, wxID_ANY,
                                  _( "Target Shape:" ),
                                  wxDefaultPosition, wxSize( -1, -1 ),
                                  2, shape_list, 1 );
    m_MireShape->SetSelection( m_Target->m_Shape ? 1 : 0 );
    LeftBoxSizer->Add( m_MireShape, 0, wxGROW | wxALL, 5 );

    GetSizer()->Fit( this );
    GetSizer()->SetSizeHints( this );
}


void TARGET_PROPERTIES_DIALOG_EDITOR::OnCancelClick( wxCommandEvent& event )
{
    EndModal( -1 );
}


/* Updates the different parameters for the component being edited
 */
void TARGET_PROPERTIES_DIALOG_EDITOR::OnOkClick( wxCommandEvent& event )
{
    m_Target->Draw( m_Parent->DrawPanel, m_DC, GR_XOR );

    // Save old item in undo list, if is is not currently edited (will be later if so)
    if( m_Target->m_Flags == 0 )
        m_Parent->SaveCopyInUndoList( m_Target, UR_CHANGED );

    if( m_Target->m_Flags != 0 )           // other edition in progress (MOVE, NEW ..)
        m_Target->m_Flags |= IN_EDIT;      // set flag in edit to force
                                           // undo/redo/abort proper operation

    m_Target->m_Width = m_MireWidthCtrl->GetValue();
    MireDefaultSize    = m_Target->m_Size = m_MireSizeCtrl->GetValue();
    m_Target->m_Shape = m_MireShape->GetSelection() ? 1 : 0;

    m_Target->Draw( m_Parent->DrawPanel, m_DC, ( m_Target->m_Flags & IS_MOVED ) ? GR_XOR : GR_OR );

    m_Parent->OnModify();
    EndModal( 1 );
}


void PCB_EDIT_FRAME::DeleteTarget( PCB_TARGET* aTarget, wxDC* DC )
{
    if( aTarget == NULL )
        return;

    aTarget->Draw( DrawPanel, DC, GR_XOR );
    SaveCopyInUndoList( aTarget, UR_DELETED );
    aTarget->UnLink();
}


static void AbortMoveAndEditTarget( EDA_DRAW_PANEL* Panel, wxDC* DC )
{
    BASE_SCREEN* screen  = Panel->GetScreen();
    PCB_TARGET*  target = (PCB_TARGET*) screen->GetCurItem();

    ( (PCB_EDIT_FRAME*) Panel->GetParent() )->SetCurItem( NULL );

    Panel->SetMouseCapture( NULL, NULL );

    if( target == NULL )
        return;

    target->Draw( Panel, DC, GR_XOR );

    if( target->IsNew() )     // If it is new, delete it
    {
        target->Draw( Panel, DC, GR_XOR );
        target->DeleteStructure();
        target = NULL;
    }
    else    /* it is an existing item: retrieve initial values of parameters */
    {
        if( ( target->m_Flags & (IN_EDIT | IS_MOVED) ) )
        {
            target->m_Pos   = s_TargetCopy.m_Pos;
            target->m_Width = s_TargetCopy.m_Width;
            target->m_Size  = s_TargetCopy.m_Size;
            target->m_Shape = s_TargetCopy.m_Shape;
        }
        target->m_Flags = 0;
        target->Draw( Panel, DC, GR_OR );
    }
}


/* Draw Symbol PCB type MIRE.
 */
PCB_TARGET* PCB_EDIT_FRAME::CreateTarget( wxDC* DC )
{
    PCB_TARGET* target = new PCB_TARGET( GetBoard() );

    target->m_Flags = IS_NEW;

    GetBoard()->Add( target );

    target->SetLayer( EDGE_N );
    target->m_Width = GetBoard()->GetDesignSettings().m_EdgeSegmentWidth;
    target->m_Size  = MireDefaultSize;
    target->m_Pos  = DrawPanel->GetScreen()->GetCrossHairPosition();

    PlaceTarget( target, DC );

    return target;
}


/* Routine to initialize the displacement of a focal
 */
void PCB_EDIT_FRAME::BeginMoveTarget( PCB_TARGET* aTarget, wxDC* DC )
{
    if( aTarget == NULL )
        return;

    s_TargetCopy      = *aTarget;
    aTarget->m_Flags |= IS_MOVED;
    DrawPanel->SetMouseCapture( ShowTargetShapeWhileMovingMouse, AbortMoveAndEditTarget );
    SetCurItem( aTarget );
}


void PCB_EDIT_FRAME::PlaceTarget( PCB_TARGET* aTarget, wxDC* DC )
{
    if( aTarget == NULL )
        return;

    aTarget->Draw( DrawPanel, DC, GR_OR );
    DrawPanel->SetMouseCapture( NULL, NULL );
    SetCurItem( NULL );
    OnModify();

    if( aTarget->IsNew() )
    {
        SaveCopyInUndoList( aTarget, UR_NEW );
        aTarget->m_Flags = 0;
        return;
    }

    if( aTarget->m_Flags == IS_MOVED )
    {
        SaveCopyInUndoList( aTarget, UR_MOVED, aTarget->m_Pos - s_TargetCopy.m_Pos );
        aTarget->m_Flags = 0;
        return;
    }

    if( (aTarget->m_Flags & IN_EDIT) )
    {
        SwapData( aTarget, &s_TargetCopy );
        SaveCopyInUndoList( aTarget, UR_CHANGED );
        SwapData( aTarget, &s_TargetCopy );
    }

    aTarget->m_Flags = 0;
}


/* Redraw the contour of the track while moving the mouse */
static void ShowTargetShapeWhileMovingMouse( EDA_DRAW_PANEL* aPanel, wxDC* aDC,
                                             const wxPoint& aPosition, bool aErase )
{
    BASE_SCREEN* screen  = aPanel->GetScreen();
    PCB_TARGET*  target = (PCB_TARGET*) screen->GetCurItem();

    if( target == NULL )
        return;

    if( aErase )
        target->Draw( aPanel, aDC, GR_XOR );

    target->m_Pos = screen->GetCrossHairPosition();

    target->Draw( aPanel, aDC, GR_XOR );
}
