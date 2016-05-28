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

#include <algorithm>


enum colors_id {
    ID_COLOR_BLACK = 2000 // colors_id = ID_COLOR_BLACK a ID_COLOR_BLACK + NBCOLORS-1
};


class CHOOSE_COLOR_DLG : public wxDialog
{
public:
    CHOOSE_COLOR_DLG( wxWindow* aParent, EDA_COLOR_T aOldColor );
    ~CHOOSE_COLOR_DLG() {};

    EDA_COLOR_T GetSelectedColor() { return m_color; }

private:
    void init_Dialog();
    void selColor( wxCommandEvent& event );

    EDA_COLOR_T m_color;

    DECLARE_EVENT_TABLE()
};


BEGIN_EVENT_TABLE( CHOOSE_COLOR_DLG, wxDialog )
    EVT_COMMAND_RANGE( ID_COLOR_BLACK, ID_COLOR_BLACK + NBCOLORS,
                       wxEVT_COMMAND_BUTTON_CLICKED,
                       CHOOSE_COLOR_DLG::selColor )
END_EVENT_TABLE()


EDA_COLOR_T DisplayColorFrame( wxWindow* aParent, EDA_COLOR_T aOldColor )
{
    CHOOSE_COLOR_DLG dlg( aParent, aOldColor );

    if( dlg.ShowModal() == wxID_OK )
    {
        return dlg.GetSelectedColor();
    }

    return UNSPECIFIED_COLOR;
}


CHOOSE_COLOR_DLG::CHOOSE_COLOR_DLG( wxWindow* aParent, EDA_COLOR_T aOldColor ) :
    wxDialog( aParent, wxID_ANY, _( "Colors" ), wxDefaultPosition, wxDefaultSize,
              wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER )
{
    m_color = aOldColor;

    init_Dialog();
    // Resize the dialog
    GetSizer()->SetSizeHints( this );

    Centre();
}

void CHOOSE_COLOR_DLG::init_Dialog()
{
    wxFlexGridSizer*        FlexColumnBoxSizer = NULL;
    wxBitmapButton*         focusedButton = NULL;
    const int w = 20, h = 20;

    wxBoxSizer* OuterBoxSizer = new wxBoxSizer( wxVERTICAL );
    SetSizer( OuterBoxSizer );

    wxBoxSizer*MainBoxSizer = new wxBoxSizer( wxHORIZONTAL );
    OuterBoxSizer->Add( MainBoxSizer, 1, wxGROW | wxLEFT | wxRIGHT | wxTOP, 5 );

    for( int ii = 0; ii < NBCOLORS; ++ii )
    {
        // Provide a separate column for every six buttons (and their
        // associated text strings), so provide a FlexGrid Sizer with
        // six rows and two columns.
        if( ii % 6 == 0 )
        {
            FlexColumnBoxSizer = new wxFlexGridSizer( 6, 2, 0, 0 );

            // Specify that all of the rows can be expanded.
            for( int kk = 0; kk < 6; kk++ )
            {
                FlexColumnBoxSizer->AddGrowableRow( kk );
            }

            // Specify that the second column can also be expanded.
            FlexColumnBoxSizer->AddGrowableCol( 1 );

            MainBoxSizer->Add( FlexColumnBoxSizer, 1, wxGROW | wxTOP, 5 );
        }

        int butt_ID = ID_COLOR_BLACK + ii;
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

        wxBitmapButton* bitmapButton = new wxBitmapButton( this, butt_ID, ButtBitmap,
                                           wxDefaultPosition, wxSize( w+8, h+6 ) );
        FlexColumnBoxSizer->Add( bitmapButton, 0,
                                 wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL |
                                 wxLEFT | wxBOTTOM, 5 );

        // Set focus to this button if its color matches the
        // color which had been selected previously (for
        // whichever layer's color is currently being edited).
        if( m_color == buttcolor )
            focusedButton = bitmapButton;

        wxStaticText* label = new wxStaticText( this, -1, wxGetTranslation( ColorGetName( buttcolor ) ),
                                        wxDefaultPosition, wxDefaultSize, 0 );
        FlexColumnBoxSizer->Add( label, 1,
                                 wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL |
                                 wxLEFT | wxRIGHT | wxBOTTOM, 5 );
    }

    // Provide a Cancel button as well, so that this dialog
    // box can also be canceled by pressing the Esc key
    // (and also provide a horizontal static line to separate
    // that button from all of the other buttons).

    wxStaticLine* sline = new wxStaticLine( this, -1, wxDefaultPosition,
                             wxDefaultSize, wxLI_HORIZONTAL );
    OuterBoxSizer->Add( sline, 0, wxGROW | wxLEFT | wxRIGHT | wxTOP, 5 );

    wxStdDialogButtonSizer* stdDialogButtonSizer = new wxStdDialogButtonSizer;
    OuterBoxSizer->Add( stdDialogButtonSizer, 0, wxGROW | wxALL, 10 );

    wxButton* cancelButton = new wxButton( this, wxID_CANCEL, _( "Cancel" ),
                                     wxDefaultPosition, wxDefaultSize, 0 );
    stdDialogButtonSizer->AddButton( cancelButton );

    stdDialogButtonSizer->Realize();

    // Set focus to the Cancel button if the currently selected color
    // does not match any of the colors provided by this dialog box.
    // (That shouldn't ever happen in practice though.)
    if( focusedButton )
        focusedButton->SetFocus();
    else
        cancelButton->SetFocus();
}


void CHOOSE_COLOR_DLG::selColor( wxCommandEvent& event )
{
    int id = event.GetId();
    m_color = EDA_COLOR_T( id - ID_COLOR_BLACK );

    // Close the dialog by calling the default dialog handler for a wxID_OK event
    event.SetId( wxID_OK );
    event.Skip();
}
