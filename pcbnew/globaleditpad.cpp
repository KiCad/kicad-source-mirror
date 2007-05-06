	/*************************/
	/* Edition des Pastilles */
	/*************************/

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "pcbnew.h"
#include "autorout.h"
#include "trigo.h"

#include "drag.h"

#include "protos.h"

/* Variables Locales */
static bool Pad_Shape_Filter = TRUE;
static bool Pad_Layer_Filter = TRUE;
static bool Pad_Orient_Filter = TRUE;
static bool Pad_Size_Change = TRUE;
static bool Pad_Shape_Change = FALSE;
static bool Pad_Orient_Change = FALSE;
static bool Pad_Drill_Change = TRUE;

enum id_pad_global_edit
{
	ID_CHANGE_CURRENT_MODULE = 1900,
	ID_CHANGE_ID_MODULES,
	ID_CHANGE_GET_PAD_SETTINGS,
	ID_CLOSE_PAD_GLOBAL_CHANGE
};


	/************************************/
	/* class WinEDA_PadGlobalEditFrame */
	/************************************/

class WinEDA_PadGlobalEditFrame: public wxDialog
{
private:

	WinEDA_BasePcbFrame * m_Parent;
	wxDC * m_DC;
	D_PAD * CurrentPad;
	wxCheckBox * m_Pad_Shape_Filter;
	wxCheckBox * m_Pad_Layer_Filter;
	wxCheckBox * m_Pad_Orient_Filter;
	wxCheckBox * m_Pad_Size_Change;
	wxCheckBox * m_Pad_Shape_Change;
	wxCheckBox * m_Pad_Drill_Change;
	wxCheckBox * m_Pad_Orient_Change;

public:
	// Constructor and destructor
	WinEDA_PadGlobalEditFrame(WinEDA_BasePcbFrame *parent,
							D_PAD * Pad, wxDC * DC, const wxPoint & pos);
	~WinEDA_PadGlobalEditFrame(void)
		{
		}

private:
	void PadPropertiesAccept(wxCommandEvent& event);
	void OnQuit(wxCommandEvent& event);

	DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(WinEDA_PadGlobalEditFrame, wxDialog)
	EVT_BUTTON(ID_CHANGE_CURRENT_MODULE, WinEDA_PadGlobalEditFrame::PadPropertiesAccept)
	EVT_BUTTON(ID_CHANGE_ID_MODULES, WinEDA_PadGlobalEditFrame::PadPropertiesAccept)
	EVT_BUTTON(ID_CHANGE_GET_PAD_SETTINGS, WinEDA_PadGlobalEditFrame::PadPropertiesAccept)
	EVT_BUTTON(ID_CLOSE_PAD_GLOBAL_CHANGE, WinEDA_PadGlobalEditFrame::OnQuit)
END_EVENT_TABLE()


/********************************************************************************/
WinEDA_PadGlobalEditFrame::WinEDA_PadGlobalEditFrame(WinEDA_BasePcbFrame *parent,
				D_PAD * Pad,wxDC * DC, const wxPoint & framepos):
		wxDialog(parent, -1, _("Pads Global Edit"), framepos, wxSize(310, 235),
				DIALOG_STYLE)
/********************************************************************************/
{
wxPoint pos;
wxButton * Button;

	m_Parent = parent;
	SetFont(*g_DialogFont);
	m_DC = DC;
	Centre();

	CurrentPad = Pad;

	/* Creation des boutons de commande */
	pos.x = 150; pos.y = 10;
	Button = new wxButton(this, ID_CHANGE_GET_PAD_SETTINGS,
						_("Pad Settings"), pos);
	Button->SetForegroundColour(wxColor(0,80,0));

	pos.y += Button->GetDefaultSize().y + 10;
	Button = new wxButton(this, ID_CHANGE_CURRENT_MODULE,
						_("Change Module"), pos);
	Button->SetForegroundColour(*wxRED);

	pos.y += Button->GetDefaultSize().y + 10;
	Button = new wxButton(this, ID_CHANGE_ID_MODULES,
						_("Change Id Modules"), pos);
	Button->SetForegroundColour(*wxRED);

	pos.y += Button->GetDefaultSize().y + 10;
	Button = new wxButton(this, ID_CLOSE_PAD_GLOBAL_CHANGE,
						_("Cancel"), pos);
	Button->SetForegroundColour(*wxBLUE);

	pos.x = 5; pos.y = 5;

	// Selection des filtres de selection des pads :
	new wxStaticBox(this, -1, _("Pad Filter :"), pos, wxSize(130, 75) );
	pos.x += 5; pos.y += 18;
	m_Pad_Shape_Filter = new wxCheckBox(this, -1, _("Shape Filter"), pos);
	m_Pad_Shape_Filter->SetValue(Pad_Shape_Filter);

	pos.y += 18;
	m_Pad_Layer_Filter = new wxCheckBox(this, -1, _("Layer Filter"), pos);
	m_Pad_Layer_Filter->SetValue(Pad_Layer_Filter);

	pos.y += 18;
	m_Pad_Orient_Filter = new wxCheckBox(this, -1, _("Orient Filter"), pos);
	m_Pad_Orient_Filter->SetValue(Pad_Orient_Filter);

	// Items a editer
	pos.x -= 5; pos.y += 25;
	new wxStaticBox(this, -1, _("Change Items :"), pos, wxSize(130, 95) );
	pos.x += 5; pos.y += 18;
	m_Pad_Size_Change = new wxCheckBox(this, -1, _("Change Size"), pos);
	m_Pad_Size_Change->SetValue(Pad_Size_Change);

	pos.y += 18;
	m_Pad_Shape_Change = new wxCheckBox(this, -1, _("Change Shape"), pos);
	m_Pad_Shape_Change->SetValue(Pad_Shape_Change);

	pos.y += 18;
	m_Pad_Drill_Change = new wxCheckBox(this, -1, _("Change Drill"), pos);
	m_Pad_Drill_Change->SetValue(Pad_Drill_Change);

	pos.y += 18;
	m_Pad_Orient_Change = new wxCheckBox(this, -1, _("Change Orient"), pos);
	m_Pad_Orient_Change->SetValue(Pad_Orient_Change);

}

/**********************************************************************/
void  WinEDA_PadGlobalEditFrame::OnQuit(wxCommandEvent& WXUNUSED(event))
/**********************************************************************/
{
    Close(true);    // true is to force the frame to close
}



/*************************************************************************/
void WinEDA_PadGlobalEditFrame::PadPropertiesAccept(wxCommandEvent& event)
/*************************************************************************/
/* Met a jour les differents parametres pour le composant en cours d'édition
*/
{
int returncode = 1;

	switch ( event.GetId() )
		{
		case ID_CLOSE_PAD_GLOBAL_CHANGE:
			EndModal(0);

		case ID_CHANGE_GET_PAD_SETTINGS:
			m_Parent->InstallPadOptionsFrame(NULL, NULL, wxPoint(-1, -1) );
			break;

		case ID_CHANGE_ID_MODULES:
			returncode = -1;
		case ID_CHANGE_CURRENT_MODULE:
			Pad_Shape_Filter = m_Pad_Shape_Filter->GetValue();
			Pad_Layer_Filter = m_Pad_Layer_Filter->GetValue();
			Pad_Orient_Filter = m_Pad_Orient_Filter->GetValue();
			Pad_Size_Change = m_Pad_Size_Change->GetValue();
			Pad_Shape_Change = m_Pad_Shape_Change->GetValue();
			Pad_Drill_Change = m_Pad_Drill_Change->GetValue();
			Pad_Orient_Change = m_Pad_Orient_Change->GetValue();
			EndModal(returncode);
			break;

		}
}



/***************************************************************************/
void WinEDA_BasePcbFrame::Global_Import_Pad_Settings( D_PAD * Pad, wxDC * DC)
/***************************************************************************/
/*
Routine de selection et de correction des dimensions des pastilles
de tous les modules
	- semblables a l'module de reference selectionnee,
			c.a.d de meme nom de librairie
	- ou sur l'module localisee, selon le menu d'appel
*/
{
D_PAD* pt_pad ;
MODULE * Module_Ref , * Module;
int diag;
bool Edit_Same_Modules = FALSE;

	if( Pad == NULL ) return ;

	Module = (MODULE *) Pad->m_Parent;

	if ( Module == NULL )
	{
		DisplayError(this, wxT("Global_Import_Pad_Settings() Error: NULL module"));
		return;
	}

wxString ref_name_module = Module->m_LibRef;

	Module->Display_Infos(this);

WinEDA_PadGlobalEditFrame * frame = new WinEDA_PadGlobalEditFrame(this, Pad,DC,
				wxPoint(-1,-1));
	diag = frame->ShowModal(); frame->Destroy();

	if ( (diag != 1) && (diag != -1) ) return;
	if ( diag == -1 ) Edit_Same_Modules = TRUE;

	/* Recherche et copie du nom librairie de reference: */
	Module_Ref = Module;

	/* Mise a jour des modules ou du module */

	Module = (MODULE*) m_Pcb->m_Modules;
	for ( ; Module != NULL ; Module = (MODULE*) Module->Pnext )
	{
		if( ! Edit_Same_Modules )
			if (Module != Module_Ref) continue;

		if( ref_name_module != Module->m_LibRef )
				continue ;

		Module->Display_Infos(this);

		/* Effacement du module */
		Module->Draw(DrawPanel, DC, wxPoint(0,0), GR_XOR);

		pt_pad = (D_PAD*) Module->m_Pads;
		for( ; pt_pad != NULL ; pt_pad = (D_PAD*) pt_pad->Pnext)
		{
			/* Filtrage des modifications interdites */
			if( Pad_Shape_Filter )
			{
				if (pt_pad->m_PadShape != g_Pad_Master.m_PadShape) continue;
			}

			if( Pad_Orient_Filter )
			{
				if ( (pt_pad->m_Orient - Module->m_Orient) != g_Pad_Master.m_Orient)
					continue;
			}

			if( Pad_Layer_Filter )
			{
				if (pt_pad->m_Masque_Layer != g_Pad_Master.m_Masque_Layer) continue;
				else m_Pcb->m_Status_Pcb &= ~( LISTE_CHEVELU_OK | CONNEXION_OK);
			}

			/* Modif des caracteristiques : */
			if( Pad_Shape_Change )
			{
				pt_pad->m_Attribut = g_Pad_Master.m_Attribut ;
				pt_pad->m_PadShape = g_Pad_Master.m_PadShape ;
			}

			pt_pad->m_Masque_Layer = g_Pad_Master.m_Masque_Layer;

			if( Pad_Size_Change )
			{
				pt_pad->m_Size = g_Pad_Master.m_Size;
				pt_pad->m_DeltaSize = g_Pad_Master.m_DeltaSize;
				pt_pad->m_Offset = g_Pad_Master.m_Offset;
			}

			if( Pad_Drill_Change )
			{
				pt_pad->m_Drill = g_Pad_Master.m_Drill;
				pt_pad->m_DrillShape = g_Pad_Master.m_DrillShape;
			}

			if( Pad_Orient_Change )
			{
				pt_pad->m_Orient = g_Pad_Master.m_Orient + Module->m_Orient;
			}
			/* Traitement des cas particuliers : */
			if( g_Pad_Master.m_PadShape != TRAPEZE )
			{
				pt_pad->m_DeltaSize.x = 0;
				pt_pad->m_DeltaSize.y = 0;
			}
			if( g_Pad_Master.m_PadShape == CIRCLE )
				pt_pad->m_Size.y = pt_pad->m_Size.x;

			switch( g_Pad_Master.m_Attribut & 0x7F)
			{
				case SMD:
				case CONN :
				pt_pad->m_Drill = wxSize(0,0);
				pt_pad->m_Offset.x = 0;
				pt_pad->m_Offset.y = 0;
			}

			pt_pad->ComputeRayon();
		}
		Module->Set_Rectangle_Encadrement();
		Module->Draw(DrawPanel, DC, wxPoint(0,0), GR_OR);
	}

	GetScreen()->SetModify();

}



