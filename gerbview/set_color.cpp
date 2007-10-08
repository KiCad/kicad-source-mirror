 /* Set up the basic primitives for Layer control */

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "gerbview.h"
#include "pcbnew.h"

#include "protos.h"

/* Variables locales */
const int BUTT_SIZE_X = 20;
const int BUTT_SIZE_Y = 16;

const int COLOR_COUNT = 37;    // 37 = 32 (layers) + 2 (others) + 3 (headings)
    // Is there a better way to determine how many elements CurrentColor requires?
int CurrentColor[COLOR_COUNT]; // Holds color for each layer while dialog box open

/* Fonctions locales: */

/* Macro utile : */
#define ADR(numlayer) &g_DesignSettings.m_LayerColor[(numlayer)]

enum col_sel_id {
	ID_COLOR_RESET_SHOW_LAYER_ON = 1800,
	ID_COLOR_RESET_SHOW_LAYER_OFF,
	ID_COLOR_CHECKBOX_ONOFF,
	ID_COLOR_SETUP
};


	/**********************************/
	/* Liste des menus de Menu_Layers */
	/**********************************/
struct ColorButton
	{
	wxString m_Name;
	int * m_Color;				// Pointeur sur la variable couleur
	bool m_NoDisplayIsColor;	// TRUE si bit ITEM_NON_VISIBLE de la variable Color
	bool * m_NoDisplay;			// Pointeur sur la variable Display on/off si ce
								// n'est pas la var Color
	int m_Id;
	wxBitmapButton * m_Button;	// Button to display/change color assigned to this layer
	int m_State;
	wxCheckBox * m_CheckBox;	// Option Display ON/OFF
};

#include "set_color.h"	/* include description and list of tools and buttons */


/*************************************************************/
/* classe derivee pour la frame de Configuration des couleurs*/
/*************************************************************/

class WinEDA_SetColorsFrame: public wxDialog
{
private:
	WinEDA_DrawFrame *m_Parent;

public:

	// Constructor and destructor
	WinEDA_SetColorsFrame(WinEDA_DrawFrame *parent, const wxPoint& framepos);
	~WinEDA_SetColorsFrame() {};

private:
	void SetColor(wxCommandEvent& event);
	void OnOkClick(wxCommandEvent& event);
	void OnCancelClick(wxCommandEvent& event);
	void OnApplyClick(wxCommandEvent& event);
	void UpdateLayerSettings();
	void ResetDisplayLayersCu(wxCommandEvent& event);
	DECLARE_EVENT_TABLE()

};
/* Table des evenements pour WinEDA_SetColorsFrame */
BEGIN_EVENT_TABLE(WinEDA_SetColorsFrame, wxDialog)
	EVT_BUTTON(ID_COLOR_RESET_SHOW_LAYER_OFF, WinEDA_SetColorsFrame::ResetDisplayLayersCu)
	EVT_BUTTON(ID_COLOR_RESET_SHOW_LAYER_ON, WinEDA_SetColorsFrame::ResetDisplayLayersCu)
	EVT_BUTTON(wxID_OK, WinEDA_SetColorsFrame::OnOkClick)
	EVT_BUTTON(wxID_CANCEL, WinEDA_SetColorsFrame::OnCancelClick)
	EVT_BUTTON(wxID_APPLY, WinEDA_SetColorsFrame::OnApplyClick)
	EVT_BUTTON(ID_COLOR_SETUP, WinEDA_SetColorsFrame::SetColor)
	EVT_BUTTON(ID_COLOR_SETUP+1, WinEDA_SetColorsFrame::SetColor)
	EVT_BUTTON(ID_COLOR_SETUP+2, WinEDA_SetColorsFrame::SetColor)
	EVT_BUTTON(ID_COLOR_SETUP+3, WinEDA_SetColorsFrame::SetColor)
	EVT_BUTTON(ID_COLOR_SETUP+4, WinEDA_SetColorsFrame::SetColor)
	EVT_BUTTON(ID_COLOR_SETUP+5, WinEDA_SetColorsFrame::SetColor)
	EVT_BUTTON(ID_COLOR_SETUP+6, WinEDA_SetColorsFrame::SetColor)
	EVT_BUTTON(ID_COLOR_SETUP+7, WinEDA_SetColorsFrame::SetColor)
	EVT_BUTTON(ID_COLOR_SETUP+8, WinEDA_SetColorsFrame::SetColor)
	EVT_BUTTON(ID_COLOR_SETUP+9, WinEDA_SetColorsFrame::SetColor)
	EVT_BUTTON(ID_COLOR_SETUP+10, WinEDA_SetColorsFrame::SetColor)
	EVT_BUTTON(ID_COLOR_SETUP+11, WinEDA_SetColorsFrame::SetColor)
	EVT_BUTTON(ID_COLOR_SETUP+12, WinEDA_SetColorsFrame::SetColor)
	EVT_BUTTON(ID_COLOR_SETUP+13, WinEDA_SetColorsFrame::SetColor)
	EVT_BUTTON(ID_COLOR_SETUP+14, WinEDA_SetColorsFrame::SetColor)
	EVT_BUTTON(ID_COLOR_SETUP+15, WinEDA_SetColorsFrame::SetColor)
	EVT_BUTTON(ID_COLOR_SETUP+16, WinEDA_SetColorsFrame::SetColor)
	EVT_BUTTON(ID_COLOR_SETUP+17, WinEDA_SetColorsFrame::SetColor)
	EVT_BUTTON(ID_COLOR_SETUP+18, WinEDA_SetColorsFrame::SetColor)
	EVT_BUTTON(ID_COLOR_SETUP+19, WinEDA_SetColorsFrame::SetColor)
	EVT_BUTTON(ID_COLOR_SETUP+20, WinEDA_SetColorsFrame::SetColor)
	EVT_BUTTON(ID_COLOR_SETUP+21, WinEDA_SetColorsFrame::SetColor)
	EVT_BUTTON(ID_COLOR_SETUP+22, WinEDA_SetColorsFrame::SetColor)
	EVT_BUTTON(ID_COLOR_SETUP+23, WinEDA_SetColorsFrame::SetColor)
	EVT_BUTTON(ID_COLOR_SETUP+24, WinEDA_SetColorsFrame::SetColor)
	EVT_BUTTON(ID_COLOR_SETUP+25, WinEDA_SetColorsFrame::SetColor)
	EVT_BUTTON(ID_COLOR_SETUP+26, WinEDA_SetColorsFrame::SetColor)
	EVT_BUTTON(ID_COLOR_SETUP+27, WinEDA_SetColorsFrame::SetColor)
	EVT_BUTTON(ID_COLOR_SETUP+28, WinEDA_SetColorsFrame::SetColor)
	EVT_BUTTON(ID_COLOR_SETUP+29, WinEDA_SetColorsFrame::SetColor)
	EVT_BUTTON(ID_COLOR_SETUP+30, WinEDA_SetColorsFrame::SetColor)
	EVT_BUTTON(ID_COLOR_SETUP+31, WinEDA_SetColorsFrame::SetColor)
	EVT_BUTTON(ID_COLOR_SETUP+32, WinEDA_SetColorsFrame::SetColor)
	EVT_BUTTON(ID_COLOR_SETUP+33, WinEDA_SetColorsFrame::SetColor)
	EVT_BUTTON(ID_COLOR_SETUP+34, WinEDA_SetColorsFrame::SetColor)
	EVT_BUTTON(ID_COLOR_SETUP+35, WinEDA_SetColorsFrame::SetColor)
	EVT_BUTTON(ID_COLOR_SETUP+36, WinEDA_SetColorsFrame::SetColor)
//	EVT_BUTTON(ID_COLOR_SETUP+37, WinEDA_SetColorsFrame::SetColor)
//	EVT_BUTTON(ID_COLOR_SETUP+38, WinEDA_SetColorsFrame::SetColor)
//	EVT_BUTTON(ID_COLOR_SETUP+39, WinEDA_SetColorsFrame::SetColor)
//	EVT_BUTTON(ID_COLOR_SETUP+40, WinEDA_SetColorsFrame::SetColor)
//	EVT_BUTTON(ID_COLOR_SETUP+41, WinEDA_SetColorsFrame::SetColor)
//	EVT_BUTTON(ID_COLOR_SETUP+42, WinEDA_SetColorsFrame::SetColor)
//	EVT_BUTTON(ID_COLOR_SETUP+43, WinEDA_SetColorsFrame::SetColor)
//	EVT_BUTTON(ID_COLOR_SETUP+44, WinEDA_SetColorsFrame::SetColor)
END_EVENT_TABLE()

/**************************************************************/
/* void DisplayColorSetupFrame(WinEDA_DrawFrame * parent, 	  */
/*							const wxPoint & pos)			  */
/**************************************************************/

void DisplayColorSetupFrame(WinEDA_DrawFrame * parent,
							const wxPoint & framepos)
{
	WinEDA_SetColorsFrame * frame =
			new WinEDA_SetColorsFrame(parent, framepos);
	frame->ShowModal();
	frame->Destroy();
}


/**********************************************************************/
WinEDA_SetColorsFrame::WinEDA_SetColorsFrame(WinEDA_DrawFrame *parent,
							const wxPoint& framepos):
		wxDialog(parent, -1, _("GerbView Layer Colors:"), framepos,
			wxSize(390, 380),
			wxDEFAULT_DIALOG_STYLE|wxFRAME_FLOAT_ON_PARENT|
			MAYBE_RESIZE_BORDER )
/**********************************************************************/
{
wxBitmapButton * ButtonB;
int ii, butt_ID, buttcolor;
wxString msg;
wxStaticText * text;
wxBoxSizer * CurrBoxSizer = NULL;

	m_Parent = parent;
	SetFont(*g_DialogFont);

	wxBoxSizer * MainBoxSizer = new wxBoxSizer(wxHORIZONTAL);
	SetSizer(MainBoxSizer);

	for ( ii = 0; laytool_list[ii] != NULL; ii++ )
	{
		if( ! CurrBoxSizer || ! laytool_list[ii]->m_Color && ! laytool_list[ii]->m_NoDisplay )
		{
			CurrBoxSizer = new wxBoxSizer(wxVERTICAL);
			MainBoxSizer->Add(CurrBoxSizer, 0, wxGROW|wxALL, 5);
			msg = wxGetTranslation(laytool_list[ii]->m_Name.GetData());
			text = new wxStaticText( this, -1, msg );
			CurrBoxSizer->Add(text, 0, wxGROW|wxALL, 5);
			continue;
		}

		if ( laytool_list[ii]->m_Id == 0 )
			laytool_list[ii]->m_Id = ID_COLOR_SETUP + ii;
		butt_ID = laytool_list[ii]->m_Id;

		wxBoxSizer * LineBoxSizer = new wxBoxSizer(wxHORIZONTAL);
		wxCheckBox * Checkb;
		CurrBoxSizer->Add(LineBoxSizer, 0, wxGROW|wxALL, 0);
		laytool_list[ii]->m_CheckBox = Checkb = new wxCheckBox( this,
						ID_COLOR_CHECKBOX_ONOFF, wxEmptyString );
		LineBoxSizer->Add(Checkb, 0, wxGROW|wxALL, 1);

		if ( laytool_list[ii]->m_NoDisplayIsColor )
		{
			if ( *laytool_list[ii]->m_Color & ITEM_NOT_SHOW )
				laytool_list[ii]->m_CheckBox->SetValue(FALSE);
			else
				laytool_list[ii]->m_CheckBox->SetValue(TRUE);
		}
		else if ( laytool_list[ii]->m_NoDisplay )
			laytool_list[ii]->m_CheckBox->SetValue(*laytool_list[ii]->m_NoDisplay);

		if( laytool_list[ii]->m_Color )
		{
			wxMemoryDC iconDC;
			wxBitmap ButtBitmap( BUTT_SIZE_X, BUTT_SIZE_Y );
			iconDC.SelectObject( ButtBitmap );
			buttcolor = *laytool_list[ii]->m_Color & MASKCOLOR;
			CurrentColor[ii] = buttcolor;
			wxBrush Brush;
			iconDC.SelectObject( ButtBitmap );
			iconDC.SetPen(*wxBLACK_PEN);
			Brush.SetColour(
						ColorRefs[buttcolor].m_Red,
						ColorRefs[buttcolor].m_Green,
						ColorRefs[buttcolor].m_Blue
						);
			Brush.SetStyle(wxSOLID);

			iconDC.SetBrush(Brush);
			iconDC.DrawRectangle(0, 0, BUTT_SIZE_X, BUTT_SIZE_Y);

			ButtonB = new wxBitmapButton( this, butt_ID,
						ButtBitmap, wxDefaultPosition,
						wxSize(BUTT_SIZE_X, BUTT_SIZE_Y) );
			laytool_list[ii]->m_Button = ButtonB;
			LineBoxSizer->Add(ButtonB, 0, wxALIGN_CENTER_VERTICAL|wxALL, 1);
		}

		msg = wxGetTranslation(laytool_list[ii]->m_Name.GetData());
		text = new wxStaticText( this, -1, msg );
		LineBoxSizer->Add(text, 0, wxGROW|wxALL, 1);
	}

	CurrBoxSizer->AddSpacer(20);
	wxButton * Button = new wxButton( this, ID_COLOR_RESET_SHOW_LAYER_ON,
						_("Show All") );
	Button->SetForegroundColour(*wxBLUE);
	CurrBoxSizer->Add(Button, 0, wxALIGN_TOP|wxGROW|wxALL, 5);

	Button = new wxButton( this, ID_COLOR_RESET_SHOW_LAYER_OFF,
						_("Show None") );
	Button->SetForegroundColour(*wxRED);
	CurrBoxSizer->Add(Button, 0, wxALIGN_TOP|wxGROW|wxALL, 5);

    // Following stretch spacer ensures "OK", "Cancel", and "Apply"
    // buttons will be located at lower right corner of dialog box
    CurrBoxSizer->AddStretchSpacer();

	Button = new wxButton( this, wxID_OK, _("OK") );
	Button->SetForegroundColour(*wxRED);
	CurrBoxSizer->Add(Button, 0, wxALIGN_BOTTOM|wxALIGN_RIGHT|wxALL, 5);

	Button = new wxButton( this, wxID_CANCEL, _("Cancel") );
	Button->SetForegroundColour(*wxBLUE);
	CurrBoxSizer->Add(Button, 0, wxALIGN_BOTTOM|wxALIGN_RIGHT|wxALL, 5);

	Button = new wxButton( this, wxID_APPLY, _("Apply") );
	CurrBoxSizer->Add(Button, 0, wxALIGN_BOTTOM|wxALIGN_RIGHT|wxALL, 5);

    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);
}


/*******************************************************************/
void  WinEDA_SetColorsFrame::OnOkClick(wxCommandEvent& WXUNUSED(event))
/*******************************************************************/
{
    UpdateLayerSettings();
    m_Parent->ReDrawPanel();
    EndModal( 1 );
}


/*******************************************************************/
void  WinEDA_SetColorsFrame::OnCancelClick(wxCommandEvent& WXUNUSED(event))
/*******************************************************************/
{
    EndModal( -1 );
}


/*******************************************************************/
void  WinEDA_SetColorsFrame::OnApplyClick(wxCommandEvent& WXUNUSED(event))
/*******************************************************************/
{
    UpdateLayerSettings();
    m_Parent->ReDrawPanel();
}


/***********************************************************/
void WinEDA_SetColorsFrame::SetColor(wxCommandEvent& event)
/***********************************************************/
{
int ii;
int id = event.GetId();
int color;

	color = DisplayColorFrame( this,
			CurrentColor[id - ID_COLOR_SETUP] );
	if ( color < 0 )
		return;

	for ( ii = 0; laytool_list[ii] != NULL; ii++ )
	{
		if( laytool_list[ii]->m_Id != id )
			continue;

		if( laytool_list[ii]->m_Color == NULL )
			continue;

		if( CurrentColor[ii] == color )
			break;

		CurrentColor[ii] = color;
		wxMemoryDC iconDC;

		wxBitmapButton * Button = laytool_list[ii]->m_Button;

		wxBitmap ButtBitmap = Button->GetBitmapLabel();
		iconDC.SelectObject( ButtBitmap );
		int buttcolor = CurrentColor[ii];
		wxBrush Brush;
		iconDC.SelectObject( ButtBitmap );
		iconDC.SetPen(*wxBLACK_PEN);
		Brush.SetColour(
						ColorRefs[buttcolor].m_Red,
						ColorRefs[buttcolor].m_Green,
						ColorRefs[buttcolor].m_Blue
						);
		Brush.SetStyle(wxSOLID);

		iconDC.SetBrush(Brush);
		iconDC.DrawRectangle(0, 0, BUTT_SIZE_X, BUTT_SIZE_Y);
		Button->SetBitmapLabel(ButtBitmap);
	}
	Refresh( FALSE );
}


/******************************************************************/
void WinEDA_SetColorsFrame::UpdateLayerSettings()
/******************************************************************/
{
	for( int ii = 0; laytool_list[ii] != NULL; ii++ )
	{
		if ( ! laytool_list[ii]->m_NoDisplayIsColor &&
			 (laytool_list[ii]->m_NoDisplay == NULL) )
			continue;

		if ( laytool_list[ii]->m_NoDisplayIsColor )
		{
			if ( laytool_list[ii]->m_CheckBox->GetValue() )
				*laytool_list[ii]->m_Color = CurrentColor[ii] & ~ITEM_NOT_SHOW;
			else
				*laytool_list[ii]->m_Color = CurrentColor[ii] | ITEM_NOT_SHOW;
		}
		else
		{
			*laytool_list[ii]->m_Color = CurrentColor[ii];
			*laytool_list[ii]->m_NoDisplay = laytool_list[ii]->m_CheckBox->GetValue();

            // A hack, we have both g_DrawGrid and m_Parent->m_Draw_Grid.
            // A better way preferred, please.
            if( laytool_list[ii]->m_NoDisplay == &g_ShowGrid )
                m_Parent->m_Draw_Grid = g_ShowGrid;
		}
	}
}


/***********************************************************************/
void WinEDA_SetColorsFrame::ResetDisplayLayersCu(wxCommandEvent& event)
/***********************************************************************/
{
bool NewState = (event.GetId() == ID_COLOR_RESET_SHOW_LAYER_ON) ? TRUE : FALSE;

	for ( int ii = 1; ii < 34; ii++ )
	{
		if ( laytool_list[ii]->m_CheckBox == NULL )
			continue;
		laytool_list[ii]->m_CheckBox->SetValue(NewState);
	}
}
