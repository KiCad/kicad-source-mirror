/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2014 KiCad Developers, see CHANGELOG.TXT for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */


/* Dialog for selecting color from the palette of available colors.
 */

#include <fctsys.h>
#include <common.h>
#include <colors.h>

#include <wx/statline.h>


enum colors_id {
    ID_COLOR_BLACK = 2000 // ID_COLOR_ = ID_COLOR_BLACK a ID_COLOR_BLACK + 31
};


class WinEDA_SelColorFrame : public wxDialog
{
public:
    WinEDA_SelColorFrame( wxWindow* parent,
                          const wxPoint& framepos, int OldColor );
    ~WinEDA_SelColorFrame() {};

private:
    void Init_Dialog( int aOldColor );
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


EDA_COLOR_T DisplayColorFrame( wxWindow* parent, int OldColor )
{
    wxPoint framepos;
    EDA_COLOR_T     color;

    wxGetMousePosition( &framepos.x, &framepos.y );

    WinEDA_SelColorFrame* frame = new WinEDA_SelColorFrame( parent,
                                                            framepos, OldColor );
    color = static_cast<EDA_COLOR_T>( frame->ShowModal() );
    frame->Destroy();
    if( color > NBCOLORS )
        color = UNSPECIFIED_COLOR;
    return color;
}


WinEDA_SelColorFrame::WinEDA_SelColorFrame( wxWindow*      parent,
                                            const wxPoint& framepos,
                                            int            OldColor ) :
    wxDialog( parent, -1, _( "Colors" ), framepos, wxDefaultSize,
              wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER )
{

    Init_Dialog( OldColor );
    // Resize the dialog
    GetSizer()->SetSizeHints( this );

    // Ensure the whole frame is visible, whenever the asked position.
    // Moreover with a work station having dual monitors, the asked position can be relative to a monitor
    // and this frame can be displayed on the other monitor, with an "out of screen" position.
    // Give also a small margin.
    int margin = 10;
    wxPoint windowPosition = GetPosition();
    if( framepos != wxDefaultPosition )
    {
        if( windowPosition.x < margin )
            windowPosition.x = margin;
        // Under MACOS, a vertical margin >= 20 is needed by the system menubar
        int v_margin = std::max(20, margin);
        if( windowPosition.y < v_margin )
            windowPosition.y = v_margin;
        if( windowPosition != framepos )
            SetPosition(windowPosition);
    }
    wxPoint endCornerPosition = GetPosition();
    endCornerPosition.x += GetSize().x + margin;
    endCornerPosition.y += GetSize().y + margin;

    windowPosition = GetPosition();
    wxRect freeScreenArea( wxGetClientDisplayRect( ) );

    if( freeScreenArea.GetRight() < endCornerPosition.x )
    {
        windowPosition.x += freeScreenArea.GetRight() - endCornerPosition.x;

        if( windowPosition.x < freeScreenArea.x )
            windowPosition.x = freeScreenArea.x;

        // Sligly modify the vertical position to avoid the mouse to be
        // exactly on the upper side of the window
        windowPosition.y +=5;
        endCornerPosition.y += 5;
    }

    if( freeScreenArea.GetBottom() < endCornerPosition.y )
    {
        windowPosition.y += freeScreenArea.GetBottom() - endCornerPosition.y;

        if( windowPosition.y < freeScreenArea.y )
            windowPosition.y = freeScreenArea.y;
    }

    SetPosition(windowPosition);
}

void WinEDA_SelColorFrame::Init_Dialog( int aOldColor )
{
    wxBoxSizer*             OuterBoxSizer = NULL;
    wxBoxSizer*             MainBoxSizer  = NULL;
    wxFlexGridSizer*        FlexColumnBoxSizer = NULL;
    wxBitmapButton*         BitmapButton = NULL;
    wxStaticText*           Label = NULL;
    wxStaticLine*           Line  = NULL;
    wxStdDialogButtonSizer* StdDialogButtonSizer = NULL;
    wxButton* Button = NULL;

    int       ii, butt_ID;
    int       w = 20, h = 20;
    bool      ColorFound = false;

    SetReturnCode( -1 );

    OuterBoxSizer = new wxBoxSizer( wxVERTICAL );
    SetSizer( OuterBoxSizer );

    MainBoxSizer = new wxBoxSizer( wxHORIZONTAL );
    OuterBoxSizer->Add( MainBoxSizer, 1, wxGROW | wxLEFT | wxRIGHT | wxTOP, 5 );

    for( ii = 0; ii < NBCOLORS; ++ii )
    {
        // Provide a separate column for every six buttons (and their
        // associated text strings), so provide a FlexGrid Sizer with
        // eight rows and two columns.
        if( ii % 6 == 0 )
        {
            FlexColumnBoxSizer = new wxFlexGridSizer( 6, 2, 0, 0 );

            // Specify that all of the rows can be expanded.
            for( int ii = 0; ii < 6; ii++ )
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
        wxBrush    brush;

        iconDC.SelectObject( ButtBitmap );

        EDA_COLOR_T buttcolor = g_ColorRefs[ii].m_Numcolor;

        iconDC.SetPen( *wxBLACK_PEN );
        ColorSetBrush( &brush, buttcolor );
        brush.SetStyle( wxBRUSHSTYLE_SOLID );

        iconDC.SetBrush( brush );
        iconDC.SetBackground( *wxGREY_BRUSH );
        iconDC.Clear();
        iconDC.DrawRoundedRectangle( 0, 0, w, h, (double) h / 3 );

        BitmapButton = new wxBitmapButton( this, butt_ID, ButtBitmap,
                                           wxDefaultPosition, wxSize( w+8, h+6 ) );
        FlexColumnBoxSizer->Add( BitmapButton, 0,
                                 wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL |
                                 wxLEFT | wxBOTTOM, 5 );

        // Set focus to this button if its color matches the
        // color which had been selected previously (for
        // whichever layer's color is currently being edited).
        if( aOldColor == buttcolor )
        {
            ColorFound = true;
            BitmapButton->SetFocus();
        }

        Label = new wxStaticText( this, -1, ColorGetName( buttcolor ),
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
