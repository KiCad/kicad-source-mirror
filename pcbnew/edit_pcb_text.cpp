/*********************************************************************/
/* Edition of texts on copper and technical layers (TEXTE_PCB class) */
/*********************************************************************/

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "pcbnew.h"

#include "protos.h"

/* Local functions */
static void Move_Texte_Pcb( WinEDA_DrawPanel* panel, wxDC* DC, bool erase );
static void Abort_Edit_Pcb_Text( WinEDA_DrawPanel* Panel, wxDC* DC );

/* Local variables : */
static wxPoint old_pos; // initial position of the text when moving it


/*************************************************************/
void Abort_Edit_Pcb_Text( WinEDA_DrawPanel* Panel, wxDC* DC )
/*************************************************************/

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
 *  Place the current text being moving
 */
{
    if( TextePcb == NULL )
        return;

    TextePcb->Draw( DrawPanel, DC, GR_OR );
    DrawPanel->ManageCurseur = NULL;
    DrawPanel->ForceCloseManageCurseur = NULL;
    SetCurItem( NULL );
    GetScreen()->SetModify();
    TextePcb->m_Flags = 0;
}


/***********************************************************************/
void WinEDA_PcbFrame::StartMoveTextePcb( TEXTE_PCB* TextePcb, wxDC* DC )
/***********************************************************************/

/* Initialise parameters to move a pcb text
 */
{
    if( TextePcb == NULL )
        return;
    old_pos = TextePcb->m_Pos;
    TextePcb->Draw( DrawPanel, DC, GR_XOR );
    TextePcb->m_Flags |= IS_MOVED;
    TextePcb->Display_Infos( this );
    DrawPanel->ManageCurseur = Move_Texte_Pcb;
    DrawPanel->ForceCloseManageCurseur = Abort_Edit_Pcb_Text;
    SetCurItem( TextePcb );
    DrawPanel->ManageCurseur( DrawPanel, DC, FALSE );
}


/*************************************************************************/
static void Move_Texte_Pcb( WinEDA_DrawPanel* panel, wxDC* DC, bool erase )
/*************************************************************************/
/* Routine deplacant le texte PCB suivant le curseur de la souris */
{
    TEXTE_PCB* TextePcb = (TEXTE_PCB*) panel->GetScreen()->GetCurItem();

    if( TextePcb == NULL )
        return;

    /* effacement du texte : */

    if( erase )
        TextePcb->Draw( panel, DC, GR_XOR );

    TextePcb->m_Pos = panel->GetScreen()->m_Curseur;

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
    m_Pcb->Add( TextePcb );

    /* Mise a jour des caracteristiques */
    TextePcb->m_Flags  = IS_NEW;
    TextePcb->SetLayer( ((PCB_SCREEN*)GetScreen())->m_Active_Layer );
    TextePcb->m_Miroir = 1;
    if( TextePcb->GetLayer() == COPPER_LAYER_N )
        TextePcb->m_Miroir = 0;

    TextePcb->m_Size  = g_DesignSettings.m_PcbTextSize;
    TextePcb->m_Pos   = GetScreen()->m_Curseur;
    TextePcb->m_Width = g_DesignSettings.m_PcbTextWidth;

    InstallTextPCBOptionsFrame( TextePcb, DC );
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

    /* Redessin du Texte */
    TextePcb->Draw( DrawPanel, DC, drawmode );
    TextePcb->Display_Infos( this );

    GetScreen()->SetModify();
}
