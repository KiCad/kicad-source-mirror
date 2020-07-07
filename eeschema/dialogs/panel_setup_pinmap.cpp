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

#include <fctsys.h>
#include <gestfich.h>
#include <pgm_base.h>
#include <sch_edit_frame.h>
#include <kiface_i.h>
#include <bitmaps.h>
#include <reporter.h>
#include <wildcards_and_files_ext.h>
#include <netlist_object.h>
#include <schematic.h>
#include <connection_graph.h>
#include <tools/ee_actions.h>
#include <tool/tool_manager.h>
#include <panel_setup_pinmap.h>
#include <erc.h>
#include <id.h>
#include <widgets/wx_angle_text.h>


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

    wxFont infoFont = wxSystemSettings::GetFont( wxSYS_DEFAULT_GUI_FONT );
    infoFont.SetSymbolicSize( wxFONTSIZE_SMALL );

    reBuildMatrixPanel();
}


void PANEL_SETUP_PINMAP::OnResetMatrixClick( wxCommandEvent& aEvent )
{
    m_schematic->ErcSettings().ResetPinMap();
    reBuildMatrixPanel();
}


void PANEL_SETUP_PINMAP::reBuildMatrixPanel()
{
    // Try to know the size of bitmap button used in drc matrix
    wxBitmapButton * dummy = new wxBitmapButton( m_matrixPanel, wxID_ANY, KiBitmap( ercerr_xpm ) );
    wxSize bitmap_size = dummy->GetSize();
    delete dummy;

    wxPoint pos;
    // Get the current text size using a dummy text.
    wxStaticText* text = new wxStaticText( m_matrixPanel, -1, CommentERC_V[0], pos );
    EDA_RECT      bbox = WX_ANGLE_TEXT::GetBoundingBox( m_matrixPanel, CommentERC_V[0], 450 );

    int           text_height = text->GetRect().GetHeight();

    bitmap_size.y = std::max( bitmap_size.y, text_height );
    delete text;

    // compute the Y pos interval:
    pos.y = bbox.GetHeight();

    if( !m_initialized )
    {
        std::vector<wxStaticText*> labels;

        // Print row labels
        for( int ii = 0; ii < ELECTRICAL_PINTYPES_TOTAL; ii++ )
        {
            int y = pos.y + (ii * bitmap_size.y);
            text = new wxStaticText( m_matrixPanel, -1, CommentERC_H[ii],
                                     wxPoint( 5, y + ( bitmap_size.y / 2 ) - ( text_height / 2 ) ) );
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
        pos = m_buttonList[0][0]->GetPosition();

    for( int ii = 0; ii < ELECTRICAL_PINTYPES_TOTAL; ii++ )
    {
        int y = pos.y + (ii * bitmap_size.y);

        for( int jj = 0; jj <= ii; jj++ )
        {
            // Add column labels (only once)
            PIN_ERROR diag = m_schematic->ErcSettings().GetPinMapValue( ii, jj );

            int x = pos.x + ( jj * ( bitmap_size.x + 4 ) );

            if( ( ii == jj ) && !m_initialized )
            {
                wxPoint txtpos;
                txtpos.x = x + ( bitmap_size.x / 2 );
                txtpos.y = y - text_height;
                new WX_ANGLE_TEXT( m_matrixPanel, wxID_ANY, CommentERC_V[ii], txtpos, 450 );
            }

            int event_id = ID_MATRIX_0 + ii + ( jj * ELECTRICAL_PINTYPES_TOTAL );
            BITMAP_DEF bitmap_butt = erc_green_xpm;

            delete m_buttonList[ii][jj];
            wxBitmapButton* btn = new wxBitmapButton( m_matrixPanel, event_id,
                                                      KiBitmap( bitmap_butt ), wxPoint( x, y ) );
            btn->SetSize( btn->GetSize().x + 4, btn->GetSize().y );
            m_buttonList[ii][jj] = btn;
            setDRCMatrixButtonState( m_buttonList[ii][jj], diag );
        }
    }

    m_initialized = true;
}


bool PANEL_SETUP_PINMAP::Show( bool show )
{
    wxPanel::Show( show );

    // For some reason the angle text doesn't get drawn if the paint events are fired while
    // it's not the active tab.
    if( show )
    {
        for( wxWindow* win : m_matrixPanel->GetChildren() )
        {
            WX_ANGLE_TEXT* angleText = dynamic_cast<WX_ANGLE_TEXT*>( win );

            if( angleText )
                angleText->Refresh();
        }
    }

    return true;
}


void PANEL_SETUP_PINMAP::setDRCMatrixButtonState( wxBitmapButton *aButton, PIN_ERROR aState )
{
    BITMAP_DEF bitmap_butt = nullptr;
    wxString tooltip;

    switch( aState )
    {
    case PIN_ERROR::OK:
        bitmap_butt = erc_green_xpm;
        tooltip = _( "No error or warning" );
        break;

    case PIN_ERROR::WARNING:
        bitmap_butt = ercwarn_xpm;
        tooltip = _( "Generate warning" );
        break;

    case PIN_ERROR::PP_ERROR:
        bitmap_butt = ercerr_xpm;
        tooltip = _( "Generate error" );
        break;

    default:
        break;
    }

    if( bitmap_butt )
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


