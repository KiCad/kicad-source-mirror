/* Set up the items and layer colors and show/no show options  */

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "pcbnew.h"

#include "protos.h"


/* Variables locales */
int CurrentColor;
/* Fonctions locales: */

/* Macro utile : */
#define ADR(numlayer) &g_DesignSettings.m_LayerColor[(numlayer)]

#define BUTT_SIZE_X 25
#define BUTT_SIZE_Y 15


enum col_sel_id {
	ID_COLOR_RESET_SHOW_LAYER_ON = 1800,
	ID_COLOR_RESET_SHOW_LAYER_OFF,
	ID_COLOR_EXIT,
	ID_COLOR_CHECKBOX_ONOFF,
	ID_COLOR_SETUP
};

	/**********************************/
	/* Liste des menus de Menu_Layers */
	/**********************************/
struct ColorButton
	{
	const wxString m_Title;
	int m_LayerNumber;
	int * m_Color;		// Pointeur sur la variable couleur
	bool m_NoDisplayIsColor;	// TRUE si bit ITEM_NOT_SHOW de la variable Color
	bool * m_NoDisplay;	// Pointeur sur la variable Display on/off si ce n'est pas la var
						// Color
	int m_Id;
	wxBitmapButton * m_Button;
	int m_State;
	wxCheckBox * m_CheckBox;	// Option Display ON/OFF
};

static ColorButton Msg_Layers_Cu=
{
	_("Copper Layers"), -1			/* Title */
};

static ColorButton Msg_Layers_Tech=
{
	_("Tech Layers"), -1				/* Title */
};

static ColorButton Layer_1_Butt=
{
	wxEmptyString,
	CUIVRE_N,		/* Title */
	ADR(CUIVRE_N),	/* adr du parametre optionnel */
	TRUE		  	/* parametre display on/off = bit ITEM_NOT_SHOW */
};

static ColorButton Layer_2_Butt=
{
	wxEmptyString,
	1,				/* Title */
	ADR(1),		/* adr du parametre optionnel */
	TRUE		/* parametre display on/off = bit ITEM_NOT_SHOW */
};

static ColorButton Layer_3_Butt=
{
	wxEmptyString,
	2,				/* Title */
	ADR(2),		/* adr du parametre optionnel */
	TRUE		/* parametre display on/off = bit ITEM_NOT_SHOW */
};

static ColorButton Layer_4_Butt=
{
	wxEmptyString,
	3,				/* Title */
	ADR(3),		/* adr du parametre optionnel */
	TRUE		/* parametre display on/off = bit ITEM_NOT_SHOW */
};

static ColorButton Layer_5_Butt=
{
	wxEmptyString,
	4,				/* Title */
	ADR(4),		/* adr du parametre optionnel */
	TRUE		/* parametre display on/off = bit ITEM_NOT_SHOW */
};

static ColorButton Layer_6_Butt=
{
	wxEmptyString,
	5,				/* Title */
	ADR(5),		/* adr du parametre optionnel */
	TRUE		/* parametre display on/off = bit ITEM_NOT_SHOW */
};

static ColorButton Layer_7_Butt=
{
	wxEmptyString,
	6,				/* Title */
	ADR(6),		/* adr du parametre optionnel */
	TRUE		/* parametre display on/off = bit ITEM_NOT_SHOW */
};

static ColorButton Layer_8_Butt=
{
	wxEmptyString,
	7,				/* Title */
	ADR(7),		/* adr du parametre optionnel */
	TRUE		/* parametre display on/off = bit ITEM_NOT_SHOW */
};

static ColorButton Layer_9_Butt=
{
	wxEmptyString,
	8,				/* Title */
	ADR(8),		/* adr du parametre optionnel */
	TRUE		/* parametre display on/off = bit ITEM_NOT_SHOW */
};

static ColorButton Layer_10_Butt=
{
	wxEmptyString,
	9,		/* Title */
	ADR(9),			/* adr du parametre optionnel */
	TRUE		/* parametre display on/off = bit ITEM_NOT_SHOW */
};

static ColorButton Layer_11_Butt=
{
	wxEmptyString,
	10,		/* Title */
	ADR(10),   		/* adr du parametre optionnel */
	TRUE			/* parametre display on/off = bit ITEM_NOT_SHOW */
};

static ColorButton Layer_12_Butt=
{
	wxEmptyString,
	11,	/* Title */
	ADR(11),	/* adr du parametre optionnel */
	TRUE		/* parametre display on/off = bit ITEM_NOT_SHOW */
};

static ColorButton Layer_13_Butt=
{
	wxEmptyString,
	12,			/* Title */
	ADR(12),			/* adr du parametre optionnel */
	TRUE		/* parametre display on/off = bit ITEM_NOT_SHOW */
};

static ColorButton Layer_14_Butt=
{
	wxEmptyString,
	13,			/* Title */
	ADR(13),			/* adr du parametre optionnel */
	TRUE		/* parametre display on/off = bit ITEM_NOT_SHOW */
};

static ColorButton Layer_15_Butt=
{
	wxEmptyString,
	14,				/* Title */
	ADR(14),		/* adr du parametre optionnel */
	TRUE		/* parametre display on/off = bit ITEM_NOT_SHOW */
};

static ColorButton Layer_16_Butt=
{
	wxEmptyString,
	CMP_N,	 	/* Title */
	ADR(CMP_N),			/* adr du parametre optionnel */
	TRUE		/* parametre display on/off = bit ITEM_NOT_SHOW */
};

static ColorButton Layer_17_Butt=
{
	wxEmptyString,
	ADHESIVE_N_CU,	  		/* Title */
	ADR(ADHESIVE_N_CU),		/* adr du parametre optionnel */
	TRUE		/* parametre display on/off = bit ITEM_NOT_SHOW */
};

static ColorButton Layer_18_Butt=
{
	wxEmptyString,
	ADHESIVE_N_CMP,			/* Title */
	ADR(ADHESIVE_N_CMP),	/* adr du parametre optionnel */
	TRUE		/* parametre display on/off = bit ITEM_NOT_SHOW */
};

static ColorButton Layer_19_Butt=
{
	wxEmptyString,
	SOLDERPASTE_N_CU,				/* Title */
	ADR(SOLDERPASTE_N_CU),		/* adr du parametre optionnel */
	TRUE		/* parametre display on/off = bit ITEM_NOT_SHOW */
};

static ColorButton Layer_20_Butt=
{
	wxEmptyString,
	SOLDERPASTE_N_CMP,			/* Title */
	ADR(SOLDERPASTE_N_CMP),		/* adr du parametre optionnel */
	TRUE		/* parametre display on/off = bit ITEM_NOT_SHOW */
};

static ColorButton Layer_21_Butt=
{
	wxEmptyString,
	SILKSCREEN_N_CU,	 		/* Title */
	ADR(SILKSCREEN_N_CU),		/* adr du parametre optionnel */
	TRUE		/* parametre display on/off = bit ITEM_NOT_SHOW */
};

static ColorButton Layer_22_Butt=
{
	wxEmptyString,
	SILKSCREEN_N_CMP,	 		/* Title */
	ADR(SILKSCREEN_N_CMP),		/* adr du parametre optionnel */
	TRUE		/* parametre display on/off = bit ITEM_NOT_SHOW */
};

static ColorButton Layer_23_Butt=
{
	wxEmptyString,
	SOLDERMASK_N_CU,				/* Title */
	ADR(SOLDERMASK_N_CU),	/* adr du parametre optionnel */
	TRUE					/* adr du parametre display on/off */
};

static ColorButton Layer_24_Butt=
{
	wxEmptyString,
	SOLDERMASK_N_CMP,				/* Title */
	ADR(SOLDERMASK_N_CMP),	/* adr du parametre optionnel */
	TRUE		/* parametre display on/off = bit ITEM_NOT_SHOW */
};

static ColorButton Layer_25_Butt=
{
	wxEmptyString,
	DRAW_N,				/* Title */
	ADR(DRAW_N),		/* adr du parametre optionnel */
	TRUE		/* parametre display on/off = bit ITEM_NOT_SHOW */
};

static ColorButton Layer_26_Butt=
{
	wxEmptyString,
	COMMENT_N,			/* Title */
	ADR(COMMENT_N),		/* adr du parametre optionnel */
	TRUE		/* parametre display on/off = bit ITEM_NOT_SHOW */
};

static ColorButton Layer_27_Butt=
{
	wxEmptyString,
	ECO1_N,			/* Title */
	ADR(ECO1_N),		/* adr du parametre optionnel */
	TRUE		/* parametre display on/off = bit ITEM_NOT_SHOW */
};

static ColorButton Layer_28_Butt=
{
	wxEmptyString,
	ECO2_N,			/* Title */
	ADR(ECO2_N),		/* adr du parametre optionnel */
	TRUE		/* parametre display on/off = bit ITEM_NOT_SHOW */
};

static ColorButton Layer_29_Butt=
{
	wxEmptyString,
	EDGE_N,			/* Title */
	ADR(EDGE_N),		/* adr du parametre optionnel */
	TRUE		/* parametre display on/off = bit ITEM_NOT_SHOW */
};


static ColorButton Msg_Others_Items=
{
	wxT("Others"), -1					/* Title */
};

static ColorButton Via_Normale_Butt=
{
	wxT("*"),
	VIA_NORMALE,			/* Title */
	&g_DesignSettings.m_ViaColor[VIA_NORMALE],	/* adr du parametre optionnel */
	TRUE		/* parametre display on/off = bit ITEM_NOT_SHOW */
};

static ColorButton Via_Aveugle_Butt=
{
	wxT("*"),
	VIA_ENTERREE,		/* Title */
	&g_DesignSettings.m_ViaColor[VIA_ENTERREE],		/* adr du parametre optionnel */
	TRUE		/* parametre display on/off = bit ITEM_NOT_SHOW */
};

static ColorButton Via_Borgne_Butt=
{
	wxT("*"),
	VIA_BORGNE,		/* Title */
	&g_DesignSettings.m_ViaColor[VIA_BORGNE],		/* adr du parametre optionnel */
	TRUE		/* parametre display on/off = bit ITEM_NOT_SHOW */
};

static ColorButton Ratsnest_Butt=
{
	_("Ratsnest"),			/* Title */
	-1,
	&g_DesignSettings.m_RatsnestColor,	/* adr du parametre optionnel */
	FALSE, &g_Show_Ratsnest		/* adr du parametre avec flag ITEM_NOT_SHOW */
};

static ColorButton Pad_Cu_Butt=
{
	_("Pad Cu"),				/* Title */
	-1,
	&g_PadCUColor,	/* adr du parametre optionnel */
	TRUE		/* parametre display on/off = bit ITEM_NOT_SHOW */
};

static ColorButton Pad_Cmp_Butt=
{
	_("Pad Cmp"),				/* Title */
	-1,
	&g_PadCMPColor,		/* adr du parametre optionnel */
	TRUE		/* parametre display on/off = bit ITEM_NOT_SHOW */
};

static ColorButton Text_Mod_Cu_Butt=
{
	_("Text Module Cu"),					/* Title */
	-1,
	&g_ModuleTextCUColor,		/* adr du parametre optionnel */
	TRUE		/* parametre display on/off = bit ITEM_NOT_SHOW */
};

static ColorButton Text_Mod_Cmp_Butt=
{
	_("Text Module Cmp"),				/* Title */
	-1,
	&g_ModuleTextCMPColor,		/* adr du parametre optionnel */
	TRUE		/* parametre display on/off = bit ITEM_NOT_SHOW */
};

static ColorButton Text_Mod_NoVisible_Butt=
{
	_("Text Module invisible"),	/* Title */
	-1,
	&g_ModuleTextNOVColor,	/* adr du parametre optionnel */
	TRUE		/* parametre display on/off = bit ITEM_NOT_SHOW */
};

static ColorButton Anchors_Butt=
{
	_("Anchors"),				/* Title */
	-1,
	&g_AnchorColor,		/* adr du parametre optionnel */
	TRUE		/* parametre display on/off = bit ITEM_NOT_SHOW */
};

static ColorButton Grid_Butt=
{
	_("Grid"),				/* Title */
	-1,
	&g_PcbGridColor,		/* adr du parametre optionnel */
	FALSE,
	&g_ShowGrid		/* parametre display on/off = bit ITEM_NOT_SHOW */
};

static ColorButton Show_Zones_Butt=
{
	_("Show Zones"),		/* Title */
	-1,
	NULL,				/* adr du parametre optionnel */
	FALSE,
	&DisplayOpt.DisplayZones	/* adr du parametre avec flag ITEM_NOT_SHOW */
};

static ColorButton Show_Pads_Noconnect_Butt=
{
	_("Show Noconnect"),		/* Title */
	-1,
	NULL,				/* adr du parametre optionnel */
	FALSE,
	&DisplayOpt.DisplayPadNoConn	/* adr du parametre avec flag ITEM_NOT_SHOW */
};

static ColorButton Show_Modules_Cmp_Butt=
{
	_("Show Modules Cmp"),		/* Title */
	-1,
	NULL,				/* adr du parametre optionnel */
	FALSE,
	&DisplayOpt.Show_Modules_Cmp	/* adr du parametre avec flag ITEM_NOT_SHOW */
};

static ColorButton Show_Modules_Cu_Butt=
{
	_("Show Modules Cu"),		/* Title */
	-1,
	NULL,				/* adr du parametre optionnel */
	FALSE,
	&DisplayOpt.Show_Modules_Cu	/* adr du parametre avec flag ITEM_NOT_SHOW */
};


static ColorButton * laytool_list[] = {
	&Msg_Layers_Cu,
	&Layer_1_Butt,
	&Layer_2_Butt,
	&Layer_3_Butt,
	&Layer_4_Butt,
	&Layer_5_Butt,
	&Layer_6_Butt,
	&Layer_7_Butt,
	&Layer_8_Butt,
	&Layer_9_Butt,
	&Layer_10_Butt,
	&Layer_11_Butt,
	&Layer_12_Butt,
	&Layer_13_Butt,
	&Layer_14_Butt,
	&Layer_15_Butt,
	&Layer_16_Butt,

	&Msg_Layers_Tech,
	&Layer_17_Butt,
	&Layer_18_Butt,
	&Layer_19_Butt,
	&Layer_20_Butt,
	&Layer_21_Butt,
	&Layer_22_Butt,
	&Layer_23_Butt,
	&Layer_24_Butt,
	&Layer_25_Butt,
	&Layer_26_Butt,
	&Layer_27_Butt,
	&Layer_28_Butt,
	&Layer_29_Butt,
//	&Layer_30_Butt,
//	&Layer_31_Butt,

	&Msg_Others_Items,
	&Via_Normale_Butt,
	&Via_Aveugle_Butt,
	&Via_Borgne_Butt,
	&Ratsnest_Butt,
	&Pad_Cu_Butt,
	&Pad_Cmp_Butt,
	&Text_Mod_Cu_Butt,
	&Text_Mod_Cmp_Butt,
	&Text_Mod_NoVisible_Butt,
	&Anchors_Butt,
	&Grid_Butt,

	&Show_Zones_Butt,
	&Show_Pads_Noconnect_Butt,
	&Show_Modules_Cmp_Butt,
	&Show_Modules_Cu_Butt,

	NULL
};

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
	~WinEDA_SetColorsFrame(void) {};

private:
	void OnQuit(wxCommandEvent& event);
	void SetColor(wxCommandEvent& event);
	void SetDisplayOnOff(wxCommandEvent& event);
	void ResetDisplayLayersCu(wxCommandEvent& event);
	DECLARE_EVENT_TABLE()

};
/* Table des evenements pour WinEDA_SetColorsFrame */
BEGIN_EVENT_TABLE(WinEDA_SetColorsFrame, wxDialog)
	EVT_BUTTON(ID_COLOR_RESET_SHOW_LAYER_OFF, WinEDA_SetColorsFrame::ResetDisplayLayersCu)
	EVT_BUTTON(ID_COLOR_RESET_SHOW_LAYER_ON, WinEDA_SetColorsFrame::ResetDisplayLayersCu)
	EVT_BUTTON(ID_COLOR_EXIT, WinEDA_SetColorsFrame::OnQuit)
	EVT_CHECKBOX(ID_COLOR_CHECKBOX_ONOFF, WinEDA_SetColorsFrame::SetDisplayOnOff)
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
	EVT_BUTTON(ID_COLOR_SETUP+37, WinEDA_SetColorsFrame::SetColor)
	EVT_BUTTON(ID_COLOR_SETUP+38, WinEDA_SetColorsFrame::SetColor)
	EVT_BUTTON(ID_COLOR_SETUP+39, WinEDA_SetColorsFrame::SetColor)
	EVT_BUTTON(ID_COLOR_SETUP+40, WinEDA_SetColorsFrame::SetColor)
	EVT_BUTTON(ID_COLOR_SETUP+41, WinEDA_SetColorsFrame::SetColor)
	EVT_BUTTON(ID_COLOR_SETUP+42, WinEDA_SetColorsFrame::SetColor)
	EVT_BUTTON(ID_COLOR_SETUP+43, WinEDA_SetColorsFrame::SetColor)
	EVT_BUTTON(ID_COLOR_SETUP+44, WinEDA_SetColorsFrame::SetColor)
END_EVENT_TABLE()

/*****************************************************/
void DisplayColorSetupFrame(WinEDA_DrawFrame * parent,
							const wxPoint & framepos)
/*****************************************************/
{
	WinEDA_SetColorsFrame * frame =
			new WinEDA_SetColorsFrame(parent, framepos);
	frame->ShowModal(); frame->Destroy();
}


/**********************************************************************/
WinEDA_SetColorsFrame::WinEDA_SetColorsFrame(WinEDA_DrawFrame *parent,
							const wxPoint& framepos):
			wxDialog(parent, -1, _("Colors:"), framepos,
			wxSize(-1, -1),
			DIALOG_STYLE )
/**********************************************************************/
{
#define START_Y 25
wxBitmapButton * ButtonB;
int ii, yy, xx, butt_ID, buttcolor;
wxPoint pos;
wxSize winsize;
int w = BUTT_SIZE_X, h = BUTT_SIZE_Y;
wxString msg;
	
	m_Parent = parent;
	SetFont(*g_DialogFont);

	pos.x = 5; pos.y = START_Y;
	for ( ii = 0; laytool_list[ii] != NULL; ii++ )
	{
		if( ! laytool_list[ii]->m_Color && ! laytool_list[ii]->m_NoDisplay )
		{
			if( pos.y != START_Y )
			{
				pos.x += w + 120; pos.y = START_Y;
			}
			if( laytool_list[ii]->m_LayerNumber >= 0 )
			{
				if ( laytool_list[ii]->m_Title == wxT("*") )
				{
					msg = g_ViaType_Name[laytool_list[ii]->m_LayerNumber];
				}
				else msg = ReturnPcbLayerName(laytool_list[ii]->m_LayerNumber);
			}
			else
				msg = wxGetTranslation(laytool_list[ii]->m_Title.GetData());

			new wxStaticText(this, -1, msg ,
				wxPoint(pos.x + 10, pos.y - 18 ), wxSize(-1,-1), 0 );
			continue;
		}

		if ( laytool_list[ii]->m_Id == 0 )
			laytool_list[ii]->m_Id = ID_COLOR_SETUP + ii;
		butt_ID = laytool_list[ii]->m_Id;

		laytool_list[ii]->m_CheckBox = new wxCheckBox(this,
						ID_COLOR_CHECKBOX_ONOFF, wxEmptyString,
						pos);

		if ( laytool_list[ii]->m_NoDisplayIsColor )
		{
			if ( *laytool_list[ii]->m_Color & ITEM_NOT_SHOW )
				laytool_list[ii]->m_CheckBox->SetValue(FALSE);
			else laytool_list[ii]->m_CheckBox->SetValue(TRUE);
		}

		else if ( laytool_list[ii]->m_NoDisplay )
			laytool_list[ii]->m_CheckBox->SetValue(*laytool_list[ii]->m_NoDisplay);

		xx = 3 + laytool_list[ii]->m_CheckBox->GetSize().x;

		if( laytool_list[ii]->m_Color )
		{
			wxMemoryDC iconDC;
			wxBitmap ButtBitmap(w,h);
			iconDC.SelectObject( ButtBitmap );
			buttcolor = *laytool_list[ii]->m_Color & MASKCOLOR;
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
			iconDC.DrawRectangle(0,0, w, h);

			ButtonB = new wxBitmapButton(this, butt_ID,
						ButtBitmap,
						wxPoint(pos.x + xx, pos.y),
						wxSize(w,h) );
			laytool_list[ii]->m_Button = ButtonB;
			xx += 3 + w;
		}

		if( laytool_list[ii]->m_LayerNumber >= 0 )
		{
			if ( laytool_list[ii]->m_Title == wxT("*") )
			{
				msg = g_ViaType_Name[laytool_list[ii]->m_LayerNumber];
			}
			else msg = ReturnPcbLayerName(laytool_list[ii]->m_LayerNumber);
		}
		else
			msg = wxGetTranslation(laytool_list[ii]->m_Title.GetData());

		new wxStaticText(this,-1, msg,
					wxPoint(pos.x + xx , pos.y + 1 ),
					wxSize(-1,-1), 0 );

		yy = h + 5;
		pos.y += yy;
		}

	pos.x = 150; pos.y = 300;
	wxButton * Button = new wxButton(this,ID_COLOR_RESET_SHOW_LAYER_ON,
						_("Show All"), pos);
	Button->SetForegroundColour(wxColor(0,100,0));

	pos.y += Button->GetSize().y + 2;
	Button = new wxButton(this,ID_COLOR_RESET_SHOW_LAYER_OFF,
						_("Show None"), pos);
	Button->SetForegroundColour(wxColor(100,0,0));

	pos.x += Button->GetSize().x + 50;
	Button = new wxButton(this,ID_COLOR_EXIT,
						_("Exit"), pos);
	Button->SetForegroundColour(*wxBLUE);
	winsize.x = 500; winsize.y = pos.y + Button->GetSize().y + 5;
	SetClientSize(winsize);
}


/**********************************************************************/
void  WinEDA_SetColorsFrame::OnQuit(wxCommandEvent& WXUNUSED(event))
/**********************************************************************/
{
    Close(true);    // true is to force the frame to close
}


/**********************************************************/
void WinEDA_SetColorsFrame::SetColor(wxCommandEvent& event)
/**********************************************************/
{
int ii;
int id = event.GetId();
int color;
int w = BUTT_SIZE_X, h = BUTT_SIZE_Y;


	color = DisplayColorFrame(this);
	if ( color < 0) return;

	for ( ii = 0; laytool_list[ii] != NULL; ii++ )
		{
		if( laytool_list[ii]->m_Id != id) continue;
		if( laytool_list[ii]->m_Color == NULL) continue;

		if( *laytool_list[ii]->m_Color == color) break;

		*laytool_list[ii]->m_Color = color;
		wxMemoryDC iconDC;

		wxBitmapButton * Button = laytool_list[ii]->m_Button;

		wxBitmap ButtBitmap = Button->GetBitmapLabel();
		iconDC.SelectObject( ButtBitmap );
		int buttcolor = *laytool_list[ii]->m_Color;
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
		iconDC.DrawRectangle(0,0, w, h);
		Button->Refresh();
		SetDisplayOnOff(event);
		m_Parent->m_CurrentScreen->SetRefreshReq();
		}
	Refresh(FALSE);
}


/******************************************************************/
void WinEDA_SetColorsFrame::SetDisplayOnOff(wxCommandEvent& event)
/******************************************************************/
{
	for ( int ii = 0; laytool_list[ii] != NULL; ii++ )
		{
		if ( laytool_list[ii]->m_CheckBox == NULL ) continue;
		if ( ! laytool_list[ii]->m_NoDisplayIsColor &&
			 (laytool_list[ii]->m_NoDisplay == NULL) ) continue;

		if ( laytool_list[ii]->m_NoDisplayIsColor )
			{
			if ( laytool_list[ii]->m_CheckBox->GetValue() )
				*laytool_list[ii]->m_Color &= ~ITEM_NOT_SHOW;
			else *laytool_list[ii]->m_Color |= ITEM_NOT_SHOW;
			}

		else
			{
			*laytool_list[ii]->m_NoDisplay = laytool_list[ii]->m_CheckBox->GetValue();
			}

		m_Parent->m_CurrentScreen->SetRefreshReq();
		}
}


/**********************************************************************/
void WinEDA_SetColorsFrame::ResetDisplayLayersCu(wxCommandEvent& event)
/**********************************************************************/
{
bool NewState = (event.GetId() == ID_COLOR_RESET_SHOW_LAYER_ON) ? TRUE : FALSE;

	for ( int ii = 1; ii < 17; ii++ )
		{
		if ( laytool_list[ii]->m_CheckBox == NULL ) continue;
		laytool_list[ii]->m_CheckBox->SetValue(NewState);
		}

	SetDisplayOnOff(event);
}

