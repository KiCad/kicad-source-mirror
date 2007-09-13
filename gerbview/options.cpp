	/********************************************/
	/* GERBVIEW - Gestion des Options et Reglages */
	/********************************************/

	/*	 Fichier options.cpp 	*/

/*
 Affichage et modifications des parametres de travail Gerbview
*/


#include "fctsys.h"

#include "common.h"
#include "gerbview.h"
#include "pcbplot.h"

#include "id.h"

#include "protos.h"
#include <wx/spinctrl.h>

/* Fonctions locales */

/* variables locales */

/*****************************************************************/
void WinEDA_GerberFrame::OnSelectOptionToolbar(wxCommandEvent& event)
/*****************************************************************/
{
int id = event.GetId();
wxClientDC dc(DrawPanel);

	DrawPanel->PrepareGraphicContext(&dc);
	switch ( id )
	{
		case ID_TB_OPTIONS_SHOW_GRID:
			m_Draw_Grid = g_ShowGrid = m_OptionsToolBar->GetToolState(id);
			DrawPanel->ReDraw(&dc, TRUE);
			break;

		case ID_TB_OPTIONS_SELECT_UNIT_MM:
			g_UnitMetric = MILLIMETRE;
			Affiche_Status_Box();	 /* Reaffichage des coord curseur */
			break;

		case ID_TB_OPTIONS_SELECT_UNIT_INCH:
			g_UnitMetric = INCHES;
			Affiche_Status_Box();	 /* Reaffichage des coord curseur */
			break;

		case ID_TB_OPTIONS_SHOW_POLAR_COORD:
			Affiche_Message(wxEmptyString);
			DisplayOpt.DisplayPolarCood = m_OptionsToolBar->GetToolState(id);
			Affiche_Status_Box();	 /* Reaffichage des coord curseur */
			break;

		case ID_TB_OPTIONS_SELECT_CURSOR:
			DrawPanel->CursorOff(&dc);
			g_CursorShape = m_OptionsToolBar->GetToolState(id);
			DrawPanel->CursorOn(&dc);
			break;

		case ID_TB_OPTIONS_SHOW_PADS_SKETCH:
			if ( m_OptionsToolBar->GetToolState(id) )
			{
				m_DisplayPadFill = FALSE;
				DisplayOpt.DisplayPadFill = FALSE;
			}
			else
			{
				m_DisplayPadFill = TRUE;
				DisplayOpt.DisplayPadFill = TRUE;
			}
			DrawPanel->ReDraw(&dc, TRUE);
			break;

		case ID_TB_OPTIONS_SHOW_TRACKS_SKETCH:
			if( m_OptionsToolBar->GetToolState(id) )
			{
				m_DisplayPcbTrackFill = FALSE;
				DisplayOpt.DisplayPcbTrackFill = FALSE;
			}
			else
			{
				m_DisplayPcbTrackFill = TRUE;
				DisplayOpt.DisplayPcbTrackFill = TRUE;
			}
			DrawPanel->ReDraw(&dc, TRUE);
			break;

		case ID_TB_OPTIONS_SHOW_DCODES:
			DisplayOpt.DisplayPadNum = m_OptionsToolBar->GetToolState(id);
			DrawPanel->ReDraw(&dc, TRUE);
			break;


		default:
			DisplayError(this, wxT("WinEDA_PcbFrame::OnSelectOptionToolbar error"));
			break;
	}

	SetToolbars();
}


enum id_optpcb
{
	ID_ACCEPT_OPT = 1000,
	ID_CANCEL_OPT
};
	/*************************************************/
	/* classe derivee pour la frame de Configuration */
	/*************************************************/

class WinEDA_GerberGeneralOptionsFrame: public wxDialog
{
private:

	WinEDA_BasePcbFrame * m_Parent;
	wxRadioBox * m_PolarDisplay;
	wxRadioBox * m_BoxUnits;
	wxRadioBox * m_CursorShape;
	wxRadioBox * m_GerberDefaultScale;



	// Constructor and destructor
public:
	WinEDA_GerberGeneralOptionsFrame(WinEDA_BasePcbFrame *parent,const wxPoint& pos);
	~WinEDA_GerberGeneralOptionsFrame() {};

private:
	void AcceptPcbOptions(wxCommandEvent& event);
	void OnQuit(wxCommandEvent & event);

	DECLARE_EVENT_TABLE()

};
/* Construction de la table des evenements pour WinEDA_GerberGeneralOptionsFrame */
BEGIN_EVENT_TABLE(WinEDA_GerberGeneralOptionsFrame, wxDialog)
	EVT_BUTTON(ID_ACCEPT_OPT, WinEDA_GerberGeneralOptionsFrame::AcceptPcbOptions)
	EVT_BUTTON(ID_CANCEL_OPT, WinEDA_GerberGeneralOptionsFrame::OnQuit)
END_EVENT_TABLE()




	/*************************************************/
	/* Constructeur de WinEDA_GerberGeneralOptionsFrame */
	/************************************************/

WinEDA_GerberGeneralOptionsFrame::WinEDA_GerberGeneralOptionsFrame(WinEDA_BasePcbFrame *parent,
		const wxPoint& framepos):
		wxDialog(parent, -1, _("Gerbview Options"),
				 framepos, wxSize(300, 240),
				wxDEFAULT_DIALOG_STYLE|wxFRAME_FLOAT_ON_PARENT )
{
	m_Parent = parent;
	SetFont(*g_DialogFont);

	wxBoxSizer * MainBoxSizer = new wxBoxSizer(wxHORIZONTAL);
	SetSizer(MainBoxSizer);
	wxBoxSizer * RightBoxSizer = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer * MiddleBoxSizer = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer * LeftBoxSizer = new wxBoxSizer(wxVERTICAL);
	MainBoxSizer->Add(LeftBoxSizer, 0, wxGROW|wxALL, 5);
	MainBoxSizer->Add(MiddleBoxSizer, 0, wxGROW|wxALL, 5);
	MainBoxSizer->Add(RightBoxSizer, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

	wxButton * Button = new wxButton(this, ID_ACCEPT_OPT, _("Accept"));
	Button->SetForegroundColour(*wxRED);
	RightBoxSizer->Add(Button, 0, wxGROW|wxALL, 5);

	Button = new wxButton(this, ID_CANCEL_OPT, _("Cancel"));
	Button->SetForegroundColour(*wxBLUE);
	RightBoxSizer->Add(Button, 0, wxGROW|wxALL, 5);

	/* Display Selection affichage des coordonnées polaires */
wxString list_coord[2] =
	{ _("No Display"),
	_("Display") };
	m_PolarDisplay = new wxRadioBox(this, -1, _("Display Polar Coord"),
					wxDefaultPosition, wxDefaultSize,
					2, list_coord, 1);
	m_PolarDisplay->SetSelection(DisplayOpt.DisplayPolarCood ? 1 : 0);
	LeftBoxSizer->Add(m_PolarDisplay, 0, wxGROW|wxALL, 5);

	/* Selection choix des unités d'affichage */
wxString list_units[2] = {
	_("Inches"),
	_("millimeters") };
	m_BoxUnits = new wxRadioBox(this, -1, _("Units"), wxDefaultPosition, wxDefaultSize,
					2, list_units, 1);
	m_BoxUnits->SetSelection( g_UnitMetric ? 1 : 0);
	LeftBoxSizer->Add(m_BoxUnits, 0, wxGROW|wxALL, 5);

	/* Selection forme du curseur */
wxString list_cursors[2] = { _("Small"), _("Big") };
	m_CursorShape = new wxRadioBox(this, -1, _("Cursor"), wxDefaultPosition, wxDefaultSize,
					2, list_cursors, 1);
	m_CursorShape->SetSelection( g_CursorShape ? 1 : 0);
	MiddleBoxSizer->Add(m_CursorShape, 0, wxGROW|wxALL, 5);

	/* Selection Default Scale (i.e. format 2.3 ou 3.4) */
wxString list_scales[2] = { _("format: 2.3"), _("format 3.4") };
	m_GerberDefaultScale = new wxRadioBox(this, -1, _("Default format"),
						wxDefaultPosition, wxDefaultSize,
					2, list_scales, 1);
	m_GerberDefaultScale->SetSelection( (g_Default_GERBER_Format == 23) ? 0 : 1);
	MiddleBoxSizer->Add(m_GerberDefaultScale, 0, wxGROW|wxALL, 5);


	GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
}

/************************************************************************/
void  WinEDA_GerberGeneralOptionsFrame::OnQuit(wxCommandEvent& WXUNUSED(event))
/************************************************************************/
{
    // true is to force the frame to close
    Close(true);
}



/*****************************************************************************/
void WinEDA_GerberGeneralOptionsFrame::AcceptPcbOptions(wxCommandEvent& event)
/*****************************************************************************/
{
	DisplayOpt.DisplayPolarCood =
		(m_PolarDisplay->GetSelection() == 0) ? FALSE : TRUE;
	g_UnitMetric = (m_BoxUnits->GetSelection() == 0)  ? 0 : 1;
	g_CursorShape = m_CursorShape->GetSelection();
	g_Default_GERBER_Format =
		(m_GerberDefaultScale->GetSelection() == 0) ? 23 : 34;

	EndModal(1);
}



/******************************************************************/
/* classe derivee pour la frame de Configuration WinEDA_LookFrame */
/******************************************************************/

class WinEDA_LookFrame: public wxDialog
{
private:
protected:
public:

	WinEDA_BasePcbFrame * m_Parent;

	wxRadioBox * m_OptDisplayLines;

	wxRadioBox * m_OptDisplayFlashes;
	wxCheckBox * m_OptDisplayDCodes;

	wxRadioBox * m_OptDisplayDrawings;


	// Constructor and destructor
	WinEDA_LookFrame(WinEDA_BasePcbFrame *parent,const wxPoint& pos);
	~WinEDA_LookFrame() {};

	void AcceptPcbOptions(wxCommandEvent& event);
	void OnQuit(wxCommandEvent & event);

	DECLARE_EVENT_TABLE()

};
/* Construction de la table des evenements pour WinEDA_LookFrame */
BEGIN_EVENT_TABLE(WinEDA_LookFrame, wxDialog)
	EVT_BUTTON(ID_ACCEPT_OPT, WinEDA_LookFrame::AcceptPcbOptions)
	EVT_BUTTON(ID_CANCEL_OPT, WinEDA_LookFrame::OnQuit)
END_EVENT_TABLE()



/*******************************************************************************/
WinEDA_LookFrame::WinEDA_LookFrame(WinEDA_BasePcbFrame *parent,
		const wxPoint& framepos):
		wxDialog(parent, -1, _("Gerbview Draw Options"), framepos, wxSize(350, 200),
		wxDEFAULT_DIALOG_STYLE|wxFRAME_FLOAT_ON_PARENT )
/*******************************************************************************/
{
	m_Parent = parent;
	SetFont(*g_DialogFont);

	wxBoxSizer * MainBoxSizer = new wxBoxSizer(wxHORIZONTAL);
	SetSizer(MainBoxSizer);
	wxBoxSizer * RightBoxSizer = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer * MiddleBoxSizer = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer * LeftBoxSizer = new wxBoxSizer(wxVERTICAL);
	MainBoxSizer->Add(LeftBoxSizer, 0, wxGROW|wxALL, 5);
	MainBoxSizer->Add(MiddleBoxSizer, 0, wxGROW|wxALL, 5);
	MainBoxSizer->Add(RightBoxSizer, 0, wxALIGN_CENTER_VERTICAL|wxALL, 5);

	wxButton * Button = new wxButton(this, ID_ACCEPT_OPT, _("Accept"));
	Button->SetForegroundColour(*wxRED);
	RightBoxSizer->Add(Button, 0, wxGROW|wxALL, 5);

	Button = new wxButton(this, ID_CANCEL_OPT, _("Cancel"));
	Button->SetForegroundColour(*wxBLUE);
	RightBoxSizer->Add(Button, 0, wxGROW|wxALL, 5);

	// Show Option Draw Tracks
wxString list_opt2[2] = { _("Sketch"), _("Filled") };
	m_OptDisplayLines = new wxRadioBox(this, -1, _("Lines:"),
				wxDefaultPosition, wxDefaultSize,
				2, list_opt2, 1);
	if ( DisplayOpt.DisplayPcbTrackFill ) m_OptDisplayLines->SetSelection(1);
	LeftBoxSizer->Add(m_OptDisplayLines, 0, wxGROW|wxALL, 5);

	m_OptDisplayFlashes = new wxRadioBox(this, -1, _("Spots:"),
				wxDefaultPosition, wxDefaultSize,
				2, list_opt2, 1);
	if ( DisplayOpt.DisplayPadFill ) m_OptDisplayFlashes->SetSelection(1);
	LeftBoxSizer->Add(m_OptDisplayFlashes, 0, wxGROW|wxALL, 5);

wxString list_opt3[3] = { _("Sketch"), _("Filled"), _("Line") };
	m_OptDisplayDrawings = new wxRadioBox(this, -1, _("Display other items:"),
				wxDefaultPosition, wxDefaultSize,
				3, list_opt3, 1);
	m_OptDisplayDrawings->SetSelection(DisplayOpt.DisplayDrawItems);
	MiddleBoxSizer->Add(m_OptDisplayDrawings, 0, wxGROW|wxALL, 5);

	m_OptDisplayDCodes = new wxCheckBox(this, -1, _("Show D-Codes"));
	if ( DisplayOpt.DisplayPadNum ) m_OptDisplayDCodes->SetValue(TRUE);
	MiddleBoxSizer->Add(m_OptDisplayDCodes, 0, wxGROW|wxALL, 5);
 
	GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
}


/**************************************************************/
void  WinEDA_LookFrame::OnQuit(wxCommandEvent& WXUNUSED(event))
/**************************************************************/
{
    // true is to force the frame to close
    Close(true);
}


/*************************************************************/
void WinEDA_LookFrame::AcceptPcbOptions(wxCommandEvent& event)
/*************************************************************/
/* Met a jour les options
*/
{
	if ( m_OptDisplayLines->GetSelection() == 1)
		DisplayOpt.DisplayPcbTrackFill = TRUE;
	else DisplayOpt.DisplayPcbTrackFill = FALSE;

	if (m_OptDisplayFlashes->GetSelection() == 1 )
		 DisplayOpt.DisplayPadFill = TRUE;
	else DisplayOpt.DisplayPadFill = FALSE;

	DisplayOpt.DisplayPadNum = m_OptDisplayDCodes->GetValue();

	DisplayOpt.DisplayDrawItems = m_OptDisplayDrawings->GetSelection();

	m_Parent->m_DisplayPadFill = DisplayOpt.DisplayPadFill;
	m_Parent->m_DisplayPcbTrackFill = DisplayOpt.DisplayPcbTrackFill;

	m_Parent->GetScreen()->SetRefreshReq();

	EndModal(1);
}





/***************************************************************************/
void WinEDA_GerberFrame::InstallPcbOptionsFrame(const wxPoint & pos, int id)
/***************************************************************************/
{

	switch ( id )
		{
		case ID_PCB_LOOK_SETUP:
			{
			WinEDA_LookFrame * OptionsFrame =
				new WinEDA_LookFrame(this, pos);
			OptionsFrame->ShowModal(); OptionsFrame->Destroy();
			}
			break;

		case ID_OPTIONS_SETUP:
			{
			WinEDA_GerberGeneralOptionsFrame * OptionsFrame =
				new WinEDA_GerberGeneralOptionsFrame(this, pos);
			OptionsFrame->ShowModal(); OptionsFrame->Destroy();
			}
			break;
		}
}


