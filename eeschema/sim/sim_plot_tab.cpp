/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016-2023 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
#include "sim_plot_tab.h"
#include "simulator_frame.h"
#include "core/kicad_algo.h"

#include <algorithm>
#include <cmath>
#include <limits>


static wxString formatFloat( double x, int nDigits )
{
    wxString rv, fmt;

    if( nDigits )
        fmt.Printf( "%%.0%df", nDigits );
    else
        fmt = wxT( "%.0f" );

    rv.Printf( fmt, x );

    return rv;
}


static void getSISuffix( double x, const wxString& unit, int& power, wxString& suffix )
{
    const int n_powers = 11;

    const struct
    {
        int  exponent;
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
        return 0;

    auto countSignificantDigits =
            [&]( int64_t k )
            {
                while( k && ( k % 10LL ) == 0LL )
                    k /= 10LL;

                int n = 0;

                while( k != 0LL )
                {
                    n++;
                    k /= 10LL;
                }

                return n;
            };

    int64_t k = (int)( ( x - floor( x ) ) * pow( 10.0, (double) maxDigits ) );
    int     n = countSignificantDigits( k );

    // check for trailing 9's
    n = std::min( n, countSignificantDigits( k + 1 ) );

    return n;
}


template <typename T_PARENT>
class LIN_SCALE : public T_PARENT
{
public:
    LIN_SCALE( const wxString& name, const wxString& unit, int flags ) :
            T_PARENT( name, flags, false ),
            m_unit( unit )
    {};

    wxString GetUnits() const { return m_unit; }

private:
    void formatLabels() override
    {
        double        maxVis = T_PARENT::AbsVisibleMaxValue();

        wxString      suffix;
        int           power = 0;
        int           digits = 0;
        int constexpr MAX_DIGITS = 3;
        int constexpr MAX_DISAMBIGUATION_DIGITS = 6;
        bool          duplicateLabels = false;

        getSISuffix( maxVis, m_unit, power, suffix );

        double sf = pow( 10.0, power );

        for( mpScaleBase::TICK_LABEL& l : T_PARENT::m_tickLabels )
            digits = std::max( digits, countDecimalDigits( l.pos / sf, MAX_DIGITS ) );

        do
        {
            for( size_t ii = 0; ii < T_PARENT::m_tickLabels.size(); ++ii )
            {
                mpScaleBase::TICK_LABEL& l = T_PARENT::m_tickLabels[ii];

                l.label = formatFloat( l.pos / sf, digits );
                l.visible = true;

                if( ii > 0 && l.label == T_PARENT::m_tickLabels[ii-1].label )
                    duplicateLabels = true;
            }
        }
        while( duplicateLabels && ++digits <= MAX_DISAMBIGUATION_DIGITS );

        if( m_base_axis_label.IsEmpty() )
            m_base_axis_label = T_PARENT::GetName();

        T_PARENT::SetName( wxString::Format( "%s (%s)", m_base_axis_label, suffix ) );
    }

private:
    const wxString m_unit;
    wxString       m_base_axis_label;
};


class TIME_SCALE : public LIN_SCALE<mpScaleX>
{
public:
    TIME_SCALE( const wxString& name, const wxString& unit, int flags ) :
            LIN_SCALE( name, unit, flags ),
            m_startTime( 0.0 ),
            m_endTime( 1.0 )
    {};

    void ExtendDataRange( double minV, double maxV ) override
    {
        LIN_SCALE::ExtendDataRange( minV, maxV );

        // Time is never longer than the simulation itself
        if( m_minV < m_startTime )
            m_minV = m_startTime;

        if( m_maxV > m_endTime )
            m_maxV = m_endTime;
    };

    void SetStartAndEnd( double aStartTime, double aEndTime )
    {
        m_startTime = aStartTime;
        m_endTime = aEndTime;
        ResetDataRange();
    }

    void ResetDataRange() override
    {
        m_minV = m_startTime;
        m_maxV = m_endTime;
        m_rangeSet = true;
    }

protected:
    double m_startTime;
    double m_endTime;
};


template <typename T_PARENT>
class LOG_SCALE : public T_PARENT
{
public:
    LOG_SCALE( const wxString& name, const wxString& unit, int flags ) :
            T_PARENT( name, flags, false ),
            m_unit( unit )
    {};

    wxString GetUnits() const { return m_unit; }

private:
    void formatLabels() override
    {
        wxString      suffix;
        int           power;
        int constexpr MAX_DIGITS = 3;

        for( mpScaleBase::TICK_LABEL& l : T_PARENT::m_tickLabels )
        {
            getSISuffix( l.pos, m_unit, power, suffix );
            double sf = pow( 10.0, power );
            int    k = countDecimalDigits( l.pos / sf, MAX_DIGITS );

            l.label = formatFloat( l.pos / sf, k ) + suffix;
            l.visible = true;
        }
    }

private:
    const wxString m_unit;
};


void CURSOR::SetCoordX( double aValue )
{
    wxRealPoint oldCoords = m_coords;

    doSetCoordX( aValue );
    m_updateRequired = false;
    m_updateRef = true;

    if( m_window )
    {
        wxRealPoint delta = m_coords - oldCoords;
        mpInfoLayer::Move( wxPoint( m_window->x2p( m_trace->x2s( delta.x ) ),
                                    m_window->y2p( m_trace->y2s( delta.y ) ) ) );

        m_window->Refresh();
    }
}


void CURSOR::Move( wxPoint aDelta )
{
    Update();

    if( m_trace->IsMultiRun() && m_window && m_trace->GetSweepCount() > 1
            && m_trace->GetSweepSize() != std::numeric_limits<size_t>::max() )
    {
        int newY = m_reference.y + aDelta.y;

        double plotY = m_window->p2y( newY );
        m_snapTargetY = m_trace->s2y( plotY );
        m_snapToNearest = true;
    }

    mpInfoLayer::Move( aDelta );
}


bool CURSOR::OnDoubleClick( const wxPoint& aPoint, mpWindow& aWindow )
{
    if( !Inside( aPoint ) )
        return false;

    if( !m_trace->IsMultiRun() )
        return false;

    int sweepCount = m_trace->GetSweepCount();
    size_t sweepSize = m_trace->GetSweepSize();

    if( sweepCount <= 1 )
        return false;

    if( sweepSize == std::numeric_limits<size_t>::max() || sweepSize == 0 )
        return false;

    if( m_sweepIndex < 0 || m_sweepIndex >= sweepCount )
        m_sweepIndex = 0;

    m_sweepIndex = ( m_sweepIndex + 1 ) % sweepCount;

    Update();
    m_updateRef = true;
    m_window = &aWindow;
    aWindow.Refresh();

    return true;
}


void CURSOR::doSetCoordX( double aValue )
{
    m_coords.x = aValue;

    const std::vector<double>& dataX = m_trace->GetDataX();
    const std::vector<double>& dataY = m_trace->GetDataY();

    if( dataX.size() <= 1 )
        return;

    bool   snapToNearest = m_snapToNearest;
    double snapTargetY = m_snapTargetY;
    m_snapToNearest = false;

    size_t startIdx = 0;
    size_t endIdx = dataX.size();
    int    sweepCount = m_trace->GetSweepCount();
    size_t sweepSize = m_trace->GetSweepSize();

    if( snapToNearest && m_trace->IsMultiRun() && sweepCount > 1
            && sweepSize != std::numeric_limits<size_t>::max() && sweepSize > 0
            && std::isfinite( snapTargetY ) )
    {
        double bestDistance = std::numeric_limits<double>::infinity();
        int    bestSweep = m_sweepIndex;
        bool   found = false;

        for( int sweepIdx = 0; sweepIdx < sweepCount; ++sweepIdx )
        {
            size_t candidateStart = static_cast<size_t>( sweepIdx ) * sweepSize;
            size_t candidateEnd = std::min( dataX.size(), candidateStart + sweepSize );

            if( candidateStart >= candidateEnd )
                continue;

            auto candidateBegin = dataX.begin() + candidateStart;
            auto candidateEndIt = dataX.begin() + candidateEnd;
            auto candidateMaxIt = std::upper_bound( candidateBegin, candidateEndIt, m_coords.x );
            int  candidateMaxIdx = candidateMaxIt - dataX.begin();
            int  candidateMinIdx = candidateMaxIdx - 1;

            if( candidateMinIdx < (int) candidateStart
                    || candidateMaxIdx >= (int) candidateEnd
                    || candidateMaxIdx >= (int) dataX.size() )
            {
                continue;
            }

            double leftX = dataX[candidateMinIdx];
            double rightX = dataX[candidateMaxIdx];

            if( leftX == rightX )
                continue;

            double leftY = dataY[candidateMinIdx];
            double rightY = dataY[candidateMaxIdx];
            double value = leftY + ( rightY - leftY ) / ( rightX - leftX ) * ( m_coords.x - leftX );
            double distance = std::fabs( value - snapTargetY );

            if( distance < bestDistance )
            {
                bestDistance = distance;
                bestSweep = sweepIdx;
                found = true;
            }
        }

        if( found )
            m_sweepIndex = bestSweep;
    }

    if( m_trace->IsMultiRun() && sweepCount > 1
            && sweepSize != std::numeric_limits<size_t>::max() && sweepSize > 0 )
    {
        size_t available = static_cast<size_t>( sweepCount ) * sweepSize;

        if( available <= dataX.size() )
        {
            if( m_sweepIndex < 0 || m_sweepIndex >= sweepCount )
                m_sweepIndex = std::max( sweepCount - 1, 0 );

            startIdx = static_cast<size_t>( m_sweepIndex ) * sweepSize;
            endIdx = std::min( dataX.size(), startIdx + sweepSize );
        }
        else
        {
            m_sweepIndex = 0;
        }
    }
    else
    {
        m_sweepIndex = 0;
    }

    if( startIdx >= endIdx )
    {
        m_coords.y = NAN;
        return;
    }

    auto beginIt = dataX.begin() + startIdx;
    auto endIt = dataX.begin() + endIdx;

    // Find the closest point coordinates
    auto maxXIt = std::upper_bound( beginIt, endIt, m_coords.x );
    int maxIdx = maxXIt - dataX.begin();
    int minIdx = maxIdx - 1;

    // Out of bounds checks
    if( minIdx < (int) startIdx || maxIdx >= (int) endIdx || maxIdx >= (int) dataX.size() )
    {
        // Simulation may not be complete yet, or we may have a cursor off the beginning or end
        // of the data.  Either way, that's where the user put it.  Don't second guess them; just
        // leave its y value undefined.
        m_coords.y = NAN;
        return;
    }

    const double leftX = dataX[minIdx];
    const double rightX = dataX[maxIdx];
    const double leftY = dataY[minIdx];
    const double rightY = dataY[maxIdx];

    // Linear interpolation
    m_coords.y = leftY + ( rightY - leftY ) / ( rightX - leftX ) * ( m_coords.x - leftX );
}


wxString CURSOR::getID()
{
    for( const auto& [ id, cursor ] : m_trace->GetCursors() )
    {
        if( cursor == this )
            return wxString::Format( _( "%d" ), id );
    }

    return wxEmptyString;
}


void CURSOR::Plot( wxDC& aDC, mpWindow& aWindow )
{
    if( !m_window )
        m_window = &aWindow;

    if( !m_visible || m_trace->GetDataX().size() <= 1 )
        return;

    if( m_updateRequired )
    {
        doSetCoordX( m_trace->s2x( aWindow.p2x( m_dim.x ) ) );
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

    wxCoord leftPx   = aWindow.GetMarginLeft();
    wxCoord rightPx  = aWindow.GetScrX() - aWindow.GetMarginRight();
    wxCoord topPx    = aWindow.GetMarginTop();
    wxCoord bottomPx = aWindow.GetScrY() - aWindow.GetMarginBottom();

    wxPen    pen = GetPen();
    wxColour fg = aWindow.GetForegroundColour();
    COLOR4D  cursorColor = COLOR4D( m_trace->GetTraceColour() ).Mix( fg, 0.6 );
    COLOR4D  textColor = fg;

    if( cursorColor.Distance( textColor ) < 0.66 )
        textColor.Invert();

    pen.SetColour( cursorColor.ToColour() );
    pen.SetStyle( m_continuous ? wxPENSTYLE_SOLID : wxPENSTYLE_LONG_DASH );
    aDC.SetPen( pen );

    if( topPx < cursorPos.y && cursorPos.y < bottomPx )
        aDC.DrawLine( leftPx, cursorPos.y, rightPx, cursorPos.y );

    if( leftPx < cursorPos.x && cursorPos.x < rightPx )
    {
        aDC.DrawLine( cursorPos.x, topPx, cursorPos.x, bottomPx );

        wxString id = getID();
        wxSize   size = aDC.GetTextExtent( wxS( "M" ) );
        wxRect   textRect( wxPoint( cursorPos.x + 1 - size.x / 2, topPx - 4 - size.y ), size );
        wxBrush  brush;
        wxPoint  poly[3];

        // Because a "1" looks off-center if it's actually centred.
        if( id == "1" )
            textRect.x -= 1;

        // We want an equalateral triangle, so use size.y for both axes.
        size.y += 3;
        // Make sure it's an even number so the slopes of the sides will be identical.
        size.y = ( size.y / 2 ) * 2;
        poly[0] = { cursorPos.x - 1 - size.y / 2, topPx - size.y };
        poly[1] = { cursorPos.x + 1 + size.y / 2, topPx - size.y };
        poly[2] = { cursorPos.x, topPx };

        brush.SetStyle( wxBRUSHSTYLE_SOLID );
        brush.SetColour( m_trace->GetTraceColour() );
        aDC.SetBrush( brush );
        aDC.DrawPolygon( 3, poly );

        aDC.SetTextForeground( textColor.ToColour() );
        aDC.DrawLabel( id, textRect, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL );

        if( m_trace->IsMultiRun() && m_trace->GetSweepCount() > 1
                && m_trace->GetSweepSize() != std::numeric_limits<size_t>::max() )
        {
            wxString runLabel;
            const std::vector<wxString>& labels = m_trace->GetMultiRunLabels();
            
            if( m_sweepIndex >= 0 && m_sweepIndex < (int) labels.size() )
            {
                runLabel = labels[m_sweepIndex];
            }
            else
            {
                runLabel = wxString::Format( _( "Run %d" ), m_sweepIndex + 1 );
            }
            
            wxSize   runSize = aDC.GetTextExtent( runLabel );
            int      runX = textRect.GetRight() + 6;
            wxRect   runRect( wxPoint( runX, textRect.y ), runSize );

            runRect.Inflate( 3, 1 );

            wxBrush labelBrush( aWindow.GetBackgroundColour() );
            wxPen   labelPen( cursorColor.ToColour() );

            aDC.SetPen( labelPen );
            aDC.SetBrush( labelBrush );
            aDC.DrawRectangle( runRect );
            aDC.SetTextForeground( cursorColor.ToColour() );
            aDC.DrawLabel( runLabel, runRect, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL );
        }
    }
}


bool CURSOR::Inside( const wxPoint& aPoint ) const
{
    if( !m_window || !m_trace )
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


SIM_PLOT_TAB::SIM_PLOT_TAB( const wxString& aSimCommand, wxWindow* parent ) :
        SIM_TAB( aSimCommand, parent ),
        m_axis_x( nullptr ),
        m_axis_y1( nullptr ),
        m_axis_y2( nullptr ),
        m_axis_y3( nullptr ),
        m_dotted_cp( false )
{
    m_sizer   = new wxBoxSizer( wxVERTICAL );
    m_plotWin = new mpWindow( this, wxID_ANY );

    m_plotWin->LimitView( true );
    m_plotWin->SetMargins( 30, 70, 45, 70 );
    UpdatePlotColors();

    updateAxes();

    // a mpInfoLegend displays le name of traces on the left top panel corner:
    m_legend = new mpInfoLegend( wxRect( 0, 0, 200, 40 ), wxTRANSPARENT_BRUSH );
    m_legend->SetVisible( false );
    m_plotWin->AddLayer( m_legend );
    m_LastLegendPosition = m_legend->GetPosition();

    m_plotWin->EnableDoubleBuffer( true );
    m_plotWin->UpdateAll();

    m_sizer->Add( m_plotWin, 1, wxALL | wxEXPAND, 1 );
    SetSizer( m_sizer );
}


SIM_PLOT_TAB::~SIM_PLOT_TAB()
{
    // ~mpWindow destroys all the added layers, so there is no need to destroy m_traces contents
}


void SIM_PLOT_TAB::SetY1Scale( bool aLock, double aMin, double aMax )
{
    wxCHECK( m_axis_y1, /* void */ );
    m_axis_y1->SetAxisMinMax( aLock, aMin, aMax );
}


void SIM_PLOT_TAB::SetY2Scale( bool aLock, double aMin, double aMax )
{
    wxCHECK( m_axis_y2, /* void */ );
    m_axis_y2->SetAxisMinMax( aLock, aMin, aMax );
}


void SIM_PLOT_TAB::SetY3Scale( bool aLock, double aMin, double aMax )
{
    wxCHECK( m_axis_y3, /* void */ );
    m_axis_y3->SetAxisMinMax( aLock, aMin, aMax );
}


wxString SIM_PLOT_TAB::GetUnitsX() const
{
    LOG_SCALE<mpScaleXLog>* logScale = dynamic_cast<LOG_SCALE<mpScaleXLog>*>( m_axis_x );
    LIN_SCALE<mpScaleX>*    linScale = dynamic_cast<LIN_SCALE<mpScaleX>*>( m_axis_x );

    if( logScale )
        return logScale->GetUnits();
    else if( linScale )
        return linScale->GetUnits();
    else
        return wxEmptyString;
}


wxString SIM_PLOT_TAB::GetUnitsY1() const
{
    LIN_SCALE<mpScaleY>* linScale = dynamic_cast<LIN_SCALE<mpScaleY>*>( m_axis_y1 );

    if( linScale )
        return linScale->GetUnits();
    else
        return wxEmptyString;
}


wxString SIM_PLOT_TAB::GetUnitsY2() const
{
    LIN_SCALE<mpScaleY>* linScale = dynamic_cast<LIN_SCALE<mpScaleY>*>( m_axis_y2 );

    if( linScale )
        return linScale->GetUnits();
    else
        return wxEmptyString;
}


wxString SIM_PLOT_TAB::GetUnitsY3() const
{
    LIN_SCALE<mpScaleY>* linScale = dynamic_cast<LIN_SCALE<mpScaleY>*>( m_axis_y3 );

    if( linScale )
        return linScale->GetUnits();
    else
        return wxEmptyString;
}


void SIM_PLOT_TAB::updateAxes( int aNewTraceType )
{
    switch( GetSimType() )
    {
        case ST_AC:
            if( !m_axis_x )
            {
                m_axis_x = new LOG_SCALE<mpScaleXLog>( wxEmptyString, wxT( "Hz" ), mpALIGN_BOTTOM );
                m_axis_x->SetNameAlign( mpALIGN_BOTTOM );
                m_plotWin->AddLayer( m_axis_x );

                m_axis_y1 = new LIN_SCALE<mpScaleY>( wxEmptyString, wxT( "dB" ), mpALIGN_LEFT );
                m_axis_y1->SetNameAlign( mpALIGN_LEFT );
                m_plotWin->AddLayer( m_axis_y1 );

                m_axis_y2 = new LIN_SCALE<mpScaleY>( wxEmptyString, wxT( "°" ), mpALIGN_RIGHT );
                m_axis_y2->SetNameAlign( mpALIGN_RIGHT );
                m_axis_y2->SetMasterScale( m_axis_y1 );
                m_plotWin->AddLayer( m_axis_y2 );
            }

            m_axis_x->SetName( _( "Frequency" ) );
            m_axis_y1->SetName( _( "Gain" ) );
            m_axis_y2->SetName( _( "Phase" ) );
            break;

        case ST_SP:
            if( !m_axis_x )
            {
                m_axis_x = new LOG_SCALE<mpScaleXLog>( wxEmptyString, wxT( "Hz" ), mpALIGN_BOTTOM );
                m_axis_x->SetNameAlign( mpALIGN_BOTTOM );
                m_plotWin->AddLayer( m_axis_x );

                m_axis_y1 = new LIN_SCALE<mpScaleY>( wxEmptyString, wxT( "" ), mpALIGN_LEFT );
                m_axis_y1->SetNameAlign( mpALIGN_LEFT );
                m_plotWin->AddLayer( m_axis_y1 );

                m_axis_y2 = new LIN_SCALE<mpScaleY>( wxEmptyString, wxT( "°" ), mpALIGN_RIGHT );
                m_axis_y2->SetNameAlign( mpALIGN_RIGHT );
                m_axis_y2->SetMasterScale( m_axis_y1 );
                m_plotWin->AddLayer( m_axis_y2 );
            }

            m_axis_x->SetName( _( "Frequency" ) );
            m_axis_y1->SetName( _( "Amplitude" ) );
            m_axis_y2->SetName( _( "Phase" ) );
            break;

        case ST_DC:
            prepareDCAxes( aNewTraceType );
            break;

        case ST_NOISE:
            if( !m_axis_x )
            {
                m_axis_x = new LOG_SCALE<mpScaleXLog>( wxEmptyString, wxT( "Hz" ), mpALIGN_BOTTOM );
                m_axis_x->SetNameAlign( mpALIGN_BOTTOM );
                m_plotWin->AddLayer( m_axis_x );

                if( ( aNewTraceType & SPT_CURRENT ) == 0 )
                {
                    m_axis_y1 = new LIN_SCALE<mpScaleY>( wxEmptyString, wxT( "" ), mpALIGN_LEFT );
                    m_axis_y1->SetNameAlign( mpALIGN_LEFT );
                    m_plotWin->AddLayer( m_axis_y1 );
                }
                else
                {
                    m_axis_y2 = new LIN_SCALE<mpScaleY>( wxEmptyString, wxT( "" ), mpALIGN_RIGHT );
                    m_axis_y2->SetNameAlign( mpALIGN_RIGHT );
                    m_plotWin->AddLayer( m_axis_y2 );
                }
            }

            m_axis_x->SetName( _( "Frequency" ) );

            if( m_axis_y1 )
                m_axis_y1->SetName( _( "Noise (V/√Hz)" ) );

            if( m_axis_y2 )
                m_axis_y2->SetName( _( "Noise (A/√Hz)" ) );

            break;

        case ST_FFT:
            if( !m_axis_x )
            {
                m_axis_x = new LOG_SCALE<mpScaleXLog>( wxEmptyString, wxT( "Hz" ), mpALIGN_BOTTOM );
                m_axis_x->SetNameAlign( mpALIGN_BOTTOM );
                m_plotWin->AddLayer( m_axis_x );

                m_axis_y1 = new LIN_SCALE<mpScaleY>( wxEmptyString, wxT( "dB" ), mpALIGN_LEFT );
                m_axis_y1->SetNameAlign( mpALIGN_LEFT );
                m_plotWin->AddLayer( m_axis_y1 );
            }

            m_axis_x->SetName( _( "Frequency" ) );
            m_axis_y1->SetName( _( "Intensity" ) );
            break;

        case ST_TRAN:
            if( !m_axis_x )
            {
                m_axis_x = new TIME_SCALE( wxEmptyString, wxT( "s" ), mpALIGN_BOTTOM );
                m_axis_x->SetNameAlign( mpALIGN_BOTTOM );
                m_plotWin->AddLayer( m_axis_x );

                m_axis_y1 = new LIN_SCALE<mpScaleY>(wxEmptyString, wxT( "V" ), mpALIGN_LEFT );
                m_axis_y1->SetNameAlign( mpALIGN_LEFT );
                m_plotWin->AddLayer( m_axis_y1 );

                m_axis_y2 = new LIN_SCALE<mpScaleY>( wxEmptyString, wxT( "A" ), mpALIGN_RIGHT );
                m_axis_y2->SetNameAlign( mpALIGN_RIGHT );
                m_axis_y2->SetMasterScale( m_axis_y1 );
                m_plotWin->AddLayer( m_axis_y2 );
            }

            m_axis_x->SetName( _( "Time" ) );
            m_axis_y1->SetName( _( "Voltage" ) );
            m_axis_y2->SetName( _( "Current" ) );

            if( aNewTraceType & SPT_POWER )
                EnsureThirdYAxisExists();

            if( m_axis_y3 )
                m_axis_y3->SetName( _( "Power" ) );

            break;

        default:
            // suppress warnings
            break;
    }

    if( GetSimType() == ST_TRAN || GetSimType() == ST_DC )
    {
        if( m_axis_y3 )
        {
            m_plotWin->SetMargins( 30, 160, 45, 70 );

            if( m_axis_y2 )
                m_axis_y2->SetNameAlign( mpALIGN_BORDER_RIGHT );

            m_axis_y3->SetAlign( mpALIGN_BORDER_RIGHT );
            m_axis_y3->SetNameAlign( mpALIGN_BORDER_RIGHT );
        }
        else
        {
            m_plotWin->SetMargins( 30, 70, 45, 70 );

            if( m_axis_y2 )
                m_axis_y2->SetNameAlign( mpALIGN_RIGHT );
        }
    }

    if( m_axis_x )
        m_axis_x->SetFont( KIUI::GetStatusFont( m_plotWin ) );

    if( m_axis_y1 )
        m_axis_y1->SetFont( KIUI::GetStatusFont( m_plotWin ) );

    if( m_axis_y2 )
        m_axis_y2->SetFont( KIUI::GetStatusFont( m_plotWin ) );

    if( m_axis_y3 )
        m_axis_y3->SetFont( KIUI::GetStatusFont( m_plotWin ) );

    UpdateAxisVisibility();
}


void SIM_PLOT_TAB::prepareDCAxes( int aNewTraceType )
{
    wxString sim_cmd = GetSimCommand().Lower();
    wxString rem;

    if( sim_cmd.StartsWith( ".dc", &rem ) )
    {
        wxChar ch = 0;

        rem.Trim( false );

        try
        {
            ch = rem.GetChar( 0 );
        }
        catch( ... )
        {
            // Best efforts
        }

        switch( ch )
        {
        // Make sure that we have a reliable default (even if incorrectly labeled)
        default:
        case 'v':
            if( !m_axis_x )
            {
                m_axis_x = new LIN_SCALE<mpScaleX>( wxEmptyString, wxT( "V" ), mpALIGN_BOTTOM );
                m_axis_x->SetNameAlign( mpALIGN_BOTTOM );
                m_plotWin->AddLayer( m_axis_x );
            }

            m_axis_x->SetName( _( "Voltage (swept)" ) );
            break;

        case 'i':
            if( !m_axis_x )
            {
                m_axis_x = new LIN_SCALE<mpScaleX>( wxEmptyString, wxT( "A" ), mpALIGN_BOTTOM );
                m_axis_x->SetNameAlign( mpALIGN_BOTTOM );
                m_plotWin->AddLayer( m_axis_x );
            }

            m_axis_x->SetName( _( "Current (swept)" ) );
            break;

        case 'r':
            if( !m_axis_x )
            {
                m_axis_x = new LIN_SCALE<mpScaleX>( wxEmptyString, wxT( "Ω" ), mpALIGN_BOTTOM );
                m_axis_x->SetNameAlign( mpALIGN_BOTTOM );
                m_plotWin->AddLayer( m_axis_x );
            }

            m_axis_x->SetName( _( "Resistance (swept)" ) );
            break;

        case 't':
            if( !m_axis_x )
            {
                m_axis_x = new LIN_SCALE<mpScaleX>( wxEmptyString, wxT( "°C" ), mpALIGN_BOTTOM );
                m_axis_x->SetNameAlign( mpALIGN_BOTTOM );
                m_plotWin->AddLayer( m_axis_x );
            }

            m_axis_x->SetName( _( "Temperature (swept)" ) );
            break;
        }

        if( !m_axis_y1 )
        {
            m_axis_y1 = new LIN_SCALE<mpScaleY>( wxEmptyString, wxT( "V" ), mpALIGN_LEFT );
            m_axis_y1->SetNameAlign( mpALIGN_LEFT );
            m_plotWin->AddLayer( m_axis_y1 );
        }

        if( !m_axis_y2 )
        {
            m_axis_y2 = new LIN_SCALE<mpScaleY>( wxEmptyString, wxT( "A" ), mpALIGN_RIGHT );
            m_axis_y2->SetNameAlign( mpALIGN_RIGHT );
            m_plotWin->AddLayer( m_axis_y2 );
        }

        m_axis_y1->SetName( _( "Voltage (measured)" ) );
        m_axis_y2->SetName( _( "Current" ) );

        if( ( aNewTraceType & SPT_POWER ) )
            EnsureThirdYAxisExists();

        if( m_axis_y3 )
            m_axis_y3->SetName( _( "Power" ) );
    }
}


void SIM_PLOT_TAB::EnsureThirdYAxisExists()
{
    if( !m_axis_y3 )
    {
        m_plotWin->SetMargins( 30, 160, 45, 70 );
        m_axis_y3 = new LIN_SCALE<mpScaleY>( wxEmptyString, wxT( "W" ), mpALIGN_BORDER_RIGHT );
        m_axis_y3->SetNameAlign( mpALIGN_BORDER_RIGHT );
        m_axis_y3->SetMasterScale( m_axis_y1 );
        m_plotWin->AddLayer( m_axis_y3 );
    }

    if( m_axis_y3 )
    {
        m_axis_y3->SetAlign( mpALIGN_BORDER_RIGHT );
        m_axis_y3->SetNameAlign( mpALIGN_BORDER_RIGHT );
    }

    if( m_axis_y2 )
        m_axis_y2->SetNameAlign( mpALIGN_BORDER_RIGHT );
}


void SIM_PLOT_TAB::UpdatePlotColors()
{
    // Update bg and fg colors:
    m_plotWin->SetColourTheme( m_colors.GetPlotColor( SIM_PLOT_COLORS::COLOR_SET::BACKGROUND ),
                               m_colors.GetPlotColor( SIM_PLOT_COLORS::COLOR_SET::FOREGROUND ),
                               m_colors.GetPlotColor( SIM_PLOT_COLORS::COLOR_SET::AXIS ) );

    m_plotWin->UpdateAll();
}


void SIM_PLOT_TAB::OnLanguageChanged()
{
    updateAxes();
    m_plotWin->UpdateAll();
}


void SIM_PLOT_TAB::UpdateTraceStyle( TRACE* trace )
{
    int        type = trace->GetType();
    wxPenStyle penStyle;

    if( ( type & SPT_AC_GAIN ) > 0 )
        penStyle = wxPENSTYLE_SOLID;
    else if( ( type & SPT_AC_PHASE ) > 0 )
        penStyle = m_dotted_cp ? wxPENSTYLE_DOT : wxPENSTYLE_SOLID;
    else if( ( type & SPT_CURRENT ) > 0 )
        penStyle = m_dotted_cp ? wxPENSTYLE_DOT : wxPENSTYLE_SOLID;
    else
        penStyle = wxPENSTYLE_SOLID;

    trace->SetPen( wxPen( trace->GetTraceColour(), 2, penStyle ) );
    m_sessionTraceColors[ trace->GetName() ] = trace->GetTraceColour();
}


TRACE* SIM_PLOT_TAB::GetOrAddTrace( const wxString& aVectorName, int aType )
{
    TRACE* trace = GetTrace( aVectorName, aType );

    if( !trace )
    {
        updateAxes( aType );

        if( GetSimType() == ST_TRAN || GetSimType() == ST_DC )
        {
            bool hasVoltageTraces = false;

            for( const auto& [ id, candidate ] : m_traces )
            {
                if( candidate->GetType() & SPT_VOLTAGE )
                {
                    hasVoltageTraces = true;
                    break;
                }
            }

            if( !hasVoltageTraces )
            {
                if( m_axis_y2 )
                    m_axis_y2->SetMasterScale( nullptr );

                if( m_axis_y3 )
                    m_axis_y3->SetMasterScale( nullptr );
            }
        }

        trace = new TRACE( aVectorName, (SIM_TRACE_TYPE) aType );

        if( m_sessionTraceColors.count( aVectorName ) )
            trace->SetTraceColour( m_sessionTraceColors[ aVectorName ] );
        else
            trace->SetTraceColour( m_colors.GenerateColor( m_sessionTraceColors ) );

        UpdateTraceStyle( trace );
        m_traces[ getTraceId( aVectorName, aType ) ] = trace;

        m_plotWin->AddLayer( (mpLayer*) trace );
    }

    return trace;
}


void SIM_PLOT_TAB::SetTraceData( TRACE* trace, std::vector<double>& aX, std::vector<double>& aY,
                                 int aSweepCount, size_t aSweepSize, bool aIsMultiRun,
                                 const std::vector<wxString>& aMultiRunLabels )
{
    if( dynamic_cast<LOG_SCALE<mpScaleXLog>*>( m_axis_x ) )
    {
        // log( 0 ) is not valid.
        if( aX.size() > 0 && aX[0] == 0 )
        {
            aX.erase( aX.begin() );
            aY.erase( aY.begin() );
        }
    }

    if( GetSimType() == ST_AC || GetSimType() == ST_FFT )
    {
        if( trace->GetType() & SPT_AC_PHASE )
        {
            for( double& pt : aY )
                pt = pt * 180.0 / M_PI;                     // convert to degrees
        }
        else
        {
            for( double& pt : aY )
            {
                // log( 0 ) is not valid.
                if( pt != 0 )
                    pt = 20 * log( pt ) / log( 10.0 );      // convert to dB
            }
        }
    }

    trace->SetData( aX, aY );
    trace->SetSweepCount( aSweepCount );
    trace->SetSweepSize( aSweepSize );
    trace->SetIsMultiRun( aIsMultiRun );
    trace->SetMultiRunLabels( aMultiRunLabels );

    // Phase and currents on second Y axis, except for AC currents, those use the same axis as voltage
    if( ( trace->GetType() & SPT_AC_PHASE )
        || ( ( GetSimType() != ST_AC ) && ( trace->GetType() & SPT_CURRENT ) ) )
    {
        trace->SetScale( m_axis_x, m_axis_y2 );
    }
    else if( trace->GetType() & SPT_POWER )
    {
        trace->SetScale( m_axis_x, m_axis_y3 );
    }
    else
    {
        trace->SetScale( m_axis_x, m_axis_y1 );
    }

    for( auto& [ cursorId, cursor ] : trace->GetCursors() )
    {
        if( cursor )
            cursor->SetCoordX( cursor->GetCoords().x );
    }

    UpdateAxisVisibility();
}


void SIM_PLOT_TAB::UpdateAxisVisibility()
{
    bool hasY1Traces = false;
    bool hasY2Traces = false;
    bool hasY3Traces = false;

    for( const auto& [ name, trace ] : m_traces )
    {
        if( !trace )
            continue;

        if( trace->GetType() & SPT_POWER )
        {
            hasY3Traces = true;
        }
        else if( ( trace->GetType() & SPT_AC_PHASE )
                 || ( ( GetSimType() != ST_AC ) && ( trace->GetType() & SPT_CURRENT ) ) )
        {
            hasY2Traces = true;
        }
        else
        {
            hasY1Traces = true;
        }
    }

    bool visibilityChanged = false;

    if( m_axis_y1 && m_axis_y1->IsVisible() != hasY1Traces )
    {
        m_axis_y1->SetVisible( hasY1Traces );
        visibilityChanged = true;
    }

    if( m_axis_y2 && m_axis_y2->IsVisible() != hasY2Traces )
    {
        m_axis_y2->SetVisible( hasY2Traces );
        visibilityChanged = true;
    }

    if( m_axis_y3 && m_axis_y3->IsVisible() != hasY3Traces )
    {
        m_axis_y3->SetVisible( hasY3Traces );
        visibilityChanged = true;
    }

    if( visibilityChanged )
        m_plotWin->UpdateAll();
}


void SIM_PLOT_TAB::DeleteTrace( TRACE* aTrace )
{
    for( const auto& [ name, trace ] : m_traces )
    {
        if( trace == aTrace )
        {
            m_traces.erase( name );
            break;
        }
    }

    for( const auto& [ id, cursor ] : aTrace->GetCursors() )
    {
        if( cursor )
            m_plotWin->DelLayer( cursor, true );
    }

    m_plotWin->DelLayer( aTrace, true, true );
    ResetScales( false );
    UpdateAxisVisibility();
}


bool SIM_PLOT_TAB::DeleteTrace( const wxString& aVectorName, int aTraceType )
{
    if( TRACE* trace = GetTrace( aVectorName, aTraceType ) )
    {
        DeleteTrace( trace );
        return true;
    }

    return false;
}


void SIM_PLOT_TAB::EnableCursor( TRACE* aTrace, int aCursorId, const wxString& aSignalName )
{
    CURSOR*   cursor = new CURSOR( aTrace, this );
    mpWindow* win = GetPlotWin();
    int       width = win->GetXScreen() - win->GetMarginLeft() - win->GetMarginRight();
    int       center = win->GetMarginLeft() + KiROUND( width * ( aCursorId == 1 ? 0.4 : 0.6 ) );

    cursor->SetName( aSignalName );
    cursor->SetX( center );

    aTrace->SetCursor( aCursorId, cursor );
    m_plotWin->AddLayer( cursor );

    // Notify the parent window about the changes
    wxQueueEvent( this, new wxCommandEvent( EVT_SIM_CURSOR_UPDATE ) );
}


void SIM_PLOT_TAB::DisableCursor( TRACE* aTrace, int aCursorId )
{
    if( CURSOR* cursor = aTrace->GetCursor( aCursorId ) )
    {
        aTrace->SetCursor( aCursorId, nullptr );
        GetPlotWin()->DelLayer( cursor, true );

        // Notify the parent window about the changes
        wxQueueEvent( this, new wxCommandEvent( EVT_SIM_CURSOR_UPDATE ) );
    }
}


void SIM_PLOT_TAB::ResetScales( bool aIncludeX )
{
    if( m_axis_x && aIncludeX )
    {
        m_axis_x->ResetDataRange();

        if( GetSimType() == ST_TRAN )
        {
            wxStringTokenizer tokenizer( GetSimCommand(), " \t\r\n", wxTOKEN_STRTOK );
            wxString          cmd = tokenizer.GetNextToken().Lower();

            wxASSERT( cmd == wxS( ".tran" ) );

            SPICE_VALUE step;
            SPICE_VALUE end( 1.0 );
            SPICE_VALUE start( 0.0 );

            if( tokenizer.HasMoreTokens() )
                step = SPICE_VALUE( tokenizer.GetNextToken() );

            if( tokenizer.HasMoreTokens() )
                end = SPICE_VALUE( tokenizer.GetNextToken() );

            if( tokenizer.HasMoreTokens() )
                start = SPICE_VALUE( tokenizer.GetNextToken() );

            static_cast<TIME_SCALE*>( m_axis_x )->SetStartAndEnd( start.ToDouble(), end.ToDouble() );
        }
    }

    if( m_axis_y1 )
        m_axis_y1->ResetDataRange();

    if( m_axis_y2 )
        m_axis_y2->ResetDataRange();

    if( m_axis_y3 )
        m_axis_y3->ResetDataRange();

    for( auto& [ name, trace ] : m_traces )
        trace->UpdateScales();
}


wxDEFINE_EVENT( EVT_SIM_CURSOR_UPDATE, wxCommandEvent );
