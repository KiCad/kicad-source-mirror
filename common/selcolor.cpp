/****************/
/* SETCOLOR.CPP */
/****************/
/* Affichage et selection de la palette des couleurs disponibles
 * dans une frame
 */

#include "fctsys.h"
#include "gr_basic.h"
#include "macros.h"

#include "common.h"
#include "colors.h"

#include "wx/statline.h"


enum colors_id {
    ID_COLOR_BLACK = 2000, // ID_COLOR_ = ID_COLOR_BLACK a ID_COLOR_BLACK + 31
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
    WinEDA_SelColorFrame( wxWindow *parent,
                          const wxPoint& framepos, int OldColor );
    ~WinEDA_SelColorFrame() {};

private:
    void OnCancel(wxCommandEvent& event);
    void SelColor(wxCommandEvent& event);
    DECLARE_EVENT_TABLE()
};


/* Construction de la table des evenements pour FrameClassMain */
BEGIN_EVENT_TABLE(WinEDA_SelColorFrame, wxDialog)
    EVT_BUTTON( wxID_CANCEL, WinEDA_SelColorFrame::OnCancel )
    EVT_COMMAND_RANGE( ID_COLOR_BLACK, ID_COLOR_BLACK + 31,
                       wxEVT_COMMAND_BUTTON_CLICKED,
                       WinEDA_SelColorFrame::SelColor )
END_EVENT_TABLE()


/***************************************/
int DisplayColorFrame(wxWindow * parent, int OldColor)
/***************************************/
{
wxPoint framepos;
int color;

    wxGetMousePosition(&framepos.x, &framepos.y);

    WinEDA_SelColorFrame * frame = new WinEDA_SelColorFrame( parent,
                                       framepos, OldColor );
    color = frame->ShowModal();
    frame->Destroy();
    if( color > NBCOLOR )
        color = -1;
    return color;
}


/*******************************************************************/
WinEDA_SelColorFrame::WinEDA_SelColorFrame( wxWindow *parent,
                             const wxPoint& framepos, int OldColor ):
        wxDialog( parent, -1, _("Colors"), framepos, wxDefaultSize,
                  DIALOG_STYLE )
/*******************************************************************/
{
wxBoxSizer* OuterBoxSizer = NULL;
wxBoxSizer* MainBoxSizer = NULL;
wxBoxSizer* ColumnBoxSizer = NULL;
wxBoxSizer* RowBoxSizer = NULL;
wxBitmapButton* BitmapButton = NULL;
wxStaticText* text = NULL;
wxStaticLine* line = NULL;
wxButton* Button = NULL;
int ii, butt_ID, buttcolor;
int w = 20, h = 20;
bool ColorFound = false;

    SetFont( *g_DialogFont );

    SetReturnCode( -1 );


    OuterBoxSizer = new wxBoxSizer(wxVERTICAL);
    SetSizer(OuterBoxSizer);

    MainBoxSizer = new wxBoxSizer(wxHORIZONTAL);
    OuterBoxSizer->Add(MainBoxSizer, 0, wxALIGN_CENTER_HORIZONTAL|wxLEFT|wxRIGHT|wxTOP, 5);

    for( ii = 0; ColorRefs[ii].m_Name != NULL; ii++ )
    {
        // Provide a separate column for every eight buttons (and
        // their associated text strings)
        if( ii % 8 == 0 )
        {
            ColumnBoxSizer = new wxBoxSizer(wxVERTICAL);
            MainBoxSizer->Add(ColumnBoxSizer, 0, wxALIGN_TOP|wxTOP, 5);
        }

        // Provide a sizer for each button and its associated text string
        RowBoxSizer = new wxBoxSizer(wxHORIZONTAL);
        ColumnBoxSizer->Add(RowBoxSizer, 0, wxALIGN_LEFT, 5);

        butt_ID = ID_COLOR_BLACK + ii;
        wxMemoryDC iconDC;
        wxBitmap ButtBitmap( w, h );
        wxBrush Brush;
        iconDC.SelectObject( ButtBitmap );
        buttcolor = ColorRefs[ii].m_Numcolor;
        iconDC.SetPen( *wxBLACK_PEN );
        Brush.SetColour(
                        ColorRefs[buttcolor].m_Red,
                        ColorRefs[buttcolor].m_Green,
                        ColorRefs[buttcolor].m_Blue
                        );
        Brush.SetStyle( wxSOLID );

        iconDC.SetBrush( Brush );
        iconDC.SetBackground( *wxGREY_BRUSH );
        iconDC.Clear();
        iconDC.DrawRoundedRectangle( 0, 0, w, h, (double)h / 3 );

        BitmapButton = new wxBitmapButton( this, butt_ID, ButtBitmap,
                                           wxDefaultPosition, wxSize( w, h ) );
        RowBoxSizer->Add(BitmapButton, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxBOTTOM, 5);

        // Set focus to this button if its color matches the
        // color which had been selected previously (for
        // whichever layer's color is currently being edited).
        if( OldColor == buttcolor )
        {
            ColorFound = true;
            BitmapButton->SetFocus();
        }

        text = new wxStaticText( this, -1, ColorRefs[ii].m_Name,
                                 wxDefaultPosition, wxDefaultSize, 0 );
        RowBoxSizer->Add(text, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT|wxBOTTOM, 5);
    }

    // Provide a Cancel button as well, so that this dialog
    // box can also be cancelled by pressing the Esc key
    // (and also provide a horizontal static line to separate
    // that button from all of the other buttons).

    line = new wxStaticLine( this, -1, wxDefaultPosition, wxDefaultSize, wxLI_HORIZONTAL );
    OuterBoxSizer->Add(line, 0, wxGROW|wxLEFT|wxRIGHT|wxTOP, 5);

#if 0
    BottomBoxSizer = new wxBoxSizer(wxHORIZONTAL);
    OuterBoxSizer->Add(BottomBoxSizer, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

    Button = new wxButton( this, wxID_OK, _( "OK" ), wxDefaultPosition, wxDefaultSize, 0 );
    Button->SetForegroundColour( *wxRED );
    BottomBoxSizer->Add(Button, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

    Button = new wxButton( this, wxID_CANCEL, _( "Cancel" ), wxDefaultPosition, wxDefaultSize, 0 );
    Button->SetForegroundColour( *wxBLUE );
    BottomBoxSizer->Add(Button, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);

    // Dialog boxes usually contain both an "OK" button and a "Cancel" button (and sometimes
    // also contain an "Apply" button). The previous code implements an additional sizer
    // (to contain such buttons), then installs that sizer into the outermost sizer, then
    // implements "OK" and "Cancel" buttons, and then installs those buttons into that sizer.
    //
    // However, as this particular dialog does not contain an "OK" button (nor an "Apply"
    // button), it is not necessary to provide an additional sizer to contain (just) a
    // "Cancel" button; that button can be installed directly into the outermost sizer
    // instead. (Note that a value of 10 has been specified for the margin surrounding that
    // button; that provides the same outcome as specifying the customary value of 5 for both
    // that button, and the BottomBoxSizer that it would otherwise be installed within.)
#endif

    Button = new wxButton( this, wxID_CANCEL, _( "Cancel" ), wxDefaultPosition, wxDefaultSize, 0 );
    Button->SetForegroundColour( *wxBLUE );
    OuterBoxSizer->Add(Button, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 10);

    // Set focus to the Cancel button if the currently selected color
    // does not match any of the colors provided by this dialog box.
    // (That shouldn't ever happen in practice though.)
    if( !ColorFound )
        Button->SetFocus();

    // Resize the dialog
    if (GetSizer())
    {
        GetSizer()->SetSizeHints(this);
    }
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

    EndModal( id - ID_COLOR_BLACK );
}
