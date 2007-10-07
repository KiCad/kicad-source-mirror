		/*******************************************************/
		/* Dialog frame to choose gerber layers and pcb layers */
		/*******************************************************/

	/* select_layers_to_pcb.cpp*/

#include "fctsys.h"
#include "common.h"
#include "gerbview.h"

#include "protos.h"


/* Variables locales */
static int RadioButtonTable[32]; // Indexes radiobuttons to Gerber layers
static int LayerLookUpTable[32]; // Indexes Gerber layers to PCB file layers

enum swap_layer_id {
    ID_SWAP_LAYER_BUTTON_SELECT = 1800,
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
	~WinEDA_SwapLayerFrame() {};

private:
    void Sel_Layer(wxCommandEvent& event);
    void OnOkClick(wxCommandEvent& event);
    void OnCancelClick(wxCommandEvent& event);

    DECLARE_EVENT_TABLE()

};

/* Table des evenements pour WinEDA_SwapLayerFrame */
BEGIN_EVENT_TABLE(WinEDA_SwapLayerFrame, wxDialog)
    EVT_BUTTON(wxID_OK, WinEDA_SwapLayerFrame::OnOkClick)
    EVT_BUTTON(wxID_CANCEL, WinEDA_SwapLayerFrame::OnCancelClick)
    EVT_BUTTON(ID_SWAP_LAYER_DESELECT, WinEDA_SwapLayerFrame::Sel_Layer)
    EVT_BUTTON(ID_SWAP_LAYER_BUTTON_SELECT, WinEDA_SwapLayerFrame::Sel_Layer)
    EVT_RADIOBOX(ID_SWAP_LAYER_SELECT, WinEDA_SwapLayerFrame::Sel_Layer)
END_EVENT_TABLE()


/*************************************************************/
int * InstallDialogLayerPairChoice(WinEDA_GerberFrame * parent)
/*************************************************************/
/* Install a dialog frame to choose the equivalence
 * between gerber layers and pcbnew layers
 * return the "lookup table" if ok, or NULL
 */
{
	WinEDA_SwapLayerFrame * frame = new WinEDA_SwapLayerFrame(parent);
		int ii = frame->ShowModal();
		frame->Destroy();
		if( ii >= 0 )
			return LayerLookUpTable;
		else
			return NULL;
}


/*************************************************************************/
WinEDA_SwapLayerFrame::WinEDA_SwapLayerFrame(WinEDA_GerberFrame *parent):
			wxDialog( parent, -1, _("Layer selection:"), wxPoint(-1, -1),
					wxDefaultSize, DIALOG_STYLE )
/*************************************************************************/
{
wxButton * Button;
int ii, nb_items;
wxString g_Layer_Name_Pair[32];

	m_Parent = parent;
	SetFont(*g_DialogFont);

	// Compute a reasonable number of copper layers
	g_DesignSettings.m_CopperLayerCount = 0;
	for( ii = 0; ii < 32; ii++ )
	{
		if( g_GERBER_Descr_List[ii] != NULL )
			g_DesignSettings.m_CopperLayerCount++;

		// Specify the default value for each member of these arrays.
		RadioButtonTable[ii] = -1;
		LayerLookUpTable[ii] = NB_LAYERS; // Value associated with deselected Gerber layer
	}

	int pcb_layer_number = 0;
	for( nb_items = 0, ii = 0; ii < 32; ii++ )
	{
		if( g_GERBER_Descr_List[ii] == NULL )
			continue;

		if( (pcb_layer_number == g_DesignSettings.m_CopperLayerCount - 1)
			 && (g_DesignSettings.m_CopperLayerCount > 1) )
			pcb_layer_number = CMP_N;

		RadioButtonTable[nb_items] = ii;
		LayerLookUpTable[ii] = pcb_layer_number;

		// Specify initial (temporary) caption for associated radiobutton,
		// which will be appropriately updated after dialog box has been sized.
		// (If the radiobuttons' captions are not changed in this way, some of
		// each radiobutton's caption could be truncated if the associated
		// (Gerber) layer is ever subsequently deselected by the user.)
		g_Layer_Name_Pair[nb_items] = _("Gerber layer ");
		g_Layer_Name_Pair[nb_items] << ii + 1 << wxT(" -> ") << _("Do not export");

		nb_items++;
		pcb_layer_number++;
	}

    wxBoxSizer* FrameBoxSizer = new wxBoxSizer(wxVERTICAL);
    SetSizer(FrameBoxSizer);

    wxBoxSizer* MainBoxSizer = new wxBoxSizer(wxHORIZONTAL);
    FrameBoxSizer->Add(MainBoxSizer, 0, wxGROW|wxALIGN_LEFT|wxALL, 5);

	m_LayerList = new wxRadioBox(this, ID_SWAP_LAYER_SELECT, _("Layers"),
			wxDefaultPosition, wxDefaultSize,
			nb_items, g_Layer_Name_Pair,
			nb_items < 16 ? nb_items : 16,
			wxRA_SPECIFY_ROWS);

    // Specify a minimum size for this radiobox (with the objective
    // of attempting to prevent any radiobutton's caption from being
    // truncated if any of the layers are subsequently deselected)
    m_LayerList->SetMinSize( m_LayerList->GetSize() );

    MainBoxSizer->Add(m_LayerList, 0, wxALIGN_TOP|wxALL, 5);

    wxBoxSizer* RightBoxSizer = new wxBoxSizer(wxVERTICAL);
    MainBoxSizer->Add(RightBoxSizer, 0, wxALIGN_TOP|wxALL, 0);

	RightBoxSizer->AddSpacer(10);

	Button = new wxButton(this, ID_SWAP_LAYER_BUTTON_SELECT, _("Select..."));
	Button->SetForegroundColour(wxColour(0,100,100));
    RightBoxSizer->Add(Button, 0, wxGROW|wxALL, 5);

	Button = new wxButton(this, ID_SWAP_LAYER_DESELECT, _("Deselect"));
	Button->SetForegroundColour(wxColour(0,100,0));
    RightBoxSizer->Add(Button, 0, wxGROW|wxALL, 5);

    wxBoxSizer* BottomBoxSizer = new wxBoxSizer(wxHORIZONTAL);
    FrameBoxSizer->Add(BottomBoxSizer, 0, wxGROW|wxALIGN_RIGHT|wxALL, 5);

	// The following stretch spacer ensures that the "OK" and "Cancel" buttons
    // will be positioned at the lower right corner of the dialog box.
	BottomBoxSizer->AddStretchSpacer();

	Button = new wxButton(this, wxID_OK, _("OK"));
	Button->SetForegroundColour(*wxRED);
    BottomBoxSizer->Add(Button, 0, wxGROW|wxALL, 5);
	
	Button = new wxButton(this, wxID_CANCEL, _("Cancel"));
	Button->SetForegroundColour(*wxBLUE);
    BottomBoxSizer->Add(Button, 0, wxGROW|wxALL, 5);

    if( GetSizer() )
    {
        GetSizer()->SetSizeHints(this);
    }

    // Now specify the correct caption for each radiobutton.
	// (Regrettably though there are still problems with the Windows
    // version; captions for each radiobutton can still be truncated.) :-(
	for( ii = 0; ii < nb_items; ii++ )
	{
		g_Layer_Name_Pair[ii] = _("Gerber layer ");
		g_Layer_Name_Pair[ii] << RadioButtonTable[ii] + 1 << wxT(" -> ")
				<< ReturnPcbLayerName(LayerLookUpTable[RadioButtonTable[ii]]);

		m_LayerList->SetString( ii, g_Layer_Name_Pair[ii] );
	}
}


/***************************************************************/
void WinEDA_SwapLayerFrame::Sel_Layer(wxCommandEvent& event)
/***************************************************************/
{
int ii, jj;
// int gerber_layer_number;
wxString msg;

	ii = m_LayerList->GetSelection();
	if( ii < 0 )
		return;

	switch ( event.GetId() )
	{
	case ID_SWAP_LAYER_DESELECT:
		if( LayerLookUpTable[RadioButtonTable[ii]] != NB_LAYERS )
		{
			LayerLookUpTable[RadioButtonTable[ii]] = NB_LAYERS;
			msg = _("Gerber layer ");
			msg << RadioButtonTable[ii] + 1
					<< wxT(" -> ") << _("Do not export");
			m_LayerList->SetString( ii, msg );
		}
		break;

	case ID_SWAP_LAYER_BUTTON_SELECT:
	case ID_SWAP_LAYER_SELECT:
		jj = LayerLookUpTable[RadioButtonTable[ii]];
		if( (jj < 0) || (jj > NB_LAYERS) )
			jj = 0; // (Defaults to "Copper" layer.)
		jj = m_Parent->SelectLayer(jj, -1, -1, true);
		if( (jj < 0) || (jj > NB_LAYERS) )
			return;

		if( jj != LayerLookUpTable[RadioButtonTable[ii]] )
		{
			LayerLookUpTable[RadioButtonTable[ii]] = jj;
			msg = _("Gerber layer ");
			msg << RadioButtonTable[ii] + 1 << wxT(" -> ");
            if( jj == NB_LAYERS )
                msg << _("Do not export");
            else
                msg << ReturnPcbLayerName(jj);
			m_LayerList->SetString( ii, msg );
		}
		break;
	}
}


/*********************************************************/
void WinEDA_SwapLayerFrame::OnCancelClick(wxCommandEvent& event)
/*********************************************************/
{
    EndModal( -1 );
}


/*********************************************************/
void WinEDA_SwapLayerFrame::OnOkClick(wxCommandEvent& event)
/*********************************************************/
{
int ii;
bool AsCmpLayer = false;

	/* Compute the number of copper layers
	 * this is the max layer number + 1 (if some internal layers exist)
	 */
	g_DesignSettings.m_CopperLayerCount = 1;
	for( ii = 0; ii < 32; ii++ )
	{
		if( LayerLookUpTable[ii] == CMP_N )
			AsCmpLayer = true;
		else
		{
			if( LayerLookUpTable[ii] >= CMP_N )
				continue;	// not a copper layer
			if( LayerLookUpTable[ii] >= g_DesignSettings.m_CopperLayerCount )
				g_DesignSettings.m_CopperLayerCount++;
		}
	}

	if( AsCmpLayer )
		g_DesignSettings.m_CopperLayerCount++;
	if( g_DesignSettings.m_CopperLayerCount > CMP_N + 1 )	// should not occur.
		g_DesignSettings.m_CopperLayerCount = CMP_N + 1;

    EndModal( 1 );
}
