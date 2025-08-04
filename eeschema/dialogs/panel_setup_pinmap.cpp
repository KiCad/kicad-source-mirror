/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <sch_edit_frame.h>
#include <kiface_base.h>
#include <bitmaps.h>
#include <wildcards_and_files_ext.h>
#include <schematic.h>
#include <connection_graph.h>
#include <tool/tool_manager.h>
#include <panel_setup_pinmap.h>
#include <erc/erc.h>
#include <id.h>
#include <wx/bmpbuttn.h>
#include <wx/statline.h>
#include <wx/stattext.h>
#include <widgets/bitmap_button.h>


// Control identifiers for events
#define ID_MATRIX_0 1800


// NC is not included in the pin map as it generates errors separately
#define PINMAP_TYPE_COUNT ( ELECTRICAL_PINTYPES_TOTAL - 1 )


BEGIN_EVENT_TABLE( PANEL_SETUP_PINMAP, PANEL_SETUP_PINMAP_BASE )
    EVT_COMMAND_RANGE( ID_MATRIX_0, ID_MATRIX_0 + ( PINMAP_TYPE_COUNT * PINMAP_TYPE_COUNT ) - 1,
                       wxEVT_COMMAND_BUTTON_CLICKED, PANEL_SETUP_PINMAP::changeErrorLevel )
END_EVENT_TABLE()


PANEL_SETUP_PINMAP::PANEL_SETUP_PINMAP( wxWindow* aWindow, SCH_EDIT_FRAME* parent ) :
    PANEL_SETUP_PINMAP_BASE( aWindow ),
    m_buttonList(),
    m_initialized( false )
{
    m_parent    = parent;
    m_schematic = &parent->Schematic();
    m_btnBackground = wxSystemSettings::GetColour( wxSystemColour::wxSYS_COLOUR_WINDOW );

    reBuildMatrixPanel();
}


PANEL_SETUP_PINMAP::~PANEL_SETUP_PINMAP()
{
#ifndef __WXMAC__
    if( m_initialized )
    {
        for( int ii = 0; ii < PINMAP_TYPE_COUNT; ii++ )
        {
            for( int jj = 0; jj <= ii; jj++ )
            {
                m_buttonList[ii][jj]->Unbind( wxEVT_ENTER_WINDOW,
                        &PANEL_SETUP_PINMAP::OnMouseLeave, this );
                m_buttonList[ii][jj]->Unbind( wxEVT_LEAVE_WINDOW,
                        &PANEL_SETUP_PINMAP::OnMouseLeave, this );
            }
        }
    }
#endif
}


void PANEL_SETUP_PINMAP::ResetPanel()
{
    m_schematic->ErcSettings().ResetPinMap();
    reBuildMatrixPanel();
}


void PANEL_SETUP_PINMAP::OnMouseEnter( wxMouseEvent& aEvent )
{
    wxBitmapButton* btn = static_cast<wxBitmapButton*>( aEvent.GetEventObject() );
    m_btnBackground = btn->GetBackgroundColour();

    btn->SetBackgroundColour(
            wxSystemSettings::GetColour( wxSystemColour::wxSYS_COLOUR_HIGHLIGHT ) );
}


void PANEL_SETUP_PINMAP::OnMouseLeave( wxMouseEvent& aEvent )
{
    wxBitmapButton* btn = static_cast<wxBitmapButton*>( aEvent.GetEventObject() );
    btn->SetBackgroundColour( m_btnBackground );
}


void PANEL_SETUP_PINMAP::reBuildMatrixPanel()
{
    // Try to know the size of bitmap button used in drc matrix
    wxBitmapButton* dummy =
            new wxBitmapButton( m_matrixPanel, wxID_ANY, KiBitmapBundle( BITMAPS::ercerr ),
                                wxDefaultPosition, wxDefaultSize, wxBORDER_NONE );
    wxSize          bitmapSize = dummy->GetSize();
    delete dummy;

#ifdef __WXMAC__
    const wxSize text_padding( 2, 1 );
    const int    twiddle = -1;
#else
    const wxSize text_padding( 8, 6 );
    const int    twiddle = 1;
#endif

    wxSize        charSize = KIUI::GetTextSize( wxS( "X" ), m_matrixPanel );
    wxPoint       pos( 0, charSize.y * 2 );
    wxStaticText* text;

    if( !m_initialized )
    {
        std::vector<wxStaticText*> labels;

        // Print row labels
        for( int ii = 0; ii < PINMAP_TYPE_COUNT; ii++ )
        {
            int y = pos.y + ( ii * ( bitmapSize.y + text_padding.y ) );
            text = new wxStaticText( m_matrixPanel, - 1, CommentERC_H[ii],
                                     wxPoint( 5, y + ( bitmapSize.y / 2 ) - ( 12 / 2 ) ) );
            labels.push_back( text );

            int x = text->GetRect().GetRight();
            pos.x = std::max( pos.x, x );
        }

        // Right-align
        for( int ii = 0; ii < PINMAP_TYPE_COUNT; ii++ )
        {
            wxPoint labelPos = labels[ ii ]->GetPosition();
            labelPos.x = pos.x - labels[ ii ]->GetRect().GetWidth();
            labelPos.y += twiddle;
            labels[ ii ]->SetPosition( labelPos );
        }

        pos.x += 5;
    }
    else
    {
        pos = m_buttonList[0][0]->GetPosition();
    }

    for( int ii = 0; ii < PINMAP_TYPE_COUNT; ii++ )
    {
        int y = pos.y + (ii * ( bitmapSize.y + text_padding.y ) );

        for( int jj = 0; jj <= ii; jj++ )
        {
            // Add column labels (only once)
            PIN_ERROR diag = m_schematic->ErcSettings().GetPinMapValue( ii, jj );

            int x = pos.x + ( jj * ( bitmapSize.x + text_padding.x ) );

            if( ( ii == jj ) && !m_initialized )
            {
                wxPoint textPos( x + KiROUND( bitmapSize.x / 2.0 ),
                                 y - charSize.y * 2 );
                new wxStaticText( m_matrixPanel, wxID_ANY, CommentERC_V[ii], textPos );

                wxPoint calloutPos( x + KiROUND( bitmapSize.x / 2.0 ),
                                    y - charSize.y );
                new wxStaticText( m_matrixPanel, wxID_ANY, "|", calloutPos );
            }

            int id = ID_MATRIX_0 + ii + ( jj * PINMAP_TYPE_COUNT );
            BITMAPS bitmap_butt = BITMAPS::erc_green;

#ifdef __WXMAC__
            BITMAP_BUTTON* btn = new BITMAP_BUTTON( m_matrixPanel, id, KiBitmap( bitmap_butt ),
                                                    wxPoint( x, y ), bitmapSize );
#else
            if( m_initialized )
            {
                m_buttonList[ii][jj]->Unbind( wxEVT_ENTER_WINDOW,
                                              &PANEL_SETUP_PINMAP::OnMouseLeave, this );
                m_buttonList[ii][jj]->Unbind( wxEVT_LEAVE_WINDOW,
                                              &PANEL_SETUP_PINMAP::OnMouseLeave, this );
            }

            wxBitmapButton* btn =
                    new wxBitmapButton( m_matrixPanel, id, KiBitmapBundle( bitmap_butt ),
                                        wxPoint( x, y ), wxDefaultSize, wxBORDER_NONE );
            btn->Bind( wxEVT_LEAVE_WINDOW, &PANEL_SETUP_PINMAP::OnMouseLeave, this );
            btn->Bind( wxEVT_ENTER_WINDOW, &PANEL_SETUP_PINMAP::OnMouseEnter, this );
#endif
            btn->SetSize( btn->GetSize() + text_padding );

            delete m_buttonList[ii][jj];
            m_buttonList[ii][jj] = btn;
            setDRCMatrixButtonState( m_buttonList[ii][jj], diag );
        }
    }

    m_initialized = true;
}


void PANEL_SETUP_PINMAP::setDRCMatrixButtonState( wxWindow *aButton, PIN_ERROR aState )
{
    BITMAPS bitmap_butt = BITMAPS::INVALID_BITMAP;
    wxString tooltip;

    switch( aState )
    {
    case PIN_ERROR::OK:
        bitmap_butt = BITMAPS::erc_green;
        tooltip = _( "No error or warning" );
        break;

    case PIN_ERROR::WARNING:
        bitmap_butt = BITMAPS::ercwarn;
        tooltip = _( "Generate warning" );
        break;

    case PIN_ERROR::PP_ERROR:
        bitmap_butt = BITMAPS::ercerr;
        tooltip = _( "Generate error" );
        break;

    default:
        break;
    }

    if( !!bitmap_butt )
    {
        if( wxBitmapButton* wx_btn = dynamic_cast<wxBitmapButton*>( aButton ) )
            wx_btn->SetBitmap( KiBitmapBundle( bitmap_butt ) );
        else if( BITMAP_BUTTON* ki_btn = dynamic_cast<BITMAP_BUTTON*>( aButton ) )
            ki_btn->SetBitmap( KiBitmapBundle( bitmap_butt ) );

        aButton->SetToolTip( tooltip );
    }
}


void PANEL_SETUP_PINMAP::changeErrorLevel( wxCommandEvent& event )
{
    int id = event.GetId();
    int ii = id - ID_MATRIX_0;
    ELECTRICAL_PINTYPE x = static_cast<ELECTRICAL_PINTYPE>( ii / PINMAP_TYPE_COUNT );
    ELECTRICAL_PINTYPE y = static_cast<ELECTRICAL_PINTYPE>( ii % PINMAP_TYPE_COUNT );
    wxWindow* butt = static_cast<wxWindow*>( event.GetEventObject() );

    int level = static_cast<int>( m_schematic->ErcSettings().GetPinMapValue( y, x ) );
    level     = ( level + 1 ) % 3;

    setDRCMatrixButtonState( butt, static_cast<PIN_ERROR>( level ) );

    m_schematic->ErcSettings().SetPinMapValue( y, x, static_cast<PIN_ERROR>( level ) );
    m_schematic->ErcSettings().SetPinMapValue( x, y, static_cast<PIN_ERROR>( level ) );
}


void PANEL_SETUP_PINMAP::ImportSettingsFrom( PIN_ERROR aPinMap[][ELECTRICAL_PINTYPES_TOTAL] )
{
    for( int ii = 0; ii < PINMAP_TYPE_COUNT; ii++ )
    {
        for( int jj = 0; jj <= ii; jj++ )
            setDRCMatrixButtonState( m_buttonList[ii][jj], aPinMap[ii][jj] );
    }
}


