	/************************/
	/*	 SETCOLOR.CPP		*/
	/************************/
/* Affichage et selection de la palette des couleurs disponibles
dans une frame
*/

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "colors.h"


enum colors_id {
	ID_COLOR_BLACK = 2000,	// ID_COLOR_ = ID_COLOR_BLACK a ID_COLOR_BLACK + 31
};


/*******************************************/
class WinEDA_SelColorFrame: public wxDialog
/*******************************************/
/* Frame d'affichage de la palette des couleurs disponibles
*/
{
private:
public:

	// Constructor and destructor
	WinEDA_SelColorFrame(wxWindow *parent, const wxPoint& framepos);
	~WinEDA_SelColorFrame(void) {};

private:
	void SelColor(wxCommandEvent& event);

	DECLARE_EVENT_TABLE()

};
/* Construction de la table des evenements pour FrameClassMain */
BEGIN_EVENT_TABLE(WinEDA_SelColorFrame, wxDialog)
	EVT_BUTTON(ID_COLOR_BLACK, WinEDA_SelColorFrame::SelColor)
	EVT_BUTTON(ID_COLOR_BLACK+1, WinEDA_SelColorFrame::SelColor)
	EVT_BUTTON(ID_COLOR_BLACK+2, WinEDA_SelColorFrame::SelColor)
	EVT_BUTTON(ID_COLOR_BLACK+3, WinEDA_SelColorFrame::SelColor)
	EVT_BUTTON(ID_COLOR_BLACK+4, WinEDA_SelColorFrame::SelColor)
	EVT_BUTTON(ID_COLOR_BLACK+5, WinEDA_SelColorFrame::SelColor)
	EVT_BUTTON(ID_COLOR_BLACK+6, WinEDA_SelColorFrame::SelColor)
	EVT_BUTTON(ID_COLOR_BLACK+7, WinEDA_SelColorFrame::SelColor)
	EVT_BUTTON(ID_COLOR_BLACK+8, WinEDA_SelColorFrame::SelColor)
	EVT_BUTTON(ID_COLOR_BLACK+9, WinEDA_SelColorFrame::SelColor)
	EVT_BUTTON(ID_COLOR_BLACK+10, WinEDA_SelColorFrame::SelColor)
	EVT_BUTTON(ID_COLOR_BLACK+11, WinEDA_SelColorFrame::SelColor)
	EVT_BUTTON(ID_COLOR_BLACK+12, WinEDA_SelColorFrame::SelColor)
	EVT_BUTTON(ID_COLOR_BLACK+13, WinEDA_SelColorFrame::SelColor)
	EVT_BUTTON(ID_COLOR_BLACK+14, WinEDA_SelColorFrame::SelColor)
	EVT_BUTTON(ID_COLOR_BLACK+15, WinEDA_SelColorFrame::SelColor)
	EVT_BUTTON(ID_COLOR_BLACK+16, WinEDA_SelColorFrame::SelColor)
	EVT_BUTTON(ID_COLOR_BLACK+17, WinEDA_SelColorFrame::SelColor)
	EVT_BUTTON(ID_COLOR_BLACK+18, WinEDA_SelColorFrame::SelColor)
	EVT_BUTTON(ID_COLOR_BLACK+19, WinEDA_SelColorFrame::SelColor)
	EVT_BUTTON(ID_COLOR_BLACK+20, WinEDA_SelColorFrame::SelColor)
	EVT_BUTTON(ID_COLOR_BLACK+21, WinEDA_SelColorFrame::SelColor)
	EVT_BUTTON(ID_COLOR_BLACK+22, WinEDA_SelColorFrame::SelColor)
	EVT_BUTTON(ID_COLOR_BLACK+23, WinEDA_SelColorFrame::SelColor)
	EVT_BUTTON(ID_COLOR_BLACK+24, WinEDA_SelColorFrame::SelColor)
	EVT_BUTTON(ID_COLOR_BLACK+25, WinEDA_SelColorFrame::SelColor)
	EVT_BUTTON(ID_COLOR_BLACK+26, WinEDA_SelColorFrame::SelColor)
	EVT_BUTTON(ID_COLOR_BLACK+27, WinEDA_SelColorFrame::SelColor)
	EVT_BUTTON(ID_COLOR_BLACK+28, WinEDA_SelColorFrame::SelColor)
	EVT_BUTTON(ID_COLOR_BLACK+29, WinEDA_SelColorFrame::SelColor)
	EVT_BUTTON(ID_COLOR_BLACK+30, WinEDA_SelColorFrame::SelColor)
	EVT_BUTTON(ID_COLOR_BLACK+31, WinEDA_SelColorFrame::SelColor)
END_EVENT_TABLE()

/***************************************/
int DisplayColorFrame(wxWindow * parent)
/***************************************/
{
wxPoint framepos;
int color;

	wxGetMousePosition(&framepos.x, &framepos.y);

	WinEDA_SelColorFrame * frame = new WinEDA_SelColorFrame(parent,framepos);
	color = frame->ShowModal(); frame->Destroy();
	if (color > NBCOLOR) color = -1;
	return color;
}

/*******************************************************************/
WinEDA_SelColorFrame::WinEDA_SelColorFrame(wxWindow *parent,
							const wxPoint& framepos):
		wxDialog(parent, -1, _("Colors"), framepos, wxSize(375, 240),
				DIALOG_STYLE )
/*******************************************************************/
{
#define START_Y 10
wxBitmapButton * Button;
int ii, yy, butt_ID, buttcolor;
wxPoint pos;
int w = 20, h = 20;

	SetFont(*g_DialogFont);

	SetReturnCode(-1);

	pos.x = 5; pos.y = START_Y;
	for ( ii = 0; ColorRefs[ii].m_Name != NULL ; ii++ )
		{
		butt_ID = ID_COLOR_BLACK + ii;
		wxMemoryDC iconDC;
		wxBitmap ButtBitmap(w,h);
		wxBrush Brush;
		iconDC.SelectObject( ButtBitmap );
		buttcolor = ColorRefs[ii].m_Numcolor;
		iconDC.SetPen(*wxBLACK_PEN);
		Brush.SetColour(
						ColorRefs[buttcolor].m_Red,
						ColorRefs[buttcolor].m_Green,
						ColorRefs[buttcolor].m_Blue
						);
		Brush.SetStyle(wxSOLID);

		iconDC.SetBrush(Brush);
		iconDC.SetBackground(*wxGREY_BRUSH);
		iconDC.Clear();
		iconDC.DrawRoundedRectangle(0,0, w, h, (double)h/3);

		Button = new wxBitmapButton(this, butt_ID,
						ButtBitmap,
						wxPoint(pos.x, pos.y), wxSize(w,h) );

		new wxStaticText(this,-1,
					ColorRefs[ii].m_Name,
					wxPoint(pos.x + 2 + w , pos.y + 4 ),
					wxSize(-1,-1), 0 );

		yy = h + 5;
		pos.y += yy;
		if ( ii == 7 )
			{
			pos.x += w + 80; pos.y = START_Y;
			}
		else if ( (ii == 15) || (ii == 23) )
			{
			pos.x += w + 110; pos.y = START_Y;
			}
		}
}


/*********************************************************/
void WinEDA_SelColorFrame::SelColor(wxCommandEvent& event)
/*********************************************************/
{
int id = event.GetId();

	EndModal(id  - ID_COLOR_BLACK);
}



