	/************************/
	/*	 SETCOLOR.CPP		*/
	/************************/
/* Affichage et selection de la palette des couleurs disponibles
dans une frame
*/

#include "fctsys.h"
#include "gr_basic.h"
#include "macros.h"

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
	EVT_COMMAND_RANGE(ID_COLOR_BLACK,ID_COLOR_BLACK+31,
		wxEVT_COMMAND_BUTTON_CLICKED,
		WinEDA_SelColorFrame::SelColor)
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
int ii, butt_ID, buttcolor;
wxPoint pos;
int w = 20, h = 20;
wxStaticText * text;
int right, bottom, line_height;
	
	SetFont(*g_DialogFont);

	SetReturnCode(-1);

	bottom = pos.x = 5; right = pos.y = START_Y;
	line_height = h;
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

		text = new wxStaticText(this,-1,
					ColorRefs[ii].m_Name,
					wxPoint(pos.x + 2 + w , pos.y ),
					wxSize(-1,-1), 0 );
		line_height = MAX( line_height, text->GetRect().GetHeight());
		right = MAX(right, text->GetRect().GetRight());
		bottom = MAX(bottom, text->GetRect().GetBottom());
		
		Button = new wxBitmapButton(this, butt_ID,
						ButtBitmap,
						wxPoint(pos.x, pos.y  - ((h -line_height)/2)),
						wxSize(w,h) );

		pos.y +=  line_height + 5;
		if ( ii == 7 )
		{
			pos.x = right + 10; pos.y = START_Y;
		}
		else if ( (ii == 15) || (ii == 23) )
		{
			pos.x = right+ 10; pos.y = START_Y;
		}
	}
	
	SetClientSize( wxSize(right + 10, bottom + 10) );
}


/*********************************************************/
void WinEDA_SelColorFrame::SelColor(wxCommandEvent& event)
/*********************************************************/
{
int id = event.GetId();

	EndModal(id  - ID_COLOR_BLACK);
}



