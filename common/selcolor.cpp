/************************/
/*	 SETCOLOR.CPP		*/
/************************/
/* Affichage et selection de la palette des couleurs disponibles
 * dans une frame
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
	WinEDA_SelColorFrame(wxWindow *parent,
					const wxPoint& framepos, int OldColor);
	~WinEDA_SelColorFrame(void) {};

private:
	void OnCancel(wxCommandEvent& event);
	void SelColor(wxCommandEvent& event);
	DECLARE_EVENT_TABLE()
};


/* Construction de la table des evenements pour FrameClassMain */
BEGIN_EVENT_TABLE(WinEDA_SelColorFrame, wxDialog)
	EVT_BUTTON(wxID_CANCEL, WinEDA_SelColorFrame::OnCancel)
	EVT_COMMAND_RANGE( ID_COLOR_BLACK, ID_COLOR_BLACK + 31,
		wxEVT_COMMAND_BUTTON_CLICKED, WinEDA_SelColorFrame::SelColor )
END_EVENT_TABLE()


/***************************************/
int DisplayColorFrame(wxWindow * parent, int OldColor)
/***************************************/
{
wxPoint framepos;
int color;

	wxGetMousePosition(&framepos.x, &framepos.y);

	WinEDA_SelColorFrame * frame = new WinEDA_SelColorFrame(parent,
										framepos, OldColor);
	color = frame->ShowModal();
	frame->Destroy();
	if( color > NBCOLOR )
		color = -1;
	return color;
}


/*******************************************************************/
WinEDA_SelColorFrame::WinEDA_SelColorFrame(wxWindow *parent,
							const wxPoint& framepos, int OldColor):
		wxDialog(parent, -1, _("Colors"), framepos, wxSize(375, 240),
				DIALOG_STYLE )
/*******************************************************************/
{
#define START_Y 10
wxBitmapButton * BitmapButton;
wxButton * Button;
int ii, butt_ID, buttcolor;
wxPoint pos;
int w = 20, h = 20;
wxStaticText * text;
int right, bottom, line_height;
bool ColorFound = false;

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
		iconDC.DrawRoundedRectangle(0, 0, w, h, (double)h / 3);

		text = new wxStaticText( this, -1,
					ColorRefs[ii].m_Name,
					wxPoint( pos.x + 2 + w, pos.y ),
					wxSize(-1, -1), 0 );
		line_height = MAX( line_height, text->GetRect().GetHeight() );
		right = MAX( right, text->GetRect().GetRight() );
		bottom = MAX( bottom, text->GetRect().GetBottom() );

		BitmapButton = new wxBitmapButton( this, butt_ID,
							ButtBitmap,
							wxPoint( pos.x, pos.y  - (h - line_height) / 2 ),
							wxSize(w, h) );

		// Set focus to this button if its color matches the
		// color which had been selected previously (for
		// whichever layer's color is currently being edited).
		if( OldColor == buttcolor )
		{
			ColorFound = true;
			BitmapButton->SetFocus();
		}

		pos.y += line_height + 5;
		if ( ii == 7 || ii == 15 )
		{
			pos.x = right + 10;
			pos.y = START_Y;
		}
	}

	pos.x = 140;

	// Provide a Cancel button as well, so that this dialog
	// box can also be cancelled by pressing the Esc key.
	Button = new wxButton( this, wxID_CANCEL, _( "Cancel" ), pos );
	Button->SetForegroundColour( *wxBLUE );
	// Set focus to the Cancel button if the currently selected color
	// does not match any of the colors provided by this dialog box.
	// (That shouldn't ever happen in practice though.)
	if( !ColorFound )
		Button->SetFocus();

	SetClientSize( wxSize( right + 10, bottom + 40 ) );
}


/***************************************************************/
void WinEDA_SelColorFrame::OnCancel(wxCommandEvent& WXUNUSED(event))
/***************************************************************/
/* Called by the Cancel button
 */
{
	// Setting the return value to -1 indicates that the
	// dialog box has been cancelled (and thus that the
	// previously selected color is to be retained).
	EndModal(-1);
}


/*********************************************************/
void WinEDA_SelColorFrame::SelColor(wxCommandEvent& event)
/*********************************************************/
{
int id = event.GetId();

	EndModal(id  - ID_COLOR_BLACK);
}

