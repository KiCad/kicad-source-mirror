/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 CERN
 * Copyright (C) 2021 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 * @author Maciej Suminski <maciej.suminski@cern.ch>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * https://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "sim_plot_colors.h"
#include "sim_plot_panel.h"
#include "sim_plot_frame.h"

#include <algorithm>
#include <limits>


static wxString formatFloat( double x, int nDigits )
{
    wxString rv, fmt;

    if( nDigits )
    {
        fmt.Printf( "%%.0%df", nDigits );
    }
    else
    {
        fmt = wxT( "%.0f" );
    }

    rv.Printf( fmt, x );

    return rv;
}


static void getSISuffix( double x, const wxString& unit, int& power, wxString& suffix )
{
    const int n_powers = 11;

    const struct
    {
        double exponent;
        char suffix;
    } powers[] =
    {
        { -18, 'a' },
        { -15, 'f' },
        { -12, 'p' },
        { -9,  'n' },
        { -6,  'u' },
        { -3,  'm' },
        { 0,   0   },
        { 3,   'k' },
        { 6,   'M' },
        { 9,   'G' },
        { 12,  'T' },
        { 14,  'P' }
    };

    power = 0;
    suffix = unit;

    if( x == 0.0 )
        return;

    for( int i = 0; i < n_powers - 1; i++ )
    {
        double r_cur = pow( 10, powers[i].exponent );

        if( fabs( x ) >= r_cur && fabs( x ) < r_cur * 1000.0 )
        {
            power = powers[i].exponent;

            if( powers[i].suffix )
                suffix = wxString( powers[i].suffix ) + unit;
            else
                suffix = unit;

            return;
        }
    }
}


static int countDecimalDigits( double x, int maxDigits )
{
    if( std::isnan( x ) )
    {
        // avoid trying to count the decimals of NaN
        return 0;
    }

    int64_t k = (int)( ( x - floor( x ) ) * pow( 10.0, (double) maxDigits ) );
    int n = 0;

    while( k && ( ( k % 10LL ) == 0LL || ( k % 10LL ) == 9LL ) )
    {
        k /= 10LL;
    }

    n = 0;

    while( k != 0LL )
    {
        n++;
        k /= 10LL;
    }

    return n;
}


template <typename parent>
class LIN_SCALE : public parent
{
public:
    LIN_SCALE( wxString name, wxString unit, int flags ) : parent( name, flags ), m_unit( unit ){};

    void formatLabels() override
    {
        double maxVis = parent::AbsVisibleMaxValue();

        wxString suffix;
        int power, digits = 0;
        int constexpr DIGITS = 3;

        getSISuffix( maxVis, m_unit, power, suffix );

        double sf = pow( 10.0, power );

        for( auto& l : parent::TickLabels() )
        {
            int k = countDecimalDigits( l.pos / sf, DIGITS );

            digits = std::max( digits, k );
        }

        for( auto& l : parent::TickLabels() )
        {
            l.label = formatFloat( l.pos / sf, digits ) + suffix;
            l.visible = true;
        }
    }

private:
    const wxString m_unit;
};


template <typename parent>
class LOG_SCALE : public parent
{
public:
    LOG_SCALE( wxString name, wxString unit, int flags ) : parent( name, flags ), m_unit( unit ){};

    void formatLabels() override
    {
        wxString suffix;
        int      power;

        for( auto& l : parent::TickLabels() )
        {
            getSISuffix( l.pos, m_unit, power, suffix );
            double sf = pow( 10.0, power );
            int    k = countDecimalDigits( l.pos / sf, 3 );

            l.label = formatFloat( l.pos / sf, k ) + suffix;
            l.visible = true;
        }
    }

private:
    const wxString m_unit;
};


void CURSOR::Plot( wxDC& aDC, mpWindow& aWindow )
{
    if( !m_window )
        m_window = &aWindow;

    if( !m_visible )
        return;

    const auto& dataX = m_trace->GetDataX();
    const auto& dataY = m_trace->GetDataY();

    if( dataX.size() <= 1 )
        return;

    if( m_updateRequired )
    {
        m_coords.x = m_trace->s2x( aWindow.p2x( m_dim.x ) );

        // Find the closest point coordinates
        auto maxXIt = std::upper_bound( dataX.begin(), dataX.end(), m_coords.x );
        int maxIdx = maxXIt - dataX.begin();
        int minIdx = maxIdx - 1;

        // Out of bounds checks
        if( minIdx < 0 )
        {
            minIdx = 0;
            maxIdx = 1;
            m_coords.x = dataX[0];
        }
        else if( maxIdx >= (int) dataX.size() )
        {
            maxIdx = dataX.size() - 1;
            minIdx = maxIdx - 1;
            m_coords.x = dataX[maxIdx];
        }

        const double leftX = dataX[minIdx];
        const double rightX = dataX[maxIdx];
        const double leftY = dataY[minIdx];
        const double rightY = dataY[maxIdx];

        // Linear interpolation
        m_coords.y = leftY + ( rightY - leftY ) / ( rightX - leftX ) * ( m_coords.x - leftX );
        m_updateRequired = false;

        // Notify the parent window about the changes
        wxQueueEvent( aWindow.GetParent(), new wxCommandEvent( EVT_SIM_CURSOR_UPDATE ) );
    }
    else
    {
        m_updateRef = true;
    }

    if( m_updateRef )
    {
        UpdateReference();
        m_updateRef = false;
    }

    // Line length in horizontal and vertical dimensions
    const wxPoint cursorPos( aWindow.x2p( m_trace->x2s( m_coords.x ) ),
                             aWindow.y2p( m_trace->y2s( m_coords.y ) ) );

    wxCoord leftPx   = m_drawOutsideMargins ? 0 : aWindow.GetMarginLeft();
    wxCoord rightPx  = m_drawOutsideMargins ? aWindow.GetScrX() :
                                              aWindow.GetScrX() - aWindow.GetMarginRight();
    wxCoord topPx    = m_drawOutsideMargins ? 0 : aWindow.GetMarginTop();
    wxCoord bottomPx = m_drawOutsideMargins ? aWindow.GetScrY() :
                                              aWindow.GetScrY() - aWindow.GetMarginBottom();

    wxPen pen = GetPen();
    pen.SetStyle( m_continuous ? wxPENSTYLE_SOLID : wxPENSTYLE_LONG_DASH );
    aDC.SetPen( pen );

    if( topPx < cursorPos.y && cursorPos.y < bottomPx )
        aDC.DrawLine( leftPx, cursorPos.y, rightPx, cursorPos.y );

    if( leftPx < cursorPos.x && cursorPos.x < rightPx )
        aDC.DrawLine( cursorPos.x, topPx, cursorPos.x, bottomPx );
}


bool CURSOR::Inside( wxPoint& aPoint )
{
    if( !m_window )
        return false;

    return ( std::abs( (double) aPoint.x -
                       m_window->x2p( m_trace->x2s( m_coords.x ) ) ) <= DRAG_MARGIN )
        || ( std::abs( (double) aPoint.y -
                       m_window->y2p( m_trace->y2s( m_coords.y ) ) ) <= DRAG_MARGIN );
}


void CURSOR::UpdateReference()
{
    if( !m_window )
        return;

    m_reference.x = m_window->x2p( m_trace->x2s( m_coords.x ) );
    m_reference.y = m_window->y2p( m_trace->y2s( m_coords.y ) );
}


SIM_PLOT_PANEL::SIM_PLOT_PANEL( const wxString& aCommand, wxWindow* parent,
                                SIM_PLOT_FRAME* aMainFrame, wxWindowID id, const wxPoint& pos,
                                const wxSize& size, long style, const wxString& name )
    : SIM_PANEL_BASE( aCommand, parent, id, pos, size, style, name ),
      m_axis_x( nullptr ),
      m_axis_y1( nullptr ),
      m_axis_y2( nullptr ),
      m_dotted_cp( false ),
      m_masterFrame( aMainFrame )
{
    m_sizer   = new wxBoxSizer( wxVERTICAL );
    m_plotWin = new mpWindow( this, wxID_ANY, pos, size, style );

    m_plotWin->LimitView( true );
    m_plotWin->SetMargins( 50, 80, 50, 80 );

    UpdatePlotColors();

    updateAxes();

    // a mpInfoLegend displays le name of traces on the left top panel corner:
    m_legend = new mpInfoLegend( wxRect( 0, 40, 200, 40 ), wxTRANSPARENT_BRUSH );
    m_legend->SetVisible( false );
    m_plotWin->AddLayer( m_legend );

    m_plotWin->EnableDoubleBuffer( true );
    m_plotWin->UpdateAll();

    m_sizer->Add( m_plotWin, 1, wxALL | wxEXPAND, 1 );
    SetSizer( m_sizer );
}


SIM_PLOT_PANEL::~SIM_PLOT_PANEL()
{
    // ~mpWindow destroys all the added layers, so there is no need to destroy m_traces contents
}


void SIM_PLOT_PANEL::updateAxes()
{
    if( m_axis_x )
        return;

    switch( GetType() )
    {
        case ST_AC:
            m_axis_x = new LOG_SCALE<mpScaleXLog>( _( "Frequency" ), wxT( "Hz" ), mpALIGN_BOTTOM );
            m_axis_y1 = new LIN_SCALE<mpScaleY>( _( "Gain" ), wxT( "dBV" ), mpALIGN_LEFT );
            m_axis_y2 = new LIN_SCALE<mpScaleY>( _( "Phase" ), wxT( "\u00B0" ),
                                                 mpALIGN_RIGHT ); // degree sign
            m_axis_y2->SetMasterScale( m_axis_y1 );
            break;

        case ST_DC:
            prepareDCAxes();
            break;

        case ST_NOISE:
            m_axis_x = new LOG_SCALE<mpScaleXLog>( _( "Frequency" ), wxT( "Hz" ), mpALIGN_BOTTOM );
            m_axis_y1 = new mpScaleY( _( "noise [(V or A)^2/Hz]" ), mpALIGN_LEFT );
            break;

        case ST_TRANSIENT:
            m_axis_x = new LIN_SCALE<mpScaleX>( _( "Time" ), wxT( "s" ), mpALIGN_BOTTOM );
            m_axis_y1 = new LIN_SCALE<mpScaleY>( _( "Voltage" ), wxT( "V" ), mpALIGN_LEFT );
            m_axis_y2 = new LIN_SCALE<mpScaleY>( _( "Current" ), wxT( "A" ), mpALIGN_RIGHT );
            m_axis_y2->SetMasterScale( m_axis_y1 );
            break;

        default:
            // suppress warnings
            break;
    }

    if( m_axis_x )
    {
        m_axis_x->SetTicks( false );
        m_axis_x->SetNameAlign ( mpALIGN_BOTTOM );

        m_plotWin->AddLayer( m_axis_x );
    }

    if( m_axis_y1 )
    {
        m_axis_y1->SetTicks( false );
        m_axis_y1->SetNameAlign ( mpALIGN_LEFT );
        m_plotWin->AddLayer( m_axis_y1 );
    }

    if( m_axis_y2 )
    {
        m_axis_y2->SetTicks( false );
        m_axis_y2->SetNameAlign ( mpALIGN_RIGHT );
        m_plotWin->AddLayer( m_axis_y2 );
    }
}

void SIM_PLOT_PANEL::prepareDCAxes()
{
    wxString sim_cmd = getSimCommand().Lower();
    wxString rem;

    if( sim_cmd.StartsWith( ".dc", &rem ) )
    {
        wxChar ch;

        rem.Trim( false );

        try
        {
            ch = rem.GetChar( 0 );
        }
        catch( ... )
        {;}

        switch( ch )
        {
        // Make sure that we have a reliable default (even if incorrectly labeled)
        default:
        case 'v':
            m_axis_x =
                    new LIN_SCALE<mpScaleX>( _( "Voltage (swept)" ), wxT( "V" ), mpALIGN_BOTTOM );
            break;
        case 'i':
            m_axis_x =
                    new LIN_SCALE<mpScaleX>( _( "Current (swept)" ), wxT( "A" ), mpALIGN_BOTTOM );
            break;
        case 'r':
            m_axis_x = new LIN_SCALE<mpScaleX>( _( "Resistance (swept)" ), wxT( "\u03A9" ),
                                                mpALIGN_BOTTOM );
            break;
        case 't':
            m_axis_x = new LIN_SCALE<mpScaleX>( _( "Temperature (swept)" ), wxT( "\u00B0C" ),
                                                mpALIGN_BOTTOM );
            break;
        }

        m_axis_y1 = new LIN_SCALE<mpScaleY>( _( "Voltage (measured)" ), wxT( "V" ), mpALIGN_LEFT );
        m_axis_y2 = new LIN_SCALE<mpScaleY>( _( "Current" ), wxT( "A" ), mpALIGN_RIGHT );
    }
}


void SIM_PLOT_PANEL::UpdatePlotColors()
{
    // Update bg and fg colors:
    m_plotWin->SetColourTheme( m_colors.GetPlotColor( SIM_PLOT_COLORS::COLOR_SET::BACKGROUND ),
                               m_colors.GetPlotColor( SIM_PLOT_COLORS::COLOR_SET::FOREGROUND ),
                               m_colors.GetPlotColor( SIM_PLOT_COLORS::COLOR_SET::AXIS ) );

    // Update color of all traces
    for( auto& t : m_traces )
        if( t.second->GetCursor() )
            t.second->GetCursor()->SetPen(
                    wxPen( m_colors.GetPlotColor( SIM_PLOT_COLORS::COLOR_SET::CURSOR ) ) );

    m_plotWin->UpdateAll();
}


void SIM_PLOT_PANEL::UpdateTraceStyle( TRACE* trace )
{
    int        type = trace->GetType();
    wxPenStyle penStyle = ( ( ( type & SPT_AC_PHASE ) || ( type & SPT_CURRENT ) ) && m_dotted_cp )
                                  ? wxPENSTYLE_DOT
                                  : wxPENSTYLE_SOLID;
    trace->SetPen( wxPen( trace->GetTraceColour(), 2, penStyle ) );
}


bool SIM_PLOT_PANEL::addTrace( const wxString& aTitle, const wxString& aName, int aPoints,
                               const double* aX, const double* aY, SIM_PLOT_TYPE aType,
                               const wxString& aParam )
{
    TRACE* trace = nullptr;
    wxString name = aTitle;

    updateAxes();

    // Find previous entry, if there is one
    auto prev = m_traces.find( name );
    bool addedNewEntry = ( prev == m_traces.end() );

    if( addedNewEntry )
    {
        if( GetType() == ST_TRANSIENT )
        {
            bool hasVoltageTraces = false;

            for( const auto& tr : m_traces )
            {
                if( !( tr.second->GetType() & SPT_CURRENT ) )
                {
                    hasVoltageTraces = true;
                    break;
                }
            }

            if( !hasVoltageTraces )
                m_axis_y2->SetMasterScale( nullptr );
            else
                m_axis_y2->SetMasterScale( m_axis_y1 );
        }

        // New entry
        trace = new TRACE( aName, aType, aParam );
        trace->SetTraceColour( m_colors.GenerateColor( m_traces ) );
        UpdateTraceStyle( trace );
        m_traces[name] = trace;

        // It is a trick to keep legend & coords always on the top
        for( mpLayer* l : m_topLevel )
            m_plotWin->DelLayer( l );

        m_plotWin->AddLayer( (mpLayer*) trace );

        for( mpLayer* l : m_topLevel )
            m_plotWin->AddLayer( l );
    }
    else
    {
        trace = prev->second;
    }

    std::vector<double> tmp( aY, aY + aPoints );

    if( GetType() == ST_AC )
    {
        if( aType & SPT_AC_PHASE )
        {
            for( int i = 0; i < aPoints; i++ )
                tmp[i] = tmp[i] * 180.0 / M_PI;                 // convert to degrees
        }
        else
        {
            for( int i = 0; i < aPoints; i++ )
            {
                // log( 0 ) is not valid.
                if( tmp[i] != 0 )
                    tmp[i] = 20 * log( tmp[i] ) / log( 10.0 );  // convert to dB
            }
        }
    }

    trace->SetData( std::vector<double>( aX, aX + aPoints ), tmp );

    if( ( aType & SPT_AC_PHASE ) || ( aType & SPT_CURRENT ) )
        trace->SetScale( m_axis_x, m_axis_y2 );
    else
        trace->SetScale( m_axis_x, m_axis_y1 );

    m_plotWin->UpdateAll();

    return addedNewEntry;
}


bool SIM_PLOT_PANEL::deleteTrace( const wxString& aName )
{
    auto it = m_traces.find( aName );

    if( it != m_traces.end() )
    {
        TRACE* trace = it->second;
        m_traces.erase( it );

        if( CURSOR* cursor = trace->GetCursor() )
            m_plotWin->DelLayer( cursor, true );

        m_plotWin->DelLayer( trace, true, true );
        ResetScales();

        return true;
    }

    return false;
}


void SIM_PLOT_PANEL::deleteAllTraces()
{
    for( auto& t : m_traces )
    {
        deleteTrace( t.first );
    }

    m_traces.clear();
}


bool SIM_PLOT_PANEL::HasCursorEnabled( const wxString& aName ) const
{
    TRACE* t = GetTrace( aName );

    return t ? t->HasCursor() : false;
}


void SIM_PLOT_PANEL::EnableCursor( const wxString& aName, bool aEnable )
{
    TRACE* t = GetTrace( aName );

    if( t == nullptr || t->HasCursor() == aEnable )
        return;

    if( aEnable )
    {
        CURSOR* c = new CURSOR( t, this );
        int     plotCenter = GetPlotWin()->GetMarginLeft()
                         + ( GetPlotWin()->GetXScreen() - GetPlotWin()->GetMarginLeft()
                                   - GetPlotWin()->GetMarginRight() )
                                   / 2;
        c->SetX( plotCenter );
        c->SetPen( wxPen( m_colors.GetPlotColor( SIM_PLOT_COLORS::COLOR_SET::CURSOR ) ) );
        t->SetCursor( c );
        m_plotWin->AddLayer( c );
    }
    else
    {
        CURSOR* c = t->GetCursor();
        t->SetCursor( nullptr );
        m_plotWin->DelLayer( c, true );
    }

    // Notify the parent window about the changes
    wxQueueEvent( GetParent(), new wxCommandEvent( EVT_SIM_CURSOR_UPDATE ) );
}


void SIM_PLOT_PANEL::ResetScales()
{
    if( m_axis_x )
        m_axis_x->ResetDataRange();

    if( m_axis_y1 )
        m_axis_y1->ResetDataRange();

    if( m_axis_y2 )
        m_axis_y2->ResetDataRange();

    for( auto t : m_traces )
        t.second->UpdateScales();
}


wxDEFINE_EVENT( EVT_SIM_CURSOR_UPDATE, wxCommandEvent );
