/********************************************/
/*  Functions to edit targets (class MIRE)  */
/********************************************/

#include "fctsys.h"
#include "common.h"
#include "class_drawpanel.h"

#include "pcbnew.h"
#include "wxPcbStruct.h"
#include "class_board_design_settings.h"
#include "dialog_helpers.h"

#include "protos.h"


/* Routines Locales */
static void AbortMoveAndEditTarget( EDA_DRAW_PANEL* Panel, wxDC* DC );
static void ShowTargetShapeWhileMovingMouse( EDA_DRAW_PANEL* panel,
                                             wxDC*           DC,
                                             bool            erase );

/* Local variables : */
static int     MireDefaultSize = 5000;
static MIREPCB s_TargetCopy( NULL );    /* Used to store "old" values of the
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

    WinEDA_PcbFrame*  m_Parent;
    wxDC*             m_DC;
    MIREPCB*          m_MirePcb;
    WinEDA_ValueCtrl* m_MireWidthCtrl;
    WinEDA_ValueCtrl* m_MireSizeCtrl;
    wxRadioBox*       m_MireShape;

public:
    TARGET_PROPERTIES_DIALOG_EDITOR( WinEDA_PcbFrame* parent, MIREPCB* Mire, wxDC* DC );
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


void WinEDA_PcbFrame::InstallMireOptionsFrame( MIREPCB* MirePcb, wxDC* DC )
{
    TARGET_PROPERTIES_DIALOG_EDITOR* frame =
        new TARGET_PROPERTIES_DIALOG_EDITOR( this, MirePcb, DC );

    frame->ShowModal();
    frame->Destroy();
}


TARGET_PROPERTIES_DIALOG_EDITOR::TARGET_PROPERTIES_DIALOG_EDITOR(
    WinEDA_PcbFrame* parent,
    MIREPCB* Mire, wxDC* DC ) :
    wxDialog( parent, wxID_ANY, wxString( _( "Target Properties" ) ) )
{
    wxString  number;
    wxButton* Button;

    m_Parent = parent;
    m_DC     = DC;
    Centre();

    m_MirePcb = Mire;

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
    m_MireSizeCtrl = new WinEDA_ValueCtrl( this, _( "Size" ),
                                           m_MirePcb->m_Size,
                                           g_UserUnit, LeftBoxSizer,
                                           m_Parent->m_InternalUnits );

    // Width:
    m_MireWidthCtrl = new WinEDA_ValueCtrl( this, _( "Width" ),
                                            m_MirePcb->m_Width,
                                            g_UserUnit, LeftBoxSizer,
                                            m_Parent->m_InternalUnits );

    // Shape
    wxString shape_list[2] = { _( "shape +" ), _( "shape X" ) };
    m_MireShape = new wxRadioBox( this, wxID_ANY,
                                  _( "Target Shape:" ),
                                  wxDefaultPosition, wxSize( -1, -1 ),
                                  2, shape_list, 1 );
    m_MireShape->SetSelection( m_MirePcb->m_Shape ? 1 : 0 );
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
    m_MirePcb->Draw( m_Parent->DrawPanel, m_DC, GR_XOR );

    // Save old item in undo list, if is is not currently edited (will be later
    // if so)
    if( m_MirePcb->m_Flags == 0 )
        m_Parent->SaveCopyInUndoList( m_MirePcb, UR_CHANGED );

    if( m_MirePcb->m_Flags != 0 )           // other edition in progress (MOVE,
                                            // NEW ..)
        m_MirePcb->m_Flags |= IN_EDIT;      // set flag in edit to force
                                            // undo/redo/abort proper operation

    m_MirePcb->m_Width = m_MireWidthCtrl->GetValue();
    MireDefaultSize    = m_MirePcb->m_Size = m_MireSizeCtrl->GetValue();
    m_MirePcb->m_Shape = m_MireShape->GetSelection() ? 1 : 0;

    m_MirePcb->Draw( m_Parent->DrawPanel, m_DC,
                     ( m_MirePcb->m_Flags & IS_MOVED ) ? GR_XOR : GR_OR );

    m_Parent->OnModify();
    EndModal( 1 );
}


void WinEDA_PcbFrame::Delete_Mire( MIREPCB* MirePcb, wxDC* DC )
{
    if( MirePcb == NULL )
        return;

    MirePcb->Draw( DrawPanel, DC, GR_XOR );
    SaveCopyInUndoList( MirePcb, UR_DELETED );
    MirePcb->UnLink();
}


static void AbortMoveAndEditTarget( EDA_DRAW_PANEL* Panel, wxDC* DC )
{
    BASE_SCREEN* screen  = Panel->GetScreen();
    MIREPCB*     MirePcb = (MIREPCB*) screen->GetCurItem();

    Panel->ManageCurseur = NULL;
    Panel->ForceCloseManageCurseur = NULL;
    ( (WinEDA_PcbFrame*) Panel->GetParent() )->SetCurItem( NULL );

    if( MirePcb == NULL )
        return;

    MirePcb->Draw( Panel, DC, GR_XOR );

    if( MirePcb->m_Flags & IS_NEW )     // If it is new, delete it
    {
        MirePcb->Draw( Panel, DC, GR_XOR );
        MirePcb->DeleteStructure();
        MirePcb = NULL;
    }
    else    /* it is an existing item: retrieve initial values of parameters */
    {
        if( ( MirePcb->m_Flags & (IN_EDIT | IS_MOVED) ) )
        {
            MirePcb->m_Pos   = s_TargetCopy.m_Pos;
            MirePcb->m_Width = s_TargetCopy.m_Width;
            MirePcb->m_Size  = s_TargetCopy.m_Size;
            MirePcb->m_Shape = s_TargetCopy.m_Shape;
        }
        MirePcb->m_Flags = 0;
        MirePcb->Draw( Panel, DC, GR_OR );
    }
}


/* Draw Symbol PCB type MIRE.
 */
MIREPCB* WinEDA_PcbFrame::Create_Mire( wxDC* DC )
{
    MIREPCB* MirePcb = new MIREPCB( GetBoard() );

    MirePcb->m_Flags = IS_NEW;

    GetBoard()->Add( MirePcb );

    MirePcb->SetLayer( EDGE_N );
    MirePcb->m_Width = GetBoard()->GetBoardDesignSettings()->m_EdgeSegmentWidth;
    MirePcb->m_Size  = MireDefaultSize;
    MirePcb->m_Pos  = DrawPanel->GetScreen()->m_Curseur;

    Place_Mire( MirePcb, DC );

    return MirePcb;
}


/* Routine to initialize the displacement of a focal
 */
void WinEDA_PcbFrame::StartMove_Mire( MIREPCB* MirePcb, wxDC* DC )
{
    if( MirePcb == NULL )
        return;

    s_TargetCopy      = *MirePcb;
    MirePcb->m_Flags |= IS_MOVED;
    DrawPanel->ManageCurseur = ShowTargetShapeWhileMovingMouse;
    DrawPanel->ForceCloseManageCurseur = AbortMoveAndEditTarget;
    SetCurItem( MirePcb );
}


void WinEDA_PcbFrame::Place_Mire( MIREPCB* MirePcb, wxDC* DC )
{
    if( MirePcb == NULL )
        return;

    MirePcb->Draw( DrawPanel, DC, GR_OR );
    DrawPanel->ManageCurseur = NULL;
    DrawPanel->ForceCloseManageCurseur = NULL;
    SetCurItem( NULL );
    OnModify();

    if( (MirePcb->m_Flags & IS_NEW) )
    {
        SaveCopyInUndoList( MirePcb, UR_NEW );
        MirePcb->m_Flags = 0;
        return;
    }


    if( MirePcb->m_Flags == IS_MOVED )
    {
        SaveCopyInUndoList( MirePcb,
                            UR_MOVED,
                            MirePcb->m_Pos - s_TargetCopy.m_Pos );
        MirePcb->m_Flags = 0;
        return;
    }

    if( (MirePcb->m_Flags & IN_EDIT) )
    {
        SwapData( MirePcb, &s_TargetCopy );
        SaveCopyInUndoList( MirePcb, UR_CHANGED );
        SwapData( MirePcb, &s_TargetCopy );
    }
    MirePcb->m_Flags = 0;
}


/* Redraw the contour of the track while moving the mouse */
static void ShowTargetShapeWhileMovingMouse( EDA_DRAW_PANEL* panel, wxDC* DC, bool erase )
{
    BASE_SCREEN* screen  = panel->GetScreen();
    MIREPCB*     MirePcb = (MIREPCB*) screen->GetCurItem();

    if( MirePcb == NULL )
        return;

    if( erase )
        MirePcb->Draw( panel, DC, GR_XOR );

    MirePcb->m_Pos = screen->m_Curseur;

    MirePcb->Draw( panel, DC, GR_XOR );
}
