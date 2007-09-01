/*******************************/
/* Dialog frame to swap layers */
/*******************************/

/* Fichier swap_layers */

#include "fctsys.h"
#include "common.h"
#include "pcbnew.h"

#include "protos.h"

/* Variables locales */
static int New_Layer[32];

enum swap_layer_id {
    ID_SWAP_LAYER_EXECUTE = 1800,
    ID_SWAP_LAYER_CANCEL,
    ID_SWAP_LAYER_BUTTON_SELECT,
    ID_SWAP_LAYER_DESELECT,
    ID_SWAP_LAYER_SELECT
};


/***********************************************/
/* classe pour la frame de selection de layers */
/***********************************************/

class WinEDA_SwapLayerFrame : public wxDialog
{
private:
    WinEDA_BasePcbFrame* m_Parent;
    wxRadioBox*          m_LayerList;

public:

    // Constructor and destructor
    WinEDA_SwapLayerFrame( WinEDA_BasePcbFrame * parent );
    ~WinEDA_SwapLayerFrame() { };

private:
    void    Sel_Layer( wxCommandEvent& event );
    void    Cancel( wxCommandEvent& event );
    void    Execute( wxCommandEvent& event );

    DECLARE_EVENT_TABLE()
};
/* Table des evenements pour WinEDA_SwapLayerFrame */
BEGIN_EVENT_TABLE( WinEDA_SwapLayerFrame, wxDialog )
EVT_BUTTON( ID_SWAP_LAYER_EXECUTE, WinEDA_SwapLayerFrame::Execute )
EVT_BUTTON( ID_SWAP_LAYER_CANCEL, WinEDA_SwapLayerFrame::Cancel )
EVT_BUTTON( ID_SWAP_LAYER_DESELECT, WinEDA_SwapLayerFrame::Sel_Layer )
EVT_BUTTON( ID_SWAP_LAYER_BUTTON_SELECT, WinEDA_SwapLayerFrame::Sel_Layer )
EVT_RADIOBOX( ID_SWAP_LAYER_SELECT, WinEDA_SwapLayerFrame::Sel_Layer )
END_EVENT_TABLE()


/*************************************************************************/
WinEDA_SwapLayerFrame::WinEDA_SwapLayerFrame( WinEDA_BasePcbFrame* parent ) :
    wxDialog( parent, -1, _( "Swap Layers:" ), wxPoint( -1, -1 ),
              wxSize( 470, 450 ), DIALOG_STYLE )
/*************************************************************************/
{
#define START_Y 15
    wxButton* Button;
    int       ii;
    wxPoint   pos;
    wxString  g_Layer_Name_Pair[32];
    wxSize    winsize;

    m_Parent = parent;
    SetFont( *g_DialogFont );

    for( ii = 0; ii < NB_LAYERS; ii++ )
    {
        g_Layer_Name_Pair[ii] = ReturnPcbLayerName( ii ) + wxT( " -> " ) + _( "No Change" );
    }

    pos.x = 5; pos.y = START_Y;
    m_LayerList = new wxRadioBox( this, ID_SWAP_LAYER_SELECT, _( "Layers" ),
                                  pos,
                                  wxSize( -1, -1 ), 29, g_Layer_Name_Pair, 16, wxRA_SPECIFY_ROWS );

    winsize.y = m_LayerList->GetRect().GetBottom();

    pos.x  = m_LayerList->GetRect().GetRight() + 12;
    Button = new wxButton( this, ID_SWAP_LAYER_CANCEL,
                           _( "Cancel" ), pos );

    Button->SetForegroundColour( *wxRED );
    winsize.x = MAX( winsize.x, Button->GetRect().GetRight() );

    pos.y += Button->GetSize().y + 5;
    Button = new wxButton( this, ID_SWAP_LAYER_EXECUTE,
                           _( "OK" ), pos );

    Button->SetForegroundColour( *wxBLUE );
    winsize.x = MAX( winsize.x, Button->GetRect().GetRight() );

    pos.y += Button->GetSize().y + 15;
    Button = new wxButton( this, ID_SWAP_LAYER_DESELECT,
                           _( "Deselect" ), pos );

    Button->SetForegroundColour( wxColour( 0, 100, 0 ) );
    winsize.x = MAX( winsize.x, Button->GetRect().GetRight() );

    pos.y += Button->GetSize().y + 5;
    Button = new wxButton( this, ID_SWAP_LAYER_BUTTON_SELECT,
                           _( "Select" ), pos );

    Button->SetForegroundColour( wxColour( 0, 100, 100 ) );
    winsize.x = MAX( winsize.x, Button->GetRect().GetRight() );

    winsize.x += 10; winsize.y += 10;
    SetClientSize( winsize );
}


/***************************************************************/
void WinEDA_SwapLayerFrame::Sel_Layer( wxCommandEvent& event )
/***************************************************************/
{
    int ii, jj;

    ii = m_LayerList->GetSelection();

    switch( event.GetId() )
    {
    case ID_SWAP_LAYER_DESELECT:
        if( New_Layer[ii] != -1 )
        {
            New_Layer[ii] = -1;
            m_LayerList->SetString( ii, ReturnPcbLayerName( ii ) +
                                   + wxT( " -> " ) + _( "No Change" ) );
        }
        break;

    case ID_SWAP_LAYER_BUTTON_SELECT:
    case ID_SWAP_LAYER_SELECT:
        jj = m_Parent->SelectLayer( ii, -1, -1 );
        if( (jj < 0) || (jj >= 29) )
            return;

        if( ii != jj )
        {
            New_Layer[ii] = jj;
            m_LayerList->SetString( ii,
                                   ReturnPcbLayerName( ii ) + wxT( " -> " ) +
                                   ReturnPcbLayerName( jj ) );
        }
        break;
    }
}


/*********************************************************/
void WinEDA_SwapLayerFrame::Cancel( wxCommandEvent& event )
/*********************************************************/
{
    EndModal( -1 );
}


/*********************************************************/
void WinEDA_SwapLayerFrame::Execute( wxCommandEvent& event )
/*********************************************************/
{
    EndModal( 1 );
}


/********************************************************/
void WinEDA_PcbFrame::Swap_Layers( wxCommandEvent& event )
/********************************************************/
/* Swap layers */
{
    int             ii, jj;
    TRACK*          pt_segm;
    DRAWSEGMENT*    pt_drawsegm;
    EDA_BaseStruct* PtStruct;


    /* Init default values */
    for( ii = 0; ii < 32; ii++ )
        New_Layer[ii] = -1;

    WinEDA_SwapLayerFrame* frame = new WinEDA_SwapLayerFrame( this );

    ii = frame->ShowModal(); frame->Destroy();

    if( ii != 1 )
        return;

    /* Modifications des pistes */
    pt_segm = (TRACK*) m_Pcb->m_Track;
    for( ; pt_segm != NULL; pt_segm = (TRACK*) pt_segm->Pnext )
    {
        m_CurrentScreen->SetModify();
        if( pt_segm->Type() == TYPEVIA )
        {
            SEGVIA* Via = (SEGVIA*) pt_segm;
            if( Via->Shape() == VIA_NORMALE )
                continue;
            int     top_layer, bottom_layer;
            Via->ReturnLayerPair( &top_layer, &bottom_layer );
            if( New_Layer[bottom_layer] >= 0 )
                bottom_layer = New_Layer[bottom_layer];
            if( New_Layer[top_layer] >= 0 )
                top_layer = New_Layer[top_layer];
            Via->SetLayerPair( top_layer, bottom_layer );
        }
        else
        {
            jj = pt_segm->GetLayer();
            if( New_Layer[jj] >= 0 )
                pt_segm->SetLayer( New_Layer[jj] );
        }
    }

    /* Modifications des zones */
    pt_segm = (TRACK*) m_Pcb->m_Zone;
    for( ; pt_segm != NULL; pt_segm = (TRACK*) pt_segm->Pnext )
    {
        m_CurrentScreen->SetModify();
        jj = pt_segm->GetLayer();
        if( New_Layer[jj] >= 0 )
            pt_segm->SetLayer( New_Layer[jj] );
    }

    /* Modifications des autres segments */
    PtStruct = m_Pcb->m_Drawings;
    for( ; PtStruct != NULL; PtStruct = PtStruct->Pnext )
    {
        if( PtStruct->Type() == TYPEDRAWSEGMENT )
        {
            m_CurrentScreen->SetModify();
            pt_drawsegm = (DRAWSEGMENT*) PtStruct;
            jj = pt_drawsegm->GetLayer();
            if( New_Layer[jj] >= 0 )
                pt_drawsegm->SetLayer( New_Layer[jj] );
        }
    }

    DrawPanel->Refresh( TRUE );
}
