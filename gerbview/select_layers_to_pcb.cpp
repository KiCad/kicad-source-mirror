		/*******************************************************/
		/* Dialog frame to choose gerber layers and pcb layers */
		/*******************************************************/

	/* select_layers_to_pcb.cpp*/

#include "fctsys.h"
#include "common.h"
#include "gerbview.h"

#include "protos.h"


/* Variables locales */
static int LayerLookUpTable[32];

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

class WinEDA_SwapLayerFrame: public wxDialog
{
private:
	WinEDA_GerberFrame *m_Parent;
	wxRadioBox * m_LayerList;

public:

	// Constructor and destructor
	WinEDA_SwapLayerFrame(WinEDA_GerberFrame *parent);
	~WinEDA_SwapLayerFrame(void) {};

private:
	void Sel_Layer(wxCommandEvent& event);
	void Cancel(wxCommandEvent& event);
	void Execute(wxCommandEvent& event);
	DECLARE_EVENT_TABLE()

};
/* Table des evenements pour WinEDA_SwapLayerFrame */
BEGIN_EVENT_TABLE(WinEDA_SwapLayerFrame, wxDialog)
	EVT_BUTTON(ID_SWAP_LAYER_EXECUTE, WinEDA_SwapLayerFrame::Execute)
	EVT_BUTTON(ID_SWAP_LAYER_CANCEL, WinEDA_SwapLayerFrame::Cancel)
	EVT_BUTTON(ID_SWAP_LAYER_DESELECT, WinEDA_SwapLayerFrame::Sel_Layer)
	EVT_BUTTON(ID_SWAP_LAYER_BUTTON_SELECT, WinEDA_SwapLayerFrame::Sel_Layer)
	EVT_RADIOBOX(ID_SWAP_LAYER_SELECT, WinEDA_SwapLayerFrame::Sel_Layer)
END_EVENT_TABLE()


/*************************************************************/
int * InstallDialogLayerPairChoice(WinEDA_GerberFrame * parent)
/*************************************************************/
/* Install a dialog frame to choose the equivalence
	between gerber layers and pcbnew layers
	return the "lookup table" if ok, or NULL
*/
{
	WinEDA_SwapLayerFrame * frame = new WinEDA_SwapLayerFrame(parent);
		int ii = frame->ShowModal(); frame->Destroy();
		if ( ii >= 0 ) return LayerLookUpTable;
		else return NULL;
}

/*************************************************************************/
WinEDA_SwapLayerFrame::WinEDA_SwapLayerFrame(WinEDA_GerberFrame *parent):
			wxDialog(parent, -1, _("Layer selection:"),wxPoint(-1,-1),
					wxDefaultSize, DIALOG_STYLE )
/*************************************************************************/
{
wxButton * Button;
int ii, nb_items;
wxString g_Layer_Name_Pair[32];
	
	m_Parent = parent;
	SetFont(*g_DialogFont);

	/* Compute a resonnable number of copper layers */
	g_DesignSettings.m_CopperLayerCount = 0;
	for ( ii = 0; ii < NB_LAYERS; ii++ )
	{
		if ( g_GERBER_Descr_List[ii] != NULL)
			g_DesignSettings.m_CopperLayerCount++;
	}

	int pcb_layer_number = 0;
	for ( nb_items = 0, ii = 0; ii < NB_LAYERS; ii++ )
	{
		if ( g_GERBER_Descr_List[ii] == NULL )
		{
			LayerLookUpTable[ii] = -1;
			continue;
		}
		if ( (pcb_layer_number == g_DesignSettings.m_CopperLayerCount-1)
			 && (g_DesignSettings.m_CopperLayerCount > 1) )
			pcb_layer_number = CMP_N;
		LayerLookUpTable[ii] = pcb_layer_number;
		g_Layer_Name_Pair[ii] = _("Gerber layer ");
		g_Layer_Name_Pair[ii] << ii+1
				<< wxT(" -> ") << ReturnPcbLayerName(pcb_layer_number);
		nb_items++;
		pcb_layer_number++;
	}
	
    wxBoxSizer* FrameBoxSizer = new wxBoxSizer(wxHORIZONTAL);
    SetSizer(FrameBoxSizer);
    wxBoxSizer* LeftBoxSizer = new wxBoxSizer(wxVERTICAL);
    FrameBoxSizer->Add(LeftBoxSizer, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);
    wxBoxSizer* RightBoxSizer = new wxBoxSizer(wxVERTICAL);
    FrameBoxSizer->Add(RightBoxSizer, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

	m_LayerList = new wxRadioBox(this, ID_SWAP_LAYER_SELECT, _("Layers"),
			wxDefaultPosition, wxDefaultSize,
			nb_items, g_Layer_Name_Pair,
			nb_items < 16 ? nb_items :16,
			wxRA_SPECIFY_ROWS);
    LeftBoxSizer->Add(m_LayerList, 0, wxGROW|wxALL, 5);

	Button = new wxButton(this,ID_SWAP_LAYER_CANCEL, _("Cancel"));
	Button->SetForegroundColour(*wxRED);
    RightBoxSizer->Add(Button, 0, wxGROW|wxALL, 5);

	Button = new wxButton(this,ID_SWAP_LAYER_EXECUTE, _("OK"));
	Button->SetForegroundColour(*wxBLUE);
    RightBoxSizer->Add(Button, 0, wxGROW|wxALL, 5);

	Button = new wxButton(this,ID_SWAP_LAYER_DESELECT, _("Deselect"));
	Button->SetForegroundColour(wxColour(0,100,0));
    RightBoxSizer->Add(Button, 0, wxGROW|wxALL, 5);

	Button = new wxButton(this,ID_SWAP_LAYER_BUTTON_SELECT, _("Select"));
	Button->SetForegroundColour(wxColour(0,100,100));
    RightBoxSizer->Add(Button, 0, wxGROW|wxALL, 5);
	
   if (GetSizer())
    {
        GetSizer()->SetSizeHints(this);
    }
}


/***************************************************************/
void WinEDA_SwapLayerFrame::Sel_Layer(wxCommandEvent& event)
/***************************************************************/
{
int ii, jj;
int gerber_layer_number;
wxString msg;

	ii = m_LayerList->GetSelection();
	if ( ii < 0 ) return;
	/* Search the gerber layer number correspondint to the selection */
	for ( jj = 0, gerber_layer_number = 0; gerber_layer_number < 32; gerber_layer_number++ )
	{
		if ( g_GERBER_Descr_List[gerber_layer_number] == NULL) continue;
		if (jj == ii ) break;
		jj++;
	}
	

	switch ( event.GetId())
	{
		case ID_SWAP_LAYER_DESELECT:
			if ( LayerLookUpTable[gerber_layer_number] != -1 )
			{
				LayerLookUpTable[gerber_layer_number] = -1;
				msg = _("Gerber layer ");
				msg << gerber_layer_number+1 << wxT(" -> ") << _("Do not export");
				m_LayerList->SetString(ii, msg );
			}
			break;

		case ID_SWAP_LAYER_BUTTON_SELECT:
		case ID_SWAP_LAYER_SELECT:
			jj = m_Parent->SelectLayer(ii, -1, -1);
			if ( (jj < 0) || (jj >= 29) ) return;

			if ( ii != jj )
			{
				LayerLookUpTable[gerber_layer_number] = jj;
				msg = _("Gerber layer ");
				msg << gerber_layer_number+1 << wxT(" -> ") << ReturnPcbLayerName(jj);
				m_LayerList->SetString(ii, msg );
			}
			break;
	}
}

/*********************************************************/
void WinEDA_SwapLayerFrame::Cancel(wxCommandEvent& event)
/*********************************************************/
{
	EndModal(-1);
}

/*********************************************************/
void WinEDA_SwapLayerFrame::Execute(wxCommandEvent& event)
/*********************************************************/
{
int ii;
bool AsCmpLayer = false;
	
	/* Compute the number of copper layers
		this is the max layer number + 1 (if some internal layers exists)
	*/
	g_DesignSettings.m_CopperLayerCount = 1;
	for ( ii = 0; ii < 32; ii++ )
	{
		if ( LayerLookUpTable[ii] == CMP_N ) AsCmpLayer = true;
		else
		{
			if ( LayerLookUpTable[ii] >= CMP_N ) continue;	// not a copper layer
			if ( LayerLookUpTable[ii] >= g_DesignSettings.m_CopperLayerCount )
				g_DesignSettings.m_CopperLayerCount++;
		}
	}
	
	if ( AsCmpLayer ) g_DesignSettings.m_CopperLayerCount++;
	if ( g_DesignSettings.m_CopperLayerCount > CMP_N+1 )	// should not occur.
		g_DesignSettings.m_CopperLayerCount = CMP_N+1;

	EndModal(1);
}


