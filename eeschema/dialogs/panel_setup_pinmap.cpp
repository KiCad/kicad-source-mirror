/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <tools/ee_actions.h>
#include <tool/tool_manager.h>
#include <panel_setup_pinmap.h>
#include <erc.h>
#include <id.h>
#include <wx/bmpbuttn.h>
#include <wx/statline.h>
#include <wx/stattext.h>


// Control identifiers for events
#define ID_MATRIX_0 1800


BEGIN_EVENT_TABLE( PANEL_SETUP_PINMAP, PANEL_SETUP_PINMAP_BASE )
    EVT_COMMAND_RANGE( ID_MATRIX_0,
                       ID_MATRIX_0 + ( ELECTRICAL_PINTYPES_TOTAL * ELECTRICAL_PINTYPES_TOTAL ) - 1,
                       wxEVT_COMMAND_BUTTON_CLICKED, PANEL_SETUP_PINMAP::changeErrorLevel )
END_EVENT_TABLE()


PANEL_SETUP_PINMAP::PANEL_SETUP_PINMAP( wxWindow* aWindow, SCH_EDIT_FRAME* parent ) :
    PANEL_SETUP_PINMAP_BASE( aWindow ),
    m_buttonList(),
    m_initialized( false )
{
    m_parent    = parent;
    m_schematic = &parent->Schematic();

    reBuildMatrixPanel();
}


void PANEL_SETUP_PINMAP::ResetPanel()
{
    m_schematic->ErcSettings().ResetPinMap();
    reBuildMatrixPanel();
}


void PANEL_SETUP_PINMAP::reBuildMatrixPanel()
{
    // Try to know the size of bitmap button used in drc matrix
    wxBitmapButton* dummy    = new wxBitmapButton( m_matrixPanel, wxID_ANY, KiBitmap( BITMAPS::ercerr ) );
    wxSize          bmapSize = dummy->GetSize();
    delete dummy;

    wxSize        charSize = KIUI::GetTextSize( "X", m_matrixPanel );
    wxPoint       pos( 0, charSize.y * 2 );
    wxStaticText* text;

#ifdef __WXMAC__
    bmapSize.y += 2;
    charSize.y += 2;
#endif

    if( !m_initialized )
    {
        std::vector<wxStaticText*> labels;

        // Print row labels
        for( int ii = 0; ii < ELECTRICAL_PINTYPES_TOTAL; ii++ )
        {
            int y = pos.y + (ii * bmapSize.y);
            text = new wxStaticText( m_matrixPanel, -1, CommentERC_H[ii],
                                     wxPoint( 5, y + ( bmapSize.y / 2 ) - ( 12 / 2 ) ) );
            labels.push_back( text );

            int x = text->GetRect().GetRight();
            pos.x = std::max( pos.x, x );
        }

        // Right-align
        for( int ii = 0; ii < ELECTRICAL_PINTYPES_TOTAL; ii++ )
        {
            wxPoint labelPos = labels[ ii ]->GetPosition();
            labelPos.x = pos.x - labels[ ii ]->GetRect().GetWidth();
            labels[ ii ]->SetPosition( labelPos );
        }

        pos.x += 5;
    }
    else
    {
        pos = m_buttonList[0][0]->GetPosition();
    }

    for( int ii = 0; ii < ELECTRICAL_PINTYPES_TOTAL; ii++ )
    {
        int y = pos.y + (ii * bmapSize.y);

        for( int jj = 0; jj <= ii; jj++ )
        {
            // Add column labels (only once)
            PIN_ERROR diag = m_schematic->ErcSettings().GetPinMapValue( ii, jj );

            int x = pos.x + ( jj * ( bmapSize.x + 2 ) );

            if( ( ii == jj ) && !m_initialized )
            {
                wxPoint textPos( x + KiROUND( bmapSize.x / 2 ) - KiROUND( charSize.x ),
                                 y - charSize.y * 2 );
                new wxStaticText( m_matrixPanel, wxID_ANY, CommentERC_V[ii], textPos );

                wxPoint calloutPos( x + KiROUND( bmapSize.x / 2 ) - KiROUND( charSize.x / 2 ),
                                    y - charSize.y );
                new wxStaticText( m_matrixPanel, wxID_ANY, "|", calloutPos );
            }

            int event_id = ID_MATRIX_0 + ii + ( jj * ELECTRICAL_PINTYPES_TOTAL );
            BITMAPS bitmap_butt = BITMAPS::erc_green;

            delete m_buttonList[ii][jj];
            wxBitmapButton* btn = new wxBitmapButton( m_matrixPanel, event_id,
                                                      KiBitmap( bitmap_butt ), wxPoint( x, y ) );

#ifdef __WXMAC__
            btn->SetSize( btn->GetSize().x - 1, btn->GetSize().y );
#else
            btn->SetSize( btn->GetSize().x + 4, btn->GetSize().y );
#endif
            m_buttonList[ii][jj] = btn;
            setDRCMatrixButtonState( m_buttonList[ii][jj], diag );
        }
    }

    m_initialized = true;
}


void PANEL_SETUP_PINMAP::setDRCMatrixButtonState( wxBitmapButton *aButton, PIN_ERROR aState )
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
        aButton->SetBitmap( KiBitmap( bitmap_butt ) );
        aButton->SetToolTip( tooltip );
    }
}


void PANEL_SETUP_PINMAP::changeErrorLevel( wxCommandEvent& event )
{
    int id = event.GetId();
    int ii = id - ID_MATRIX_0;
    int x = ii / ELECTRICAL_PINTYPES_TOTAL;
    int y = ii % ELECTRICAL_PINTYPES_TOTAL;
    wxBitmapButton* butt = (wxBitmapButton*) event.GetEventObject();

    int level = static_cast<int>( m_schematic->ErcSettings().GetPinMapValue( y, x ) );
    level     = ( level + 1 ) % 3;

    setDRCMatrixButtonState( butt, static_cast<PIN_ERROR>( level ) );

    m_schematic->ErcSettings().SetPinMapValue( y, x, static_cast<PIN_ERROR>( level ) );
    m_schematic->ErcSettings().SetPinMapValue( x, y, static_cast<PIN_ERROR>( level ) );
}


void PANEL_SETUP_PINMAP::ImportSettingsFrom( PIN_ERROR aPinMap[][ELECTRICAL_PINTYPES_TOTAL] )
{
    for( int ii = 0; ii < ELECTRICAL_PINTYPES_TOTAL; ii++ )
    {
        for( int jj = 0; jj <= ii; jj++ )
            setDRCMatrixButtonState( m_buttonList[ii][jj], aPinMap[ii][jj] );
    }
}


