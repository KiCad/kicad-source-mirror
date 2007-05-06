 /* Set up the basic primitives for Layer control */

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "pcbnew.h"

#include "protos.h"


/* Variables locales */

/* Fonctions locales: */

enum layer_sel_id {
	ID_LAYER_OK = 1800,
	ID_LAYER_CANCEL,
	ID_LAYER_SELECT_TOP,
	ID_LAYER_SELECT_BOTTOM,
	ID_LAYER_SELECT
};


/***********************************************/
/* classe pour la frame de selection de layers */
/***********************************************/

class WinEDA_SelLayerFrame: public wxDialog
{
private:
	WinEDA_BasePcbFrame *m_Parent;
	wxRadioBox * m_LayerList;
	int m_LayerId[NB_LAYERS];

public:
	// Constructor and destructor
	WinEDA_SelLayerFrame(WinEDA_BasePcbFrame *parent, int default_layer,
							int min_layer, int max_layer);
	~WinEDA_SelLayerFrame(void) {};

private:
	void Sel_Layer(wxCommandEvent& event);
	void Cancel(wxCommandEvent& event);
	DECLARE_EVENT_TABLE()

};
/* Table des evenements pour WinEDA_SelLayerFrame */
BEGIN_EVENT_TABLE(WinEDA_SelLayerFrame, wxDialog)
	EVT_BUTTON(ID_LAYER_OK, WinEDA_SelLayerFrame::Sel_Layer)
	EVT_BUTTON(ID_LAYER_CANCEL, WinEDA_SelLayerFrame::Cancel)
	EVT_RADIOBOX(ID_LAYER_SELECT, WinEDA_SelLayerFrame::Sel_Layer)
END_EVENT_TABLE()

/***********************************************************************************/
int WinEDA_BasePcbFrame::SelectLayer(int default_layer, int min_layer, int max_layer)
/***********************************************************************************/
/* Install the dialog box for layer selection
	default_layer = Preselection
	min_layer = val min de layer selectionnable (-1 si pas de val mini)
	max_layer = val max de layer selectionnable (-1 si pas de val maxi)
*/
{
int layer;
	WinEDA_SelLayerFrame * frame =
			new WinEDA_SelLayerFrame(this, default_layer,min_layer, max_layer);
	layer = frame->ShowModal(); frame->Destroy();
	DrawPanel->MouseToCursorSchema();
	return layer;
}


/***********************************************************************/
WinEDA_SelLayerFrame::WinEDA_SelLayerFrame(WinEDA_BasePcbFrame *parent,
					int default_layer, int min_layer, int max_layer):
			wxDialog(parent, -1, _("Select Layer:"),wxPoint(-1,-1),
			wxSize(470, 250),
			DIALOG_STYLE )
/***********************************************************************/
{
wxButton * Button;
int ii, yy, xx;
wxPoint pos;
wxString LayerList[NB_LAYERS];
int LayerCount, LayerSelect = -1;

	m_Parent = parent;
	SetFont(*g_DialogFont);

	/* Construction de la liste des couches autorisées */
	LayerCount = 0;
	int Masque_Layer = g_TabAllCopperLayerMask[g_DesignSettings.m_CopperLayerCount-1];
	Masque_Layer += ALL_NO_CU_LAYERS;
   	for ( ii = 0; ii < NB_LAYERS ; ii ++ )
		    {
      		m_LayerId[ii] = 0;
 		    if ( (g_TabOneLayerMask[ii] & Masque_Layer) )
       		    {
       		    if ( min_layer > ii ) continue;
       		    if ( (max_layer >= 0) && (max_layer < ii) ) break;
           	    LayerList[LayerCount] = ReturnPcbLayerName(ii);
      		    if ( ii == default_layer ) LayerSelect = LayerCount;
 				m_LayerId[LayerCount] = ii;
       		    LayerCount++;
               }
            }

	pos.x = 5; pos.y = 5;

	m_LayerList = new wxRadioBox(this, ID_LAYER_SELECT, _("Layer"),
			pos, wxSize(-1,-1), LayerCount, LayerList,
			(LayerCount < 8) ? LayerCount : 8, wxRA_SPECIFY_ROWS);

	if ( LayerSelect >= 0 ) m_LayerList->SetSelection(LayerSelect);

	m_LayerList->GetSize(&xx, &yy);
	pos.x += xx + 12;
	Button = new wxButton(this,ID_LAYER_OK,
						_("OK"), pos);
	Button->SetForegroundColour(*wxBLUE);

	pos.y += Button->GetSize().y + 5;
	Button = new wxButton(this,ID_LAYER_CANCEL,
						_("Cancel"), pos);
	Button->SetForegroundColour(*wxRED);

    /* Redimensionnement de la boite de dialogue: */
	pos.x += Button->GetSize().x + 10;
	SetSize(-1, -1, pos.x , yy + 35);
}


/***************************************************************/
void WinEDA_SelLayerFrame::Sel_Layer(wxCommandEvent& event)
/***************************************************************/
{
int ii = m_LayerId[m_LayerList->GetSelection()];
	EndModal(ii);
}

/***************************************************************/
void WinEDA_SelLayerFrame::Cancel(wxCommandEvent& event)
/***************************************************************/
{
	EndModal(-1);
}


/*********************************************************/
/* classe pour la frame de selection de paires de layers */
/*********************************************************/

class WinEDA_SelLayerPairFrame: public wxDialog
{
private:
	WinEDA_BasePcbFrame *m_Parent;
	wxRadioBox * m_LayerListTOP;
	wxRadioBox * m_LayerListBOTTOM;
	int m_LayerId[NB_COPPER_LAYERS];

public:

	// Constructor and destructor
	WinEDA_SelLayerPairFrame(WinEDA_BasePcbFrame *parent);
	~WinEDA_SelLayerPairFrame(void) {};

private:
	void Sel_Layer(wxCommandEvent& event);
	void Cancel(wxCommandEvent& event);
	DECLARE_EVENT_TABLE()

};
/* Table des evenements pour WinEDA_SelLayerPairFrame */
BEGIN_EVENT_TABLE(WinEDA_SelLayerPairFrame, wxDialog)
	EVT_BUTTON(ID_LAYER_OK, WinEDA_SelLayerPairFrame::Sel_Layer)
	EVT_BUTTON(ID_LAYER_CANCEL, WinEDA_SelLayerPairFrame::Cancel)
	EVT_RADIOBOX(ID_LAYER_SELECT, WinEDA_SelLayerPairFrame::Sel_Layer)
END_EVENT_TABLE()

/***********************************************/
void WinEDA_BasePcbFrame::SelectLayerPair(void)
/***********************************************/
/* Affiche une double liste de layers cuivre pour selection d'une paire de layers
	pour autorutage, vias...
*/
{
	WinEDA_SelLayerPairFrame * frame =
			new WinEDA_SelLayerPairFrame(this);
	frame->ShowModal(); frame->Destroy();
	DrawPanel->MouseToCursorSchema();
}


/*******************************************************************************/
WinEDA_SelLayerPairFrame::WinEDA_SelLayerPairFrame(WinEDA_BasePcbFrame *parent):
			wxDialog(parent, -1, _("Select Layer Pair:"),wxPoint(-1,-1),
			wxSize(470, 250), DIALOG_STYLE )
/*******************************************************************************/
{
wxButton * Button;
int ii, LayerCount;
int yy, xx;
wxPoint pos;
wxString LayerList[NB_COPPER_LAYERS];
int LayerTopSelect = 0, LayerBottomSelect = 0 ;

	m_Parent = parent;
	SetFont(*g_DialogFont);

PCB_SCREEN * screen = (PCB_SCREEN *) m_Parent->m_CurrentScreen;
	/* Construction de la liste des couches autorisées */
	int Masque_Layer = g_TabAllCopperLayerMask[g_DesignSettings.m_CopperLayerCount-1];
	Masque_Layer += ALL_NO_CU_LAYERS;
   	for ( ii = 0, LayerCount = 0; ii < NB_COPPER_LAYERS ; ii ++ )
		    {
      		m_LayerId[ii] = 0;
 		    if ( (g_TabOneLayerMask[ii] & Masque_Layer) )
       		    {
           	    LayerList[LayerCount] = ReturnPcbLayerName(ii);
      		    if ( ii == screen->m_Route_Layer_TOP )
					LayerTopSelect = LayerCount;
      		    if ( ii == screen->m_Route_Layer_BOTTOM )
					LayerBottomSelect = LayerCount;
				m_LayerId[LayerCount] = ii;
       		    LayerCount++;
               }
            }

	pos.x = 5; pos.y = 5;
	m_LayerListTOP = new wxRadioBox(this, ID_LAYER_SELECT_TOP, _("Top Layer"),
			pos, wxSize(-1,-1), LayerCount, LayerList,
			(LayerCount < 8) ? LayerCount : 8, wxRA_SPECIFY_ROWS);
	m_LayerListTOP->SetSelection(LayerTopSelect);

	m_LayerListTOP->GetSize(&xx, &yy);
	pos.x += xx + 12;
	m_LayerListBOTTOM = new wxRadioBox(this, ID_LAYER_SELECT_BOTTOM, _("Bottom Layer"),
			pos, wxSize(-1,-1), LayerCount, LayerList,
			(LayerCount < 8) ? LayerCount : 8, wxRA_SPECIFY_ROWS);
	m_LayerListBOTTOM->SetSelection(LayerBottomSelect);

	m_LayerListBOTTOM->GetSize(&xx, &yy);
	pos.x += xx + 12;
	Button = new wxButton(this,ID_LAYER_OK,
						_("OK"), pos);
	Button->SetForegroundColour(*wxBLUE);

	pos.y += Button->GetSize().y + 5;
	Button = new wxButton(this,ID_LAYER_CANCEL,
						_("Cancel"), pos);
	Button->SetForegroundColour(*wxRED);

    /* Redimensionnement de la boite de dialogue: */
	pos.x += Button->GetSize().x + 10;
	SetSize(-1, -1, pos.x , yy + 35);
}


/***************************************************************/
void WinEDA_SelLayerPairFrame::Sel_Layer(wxCommandEvent& event)
/***************************************************************/
{
PCB_SCREEN * screen = (PCB_SCREEN *) m_Parent->m_CurrentScreen;

	screen->m_Route_Layer_TOP = m_LayerId[m_LayerListTOP->GetSelection()];
	screen->m_Route_Layer_BOTTOM = m_LayerId[m_LayerListBOTTOM->GetSelection()];
	EndModal(0);
}

/***************************************************************/
void WinEDA_SelLayerPairFrame::Cancel(wxCommandEvent& event)
/***************************************************************/
{
	EndModal(-1);
}


