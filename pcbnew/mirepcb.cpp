/*********************************************/
/* Routines de gestion des mires de centrage */
/*********************************************/

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "pcbnew.h"

#include "protos.h"


/* Routines Locales */
static void Exit_EditMire( WinEDA_DrawPanel* Panel, wxDC* DC );
static void Montre_Position_Mire( WinEDA_DrawPanel* panel, wxDC* DC, bool erase );

/* Variables locales : */
static wxPoint OldPos;
static int     MireDefaultSize = 5000;

enum id_mire_properties {
    ID_ACCEPT_MIRE_PROPERTIES = 1900,
    ID_CANCEL_MIRE_PROPERTIES,
    ID_SIZE_MIRE,
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
    ~WinEDA_MirePropertiesFrame()
    {
    }


private:
    void    MirePropertiesAccept( wxCommandEvent& event );
    void    OnQuit( wxCommandEvent& event );

    DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE( WinEDA_MirePropertiesFrame, wxDialog )
EVT_BUTTON( ID_ACCEPT_MIRE_PROPERTIES, WinEDA_MirePropertiesFrame::MirePropertiesAccept )
EVT_BUTTON( ID_CANCEL_MIRE_PROPERTIES, WinEDA_MirePropertiesFrame::OnQuit )
END_EVENT_TABLE()


/***************************************************************/
void WinEDA_PcbFrame::InstallMireOptionsFrame( MIREPCB* MirePcb,
                                               wxDC* DC, const wxPoint& pos )
/***************************************************************/
{
    WinEDA_MirePropertiesFrame* frame = new WinEDA_MirePropertiesFrame( this,
                                                                        MirePcb, DC, pos );

    frame->ShowModal(); frame->Destroy();
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
    SetFont( *g_DialogFont );
    m_DC = DC;
    Centre();

    m_MirePcb = Mire;

    wxBoxSizer* MainBoxSizer = new wxBoxSizer( wxHORIZONTAL );
    SetSizer( MainBoxSizer );
    wxBoxSizer* LeftBoxSizer  = new wxBoxSizer( wxVERTICAL );
    wxBoxSizer* RightBoxSizer = new wxBoxSizer( wxVERTICAL );
    MainBoxSizer->Add( LeftBoxSizer, 0, wxGROW | wxALL, 5 );
    MainBoxSizer->Add( RightBoxSizer, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5 );

    /* Creation des boutons de commande */
    Button = new wxButton( this, ID_ACCEPT_MIRE_PROPERTIES, _( "Ok" ) );
    Button->SetForegroundColour( *wxRED );
    RightBoxSizer->Add( Button, 0, wxGROW | wxALL, 5 );

    Button = new wxButton( this, ID_CANCEL_MIRE_PROPERTIES, _( "Cancel" ) );
    Button->SetForegroundColour( *wxBLUE );
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
void WinEDA_MirePropertiesFrame::OnQuit( wxCommandEvent& WXUNUSED (event) )
/**********************************************************************/
{
    Close( true );    // true is to force the frame to close
}


/**************************************************************************/
void WinEDA_MirePropertiesFrame::MirePropertiesAccept( wxCommandEvent& event )
/**************************************************************************/

/* Met a jour les differents parametres pour le composant en cours d'édition
 */
{
    m_MirePcb->Draw( m_Parent->DrawPanel, m_DC, wxPoint( 0, 0 ), GR_XOR );

    m_MirePcb->m_Width = m_MireWidthCtrl->GetValue();
    MireDefaultSize    = m_MirePcb->m_Size = m_MireSizeCtrl->GetValue();
    m_MirePcb->m_Shape = m_MireShape->GetSelection() ? 1 : 0;

    m_MirePcb->Draw( m_Parent->DrawPanel, m_DC, wxPoint( 0, 0 ), GR_OR );

    m_Parent->GetScreen()->SetModify();
    Close( TRUE );
}


/**************************************************************/
void WinEDA_PcbFrame::Delete_Mire( MIREPCB* MirePcb, wxDC* DC )
/**************************************************************/
{
    if( MirePcb == NULL )
        return;

    MirePcb->Draw( DrawPanel, DC, wxPoint( 0, 0 ), GR_XOR );
    MirePcb ->DeleteStructure();
}


/**********************************************************/
static void Exit_EditMire( WinEDA_DrawPanel* Panel, wxDC* DC )
/**********************************************************/
{
    BASE_SCREEN* screen  = Panel->GetScreen();
    MIREPCB*     MirePcb = (MIREPCB*) screen->GetCurItem();

    Panel->ManageCurseur = NULL;
    Panel->ForceCloseManageCurseur = NULL;

    if( MirePcb )
    {
        /* Effacement de la mire */
        MirePcb->Draw( Panel, DC, wxPoint( 0, 0 ), GR_XOR );

        if( MirePcb->m_Flags & IS_NEW )
        {
            MirePcb->Draw( Panel, DC, wxPoint( 0, 0 ), GR_XOR );
            MirePcb ->DeleteStructure();
            MirePcb = NULL;
        }
        else    /* Ancienne mire en deplacement: Remise en ancienne position */
        {
            MirePcb->m_Pos   = OldPos;
            MirePcb->m_Flags = 0;
            MirePcb->Draw( Panel, DC, wxPoint( 0, 0 ), GR_OR );
        }
    }
}


/*****************************************************/
MIREPCB* WinEDA_PcbFrame::Create_Mire( wxDC* DC )
/*****************************************************/

/* Routine de creation d'un Draw Symbole Pcb type MIRE
 */
{
    MIREPCB* MirePcb = new MIREPCB( m_Pcb );

    MirePcb->Pnext = m_Pcb->m_Drawings;
    MirePcb->Pback = m_Pcb;
    if( m_Pcb->m_Drawings )
        m_Pcb->m_Drawings->Pback = MirePcb;
    m_Pcb->m_Drawings = MirePcb;

    MirePcb->SetLayer( EDGE_N );
    MirePcb->m_Width = g_DesignSettings.m_EdgeSegmentWidth;
    MirePcb->m_Size  = MireDefaultSize;
    MirePcb->m_Pos   = GetScreen()->m_Curseur;

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

    OldPos = MirePcb->m_Pos;
    MirePcb->m_Flags |= IS_MOVED;
    DrawPanel->ManageCurseur = Montre_Position_Mire;
    DrawPanel->ForceCloseManageCurseur = Exit_EditMire;
    SetCurItem( MirePcb );
}


/**************************************************************/
void WinEDA_PcbFrame::Place_Mire( MIREPCB* MirePcb, wxDC* DC )
/**************************************************************/
{
    if( MirePcb == NULL )
        return;

    MirePcb->Draw( DrawPanel, DC, wxPoint( 0, 0 ), GR_OR );

    MirePcb->m_Flags = 0;
    DrawPanel->ManageCurseur = NULL;
    DrawPanel->ForceCloseManageCurseur = NULL;
    SetCurItem( NULL );
    GetScreen()->SetModify();
}


/******************************************************************************/
static void Montre_Position_Mire( WinEDA_DrawPanel* panel, wxDC* DC, bool erase )
/*********************************************************************************/
/* redessin du contour de la piste  lors des deplacements de la souris */
{
    BASE_SCREEN* screen  = panel->GetScreen();
    MIREPCB*     MirePcb = (MIREPCB*) screen->GetCurItem();

    if( MirePcb == NULL )
        return;

    /* efface ancienne position */
    if( erase )
        MirePcb->Draw( panel, DC, wxPoint( 0, 0 ), GR_XOR );

    MirePcb->m_Pos = screen->m_Curseur;

    // Reaffichage
    MirePcb->Draw( panel, DC, wxPoint( 0, 0 ), GR_XOR );
}
