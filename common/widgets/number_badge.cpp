/*
 * This program source code file is part of KICAD, a free EDA CAD application.
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

#include <gal/color4d.h>
#include <widgets/number_badge.h>
#include <fmt/format.h>
#include <algorithm>
#include <kiplatform/ui.h>

NUMBER_BADGE::NUMBER_BADGE( wxWindow* aParent, wxWindowID aId, const wxPoint& aPos,
                            const wxSize& aSize, int aStyles ) :
        wxPanel( aParent, aId, aPos, aSize, aStyles ),
        m_textSize( 10 ),
        m_maxNumber( 1000 ),
        m_currentNumber( 0 ),
        m_showBadge( false )
{
    computeSize();
    Bind( wxEVT_PAINT, &NUMBER_BADGE::onPaint, this );
}


void NUMBER_BADGE::UpdateNumber( int aNumber, SEVERITY aSeverity )
{
    m_showBadge     = true;
    m_currentNumber = aNumber;

    // Choose the colors of the badge rectangle and font
    if( aNumber < 0 )
    {
        m_showBadge = false;
    }
    else if( aNumber == 0 )
    {
        if( aSeverity == RPT_SEVERITY_ERROR || aSeverity == RPT_SEVERITY_WARNING )
        {
            m_badgeColour = KIGFX::COLOR4D( GREEN ).ToColour();
            m_textColour = *wxWHITE;
        }
        else
        {
            m_showBadge = false;
        }
    }
    else
    {
        switch( aSeverity )
        {
        case RPT_SEVERITY_ERROR:
            m_badgeColour = KIPLATFORM::UI::IsDarkTheme() ? wxColour( 240, 64, 64 ) : *wxRED;
            m_textColour  = *wxWHITE;
            break;

        case RPT_SEVERITY_WARNING:
            m_badgeColour = *wxYELLOW;
            m_textColour  = *wxBLACK;
            break;

        case RPT_SEVERITY_ACTION:
            m_badgeColour = KIGFX::COLOR4D( GREEN ).ToColour();
            m_textColour  = *wxWHITE;
            break;

        case RPT_SEVERITY_EXCLUSION:
        case RPT_SEVERITY_INFO:
        default:
            m_badgeColour = *wxLIGHT_GREY;
            m_textColour  = *wxBLACK;
            break;
        }
    }

    computeSize();

    // Force the badge UI to refresh so the new number and color is displayed
    Refresh();
}


void NUMBER_BADGE::SetMaximumNumber( int aMax )
{
    m_maxNumber = aMax;
}


void NUMBER_BADGE::SetTextSize( int aSize )
{
    m_textSize = aSize;
    computeSize();
}


// OSX has prevalent badges in the application bar at the bottom of the screen so we try to
// match those.  Other platforms may also need tweaks to spacing, fontweight, etc.
#if defined( __WXMAC__ )
#define BADGE_FONTWEIGHT wxFONTWEIGHT_NORMAL
#define PLATFORM_FUDGE_X 0.92
#define PLATFORM_FUDGE_Y 1.6
#else
#define BADGE_FONTWEIGHT wxFONTWEIGHT_BOLD
#define PLATFORM_FUDGE_X 1.0
#define PLATFORM_FUDGE_Y 1.0
#endif


void NUMBER_BADGE::computeSize()
{
    wxClientDC dc( this );

    wxString test = fmt::format( "{}", m_currentNumber );
    int      len  = test.length();

    // Determine the size using the string "m999{+}" where the 'm' on the front serves as a margin
    // so the number isn't too close to the curved edge.
    test = wxS( "m" );
    test.Pad( len, '9' );

    if( m_currentNumber > m_maxNumber )
        test += wxS( "+" );

    dc.SetFont( wxFont( m_textSize, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, BADGE_FONTWEIGHT ) );
    wxSize size = dc.GetTextExtent( test );

    size.y *= PLATFORM_FUDGE_Y;
    size.x = std::max<int>( size.x * PLATFORM_FUDGE_X, size.y );

    SetMinSize( size );
    SetSize( size );
}


void NUMBER_BADGE::onPaint( wxPaintEvent& aEvt )
{
    // The drawing rectangle
    wxSize    clientSize = GetSize();
    wxPaintDC dc( this );
    wxString  text;
    wxBrush   brush;

    // Give the badge a transparent background to show the panel underneath
    dc.SetBackground( *wxTRANSPARENT_BRUSH );
    dc.Clear();

    // We always draw a transparent background, but only draw the badge when it is needed
    if( !m_showBadge )
        return;

    // The rectangle the color is drawn in needs to be shrunk by 1px on each axis because for some
    // reason it seems to be padded out by 1px and is cutoff otherwise.
    wxRect rect( wxPoint( 0, 0 ), clientSize - wxSize( 1, 1 ) );

    brush.SetStyle( wxBRUSHSTYLE_SOLID );
    brush.SetColour( m_badgeColour );
    dc.SetBrush( brush );
    dc.SetPen( wxPen( m_badgeColour, 0 ) );
    dc.DrawRoundedRectangle( rect, rect.height / 2 );

    // Cap the number displayed and add the "+" to the end if required
    if( m_currentNumber > m_maxNumber )
        text = fmt::format( "{}+", m_maxNumber );
    else
        text = fmt::format( "{}", m_currentNumber );

    dc.SetFont( wxFont( m_textSize, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, BADGE_FONTWEIGHT ) );
    dc.SetTextForeground( m_textColour );
    dc.DrawLabel( text, wxRect( wxPoint( 0, 0 ), clientSize ),
                  wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL );
}
