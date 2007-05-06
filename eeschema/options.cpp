	/*************************/
	/* eeschema: options.cpp */
	/*************************/

/*
Gestion de la fenetre des options generales:
	Grille
	Unites d'affichage
	options complementaires (increments X, Y et label pour la fct de repetition
*/

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "program.h"
#include "libcmp.h"
#include "general.h"

#include "id.h"
#include "wx/spinctrl.h"

#include "protos.h"


/* Variables locales */

/* Fonctions locales: */

enum options_id {
	ID_OPTION_ACCEPT = 3900,
	ID_OPTION_CANCEL,
	ID_GRID_SIZE,
	ID_SHOW_GRID,
	ID_GRID_NORMAL,
	ID_GRID_SMALL,
	ID_GRID_VERY_SMALL,

	ID_SEL_SHOW_PINS,
	ID_SEL_HV_WIRE,
	ID_SEL_SHOW_PAGE_LIMITS,
	ID_SEL_METRIC
};

/*************************************************************/
/* classe derivee pour la frame de Configuration des options*/
/*************************************************************/

class WinEDA_SetOptionsFrame: public wxDialog
{
private:
	WinEDA_DrawFrame * m_Parent;
	wxCheckBox * m_ShowGridButt;
	wxCheckBox * m_AutoPANOpt;
	wxRadioBox * m_SelGridSize;
	wxRadioBox * m_SelShowPins;
	wxRadioBox * m_Selunits;
	wxRadioBox * m_SelDirWires;
	wxRadioBox * m_Show_Page_Limits;
	WinEDA_SizeCtrl * m_DeltaStepCtrl;
	wxSpinCtrl * m_DeltaLabelCtrl;

public:

	// Constructor and destructor
	WinEDA_SetOptionsFrame(WinEDA_DrawFrame *parent, const wxPoint& framepos);
	~WinEDA_SetOptionsFrame(void) {};

private:
	void Accept(wxCommandEvent& event);
	void OnCancel(wxCommandEvent& event);

	DECLARE_EVENT_TABLE()

};
/* Table des evenements pour WinEDA_SetOptionsFrame */
BEGIN_EVENT_TABLE(WinEDA_SetOptionsFrame, wxDialog)
	EVT_BUTTON(ID_OPTION_ACCEPT, WinEDA_SetOptionsFrame::Accept)
	EVT_BUTTON(ID_OPTION_CANCEL, WinEDA_SetOptionsFrame::OnCancel)
END_EVENT_TABLE()

/**************************************************************************/
void DisplayOptionFrame(WinEDA_DrawFrame * parent, const wxPoint & framepos)
/**************************************************************************/
{
	WinEDA_SetOptionsFrame * frame =
			new WinEDA_SetOptionsFrame(parent, framepos);
	frame->ShowModal(); frame->Destroy();
}



/***********************************************************************/
WinEDA_SetOptionsFrame::WinEDA_SetOptionsFrame(WinEDA_DrawFrame *parent,
							const wxPoint& framepos):
		wxDialog(parent, -1, _("EESchema Preferences"), framepos,
			wxSize(450, 340), DIALOG_STYLE)
/***********************************************************************/
{
#define START_Y 10
wxButton * Button;
wxPoint pos;
wxSize size;
int w, h;

	m_Parent = parent;
	SetFont(*g_DialogFont);

	pos.x = 330; pos.y = START_Y;
	Button = new wxButton(this, ID_OPTION_ACCEPT, _("OK"), pos);
	Button->SetForegroundColour(*wxRED);

	pos.y += Button->GetDefaultSize().y + 5;
	Button = new wxButton(this, ID_OPTION_CANCEL, _("Cancel"), pos);
	Button->SetForegroundColour(*wxBLUE);


	pos.x = 5;  pos.y = START_Y;
	m_ShowGridButt = new wxCheckBox( this, ID_SHOW_GRID, _("Show grid"),
				pos);
	m_ShowGridButt->SetValue(m_Parent->m_Draw_Grid);

	pos.x += 150;
	m_AutoPANOpt = new wxCheckBox( this, ID_SHOW_GRID, _("Auto PAN"),
				pos);
	m_AutoPANOpt->SetValue(m_Parent->DrawPanel-> m_AutoPAN_Enable);
	m_AutoPANOpt->SetForegroundColour(*wxBLUE);

	pos.x = 5; pos.y += 25;
wxString grid_choice[6] = { _("Normal (50 mils)"),
							_("Small (25 mils)"),
							_("Very small (10 mils)"),
							_("Special (5 mils)"),
							_("Special (2 mils)"),
							_("Special (1 mil)"),	};
							
	m_SelGridSize = new wxRadioBox(this, ID_GRID_SIZE,
			_("Grid Size"), pos,
			wxDefaultSize, 6, grid_choice, 1, wxRA_SPECIFY_COLS);
	if ( m_Parent->GetScreen() )
	{
		switch( m_Parent->GetScreen()->GetGrid().x )
		{
			case 50:
				m_SelGridSize->SetSelection(0);
				break;

			case 25:
				m_SelGridSize->SetSelection(1);
				break;

			case 10:
				m_SelGridSize->SetSelection(2);
				break;

			case 5:
				m_SelGridSize->SetSelection(3);
				break;

			case 2:
				m_SelGridSize->SetSelection(4);
				break;

			case 1:
				m_SelGridSize->SetSelection(5);
				break;

			default:
				DisplayError(this, wxT("WinEDA_SetOptionsFrame: Grid value not handle"));
				break;
		}
	}

	/* Choix d' l'affichage des pins invisibles */
	m_SelGridSize->GetSize(&w, &h);
	pos.y += h + 20;
wxString pin_choice[2] = { _("Normal"), _("Show alls") };
	m_SelShowPins = new wxRadioBox(this, ID_SEL_SHOW_PINS,
			_("Show pins"), pos,
			wxDefaultSize, 2, pin_choice, 1, wxRA_SPECIFY_COLS);
	m_SelShowPins->SetSelection( g_ShowAllPins ? TRUE : FALSE);

	/* Choix de l'affichage des unites */
	pos.x = 15 + w;  pos.y = START_Y + 25;
wxString unit_choice[2] = { _("millimeters"), _("inches") };
	m_Selunits = new wxRadioBox(this, ID_SEL_METRIC,
			_("Units"), pos,
			wxDefaultSize, 2, unit_choice, 1, wxRA_SPECIFY_COLS);
	m_Selunits->SetSelection( UnitMetric ? 0 : 1);

	/* Choix de l'orientation des bus et wires */
	m_Selunits->GetSize(&w, &h);
	pos.y += h + 15;
wxString dir_choice[2] = { _("Horiz/Vertical"), _("Any") };
	m_SelDirWires = new wxRadioBox(this, ID_SEL_HV_WIRE,
			_("Wires - Bus orient"), pos,
			wxDefaultSize, 2, dir_choice, 1, wxRA_SPECIFY_COLS);
	m_SelDirWires->SetSelection( g_HVLines ? 0 : 1);

	m_SelDirWires->GetSize(&w, &h);
	pos.y += h + 15;
wxString show_page_limits_choice[2] = { _("Yes"), _("No") };
	m_Show_Page_Limits = new wxRadioBox(this, ID_SEL_SHOW_PAGE_LIMITS,
			_("Show page limits"), pos,
			wxDefaultSize, 2, show_page_limits_choice, 1, wxRA_SPECIFY_COLS);
	m_Show_Page_Limits->SetSelection( g_ShowPageLimits ? 0 : 1);

	/* Choix des parametres pour la fonction de repetition */
	size.x = 100; size.y = -1;
	pos.y = 90; pos.x = 320;
	m_DeltaStepCtrl = new WinEDA_SizeCtrl(this, _("Delta Step"),
			g_RepeatStep,
			UnitMetric, pos, m_Parent->m_InternalUnits);

	pos.y += 10 + m_DeltaStepCtrl->GetDimension().y;
	new wxStaticText(this, -1, _("Delta Label:"),
							wxPoint(pos.x, pos.y ),
							wxSize(-1,-1), 0 );
wxString val;
	pos.y += 14;
	val << g_RepeatDeltaLabel;
	m_DeltaLabelCtrl = new wxSpinCtrl(this, -1, val, pos);
	m_DeltaLabelCtrl->SetRange(-16, +16);
}

/**************************************************************************/
void WinEDA_SetOptionsFrame::OnCancel(wxCommandEvent& event)
/**************************************************************************/
{
	Close(TRUE);
}
/**************************************************************************/
void WinEDA_SetOptionsFrame::Accept(wxCommandEvent& event)
/**************************************************************************/
{
wxSize grid;
bool setgrid = TRUE;
	
	g_RepeatStep = m_DeltaStepCtrl->GetCoord();

	g_RepeatDeltaLabel = m_DeltaLabelCtrl->GetValue();

	if ( m_Show_Page_Limits->GetSelection() == 0 ) g_ShowPageLimits = TRUE;
	else g_ShowPageLimits = FALSE;

	if ( m_SelDirWires->GetSelection() == 0 ) g_HVLines = 1;
	else g_HVLines = 0;

	if ( m_Selunits->GetSelection() == 0 ) UnitMetric = 1;
	else  UnitMetric = 0;

	if ( m_SelShowPins->GetSelection() == 0 ) g_ShowAllPins = FALSE;
	else g_ShowAllPins = TRUE;

	g_ShowGrid = m_Parent->m_Draw_Grid = m_ShowGridButt->GetValue();
	m_Parent->DrawPanel->m_AutoPAN_Enable = m_AutoPANOpt->GetValue();

	switch( m_SelGridSize->GetSelection() )
		{
		default:
			setgrid = FALSE;
			break;
		
		case 0:
			grid = wxSize(50,50);
			break;
		case 1:
			grid = wxSize(25,25);
			break;

		case 2:
			grid = wxSize(10,10);
			break;
		}

	if ( m_Parent->m_CurrentScreen )
	{
		if ( setgrid ) m_Parent->m_CurrentScreen->SetGrid(grid);
		m_Parent->m_CurrentScreen->SetRefreshReq();
	}

	Close(TRUE);
}


