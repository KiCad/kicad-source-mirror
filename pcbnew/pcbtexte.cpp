/**************************************************/
/* traitement des editions des textes sur modules */
/**************************************************/

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "pcbnew.h"

#include "protos.h"

#define L_MIN_DESSIN 1 /* seuil de largeur des segments pour trace autre que filaire */

/* Routines Locales */
static void Move_Texte_Pcb( WinEDA_DrawPanel* panel, wxDC* DC, bool erase );
static void Exit_Texte_Pcb( WinEDA_DrawPanel* Panel, wxDC* DC );

/* Variables locales : */
static wxPoint old_pos; // position originelle du texte selecte


enum id_TextPCB_properties {
    ID_TEXTPCB_SELECT_LAYER = 1900
};

/************************************/
/* class WinEDA_TextPCBPropertiesFrame */
/************************************/

class WinEDA_TextPCBPropertiesFrame : public wxDialog
{
private:

    WinEDA_PcbFrame*     m_Parent;
    wxDC*                m_DC;
    TEXTE_PCB*           CurrentTextPCB;
    WinEDA_EnterText*    m_Name;
    WinEDA_PositionCtrl* m_TxtPosCtrl;
    WinEDA_SizeCtrl*     m_TxtSizeCtrl;
    WinEDA_ValueCtrl*    m_TxtWidthCtlr;
    wxRadioBox*          m_Orient;
    wxRadioBox*          m_Mirror;
    WinEDAChoiceBox*     m_SelLayerBox;

public:

    // Constructor and destructor
    WinEDA_TextPCBPropertiesFrame( WinEDA_PcbFrame* parent,
                                   TEXTE_PCB* TextPCB, wxDC* DC, const wxPoint& pos );
    ~WinEDA_TextPCBPropertiesFrame()
    {
    }


private:
    void    OnOkClick( wxCommandEvent& event );
    void    OnCancelClick( wxCommandEvent& event );

    DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE( WinEDA_TextPCBPropertiesFrame, wxDialog )
EVT_BUTTON( wxID_OK, WinEDA_TextPCBPropertiesFrame::OnOkClick )
EVT_BUTTON( wxID_CANCEL, WinEDA_TextPCBPropertiesFrame::OnCancelClick )
END_EVENT_TABLE()


/********************************************************************/
void WinEDA_PcbFrame::InstallTextPCBOptionsFrame( TEXTE_PCB* TextPCB,
                                                  wxDC* DC, const wxPoint& pos )
/********************************************************************/
{
    DrawPanel->m_IgnoreMouseEvents = TRUE;
    WinEDA_TextPCBPropertiesFrame* frame = new WinEDA_TextPCBPropertiesFrame( this,
                                                                              TextPCB, DC, pos );
    frame->ShowModal();
    frame->Destroy();
    DrawPanel->MouseToCursorSchema();
    DrawPanel->m_IgnoreMouseEvents = FALSE;
}


/************************************************************************************/
WinEDA_TextPCBPropertiesFrame::WinEDA_TextPCBPropertiesFrame( WinEDA_PcbFrame* parent,
                                                              TEXTE_PCB* TextPCB, wxDC* DC,
                                                              const wxPoint& framepos ) :
    wxDialog( parent, -1, _( "TextPCB properties" ), framepos, wxSize( 390, 340 ),
              DIALOG_STYLE )
/************************************************************************************/
{
    wxButton*   Button;
    BOARD*      board = parent->m_Pcb;

    m_Parent = parent;


    SetFont( *g_DialogFont );
    m_DC = DC;
    Centre();

    CurrentTextPCB = TextPCB;

    wxBoxSizer* MainBoxSizer = new wxBoxSizer( wxHORIZONTAL );
    SetSizer( MainBoxSizer );
    wxBoxSizer* LeftBoxSizer   = new wxBoxSizer( wxVERTICAL );
    wxBoxSizer* MiddleBoxSizer = new wxBoxSizer( wxVERTICAL );
    wxBoxSizer* RightBoxSizer  = new wxBoxSizer( wxVERTICAL );
    MainBoxSizer->Add( LeftBoxSizer, 0, wxGROW | wxALL, 5 );
    MainBoxSizer->Add( MiddleBoxSizer, 0, wxGROW | wxALL, 5 );
    MainBoxSizer->Add( RightBoxSizer, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5 );

    /* Creation des boutons de commande */
    Button = new wxButton( this, wxID_OK, _( "OK" ) );
    Button->SetForegroundColour( *wxRED );
    RightBoxSizer->Add( Button, 0, wxGROW | wxALL, 5 );
    Button->SetDefault();

    Button = new wxButton( this, wxID_CANCEL, _( "Cancel" ) );
    Button->SetForegroundColour( *wxBLUE );
    RightBoxSizer->Add( Button, 0, wxGROW | wxALL, 5 );

    m_Name = new WinEDA_EnterText( this, _( "Text:" ),
                                  TextPCB->m_Text,
                                  LeftBoxSizer, wxSize( 200, -1 ) );
    m_Name->SetFocus();
    m_Name->SetSelection( -1, -1 );

    m_TxtSizeCtrl = new WinEDA_SizeCtrl( this, _( "Size" ),
                                         TextPCB->m_Size,
                                         g_UnitMetric, LeftBoxSizer, m_Parent->m_InternalUnits );

    m_TxtWidthCtlr = new WinEDA_ValueCtrl( this, _( "Width" ),
                                           TextPCB->m_Width,
                                           g_UnitMetric, LeftBoxSizer, m_Parent->m_InternalUnits );

    m_TxtPosCtrl = new WinEDA_PositionCtrl( this, _( "Position" ),
                                            TextPCB->m_Pos,
                                            g_UnitMetric, LeftBoxSizer, m_Parent->m_InternalUnits );

    m_SelLayerBox = new WinEDAChoiceBox( this, ID_TEXTPCB_SELECT_LAYER,
                                         wxDefaultPosition, wxDefaultSize );
    MiddleBoxSizer->Add( m_SelLayerBox, 0, wxGROW | wxALL, 5 );

    for( int layer=0; layer<NB_LAYERS;  ++layer )
    {
        m_SelLayerBox->Append( board->GetLayerName( layer ) );
    }

    m_SelLayerBox->SetSelection( TextPCB->GetLayer() );


    static const wxString orient_msg[4] = {
        wxT( "0" ), wxT( "90" ), wxT( "180" ), wxT( "-90" ) };

    m_Orient = new wxRadioBox( this, -1, _( "Orientation" ),
                               wxDefaultPosition, wxSize( -1, -1 ), 4, orient_msg,
                               1, wxRA_SPECIFY_COLS );
    MiddleBoxSizer->Add( m_Orient, 0, wxGROW | wxALL, 5 );

    switch( TextPCB->m_Orient )
    {
    default:
        m_Orient->SetSelection( 0 );
        break;

    case 900:
        m_Orient->SetSelection( 1 );
        break;

    case 1800:
        m_Orient->SetSelection( 2 );
        break;

    case 2700:
        m_Orient->SetSelection( 3 );
        break;
    }

    wxString display_msg[2] = { _( "Normal" ), _( "Mirror" ) };
    m_Mirror = new wxRadioBox( this, -1, _( "Display" ),
                               wxDefaultPosition, wxSize( -1, -1 ), 2, display_msg,
                               1, wxRA_SPECIFY_COLS );
    if( !TextPCB->m_Miroir )
        m_Mirror->SetSelection( 1 );
    MiddleBoxSizer->Add( m_Mirror, 0, wxGROW | wxALL, 5 );

    GetSizer()->Fit( this );
    GetSizer()->SetSizeHints( this );
}


/**********************************************************************/
void WinEDA_TextPCBPropertiesFrame::OnCancelClick( wxCommandEvent& WXUNUSED (event) )
/**********************************************************************/
{
    EndModal( -1 );
}


/**************************************************************************************/
void WinEDA_TextPCBPropertiesFrame::OnOkClick( wxCommandEvent& event )
/**************************************************************************************/
{
   // test for acceptable values for parameters:
     wxSize newsize = m_TxtSizeCtrl->GetValue();
    if ( newsize.x < TEXTS_MIN_SIZE )
        newsize.x = TEXTS_MIN_SIZE;
    if ( newsize.y < TEXTS_MIN_SIZE )
        newsize.y = TEXTS_MIN_SIZE;
    if ( newsize.x > TEXTS_MAX_WIDTH )
        newsize.x = TEXTS_MAX_WIDTH;
    if ( newsize.y > TEXTS_MAX_WIDTH )
        newsize.y = TEXTS_MAX_WIDTH;

    if( m_DC )     // Effacement ancien texte
    {
        CurrentTextPCB->Draw( m_Parent->DrawPanel, m_DC, GR_XOR );
    }

    if( !m_Name->GetValue().IsEmpty() )
    {
        CurrentTextPCB->m_Text = m_Name->GetValue();
    }
    CurrentTextPCB->m_Pos    = m_TxtPosCtrl->GetValue();
    CurrentTextPCB->m_Size   = newsize;

    CurrentTextPCB->m_Width  = m_TxtWidthCtlr->GetValue();
    // test for acceptable values for parameters:
    if ( CurrentTextPCB->m_Width > TEXTS_MAX_WIDTH)
        CurrentTextPCB->m_Width = TEXTS_MAX_WIDTH;

    CurrentTextPCB->m_Miroir = (m_Mirror->GetSelection() == 0) ? 1 : 0;
    CurrentTextPCB->m_Orient = m_Orient->GetSelection() * 900;
    CurrentTextPCB->SetLayer( m_SelLayerBox->GetChoice() );
    CurrentTextPCB->CreateDrawData();

    if( m_DC )     // Affichage nouveau texte
    {
        /* Redessin du Texte */
        CurrentTextPCB->Draw( m_Parent->DrawPanel, m_DC, GR_OR );
    }
    m_Parent->m_CurrentScreen->SetModify();
    EndModal( 1 );
}


/******************************************************/
void Exit_Texte_Pcb( WinEDA_DrawPanel* Panel, wxDC* DC )
/*******************************************************/

/*
 *  Routine de sortie du menu edit texte Pcb
 *  Si un texte est selectionne, ses coord initiales sont regenerees
 */
{
    TEXTE_PCB* TextePcb;

    TextePcb = (TEXTE_PCB*) Panel->GetScreen()->GetCurItem();

    if( TextePcb )
    {
        TextePcb->Draw( Panel, DC, GR_XOR );
        TextePcb->m_Pos = old_pos;
        TextePcb->Draw( Panel, DC, GR_OR );
        TextePcb->m_Flags = 0;
    }

    Panel->ManageCurseur = NULL;
    Panel->ForceCloseManageCurseur = NULL;
    ((WinEDA_PcbFrame*)Panel->m_Parent)->SetCurItem( NULL );
}


/*********************************************************************/
void WinEDA_PcbFrame::Place_Texte_Pcb( TEXTE_PCB* TextePcb, wxDC* DC )
/*********************************************************************/

/*
 *  Routine de placement du texte en cours de deplacement
 */
{
    if( TextePcb == NULL )
        return;

    TextePcb->CreateDrawData();
    TextePcb->Draw( DrawPanel, DC, GR_OR );
    DrawPanel->ManageCurseur = NULL;
    DrawPanel->ForceCloseManageCurseur = NULL;
    SetCurItem( NULL );
    m_CurrentScreen->SetModify();
    TextePcb->m_Flags = 0;
}


/***********************************************************************/
void WinEDA_PcbFrame::StartMoveTextePcb( TEXTE_PCB* TextePcb, wxDC* DC )
/***********************************************************************/

/* Routine de preparation du deplacement d'un texte
 */
{
    if( TextePcb == NULL )
        return;
    old_pos = TextePcb->m_Pos;
    TextePcb->Draw( DrawPanel, DC, GR_XOR );
    TextePcb->m_Flags |= IS_MOVED;
    TextePcb->Display_Infos( this );
    DrawPanel->ManageCurseur = Move_Texte_Pcb;
    DrawPanel->ForceCloseManageCurseur = Exit_Texte_Pcb;
    SetCurItem( TextePcb );
    DrawPanel->ManageCurseur( DrawPanel, DC, FALSE );
}


/*************************************************************************/
static void Move_Texte_Pcb( WinEDA_DrawPanel* panel, wxDC* DC, bool erase )
/*************************************************************************/
/* Routine deplacant le texte PCB suivant le curseur de la souris */
{
    TEXTE_PCB* TextePcb = (TEXTE_PCB*)
                          panel->m_Parent->m_CurrentScreen->GetCurItem();

    if( TextePcb == NULL )
        return;

    /* effacement du texte : */

    if( erase )
        TextePcb->Draw( panel, DC, GR_XOR );

    TextePcb->m_Pos = panel->m_Parent->m_CurrentScreen->m_Curseur;

    /* Redessin du Texte */
    TextePcb->Draw( panel, DC, GR_XOR );
}


/**********************************************************************/
void WinEDA_PcbFrame::Delete_Texte_Pcb( TEXTE_PCB* TextePcb, wxDC* DC )
/**********************************************************************/
{
    if( TextePcb == NULL )
        return;

    TextePcb->Draw( DrawPanel, DC, GR_XOR );

    /* Suppression du texte en Memoire*/
    TextePcb ->DeleteStructure();
    DrawPanel->ManageCurseur = NULL;
    DrawPanel->ForceCloseManageCurseur = NULL;
    SetCurItem( NULL );
}


/*******************************************************/
TEXTE_PCB* WinEDA_PcbFrame::Create_Texte_Pcb( wxDC* DC )
/*******************************************************/
{
    TEXTE_PCB* TextePcb;

    TextePcb = new TEXTE_PCB( m_Pcb );

    /* Chainage de la nouvelle structure en debut de liste */
    TextePcb->Pnext = m_Pcb->m_Drawings;
    TextePcb->Pback = (EDA_BaseStruct*) m_Pcb;
    if( m_Pcb->m_Drawings )
        m_Pcb->m_Drawings->Pback = (EDA_BaseStruct*) TextePcb;
    m_Pcb->m_Drawings = TextePcb;

    /* Mise a jour des caracteristiques */
    TextePcb->m_Flags  = IS_NEW;
    TextePcb->SetLayer( ((PCB_SCREEN*)GetScreen())->m_Active_Layer );
    TextePcb->m_Miroir = 1;
    if( TextePcb->GetLayer() == COPPER_LAYER_N )
        TextePcb->m_Miroir = 0;

    TextePcb->m_Size  = g_DesignSettings.m_PcbTextSize;
    TextePcb->m_Pos   = m_CurrentScreen->m_Curseur;
    TextePcb->m_Width = g_DesignSettings.m_PcbTextWidth;

    InstallTextPCBOptionsFrame( TextePcb, DC, TextePcb->m_Pos );
    if( TextePcb->m_Text.IsEmpty() )
    {
        TextePcb ->DeleteStructure();
        TextePcb = NULL;
    }
    else
        StartMoveTextePcb( TextePcb, DC );

    return TextePcb;
}


/***********************************************************************/
void WinEDA_PcbFrame::Rotate_Texte_Pcb( TEXTE_PCB* TextePcb, wxDC* DC )
/***********************************************************************/
{
    int angle    = 900;
    int drawmode = GR_XOR;

    if( TextePcb == NULL )
        return;

    /* effacement du texte : */
    TextePcb->Draw( DrawPanel, DC, GR_XOR );


    TextePcb->m_Orient += angle;
    if( TextePcb->m_Orient >= 3600 )
        TextePcb->m_Orient -= 3600;
    if( TextePcb->m_Orient < 0 )
        TextePcb->m_Orient += 3600;

    TextePcb->CreateDrawData();

    /* Redessin du Texte */
    TextePcb->Draw( DrawPanel, DC, drawmode );
    TextePcb->Display_Infos( this );

    m_CurrentScreen->SetModify();
}
