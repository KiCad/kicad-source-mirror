/*****************************************/
/* Edition du pcb: Gestion des cotations */
/*****************************************/

#include "fctsys.h"

#include "common.h"
#include "pcbnew.h"

#include "protos.h"

/* Routines Locales */
static void Exit_EditCotation( WinEDA_DrawPanel* Panel, wxDC* DC );
static void Montre_Position_New_Cotation( WinEDA_DrawPanel* panel, wxDC* DC, bool erase );
static void Ajuste_Details_Cotation( COTATION* pts );

/* Variables "locales" : */
static int status_cotation; /*  = 0 : pas de cotation en cours
                             *  = 1 : debut place, fin a placer
                             *  = 2 : fin placee, texte a ajuster */

/*
 *  Les routines generent une cotation de la forme
 *  - cote usuelle:
 * 
 |			 |
 |	dist	 |
 |<---------->|
 |			 |
 * 
 */

#define MAX_CHAR 40     /* longueur maxi de la cotation */
/* Dimension des fleches */
#define FLECHE_L 500


enum id_Cotation_properties {
    ID_TEXTPCB_SELECT_LAYER = 1900
};

/************************************/
/* class WinEDA_CotationPropertiesFrame */
/************************************/

class WinEDA_CotationPropertiesFrame : public wxDialog
{
private:

    WinEDA_PcbFrame*  m_Parent;
    wxDC*             m_DC;
    COTATION*         CurrentCotation;
    WinEDA_EnterText* m_Name;
    WinEDA_SizeCtrl*  m_TxtSizeCtrl;
    WinEDA_ValueCtrl* m_TxtWidthCtrl;
    wxRadioBox*       m_Mirror;
    WinEDAChoiceBox*  m_SelLayerBox;

public:

    // Constructor and destructor
    WinEDA_CotationPropertiesFrame( WinEDA_PcbFrame* parent,
                                    COTATION* Cotation, wxDC* DC, const wxPoint& pos );
    ~WinEDA_CotationPropertiesFrame()
    {
    }


private:
    void    OnCancelClick( wxCommandEvent& event );
    void    OnOkClick( wxCommandEvent& event );

    DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE( WinEDA_CotationPropertiesFrame, wxDialog )
EVT_BUTTON( wxID_OK, WinEDA_CotationPropertiesFrame::OnOkClick )
EVT_BUTTON( wxID_CANCEL, WinEDA_CotationPropertiesFrame::OnCancelClick )
END_EVENT_TABLE()


WinEDA_CotationPropertiesFrame::WinEDA_CotationPropertiesFrame( WinEDA_PcbFrame* parent,
                                                                COTATION* Cotation, wxDC* DC,
                                                                const wxPoint& framepos ) :
    wxDialog( parent, -1, _( "Dimension properties" ), framepos, wxSize( 340, 270 ),
              DIALOG_STYLE )
{
    wxButton* Button;

    m_Parent = parent;
    SetFont( *g_DialogFont );
    m_DC = DC;
    Centre();

    CurrentCotation = Cotation;

    wxBoxSizer* MainBoxSizer = new wxBoxSizer( wxHORIZONTAL );
    SetSizer( MainBoxSizer );
    wxBoxSizer* LeftBoxSizer  = new wxBoxSizer( wxVERTICAL );
    wxBoxSizer* RightBoxSizer = new wxBoxSizer( wxVERTICAL );
    MainBoxSizer->Add( LeftBoxSizer, 0, wxGROW | wxALL, 5 );
    MainBoxSizer->Add( RightBoxSizer, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5 );

    /* Creation des boutons de commande */
    Button = new wxButton( this, wxID_OK, _( "OK" ) );
    Button->SetForegroundColour( *wxRED );
    RightBoxSizer->Add( Button, 0, wxGROW | wxALL, 5 );

    Button = new wxButton( this, wxID_CANCEL, _( "Cancel" ) );
    Button->SetForegroundColour( *wxBLUE );
    RightBoxSizer->Add( Button, 0, wxGROW | wxALL, 5 );

    wxString display_msg[2] = { _( "Normal" ), _( "Mirror" ) };
    m_Mirror = new wxRadioBox( this, -1, _( "Display" ),
                               wxDefaultPosition, wxSize( -1, -1 ), 2, display_msg,
                               1, wxRA_SPECIFY_COLS );
    if( !Cotation->m_Text->m_Miroir )
        m_Mirror->SetSelection( 1 );;
    RightBoxSizer->Add( m_Mirror, 0, wxGROW | wxALL, 5 );

    m_Name = new WinEDA_EnterText( this, wxT( "Text:" ),
                                  Cotation->m_Text->m_Text,
                                  LeftBoxSizer, wxSize( 200, -1 ) );

    m_TxtSizeCtrl = new WinEDA_SizeCtrl( this, _( "Size" ),
                                         Cotation->m_Text->m_Size,
                                         g_UnitMetric, LeftBoxSizer, m_Parent->m_InternalUnits );

    m_TxtWidthCtrl = new WinEDA_ValueCtrl( this, _( "Width" ),
                                           Cotation->m_Width,
                                           g_UnitMetric, LeftBoxSizer, m_Parent->m_InternalUnits );

    wxStaticText* text = new wxStaticText( this, -1, _( "Layer:" ) );
    LeftBoxSizer->Add( text, 0, wxGROW | wxLEFT | wxRIGHT | wxTOP, 5 );
    m_SelLayerBox = new WinEDAChoiceBox( this, ID_TEXTPCB_SELECT_LAYER,
                                         wxDefaultPosition, wxDefaultSize );
    LeftBoxSizer->Add( m_SelLayerBox, 0, wxGROW | wxLEFT | wxRIGHT | wxBOTTOM, 5 );
    int ii;
    for( ii = FIRST_NO_COPPER_LAYER; ii < NB_LAYERS; ii++ )
    {
        m_SelLayerBox->Append( ReturnPcbLayerName( ii ) );
    }

    m_SelLayerBox->SetSelection( Cotation->GetLayer() - FIRST_NO_COPPER_LAYER );

    GetSizer()->Fit( this );
    GetSizer()->SetSizeHints( this );
}


/**********************************************************************/
void WinEDA_CotationPropertiesFrame::OnCancelClick( wxCommandEvent& WXUNUSED (event) )
/**********************************************************************/
{
    EndModal( -1 );
}


/***********************************************************************************/
void WinEDA_CotationPropertiesFrame::OnOkClick( wxCommandEvent& event )
/***********************************************************************************/
{
    if( m_DC )     // Effacement ancien texte
    {
        CurrentCotation->Draw( m_Parent->DrawPanel, m_DC, wxPoint( 0, 0 ), GR_XOR );
    }

    if( m_Name->GetValue() != wxEmptyString )
    {
        CurrentCotation->SetText( m_Name->GetValue() );
    }

    CurrentCotation->m_Text->m_Size  = m_TxtSizeCtrl->GetValue();
    CurrentCotation->m_Text->m_Width = CurrentCotation->m_Width =
                                           m_TxtWidthCtrl->GetValue();
    CurrentCotation->m_Text->m_Miroir = (m_Mirror->GetSelection() == 0) ? 1 : 0;
    
    CurrentCotation->SetLayer( m_SelLayerBox->GetChoice() + FIRST_NO_COPPER_LAYER ); 
    CurrentCotation->m_Text->SetLayer( m_SelLayerBox->GetChoice() + FIRST_NO_COPPER_LAYER );

    CurrentCotation->m_Text->CreateDrawData();

    if( m_DC )     // Affichage nouveau texte
    {
        /* Redessin du Texte */
        CurrentCotation->Draw( m_Parent->DrawPanel, m_DC, wxPoint( 0, 0 ), GR_OR );
    }

    m_Parent->m_CurrentScreen->SetModify();
    EndModal( 1 );
}


/**************************************************************/
static void Exit_EditCotation( WinEDA_DrawPanel* Panel, wxDC* DC )
/**************************************************************/
{
    COTATION* Cotation = (COTATION*) Panel->GetScreen()->GetCurItem();

    if( Cotation )
    {
        if( Cotation->m_Flags & IS_NEW )
        {
            Cotation->Draw( Panel, DC, wxPoint( 0, 0 ), GR_XOR );
            Cotation->DeleteStructure();
        }
        else
        {
            Cotation->Draw( Panel, DC, wxPoint( 0, 0 ), GR_OR );
        }
    }

    status_cotation      = 0;
    Panel->ManageCurseur = NULL;
    Panel->ForceCloseManageCurseur = NULL;
    ((WinEDA_PcbFrame*)Panel->m_Parent)->SetCurItem(NULL);    
}


/*************************************************************************/
COTATION* WinEDA_PcbFrame::Begin_Cotation( COTATION* Cotation, wxDC* DC )
/*************************************************************************/
{
    wxPoint pos;

    if( Cotation == NULL )       /* debut reel du trace */
    {
        status_cotation = 1;
        pos = m_CurrentScreen->m_Curseur;

        Cotation = new COTATION( m_Pcb );
        Cotation->m_Flags = IS_NEW;

        Cotation->SetLayer( ((PCB_SCREEN*)GetScreen())->m_Active_Layer );
        Cotation->m_Width = g_DesignSettings.m_DrawSegmentWidth;
        Cotation->m_Text->m_Width = Cotation->m_Width;

        Cotation->Barre_ox = Cotation->Barre_fx = pos.x;
        Cotation->Barre_oy = Cotation->Barre_fy = pos.y;

        Cotation->TraitD_ox = Cotation->TraitD_fx = pos.x;
        Cotation->TraitD_oy = Cotation->TraitD_fy = pos.y;

        Cotation->TraitG_ox = Cotation->TraitG_fx = pos.x;
        Cotation->TraitG_oy = Cotation->TraitG_fy = pos.y;

        Cotation->FlecheG1_ox = Cotation->FlecheG1_fx = pos.x;
        Cotation->FlecheG1_oy = Cotation->FlecheG1_fy = pos.y;

        Cotation->FlecheG2_ox = Cotation->FlecheG2_fx = pos.x;
        Cotation->FlecheG2_oy = Cotation->FlecheG2_fy = pos.y;

        Cotation->FlecheD1_ox = Cotation->FlecheD1_fx = pos.x;
        Cotation->FlecheD1_oy = Cotation->FlecheD1_fy = pos.y;

        Cotation->FlecheD2_ox = Cotation->FlecheD2_fx = pos.x;
        Cotation->FlecheD2_oy = Cotation->FlecheD2_fy = pos.y;

        Cotation->m_Text->m_Miroir = 1;
        Cotation->m_Text->m_Size   = g_DesignSettings.m_PcbTextSize;
        Cotation->m_Text->m_Width  = g_DesignSettings.m_PcbTextWidth;

        Ajuste_Details_Cotation( Cotation );

        Cotation->Draw( DrawPanel, DC, wxPoint( 0, 0 ), GR_XOR );

        DrawPanel->ManageCurseur = Montre_Position_New_Cotation;
        DrawPanel->ForceCloseManageCurseur = Exit_EditCotation;
        return Cotation;
    }

    // Cotation != NULL
    if( status_cotation == 1 )
    {
        status_cotation = 2;
        return Cotation;
    }

    Cotation->Draw( DrawPanel, DC, wxPoint( 0, 0 ), GR_OR );
    Cotation->m_Flags = 0;

    /* Insertion de la structure dans le Chainage .Drawings du PCB */
    Cotation->Pback = m_Pcb;
    Cotation->Pnext = m_Pcb->m_Drawings;
    if( m_Pcb->m_Drawings )
        m_Pcb->m_Drawings->Pback = Cotation;
    m_Pcb->m_Drawings = Cotation;

    m_CurrentScreen->SetModify();
    DrawPanel->ManageCurseur = NULL;
    DrawPanel->ForceCloseManageCurseur = NULL;

    return NULL;
}


/************************************************************************************/
static void Montre_Position_New_Cotation( WinEDA_DrawPanel* panel, wxDC* DC, bool erase )
/************************************************************************************/
/* redessin du contour de la piste  lors des deplacements de la souris */
{
    PCB_SCREEN* screen   = (PCB_SCREEN*) panel->GetScreen();
    COTATION*   Cotation = (COTATION*) screen->GetCurItem();
    wxPoint     pos = screen->m_Curseur;

    if( Cotation == NULL )
        return;

    /* efface ancienne position */
    if( erase )
    {
        Cotation->Draw( panel, DC, wxPoint( 0, 0 ), GR_XOR );
    }

    Cotation->SetLayer( screen->m_Active_Layer );
    if( status_cotation == 1 )
    {
        Cotation->TraitD_ox = pos.x;
        Cotation->TraitD_oy = pos.y;
        Cotation->Barre_fx  = Cotation->TraitD_ox;
        Cotation->Barre_fy  = Cotation->TraitD_oy;
        Ajuste_Details_Cotation( Cotation );
    }
    else
    {
        int   deltax, deltay, dx, dy;
        float angle, depl;
        deltax = Cotation->TraitD_ox - Cotation->TraitG_ox;
        deltay = Cotation->TraitD_oy - Cotation->TraitG_oy;

        /* Calcul de la direction de deplacement
         *  ( perpendiculaire a l'axe de la cote ) */
        angle = atan2( deltay, deltax ) + (M_PI / 2);

        deltax = pos.x - Cotation->TraitD_ox;
        deltay = pos.y - Cotation->TraitD_oy;
        depl   = ( deltax * cos( angle ) ) + ( deltay * sin( angle ) );
        dx = (int) ( depl * cos( angle ) );
        dy = (int) ( depl * sin( angle ) );
        Cotation->Barre_ox = Cotation->TraitG_ox + dx;
        Cotation->Barre_oy = Cotation->TraitG_oy + dy;
        Cotation->Barre_fx = Cotation->TraitD_ox + dx;
        Cotation->Barre_fy = Cotation->TraitD_oy + dy;

        Ajuste_Details_Cotation( Cotation );
    }

    Cotation->Draw( panel, DC, wxPoint( 0, 0 ), GR_XOR );
}


/***************************************************************/
void WinEDA_PcbFrame::Install_Edit_Cotation( COTATION* Cotation,
                                             wxDC* DC, const wxPoint& pos )
/***************************************************************/
{
    if( Cotation == NULL )
        return;

    WinEDA_CotationPropertiesFrame* frame = new WinEDA_CotationPropertiesFrame( this,
                                                                                Cotation, DC, pos );
    frame->ShowModal();
    frame->Destroy();
}


/*******************************************************************/
void WinEDA_PcbFrame::Delete_Cotation( COTATION* Cotation, wxDC* DC )
/*******************************************************************/
{
    if( Cotation == NULL )
        return;

    if( DC )
        Cotation->Draw( DrawPanel, DC, wxPoint( 0, 0 ), GR_XOR );
    Cotation->DeleteStructure();
    m_CurrentScreen->SetModify();
}


/*****************************************************/
static void Ajuste_Details_Cotation( COTATION* Cotation )
/*****************************************************/

/* Calcule les details des coordonnees des differents segments constitutifs
 *  de la cotation
 */
{
    int      ii;
    int      mesure, deltax, deltay;            /* valeur de la mesure sur les axes X et Y */
    int      fleche_up_X = 0, fleche_up_Y = 0;  /* coord des fleches : barre / */
    int      fleche_dw_X = 0, fleche_dw_Y = 0;  /* coord des fleches : barre \ */
    int      hx, hy;                            /* coord des traits de rappel de cote */
    float    angle, angle_f;
    wxString msg;

    /* Init des couches : */
    Cotation->m_Text->SetLayer( Cotation->GetLayer() );

    /* calcul de la hauteur du texte + trait de cotation */
    ii = Cotation->m_Text->m_Size.y +
         Cotation->m_Text->m_Width + (Cotation->m_Width * 3);

    deltax = Cotation->TraitD_ox - Cotation->TraitG_ox;
    deltay = Cotation->TraitD_oy - Cotation->TraitG_oy;

    /* Calcul de la cote */
    mesure = (int) (hypot( (float) deltax, (float) deltay ) + 0.5 );

    if( deltax || deltay )
        angle = atan2( (float) deltay, (float) deltax );
    else
        angle = 0.0;

    /* Calcul des parametre dimensions X et Y des fleches et traits de cotes */
    hx = hy = ii;

    /* On tient compte de l'inclinaison de la cote */
    if( mesure )
    {
        hx = (abs) ( (int) ( ( (float) deltay * hx ) / mesure ) );
        hy = (abs) ( (int) ( ( (float) deltax * hy ) / mesure ) );

        if( Cotation->TraitG_ox > Cotation->Barre_ox )
            hx = -hx;
        if( Cotation->TraitG_ox == Cotation->Barre_ox )
            hx = 0;
        if( Cotation->TraitG_oy > Cotation->Barre_oy )
            hy = -hy;
        if( Cotation->TraitG_oy == Cotation->Barre_oy )
            hy = 0;

        angle_f     = angle + (M_PI * 27.5 / 180);
        fleche_up_X = (int) ( FLECHE_L * cos( angle_f ) );
        fleche_up_Y = (int) ( FLECHE_L * sin( angle_f ) );
        angle_f     = angle - (M_PI * 27.5 / 180);
        fleche_dw_X = (int) ( FLECHE_L * cos( angle_f ) );
        fleche_dw_Y = (int) ( FLECHE_L * sin( angle_f ) );
    }


    Cotation->FlecheG1_ox = Cotation->Barre_ox;
    Cotation->FlecheG1_oy = Cotation->Barre_oy;
    Cotation->FlecheG1_fx = Cotation->Barre_ox + fleche_up_X;
    Cotation->FlecheG1_fy = Cotation->Barre_oy + fleche_up_Y;

    Cotation->FlecheG2_ox = Cotation->Barre_ox;
    Cotation->FlecheG2_oy = Cotation->Barre_oy;
    Cotation->FlecheG2_fx = Cotation->Barre_ox + fleche_dw_X;
    Cotation->FlecheG2_fy = Cotation->Barre_oy + fleche_dw_Y;

    /*la fleche de droite est symetrique a celle de gauche:
     *  / = -\  et  \ = -/
     */
    Cotation->FlecheD1_ox = Cotation->Barre_fx;
    Cotation->FlecheD1_oy = Cotation->Barre_fy;
    Cotation->FlecheD1_fx = Cotation->Barre_fx - fleche_dw_X;
    Cotation->FlecheD1_fy = Cotation->Barre_fy - fleche_dw_Y;

    Cotation->FlecheD2_ox = Cotation->Barre_fx;
    Cotation->FlecheD2_oy = Cotation->Barre_fy;
    Cotation->FlecheD2_fx = Cotation->Barre_fx - fleche_up_X;
    Cotation->FlecheD2_fy = Cotation->Barre_fy - fleche_up_Y;


    Cotation->TraitG_fx = Cotation->Barre_ox + hx;
    Cotation->TraitG_fy = Cotation->Barre_oy + hy;

    Cotation->TraitD_fx = Cotation->Barre_fx + hx;
    Cotation->TraitD_fy = Cotation->Barre_fy + hy;

    /* Calcul de la position du centre du texte et son orientation: */
    Cotation->m_Pos.x   = Cotation->m_Text->m_Pos.x
                          = (Cotation->Barre_fx + Cotation->TraitG_fx) / 2;
    Cotation->m_Pos.y   = Cotation->m_Text->m_Pos.y
                          = (Cotation->Barre_fy + Cotation->TraitG_fy) / 2;

    Cotation->m_Text->m_Orient = -(int) (angle * 1800 / M_PI);
    if( Cotation->m_Text->m_Orient < 0 )
        Cotation->m_Text->m_Orient += 3600;
    if( Cotation->m_Text->m_Orient >= 3600 )
        Cotation->m_Text->m_Orient -= 3600;
    if( (Cotation->m_Text->m_Orient > 900) && (Cotation->m_Text->m_Orient <2700) )
        Cotation->m_Text->m_Orient -= 1800;

    Cotation->m_Value = mesure;
    valeur_param( Cotation->m_Value, msg );
    Cotation->SetText( msg );
    Cotation->m_Text->CreateDrawData();
}
