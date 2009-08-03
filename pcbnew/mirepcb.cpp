/*********************************************/
/*  Functions to edite targets (class MIRE)  */
/*********************************************/

#include "fctsys.h"
#include "common.h"
#include "class_drawpanel.h"

#include "pcbnew.h"
#include "wxPcbStruct.h"

#include "protos.h"


/* Routines Locales */
static void AbortMoveAndEditTarget( WinEDA_DrawPanel* Panel, wxDC* DC );
static void ShowTargetShapeWhileMovingMouse( WinEDA_DrawPanel* panel, wxDC* DC, bool erase );

/* Local variables : */
static int     MireDefaultSize = 5000;
static MIREPCB s_TargetCopy( NULL );      /* Used to store "old" values of the current item
                                         *  parameters before edition (used in undo/redo or cancel operations)
                                         */

enum id_mire_properties {
    ID_SIZE_MIRE = 1900,   // (Not currently used anywhere else)
    ID_LISTBOX_SHAPE_MIRE
};

/************************************/
/* class WinEDA_MirePropertiesFrame */
/************************************/

class WinEDA_MirePropertiesFrame : public wxDialog
{
private:

    WinEDA_PcbFrame*  m_Parent;
    wxDC*             m_DC;
    MIREPCB*          m_MirePcb;
    WinEDA_ValueCtrl* m_MireWidthCtrl;
    WinEDA_ValueCtrl* m_MireSizeCtrl;
    wxRadioBox*       m_MireShape;

public:

    // Constructor and destructor
    WinEDA_MirePropertiesFrame( WinEDA_PcbFrame* parent,
                                MIREPCB* Mire, wxDC* DC, const wxPoint& pos );
    ~WinEDA_MirePropertiesFrame() { }

private:
    void OnOkClick( wxCommandEvent& event );
    void OnCancelClick( wxCommandEvent& event );

    DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE( WinEDA_MirePropertiesFrame, wxDialog )
EVT_BUTTON( wxID_OK, WinEDA_MirePropertiesFrame::OnOkClick )
EVT_BUTTON( wxID_CANCEL, WinEDA_MirePropertiesFrame::OnCancelClick )
END_EVENT_TABLE()


/***************************************************************/
void WinEDA_PcbFrame::InstallMireOptionsFrame( MIREPCB* MirePcb,
                                               wxDC* DC, const wxPoint& pos )
/***************************************************************/
{
    WinEDA_MirePropertiesFrame* frame = new WinEDA_MirePropertiesFrame( this,
                                                                        MirePcb, DC, pos );

    frame->ShowModal();
    frame->Destroy();
}


WinEDA_MirePropertiesFrame::WinEDA_MirePropertiesFrame( WinEDA_PcbFrame* parent,
                                                        MIREPCB* Mire, wxDC* DC,
                                                        const wxPoint& framepos ) :
    wxDialog( parent, -1, _( "Target Properties" ), framepos, wxSize( 270, 210 ),
              DIALOG_STYLE )
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

    /* Creation des boutons de commande */
    Button = new wxButton( this, wxID_OK, _( "OK" ) );
    RightBoxSizer->Add( Button, 0, wxGROW | wxALL, 5 );

    Button = new wxButton( this, wxID_CANCEL, _( "Cancel" ) );
    RightBoxSizer->Add( Button, 0, wxGROW | wxALL, 5 );

    // Size:
    m_MireSizeCtrl = new WinEDA_ValueCtrl( this, _( "Size" ),
                                           m_MirePcb->m_Size,
                                           g_UnitMetric, LeftBoxSizer, m_Parent->m_InternalUnits );

    // Width:
    m_MireWidthCtrl = new WinEDA_ValueCtrl( this, _( "Width" ),
                                            m_MirePcb->m_Width,
                                            g_UnitMetric, LeftBoxSizer, m_Parent->m_InternalUnits );

    // Shape
    wxString shape_list[2] = { _( "shape +" ), _( "shape X" ) };
    m_MireShape = new wxRadioBox( this, ID_LISTBOX_SHAPE_MIRE,
                                  _( "Target Shape:" ),
                                  wxDefaultPosition, wxSize( -1, -1 ),
                                  2, shape_list, 1 );
    m_MireShape->SetSelection( m_MirePcb->m_Shape ? 1 : 0 );
    LeftBoxSizer->Add( m_MireShape, 0, wxGROW | wxALL, 5 );

    GetSizer()->Fit( this );
    GetSizer()->SetSizeHints( this );
}


/**********************************************************************/
void WinEDA_MirePropertiesFrame::OnCancelClick( wxCommandEvent& WXUNUSED (event) )
/**********************************************************************/
{
    EndModal( -1 );
}


/**************************************************************************/
void WinEDA_MirePropertiesFrame::OnOkClick( wxCommandEvent& event )
/**************************************************************************/

/* Met a jour les differents parametres pour le composant en cours d'édition
 */
{
    m_MirePcb->Draw( m_Parent->DrawPanel, m_DC, GR_XOR );

    // Save old item in undo list, if is is not curently edited (will be later if so)
    if( m_MirePcb->m_Flags == 0 )
        m_Parent->SaveCopyInUndoList( m_MirePcb, UR_CHANGED );

    if( m_MirePcb->m_Flags != 0)          // other edition in progress (MOVE, NEW ..)
        m_MirePcb->m_Flags |= IN_EDIT;    // set flag in edit to force undo/redo/abort proper operation

    m_MirePcb->m_Width = m_MireWidthCtrl->GetValue();
    MireDefaultSize    = m_MirePcb->m_Size = m_MireSizeCtrl->GetValue();
    m_MirePcb->m_Shape = m_MireShape->GetSelection() ? 1 : 0;

    m_MirePcb->Draw( m_Parent->DrawPanel, m_DC, (m_MirePcb->m_Flags & IS_MOVED) ? GR_XOR : GR_OR );

    m_Parent->GetScreen()->SetModify();
    EndModal( 1 );
}


/**************************************************************/
void WinEDA_PcbFrame::Delete_Mire( MIREPCB* MirePcb, wxDC* DC )
/**************************************************************/
{
    if( MirePcb == NULL )
        return;

    MirePcb->Draw( DrawPanel, DC, GR_XOR );
    SaveCopyInUndoList( MirePcb, UR_DELETED );
    MirePcb->UnLink();
}


/**********************************************************/
static void AbortMoveAndEditTarget( WinEDA_DrawPanel* Panel, wxDC* DC )
/**********************************************************/
{
    BASE_SCREEN* screen  = Panel->GetScreen();
    MIREPCB*     MirePcb = (MIREPCB*) screen->GetCurItem();

    Panel->ManageCurseur = NULL;
    Panel->ForceCloseManageCurseur = NULL;
    ((WinEDA_PcbFrame*)Panel->m_Parent)->SetCurItem( NULL );

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
        if( (MirePcb->m_Flags & IN_EDIT) )
        {
            MirePcb->m_Pos = s_TargetCopy.m_Pos;
            MirePcb->m_Width = s_TargetCopy.m_Width;
            MirePcb->m_Size  = s_TargetCopy.m_Size;
            MirePcb->m_Shape = s_TargetCopy.m_Shape;
        }
        MirePcb->m_Flags = 0;
        MirePcb->Draw( Panel, DC, GR_OR );
    }
}


/*****************************************************/
MIREPCB* WinEDA_PcbFrame::Create_Mire( wxDC* DC )
/*****************************************************/

/* Routine de creation d'un Draw Symbole Pcb type MIRE
 */
{
    MIREPCB* MirePcb = new MIREPCB( GetBoard() );

    MirePcb->m_Flags = IS_NEW;

    GetBoard()->Add( MirePcb );

    MirePcb->SetLayer( EDGE_N );
    MirePcb->m_Width = g_DesignSettings.m_EdgeSegmentWidth;
    MirePcb->m_Size  = MireDefaultSize;

    Place_Mire( MirePcb, DC );

    return MirePcb;
}


/**********************************************************************/
void WinEDA_PcbFrame::StartMove_Mire( MIREPCB* MirePcb, wxDC* DC )
/**********************************************************************/

/* Routine d'initialisation du deplacement d'une mire
 */
{
    if( MirePcb == NULL )
        return;

    s_TargetCopy = *MirePcb;
    MirePcb->m_Flags |= IS_MOVED;
    DrawPanel->ManageCurseur = ShowTargetShapeWhileMovingMouse;
    DrawPanel->ForceCloseManageCurseur = AbortMoveAndEditTarget;
    SetCurItem( MirePcb );
}


/**************************************************************/
void WinEDA_PcbFrame::Place_Mire( MIREPCB* MirePcb, wxDC* DC )
/**************************************************************/
{
    if( MirePcb == NULL )
        return;

    MirePcb->Draw( DrawPanel, DC, GR_OR );
    DrawPanel->ManageCurseur = NULL;
    DrawPanel->ForceCloseManageCurseur = NULL;
    SetCurItem( NULL );
    GetScreen()->SetModify();

    if( (MirePcb->m_Flags & IS_NEW) )
    {
        SaveCopyInUndoList( MirePcb, UR_NEW );
        MirePcb->m_Flags = 0;
        return;
    }


    if( MirePcb->m_Flags == IS_MOVED )
    {
        SaveCopyInUndoList( MirePcb, UR_MOVED, MirePcb->m_Pos - s_TargetCopy.m_Pos );
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


/******************************************************************************/
static void ShowTargetShapeWhileMovingMouse( WinEDA_DrawPanel* panel, wxDC* DC, bool erase )
/*********************************************************************************/
/* redessin du contour de la piste  lors des deplacements de la souris */
{
    BASE_SCREEN* screen  = panel->GetScreen();
    MIREPCB*     MirePcb = (MIREPCB*) screen->GetCurItem();

    if( MirePcb == NULL )
        return;

    /* efface ancienne position */
    if( erase )
        MirePcb->Draw( panel, DC, GR_XOR );

    MirePcb->m_Pos = screen->m_Curseur;

    // Reaffichage
    MirePcb->Draw( panel, DC, GR_XOR );
}
