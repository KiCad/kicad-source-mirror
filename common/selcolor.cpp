/****************/
/* SELCOLOR.CPP */
/****************/

/* Dialog for selecting color from the palette of available colors.
 */

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "colors.h"
#include "macros.h"

#include "wx/statline.h"


enum colors_id {
    ID_COLOR_BLACK = 2000 // ID_COLOR_ = ID_COLOR_BLACK a ID_COLOR_BLACK + 31
};


class WinEDA_SelColorFrame : public wxDialog
{
private:
public:
    WinEDA_SelColorFrame( wxWindow* parent,
                          const wxPoint& framepos, int OldColor );
    ~WinEDA_SelColorFrame() {};

private:
    void OnCancel( wxCommandEvent& event );
    void SelColor( wxCommandEvent& event );

    DECLARE_EVENT_TABLE()
};


BEGIN_EVENT_TABLE( WinEDA_SelColorFrame, wxDialog )
    EVT_BUTTON( wxID_CANCEL, WinEDA_SelColorFrame::OnCancel )
    EVT_COMMAND_RANGE( ID_COLOR_BLACK, ID_COLOR_BLACK + 31,
                       wxEVT_COMMAND_BUTTON_CLICKED,
                       WinEDA_SelColorFrame::SelColor )
END_EVENT_TABLE()


int DisplayColorFrame( wxWindow* parent, int OldColor )
{
    wxPoint framepos;
    int     color;

    wxGetMousePosition( &framepos.x, &framepos.y );

    WinEDA_SelColorFrame* frame = new WinEDA_SelColorFrame( parent,
                                                            framepos, OldColor );
    color = frame->ShowModal();
    frame->Destroy();
    if( color > NBCOLOR )
        color = -1;
    return color;
}


WinEDA_SelColorFrame::WinEDA_SelColorFrame( wxWindow*      parent,
                                            const wxPoint& framepos,
                                            int            OldColor ) :
    wxDialog( parent, -1, _( "Colors" ), framepos, wxDefaultSize,
              wxDEFAULT_DIALOG_STYLE | MAYBE_RESIZE_BORDER )
{
    wxBoxSizer*             OuterBoxSizer = NULL;
    wxBoxSizer*             MainBoxSizer  = NULL;
    wxFlexGridSizer*        FlexColumnBoxSizer = NULL;
    wxBitmapButton*         BitmapButton = NULL;
    wxStaticText*           Label = NULL;
    wxStaticLine*           Line  = NULL;
    wxStdDialogButtonSizer* StdDialogButtonSizer = NULL;
    wxButton* Button = NULL;

    int       ii, butt_ID, buttcolor;
    int       w = 20, h = 20;
    bool      ColorFound = false;

    SetReturnCode( -1 );

    OuterBoxSizer = new wxBoxSizer( wxVERTICAL );
    SetSizer( OuterBoxSizer );

    MainBoxSizer = new wxBoxSizer( wxHORIZONTAL );
    OuterBoxSizer->Add( MainBoxSizer, 1, wxGROW | wxLEFT | wxRIGHT | wxTOP, 5 );

    for( ii = 0; ColorRefs[ii].m_Name != NULL && ii < NBCOLOR; ii++ )
    {
        // Provide a separate column for every eight buttons (and their
        // associated text strings), so provide a FlexGrid Sizer with
        // eight rows and two columns.
        if( ii % 8 == 0 )
        {
            FlexColumnBoxSizer = new wxFlexGridSizer( 8, 2, 0, 0 );

            // Specify that all of the rows can be expanded.
            for( int ii = 0; ii < 8; ii++ )
            {
                FlexColumnBoxSizer->AddGrowableRow( ii );
            }

            // Specify that the second column can also be expanded.
            FlexColumnBoxSizer->AddGrowableCol( 1 );

            MainBoxSizer->Add( FlexColumnBoxSizer, 1, wxGROW | wxTOP, 5 );
        }

        butt_ID = ID_COLOR_BLACK + ii;
        wxMemoryDC iconDC;
        wxBitmap   ButtBitmap( w, h );
        wxBrush    Brush;
        iconDC.SelectObject( ButtBitmap );
        buttcolor = ColorRefs[ii].m_Numcolor;
        iconDC.SetPen( *wxBLACK_PEN );
        Brush.SetColour( ColorRefs[buttcolor].m_Red,
                         ColorRefs[buttcolor].m_Green,
                         ColorRefs[buttcolor].m_Blue );
        Brush.SetStyle( wxSOLID );

        iconDC.SetBrush( Brush );
        iconDC.SetBackground( *wxGREY_BRUSH );
        iconDC.Clear();
        iconDC.DrawRoundedRectangle( 0, 0, w, h, (double) h / 3 );

        BitmapButton = new wxBitmapButton( this, butt_ID, ButtBitmap,
                                           wxDefaultPosition, wxSize( w, h ) );
        FlexColumnBoxSizer->Add( BitmapButton, 0,
                                 wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL |
                                 wxLEFT | wxBOTTOM, 5 );

        // Set focus to this button if its color matches the
        // color which had been selected previously (for
        // whichever layer's color is currently being edited).
        if( OldColor == buttcolor )
        {
            ColorFound = true;
            BitmapButton->SetFocus();
        }

        Label = new wxStaticText( this, -1, ColorRefs[ii].m_Name,
                                  wxDefaultPosition, wxDefaultSize, 0 );
        FlexColumnBoxSizer->Add( Label, 1,
                                 wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL |
                                 wxLEFT | wxRIGHT | wxBOTTOM, 5 );
    }

    // Provide a Cancel button as well, so that this dialog
    // box can also be canceled by pressing the Esc key
    // (and also provide a horizontal static line to separate
    // that button from all of the other buttons).

    Line = new wxStaticLine( this, -1, wxDefaultPosition,
                             wxDefaultSize, wxLI_HORIZONTAL );
    OuterBoxSizer->Add( Line, 0, wxGROW | wxLEFT | wxRIGHT | wxTOP, 5 );

    StdDialogButtonSizer = new wxStdDialogButtonSizer;
    OuterBoxSizer->Add( StdDialogButtonSizer, 0, wxGROW | wxALL, 10 );

    Button = new wxButton( this, wxID_CANCEL, _( "Cancel" ), wxDefaultPosition,
                           wxDefaultSize, 0 );
    StdDialogButtonSizer->AddButton( Button );

    StdDialogButtonSizer->Realize();

    // Set focus to the Cancel button if the currently selected color
    // does not match any of the colors provided by this dialog box.
    // (That shouldn't ever happen in practice though.)
    if( !ColorFound )
        Button->SetFocus();

    // Resize the dialog
    if( GetSizer() )
    {
        GetSizer()->SetSizeHints( this );
    }
}


void WinEDA_SelColorFrame::OnCancel( wxCommandEvent& WXUNUSED( event ) )
{
    // Setting the return value to -1 indicates that the
    // dialog box has been canceled (and thus that the
    // previously selected color is to be retained).
    EndModal( -1 );
}


void WinEDA_SelColorFrame::SelColor( wxCommandEvent& event )
{
    int id = event.GetId();

    EndModal( id - ID_COLOR_BLACK );
}
