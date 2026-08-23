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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <wx/tokenzr.h>
#include <widgets/std_bitmap_button.h>
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


bool SMITH_GRID::GetChartView( mpWindow& aWindow, double aZoom, const wxRealPoint& aPan, SMITH_VIEW& aView )
{
    int mL = aWindow.GetMarginLeft(), mT = aWindow.GetMarginTop();
    int plotW = aWindow.GetScrX() - mL - aWindow.GetMarginRight();
    int plotH = aWindow.GetScrY() - mT - aWindow.GetMarginBottom();
    int radius = std::min( plotW, plotH ) / 2 - 15;

    if( radius <= 20 )
        return false;

    aView.center = wxPoint( mL + plotW / 2, mT + plotH / 2 );
    aView.radius = radius * aZoom;
    aView.zoom = aZoom;
    aView.pan = aPan;
    aView.plotRect = wxRect( mL, mT, plotW, plotH );

    return true;
}


static bool smithView( mpWindow& aWindow, SMITH_VIEW& aView )
{
    // the layers of a Smith chart are drawn on the SIM_VIEW carrying its own pan and zoom
    SIM_VIEW*   view = dynamic_cast<SIM_VIEW*>( &aWindow );
    double      zoom = view ? view->GetSmithZoom() : 1.0;
    wxRealPoint pan = view ? view->GetSmithPan() : wxRealPoint( 0.0, 0.0 );

    return SMITH_GRID::GetChartView( aWindow, zoom, pan, aView );
}


void SMITH_GRID::Plot( wxDC& aDC, mpWindow& aWindow )
{
    if( !m_visible )
        return;

    SMITH_VIEW view;

    if( !smithView( aWindow, view ) )
        return;

    const wxRect& plotRect = view.plotRect;
    int           mL = plotRect.x, mT = plotRect.y, plotW = plotRect.width, plotH = plotRect.height;

    aDC.SetClippingRegion( plotRect );

    // enough segments to keep the chord error under a pixel at this circle's screen radius
    auto segmentsFor =
            [&]( double aR ) -> int
            {
                return std::clamp( KiROUND( M_PI * std::sqrt( aR * view.radius ) ), 64, 4096 );
            };

    // draw a gamma-plane circle, keeping only the parts inside the unit circle
    auto drawClippedCircle =
            [&]( double aCx, double aCy, double aR )
            {
                constexpr double LIMIT = 1.0002;

                std::vector<wxPoint> run;
                int                  segments = segmentsFor( aR );

                for( int ii = 0; ii <= segments; ii++ )
                {
                    double angle = 2.0 * M_PI * ii / segments;
                    double re = aCx + aR * cos( angle );
                    double im = aCy + aR * sin( angle );

                    if( re * re + im * im <= LIMIT )
                    {
                        run.push_back( view.ToScreen( re, im ) );
                    }
                    else
                    {
                        if( run.size() > 1 )
                            aDC.DrawLines( (int) run.size(), run.data() );

                        run.clear();
                    }
                }

                if( run.size() > 1 )
                    aDC.DrawLines( (int) run.size(), run.data() );
            };

    auto formatValue =
            []( double aValue ) -> wxString
            {
                return wxString::Format( wxS( "%g" ), aValue );
            };

    // ohm labels for a single reference impedance, normalized labels without one
    double labelScale = m_z0 > 0.0 ? m_z0 : 1.0;

    static const std::vector<double> baseVals = { 0.2, 0.5, 1.0, 2.0, 5.0 };
    std::vector<double>              gridVals = baseVals;

    constexpr double FINEST_SCALE = 2.0 / ( 1.0 + 20.0 );

    wxRealPoint lowGamma = view.ToGamma( wxPoint( mL, mT + plotH ) );
    wxRealPoint highGamma = view.ToGamma( wxPoint( mL + plotW, mT ) );

    auto poleDistance =
            [&]( double aRe )
            {
                return std::hypot( std::max( { lowGamma.x - aRe, aRe - highGamma.x, 0.0 } ),
                                   std::max( { lowGamma.y, -highGamma.y, 0.0 } ) );
            };

    bool zoomedIn = ( plotW / 2.0 ) / view.radius < 1.0;
    bool converged = zoomedIn && std::min( poleDistance( 1.0 ), poleDistance( -1.0 ) ) < FINEST_SCALE;

    if( view.zoom >= 2.0 && !converged )
        gridVals.insert( gridVals.end(), { 0.1, 0.3, 0.4, 0.7, 1.5, 3.0, 10.0 } );

    if( view.zoom >= 5.0 && !converged )
        gridVals.insert( gridVals.end(), { 0.05, 0.15, 0.6, 0.8, 1.2, 1.7, 2.5, 4.0, 7.0, 20.0 } );

    wxPen gridPen = m_pen;
    gridPen.SetStyle( wxPENSTYLE_DOT );

    aDC.SetBrush( *wxTRANSPARENT_BRUSH );
    aDC.SetFont( GetPlotFont() );
    aDC.SetTextForeground( m_pen.GetColour() );
    aDC.SetPen( gridPen );

    // constant resistance circles, centered on the real axis, tangent at gamma = 1
    for( double r : gridVals )
        drawClippedCircle( r / ( 1.0 + r ), 0.0, 1.0 / ( 1.0 + r ) );

    // constant reactance arcs, one per sign, clipped to the unit circle
    for( double x : gridVals )
    {
        drawClippedCircle( 1.0, 1.0 / x, 1.0 / x );
        drawClippedCircle( 1.0, -1.0 / x, 1.0 / x );
    }

    aDC.SetPen( m_pen );
    aDC.DrawLine( view.ToScreen( -1.0, 0.0 ), view.ToScreen( 1.0, 0.0 ) );
    aDC.DrawCircle( view.ToScreen( 0.0, 0.0 ), KiROUND( view.radius ) );

    // Label each gridline at its axis/rim anchor, or at the plot edge when the anchor is
    // panned/zoomed out of view.
    constexpr double LABEL_LIMIT = 1.0002;

    auto anchorPos =
            [&]( double aRe, double aIm, wxPoint& aOut ) -> bool
            {
                if( aRe * aRe + aIm * aIm > LABEL_LIMIT )
                    return false;

                wxPoint p = view.ToScreen( aRe, aIm );

                if( !plotRect.Contains( p ) )
                    return false;

                aOut = p;
                return true;
            };

    // anchor off-screen, put the label where the gridline meets the plot edge
    auto edgePos =
            [&]( double aCx, double aCy, double aR, wxPoint& aOut ) -> bool
            {
                int  bestMargin = std::numeric_limits<int>::max();
                bool found = false;
                int  segments = segmentsFor( aR );

                for( int ii = 0; ii <= segments; ii++ )
                {
                    double angle = 2.0 * M_PI * ii / segments;
                    double re = aCx + aR * cos( angle );
                    double im = aCy + aR * sin( angle );

                    if( re * re + im * im > LABEL_LIMIT )
                        continue;

                    wxPoint p = view.ToScreen( re, im );

                    if( !plotRect.Contains( p ) )
                        continue;

                    int margin = std::min( { p.x - mL, mL + plotW - p.x, p.y - mT, mT + plotH - p.y } );

                    if( margin < bestMargin )
                    {
                        bestMargin = margin;
                        aOut = p;
                        found = true;
                    }
                }

                return found;
            };

    auto clampToPlot =
            [&]( const wxPoint& aPos, const wxSize& aExt ) -> wxPoint
            {
                return wxPoint( std::clamp( aPos.x, mL + 1, mL + plotW - aExt.x - 1 ),
                                std::clamp( aPos.y, mT + 1, mT + plotH - aExt.y - 1 ) );
            };

    std::vector<wxRect> placed;

    auto drawLabel =
            [&]( const wxString& aLabel, const wxPoint& aPos, const wxSize& aExt )
            {
                wxPoint pos = clampToPlot( aPos, aExt );
                wxRect  box( pos, aExt );

                box.Inflate( 2, 1 );

                for( const wxRect& seen : placed )
                {
                    if( seen.Intersects( box ) )
                        return;
                }

                placed.push_back( box );
                aDC.DrawText( aLabel, pos );
            };

    auto drawEdgeLabel =
            [&]( const wxString& aLabel, const wxPoint& aAt )
            {
                wxSize ext = aDC.GetTextExtent( aLabel );

                drawLabel( aLabel, wxPoint( aAt.x - ext.x / 2, aAt.y + 3 ), ext );
            };

    // push reactance labels just outside the rim, clamped so j50 and -j50 are not clipped
    auto drawRimLabel = [&]( const wxString& aLabel, const wxPoint& aAt, double aRe, double aIm )
    {
        wxSize  ext = aDC.GetTextExtent( aLabel );
        wxPoint pos = aAt;

        pos.x += KiROUND( aRe * 6 );
        pos.y -= KiROUND( aIm * 6 );
        pos.x -= KiROUND( ext.x * ( 1.0 - aRe ) / 2.0 );
        pos.y -= KiROUND( ext.y * ( 1.0 + aIm ) / 2.0 );

        drawLabel( aLabel, pos, ext );
    };

    wxPoint at;

    // short (r = 0)
    if( anchorPos( -1.0, 0.0, at ) )
        drawRimLabel( wxS( "0" ), at, -1.0, 0.0 );
    else if( edgePos( 0.0, 0.0, 1.0, at ) )
        drawEdgeLabel( wxS( "0" ), at );

    for( double r : gridVals )
    {
        if( anchorPos( ( r - 1.0 ) / ( r + 1.0 ), 0.0, at ) || edgePos( r / ( 1.0 + r ), 0.0, 1.0 / ( 1.0 + r ), at ) )
        {
            drawEdgeLabel( formatValue( r * labelScale ), at );
        }
    }

    for( double x : gridVals )
    {
        double   d = x * x + 1.0;
        double   re = ( x * x - 1.0 ) / d;
        double   im = 2.0 * x / d;
        wxString posLabel = wxS( "j" ) + formatValue( x * labelScale );
        wxString negLabel = wxS( "-j" ) + formatValue( x * labelScale );

        if( anchorPos( re, im, at ) )
            drawRimLabel( posLabel, at, re, im );
        else if( edgePos( 1.0, 1.0 / x, 1.0 / x, at ) )
            drawEdgeLabel( posLabel, at );

        if( anchorPos( re, -im, at ) )
            drawRimLabel( negLabel, at, re, -im );
        else if( edgePos( 1.0, -1.0 / x, 1.0 / x, at ) )
            drawEdgeLabel( negLabel, at );
    }

    // the ohm labels mean nothing unless the reference impedance is named
    wxString note;

    if( m_z0 > 0.0 )
        note = wxString::Format( wxS( "Z0 = %s Ω" ), formatValue( m_z0 ) );
    else if( m_mixedReferences )
        note = _( "Normalized Z/Z0 (ports differ)" );
    else
        note = _( "Normalized Z/Z0" );

    wxSize ext = aDC.GetTextExtent( note );

    aDC.DrawText( note, mL + 4, mT + plotH - ext.y - 4 );

    aDC.DestroyClippingRegion();
}


void SMITH_TRACE::Plot( wxDC& aDC, mpWindow& aWindow )
{
    if( !m_visible )
        return;

    SMITH_VIEW view;

    if( !smithView( aWindow, view ) )
        return;

    const std::vector<double>& xs = GetDataX();
    const std::vector<double>& ys = GetDataY();
    size_t                     count = std::min( xs.size(), ys.size() );

    if( count == 0 )
        return;

    aDC.SetPen( m_pen );
    aDC.SetClippingRegion( view.plotRect );

    size_t chunk = GetSweepSize();

    if( GetSweepCount() <= 1 || chunk == std::numeric_limits<size_t>::max() || chunk == 0 )
        chunk = count;

    std::vector<wxPoint> pts;

    auto flush =
            [&]()
            {
                if( pts.size() > 1 )
                {
                    aDC.DrawLines( (int) pts.size(), pts.data() );
                }
                else if( pts.size() == 1 )
                {
                    aDC.SetBrush( wxBrush( m_pen.GetColour() ) );
                    aDC.DrawCircle( pts[0], 2 );
                }

                pts.clear();
            };

    for( size_t start = 0; start < count; start += chunk )
    {
        size_t end = std::min( count, start + chunk );

        for( size_t ii = start; ii < end; ii++ )
        {
            // a non-finite sample breaks the locus rather than drawing a bogus segment
            if( !std::isfinite( xs[ii] ) || !std::isfinite( ys[ii] ) )
            {
                flush();
                continue;
            }

            pts.emplace_back( view.ToScreen( xs[ii], ys[ii] ) );
        }

        flush();
    }

    aDC.DestroyClippingRegion();
}


void SMITH_CURSOR::snapToIndex( int aIndex )
{
    const std::vector<double>& re = m_trace->GetDataX();
    const std::vector<double>& im = m_trace->GetDataY();
    const std::vector<double>& freqs = static_cast<SMITH_TRACE*>( m_trace )->GetFrequencies();

    size_t count = std::min( re.size(), im.size() );

    if( count == 0 )
        return;

    m_index = std::clamp( aIndex, 0, (int) count - 1 );
    m_gamma = wxRealPoint( re[m_index], im[m_index] );

    // no frequency data for this sample, keep the previous x so a saved position stays finite
    double freq = m_index < (int) freqs.size() ? freqs[m_index] : m_coords.x;

    m_coords = wxRealPoint( freq, std::hypot( m_gamma.x, m_gamma.y ) );
}


void SMITH_CURSOR::snapToFrequency( double aFreq )
{
    const std::vector<double>& freqs = static_cast<SMITH_TRACE*>( m_trace )->GetFrequencies();
    const std::vector<double>& re = m_trace->GetDataX();
    const std::vector<double>& im = m_trace->GetDataY();

    // frequencies repeat identically per run, search only the run the cursor is on
    // so a frequency-keyed move cannot silently hop to run 0
    size_t begin = 0;
    size_t end = freqs.size();
    size_t chunk = m_trace->GetSweepSize();

    if( m_trace->GetSweepCount() > 1
            && chunk > 0
            && chunk != std::numeric_limits<size_t>::max()
            && m_index >= 0
            && (size_t) m_index < freqs.size() )
    {
        begin = ( (size_t) m_index / chunk ) * chunk;
        end = std::min( freqs.size(), begin + chunk );
    }

    int    best = -1;
    double bestDist = std::numeric_limits<double>::max();

    for( size_t ii = begin; ii < end; ii++ )
    {
        if( !std::isfinite( freqs[ii] ) )
            continue;

        if( ii < re.size() && ii < im.size() && ( !std::isfinite( re[ii] ) || !std::isfinite( im[ii] ) ) )
            continue;

        double dist = std::fabs( freqs[ii] - aFreq );

        if( dist < bestDist )
        {
            bestDist = dist;
            best = (int) ii;
        }
    }

    if( best >= 0 )
        snapToIndex( best );
}


void SMITH_CURSOR::SetCoordX( double aValue )
{
    m_requestFreq = aValue;

    UpdateForNewData();

    if( m_window )
        m_window->Refresh();
}


void SMITH_CURSOR::UpdateForNewData()
{
    if( static_cast<SMITH_TRACE*>( m_trace )->GetFrequencies().empty() )
    {
        // no data yet, remember the frequency and resolve it once the sim fills in
        m_coords.x = m_requestFreq;
        m_pendingFreq = true;
        m_updateRequired = false;
        return;
    }

    snapToFrequency( m_requestFreq );
    m_pendingFreq = false;
    m_updateRequired = false;
    m_updateRef = true;
}


void SMITH_CURSOR::Move( wxPoint aDelta )
{
    m_dragging = true;
    Update();
    mpInfoLayer::Move( aDelta );
}


void SMITH_CURSOR::UpdateReference()
{
    // skip CURSOR's axis reference, the marker follows the locus
    mpInfoLayer::UpdateReference();
}


bool SMITH_CURSOR::Inside( const wxPoint& aPoint ) const
{
    if( !m_window || m_index < 0 )
        return false;

    SMITH_VIEW view;

    if( !smithView( *m_window, view ) )
        return false;

    wxPoint marker = view.ToScreen( m_gamma.x, m_gamma.y );

    return std::abs( aPoint.x - marker.x ) <= DRAG_MARGIN && std::abs( aPoint.y - marker.y ) <= DRAG_MARGIN;
}


void SMITH_CURSOR::Plot( wxDC& aDC, mpWindow& aWindow )
{
    if( !m_window )
        m_window = &aWindow;

    if( !m_visible )
        return;

    SMITH_VIEW view;

    if( !smithView( aWindow, view ) )
        return;

    const std::vector<double>& re = m_trace->GetDataX();
    const std::vector<double>& im = m_trace->GetDataY();
    size_t                     count = std::min( re.size(), im.size() );

    if( count == 0 )
        return;

    if( m_pendingFreq )
    {
        // sim data has arrived, restore the frequency saved from the workbook
        snapToFrequency( m_requestFreq );
        m_pendingFreq = false;
        m_updateRequired = false;
        m_updateRef = true;
    }
    else if( m_updateRequired )
    {
        if( m_dragging )
        {
            // snap to the locus sample closest to the drag position
            int    best = -1;
            double bestDist = std::numeric_limits<double>::max();

            for( size_t ii = 0; ii < count; ii++ )
            {
                if( !std::isfinite( re[ii] ) || !std::isfinite( im[ii] ) )
                    continue;

                wxPoint p = view.ToScreen( re[ii], im[ii] );
                double  dx = (double) p.x - m_dim.x;
                double  dy = (double) p.y - m_dim.y;
                double  dist = dx * dx + dy * dy;

                if( dist < bestDist )
                {
                    bestDist = dist;
                    best = (int) ii;
                }
            }

            if( best >= 0 )
            {
                snapToIndex( best );
                m_requestFreq = m_coords.x;
            }

            m_dragging = false;
        }
        else
        {
            // the trace data changed under the cursor, follow the frequency rather than
            // the screen position so a re-run cannot hop to another point of the locus
            UpdateForNewData();
        }

        m_updateRequired = false;

        // Notify the parent window about the changes
        wxQueueEvent( aWindow.GetParent(), new wxCommandEvent( EVT_SIM_CURSOR_UPDATE ) );
    }
    else
    {
        if( m_index < 0 )
        {
            snapToIndex( (int) count / 2 );
            m_requestFreq = m_coords.x;
        }

        m_updateRef = true;
    }

    wxPoint marker = view.ToScreen( m_gamma.x, m_gamma.y );

    m_dim.SetX( marker.x );
    m_dim.SetY( marker.y );

    if( m_updateRef )
    {
        mpInfoLayer::UpdateReference();
        m_updateRef = false;
    }

    wxPen    pen = GetPen();
    wxColour fg = aWindow.GetForegroundColour();
    COLOR4D  cursorColor = COLOR4D( m_trace->GetTraceColour() ).Mix( fg, 0.6 );

    pen.SetColour( cursorColor.ToColour() );
    pen.SetStyle( wxPENSTYLE_SOLID );
    aDC.SetPen( pen );
    aDC.SetBrush( *wxTRANSPARENT_BRUSH );

    aDC.DrawCircle( marker, 4 );
    aDC.DrawLine( marker.x - 8, marker.y, marker.x - 4, marker.y );
    aDC.DrawLine( marker.x + 4, marker.y, marker.x + 8, marker.y );
    aDC.DrawLine( marker.x, marker.y - 8, marker.x, marker.y - 4 );
    aDC.DrawLine( marker.x, marker.y + 4, marker.x, marker.y + 8 );

    double gm = std::hypot( m_gamma.x, m_gamma.y );
    double z0 = static_cast<SMITH_TRACE*>( m_trace )->GetReferenceImpedance();
    double freq = m_coords.x;
    double zr, zi;

    auto formatSI = []( double aValue, const wxString& aUnit ) -> wxString
    {
        if( std::isnan( aValue ) )
            return wxS( "--" );

        int      power = 0;
        wxString suffix;

        getSISuffix( aValue, aUnit, power, suffix );

        double sf = pow( 10.0, power );

        return formatFloat( aValue / sf, 3 ) + wxS( " " ) + suffix;
    };

    std::vector<wxString> lines;

    lines.push_back( getID() + wxS( ":  f = " ) + formatSI( freq, wxS( "Hz" ) ) );

    // ohms need the port impedance, without one only the normalized z is known
    bool absolute = z0 > 0.0;

    if( !SMITH_MATH::GammaToImpedance( m_gamma.x, m_gamma.y, absolute ? z0 : 1.0, zr, zi ) )
    {
        lines.push_back( wxS( "Z = inf" ) );
    }
    else if( absolute )
    {
        lines.push_back( wxString::Format( wxS( "Z = %s %s j%s" ),
                                           formatSI( zr, wxS( "Ω" ) ),
                                           zi < 0 ? wxS( "-" ) : wxS( "+" ),
                                           formatSI( std::fabs( zi ), wxS( "Ω" ) ) ) );

        // series equivalent of the reactance at the marker frequency
        if( std::isfinite( freq ) && freq > 0.0 && zi != 0.0 )
        {
            if( zi > 0.0 )
                lines.push_back( wxS( "L = " ) + formatSI( SMITH_MATH::SeriesInductance( zi, freq ), wxS( "H" ) ) );
            else
                lines.push_back( wxS( "C = " ) + formatSI( SMITH_MATH::SeriesCapacitance( zi, freq ), wxS( "F" ) ) );
        }
    }
    else
    {
        lines.push_back( wxString::Format( wxS( "z = %s %s j%s" ),
                                           formatFloat( zr, 3 ),
                                           zi < 0 ? wxS( "-" ) : wxS( "+" ),
                                           formatFloat( std::fabs( zi ), 3 ) ) );
    }

    double rl = SMITH_MATH::ReturnLoss( gm );
    double vswr = SMITH_MATH::VSWR( gm );

    if( std::isfinite( rl ) )
        lines.push_back( wxString::Format( wxS( "RL = %s dB" ), formatFloat( rl, 1 ) ) );
    else
        lines.push_back( wxS( "RL = inf" ) );

    if( std::isfinite( vswr ) )
        lines.push_back( wxString::Format( wxS( "VSWR = %s" ), formatFloat( vswr, 2 ) ) );
    else
        lines.push_back( wxS( "VSWR = inf" ) );

    aDC.SetFont( GetPlotFont() );

    int boxW = 0;
    int boxH = 0;
    int lineH = aDC.GetTextExtent( wxS( "M" ) ).y;

    for( const wxString& line : lines )
        boxW = std::max( boxW, aDC.GetTextExtent( line ).x );

    boxW += 8;
    boxH = (int) lines.size() * lineH + 6;

    wxPoint boxPos( marker.x + ( marker.x < aWindow.GetScrX() / 2 ? 12 : -12 - boxW ),
                    marker.y + ( marker.y < aWindow.GetScrY() / 2 ? 12 : -12 - boxH ) );

    boxPos.x = std::clamp( boxPos.x, 0, std::max( 0, aWindow.GetScrX() - boxW ) );
    boxPos.y = std::clamp( boxPos.y, 0, std::max( 0, aWindow.GetScrY() - boxH ) );

    wxBrush labelBrush( aWindow.GetBackgroundColour() );

    aDC.SetBrush( labelBrush );
    aDC.DrawRectangle( wxRect( boxPos, wxSize( boxW, boxH ) ) );
    aDC.SetTextForeground( cursorColor.ToColour() );

    for( size_t ii = 0; ii < lines.size(); ii++ )
        aDC.DrawText( lines[ii], boxPos.x + 4, boxPos.y + 3 + (int) ii * lineH );
}


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

    if( m_trace->IsMultiRun() && m_window
            && m_trace->GetSweepCount() > 1
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

    if( snapToNearest && m_trace->IsMultiRun()
            && sweepCount > 1
            && sweepSize != std::numeric_limits<size_t>::max()
            && sweepSize > 0
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

    if( m_trace->IsMultiRun()
            && sweepCount > 1
            && sweepSize != std::numeric_limits<size_t>::max()
            && sweepSize > 0 )
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

    if( !std::isfinite( m_coords.x ) )
        return;

    // A silent trace interpolates to no y value at all, and converting that to a pixel is
    // undefined behaviour, so carry the x cursor on its own
    const bool hasY = std::isfinite( m_coords.y );

    // Line length in horizontal and vertical dimensions
    const wxPoint cursorPos( aWindow.x2p( m_trace->x2s( m_coords.x ) ),
                             hasY ? aWindow.y2p( m_trace->y2s( m_coords.y ) ) : 0 );

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

    if( hasY && topPx < cursorPos.y && cursorPos.y < bottomPx )
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

    // An undefined coordinate draws no line, so it offers nothing to grab
    bool nearX = std::isfinite( m_coords.x )
            && std::abs( (double) aPoint.x - m_window->x2p( m_trace->x2s( m_coords.x ) ) ) <= DRAG_MARGIN;
    bool nearY = std::isfinite( m_coords.y )
            && std::abs( (double) aPoint.y - m_window->y2p( m_trace->y2s( m_coords.y ) ) ) <= DRAG_MARGIN;

    return nearX || nearY;
}


void CURSOR::UpdateReference()
{
    if( !m_window )
        return;

    // An undefined coordinate has no pixel, so keep the last good reference for a drag to
    // measure against
    if( std::isfinite( m_coords.x ) )
        m_reference.x = m_window->x2p( m_trace->x2s( m_coords.x ) );

    if( std::isfinite( m_coords.y ) )
        m_reference.y = m_window->y2p( m_trace->y2s( m_coords.y ) );
}


SIM_VIEW::SIM_VIEW( SIM_PLOT_TAB* aPlotTab, wxWindow* aParent ) :
        mpWindow( aParent, wxID_ANY ),
        m_axis_x( nullptr ),
        m_axis_y1( nullptr ),
        m_axis_y2( nullptr ),
        m_axis_y3( nullptr ),
        m_legend( nullptr ),
        m_plotTab( aPlotTab ),
        m_smithGrid( nullptr ),
        m_smithChart( false ),
        m_smithZoom( 1.0 ),
        m_smithPanning( false ),
        m_smithLeftSkipped( false )
{
    // Smith-mode pan/zoom, these run before mpWindow's handlers and skip when not a Smith chart
    Bind( wxEVT_MOUSEWHEEL, &SIM_VIEW::onSmithMouseWheel, this );
    Bind( wxEVT_MAGNIFY, &SIM_VIEW::onSmithMagnify, this );
    Bind( wxEVT_MIDDLE_DOWN, &SIM_VIEW::onSmithMiddleDown, this );
    Bind( wxEVT_LEFT_DOWN, &SIM_VIEW::onSmithLeftDown, this );
    Bind( wxEVT_MOTION, &SIM_VIEW::onSmithMotion, this );
    Bind( wxEVT_LEFT_UP, &SIM_VIEW::onSmithLeftUp, this );
    Bind( wxEVT_LEFT_DCLICK, &SIM_VIEW::onSmithDClick, this );
    Bind( wxEVT_RIGHT_DOWN, &SIM_VIEW::onSmithRightDown, this );
    Bind( wxEVT_RIGHT_UP, &SIM_VIEW::onSmithRightUp, this );

    // route the context-menu zoom commands to the Smith view
    for( int id : { mpID_ZOOM_IN, mpID_ZOOM_OUT, mpID_FIT, mpID_CENTER } )
        Bind( wxEVT_MENU, &SIM_VIEW::onSmithMenuCommand, this, id );
}


void SIM_VIEW::OnXViewChanged()
{
    if( m_plotTab )
        m_plotTab->SyncXView( this );
}


void SIM_VIEW::SetY1Scale( bool aLock, double aMin, double aMax )
{
    wxCHECK( m_axis_y1, /* void */ );
    m_axis_y1->SetAxisMinMax( aLock, aMin, aMax );
}


void SIM_VIEW::SetY2Scale( bool aLock, double aMin, double aMax )
{
    wxCHECK( m_axis_y2, /* void */ );
    m_axis_y2->SetAxisMinMax( aLock, aMin, aMax );
}


void SIM_VIEW::SetY3Scale( bool aLock, double aMin, double aMax )
{
    wxCHECK( m_axis_y3, /* void */ );
    m_axis_y3->SetAxisMinMax( aLock, aMin, aMax );
}


wxString SIM_VIEW::GetUnitsX() const
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


wxString SIM_VIEW::GetUnitsY1() const
{
    LIN_SCALE<mpScaleY>* linScale = dynamic_cast<LIN_SCALE<mpScaleY>*>( m_axis_y1 );

    if( linScale )
        return linScale->GetUnits();
    else
        return wxEmptyString;
}


wxString SIM_VIEW::GetUnitsY2() const
{
    LIN_SCALE<mpScaleY>* linScale = dynamic_cast<LIN_SCALE<mpScaleY>*>( m_axis_y2 );

    if( linScale )
        return linScale->GetUnits();
    else
        return wxEmptyString;
}


wxString SIM_VIEW::GetUnitsY3() const
{
    LIN_SCALE<mpScaleY>* linScale = dynamic_cast<LIN_SCALE<mpScaleY>*>( m_axis_y3 );

    if( linScale )
        return linScale->GetUnits();
    else
        return wxEmptyString;
}


wxString SIM_VIEW::GetUnitsForTrace( TRACE* aTrace ) const
{
    if( m_plotTab->GetSimType() == ST_AC )
    {
        if( aTrace->GetType() & SPT_AC_PHASE )
            return GetUnitsY2();
        else
            return GetUnitsY1();
    }
    else
    {
        if( aTrace->GetType() & SPT_POWER )
            return GetUnitsY3();
        else if( aTrace->GetType() & SPT_CURRENT )
            return GetUnitsY2();
        else
            return GetUnitsY1();
    }
}


int SIM_VIEW::GetAxisSlot( TRACE* aTrace ) const
{
    if( ( aTrace->GetType() & SPT_AC_PHASE )
        || ( ( m_plotTab->GetSimType() != ST_AC ) && ( aTrace->GetType() & SPT_CURRENT ) ) )
    {
        return 2;
    }
    else if( aTrace->GetType() & SPT_POWER )
    {
        return 3;
    }
    else
    {
        return 1;
    }
}


mpScaleY* SIM_VIEW::GetAxisBySlot( int aSlot ) const
{
    switch( aSlot )
    {
    case 2: return m_axis_y2;
    case 3: return m_axis_y3;
    default: return m_axis_y1;
    }
}


void SIM_VIEW::updateAxes( int aNewTraceType )
{
    switch( m_plotTab->GetSimType() )
    {
    case ST_AC:
        if( !m_axis_x )
        {
            m_axis_x = new LOG_SCALE<mpScaleXLog>( wxEmptyString, wxT( "Hz" ), mpALIGN_BOTTOM );
            m_axis_x->SetNameAlign( mpALIGN_BOTTOM );
            AddLayer( m_axis_x );

            m_axis_y1 = new LIN_SCALE<mpScaleY>( wxEmptyString, wxT( "dB" ), mpALIGN_LEFT );
            m_axis_y1->SetNameAlign( mpALIGN_LEFT );
            AddLayer( m_axis_y1 );

            m_axis_y2 = new LIN_SCALE<mpScaleY>( wxEmptyString, wxT( "°" ), mpALIGN_RIGHT );
            m_axis_y2->SetNameAlign( mpALIGN_RIGHT );
            m_axis_y2->SetMasterScale( m_axis_y1 );
            AddLayer( m_axis_y2 );
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
            AddLayer( m_axis_x );

            m_axis_y1 = new LIN_SCALE<mpScaleY>( wxEmptyString, wxT( "" ), mpALIGN_LEFT );
            m_axis_y1->SetNameAlign( mpALIGN_LEFT );
            AddLayer( m_axis_y1 );

            m_axis_y2 = new LIN_SCALE<mpScaleY>( wxEmptyString, wxT( "°" ), mpALIGN_RIGHT );
            m_axis_y2->SetNameAlign( mpALIGN_RIGHT );
            m_axis_y2->SetMasterScale( m_axis_y1 );
            AddLayer( m_axis_y2 );
        }

        m_axis_x->SetName( _( "Frequency" ) );
        m_axis_y1->SetName( _( "Amplitude" ) );
        m_axis_y2->SetName( _( "Phase" ) );
        break;

    case ST_DC: prepareDCAxes( aNewTraceType ); break;

    case ST_NOISE:
        if( !m_axis_x )
        {
            m_axis_x = new LOG_SCALE<mpScaleXLog>( wxEmptyString, wxT( "Hz" ), mpALIGN_BOTTOM );
            m_axis_x->SetNameAlign( mpALIGN_BOTTOM );
            AddLayer( m_axis_x );

            if( ( aNewTraceType & SPT_CURRENT ) == 0 )
            {
                m_axis_y1 = new LIN_SCALE<mpScaleY>( wxEmptyString, wxT( "" ), mpALIGN_LEFT );
                m_axis_y1->SetNameAlign( mpALIGN_LEFT );
                AddLayer( m_axis_y1 );
            }
            else
            {
                m_axis_y2 = new LIN_SCALE<mpScaleY>( wxEmptyString, wxT( "" ), mpALIGN_RIGHT );
                m_axis_y2->SetNameAlign( mpALIGN_RIGHT );
                AddLayer( m_axis_y2 );
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
            AddLayer( m_axis_x );

            m_axis_y1 = new LIN_SCALE<mpScaleY>( wxEmptyString, wxT( "dB" ), mpALIGN_LEFT );
            m_axis_y1->SetNameAlign( mpALIGN_LEFT );
            AddLayer( m_axis_y1 );
        }

        m_axis_x->SetName( _( "Frequency" ) );
        m_axis_y1->SetName( _( "Intensity" ) );
        break;

    case ST_TRAN:
        if( !m_axis_x )
        {
            m_axis_x = new TIME_SCALE( wxEmptyString, wxT( "s" ), mpALIGN_BOTTOM );
            m_axis_x->SetNameAlign( mpALIGN_BOTTOM );
            AddLayer( m_axis_x );

            m_axis_y1 = new LIN_SCALE<mpScaleY>( wxEmptyString, wxT( "V" ), mpALIGN_LEFT );
            m_axis_y1->SetNameAlign( mpALIGN_LEFT );
            AddLayer( m_axis_y1 );

            m_axis_y2 = new LIN_SCALE<mpScaleY>( wxEmptyString, wxT( "A" ), mpALIGN_RIGHT );
            m_axis_y2->SetNameAlign( mpALIGN_RIGHT );
            m_axis_y2->SetMasterScale( m_axis_y1 );
            AddLayer( m_axis_y2 );
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

    if( m_plotTab->GetSimType() == ST_TRAN || m_plotTab->GetSimType() == ST_DC )
    {
        if( m_axis_y3 )
        {
            SetMargins( 30, 160, 45, 70 );

            if( m_axis_y2 )
                m_axis_y2->SetNameAlign( mpALIGN_BORDER_RIGHT );

            m_axis_y3->SetAlign( mpALIGN_BORDER_RIGHT );
            m_axis_y3->SetNameAlign( mpALIGN_BORDER_RIGHT );
        }
        else
        {
            SetMargins( 30, 70, 45, 70 );

            if( m_axis_y2 )
                m_axis_y2->SetNameAlign( mpALIGN_RIGHT );
        }
    }

    if( m_axis_x )
        m_axis_x->SetFont( KIUI::GetStatusFont( this ) );

    if( m_axis_y1 )
        m_axis_y1->SetFont( KIUI::GetStatusFont( this ) );

    if( m_axis_y2 )
        m_axis_y2->SetFont( KIUI::GetStatusFont( this ) );

    if( m_axis_y3 )
        m_axis_y3->SetFont( KIUI::GetStatusFont( this ) );

    UpdateAxisVisibility();
}


void SIM_VIEW::prepareDCAxes( int aNewTraceType )
{
    wxString sim_cmd = m_plotTab->GetSimCommand().Lower();
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
                AddLayer( m_axis_x );
            }

            m_axis_x->SetName( _( "Voltage (swept)" ) );
            break;

        case 'i':
            if( !m_axis_x )
            {
                m_axis_x = new LIN_SCALE<mpScaleX>( wxEmptyString, wxT( "A" ), mpALIGN_BOTTOM );
                m_axis_x->SetNameAlign( mpALIGN_BOTTOM );
                AddLayer( m_axis_x );
            }

            m_axis_x->SetName( _( "Current (swept)" ) );
            break;

        case 'r':
            if( !m_axis_x )
            {
                m_axis_x = new LIN_SCALE<mpScaleX>( wxEmptyString, wxT( "Ω" ), mpALIGN_BOTTOM );
                m_axis_x->SetNameAlign( mpALIGN_BOTTOM );
                AddLayer( m_axis_x );
            }

            m_axis_x->SetName( _( "Resistance (swept)" ) );
            break;

        case 't':
            if( !m_axis_x )
            {
                m_axis_x = new LIN_SCALE<mpScaleX>( wxEmptyString, wxT( "°C" ), mpALIGN_BOTTOM );
                m_axis_x->SetNameAlign( mpALIGN_BOTTOM );
                AddLayer( m_axis_x );
            }

            m_axis_x->SetName( _( "Temperature (swept)" ) );
            break;
        }

        if( !m_axis_y1 )
        {
            m_axis_y1 = new LIN_SCALE<mpScaleY>( wxEmptyString, wxT( "V" ), mpALIGN_LEFT );
            m_axis_y1->SetNameAlign( mpALIGN_LEFT );
            AddLayer( m_axis_y1 );
        }

        if( !m_axis_y2 )
        {
            m_axis_y2 = new LIN_SCALE<mpScaleY>( wxEmptyString, wxT( "A" ), mpALIGN_RIGHT );
            m_axis_y2->SetNameAlign( mpALIGN_RIGHT );
            AddLayer( m_axis_y2 );
        }

        m_axis_y1->SetName( _( "Voltage (measured)" ) );
        m_axis_y2->SetName( _( "Current" ) );

        if( ( aNewTraceType & SPT_POWER ) )
            EnsureThirdYAxisExists();

        if( m_axis_y3 )
            m_axis_y3->SetName( _( "Power" ) );
    }
}


void SIM_VIEW::EnsureThirdYAxisExists()
{
    if( !m_axis_y3 )
    {
        SetMargins( 30, 160, 45, 70 );
        m_axis_y3 = new LIN_SCALE<mpScaleY>( wxEmptyString, wxT( "W" ), mpALIGN_BORDER_RIGHT );
        m_axis_y3->SetNameAlign( mpALIGN_BORDER_RIGHT );
        m_axis_y3->SetMasterScale( m_axis_y1 );
        AddLayer( m_axis_y3 );
    }

    if( m_axis_y3 )
    {
        m_axis_y3->SetAlign( mpALIGN_BORDER_RIGHT );
        m_axis_y3->SetNameAlign( mpALIGN_BORDER_RIGHT );
    }

    if( m_axis_y2 )
        m_axis_y2->SetNameAlign( mpALIGN_BORDER_RIGHT );
}


void SIM_VIEW::UpdateAxisVisibility()
{
    bool hasY1Traces = false;
    bool hasY2Traces = false;
    bool hasY3Traces = false;

    if( !m_smithChart )
    {
        for( const auto& [name, trace] : m_plotTab->GetTraces() )
        {
            if( !trace || trace->GetView() != this )
                continue;

            if( trace->GetType() & SPT_POWER )
            {
                hasY3Traces = true;
            }
            else if( ( trace->GetType() & SPT_AC_PHASE )
                     || ( ( m_plotTab->GetSimType() != ST_AC ) && ( trace->GetType() & SPT_CURRENT ) ) )
            {
                hasY2Traces = true;
            }
            else
            {
                hasY1Traces = true;
            }
        }
    }

    bool visibilityChanged = false;

    if( m_axis_x && m_axis_x->IsVisible() != !m_smithChart )
    {
        m_axis_x->SetVisible( !m_smithChart );
        visibilityChanged = true;
    }

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
        UpdateAll();
}


void SIM_VIEW::ResetScales( bool aIncludeX )
{
    if( m_axis_x && aIncludeX )
    {
        m_axis_x->ResetDataRange();

        if( m_plotTab->GetSimType() == ST_TRAN )
        {
            wxStringTokenizer tokenizer( m_plotTab->GetSimCommand(), " \t\r\n", wxTOKEN_STRTOK );
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

    for( auto& [name, trace] : m_plotTab->GetTraces() )
    {
        if( trace->GetView() == this )
            trace->UpdateScales();
    }
}


void SIM_VIEW::SetSmithChart( bool aEnable )
{
    if( m_smithChart == aEnable )
        return;

    m_smithChart = aEnable;

    // a mode switch mid-gesture must not leave a pan or a skipped click behind
    m_smithPanning = false;
    m_smithLeftSkipped = false;

    if( aEnable && !m_smithGrid )
    {
        m_smithGrid = new SMITH_GRID();
        m_smithGrid->SetFont( KIUI::GetStatusFont( this ) );
        m_smithGrid->SetPen( wxPen( m_plotTab->GetPlotColor( SIM_PLOT_COLORS::COLOR_SET::AXIS ), 1 ) );
        AddLayer( m_smithGrid );
    }

    if( m_smithGrid )
        m_smithGrid->SetVisible( aEnable );

    if( aEnable )
        UpdateSmithReferenceImpedance();

    ResetSmithView();

    UpdateAxisVisibility();
    UpdateAll();
}


void SIM_VIEW::UpdateSmithReferenceImpedance()
{
    if( !m_smithGrid )
        return;

    double z0 = 0.0;
    bool   mixed = false;
    bool   unresolved = false;

    for( const auto& [name, trace] : m_plotTab->GetTraces() )
    {
        SMITH_TRACE* smithTrace = dynamic_cast<SMITH_TRACE*>( trace );

        if( !smithTrace || smithTrace->GetView() != this )
            continue;

        double traceZ0 = smithTrace->GetReferenceImpedance();

        if( traceZ0 <= 0.0 )
            unresolved = true;
        else if( z0 == 0.0 )
            z0 = traceZ0;
        else if( traceZ0 != z0 )
            mixed = true;
    }

    // a trace whose port impedance is unknown leaves no single reference to name either
    m_smithGrid->SetReferenceImpedance( mixed || unresolved ? 0.0 : z0 );
    m_smithGrid->SetMixedReferences( mixed );
}


void SIM_VIEW::UpdateSmithGridColor()
{
    if( m_smithGrid )
        m_smithGrid->SetPen( wxPen( m_plotTab->GetPlotColor( SIM_PLOT_COLORS::COLOR_SET::AXIS ), 1 ) );
}


bool SIM_VIEW::getSmithView( SMITH_VIEW& aView ) const
{
    return SMITH_GRID::GetChartView( const_cast<SIM_VIEW&>( *this ), m_smithZoom, m_smithPan, aView );
}


void SIM_VIEW::SmithZoomAt( const wxPoint& aPos, double aFactor )
{
    SMITH_VIEW view;

    if( !getSmithView( view ) )
        return;

    double newZoom = std::clamp( m_smithZoom * aFactor, 1.0, 50.0 );

    if( newZoom <= 1.0 )
    {
        ResetSmithView();
        Refresh();
        return;
    }

    // keep the point under the cursor fixed while zooming
    m_smithPan = SMITH_MATH::ZoomAboutPoint( view, aPos, newZoom );
    m_smithZoom = newZoom;

    Refresh();
}


void SIM_VIEW::SmithPanBy( const wxPoint& aDelta )
{
    SMITH_VIEW view;

    if( !getSmithView( view ) )
        return;

    m_smithPan.x -= aDelta.x / view.radius;
    m_smithPan.y += aDelta.y / view.radius;

    Refresh();
}


void SIM_VIEW::onSmithMouseWheel( wxMouseEvent& aEvent )
{
    if( !m_smithChart )
    {
        aEvent.Skip();
        return;
    }

    // swallow horizontal scroll too, mpWindow would pan its hidden axes with it
    if( aEvent.GetWheelAxis() != wxMOUSE_WHEEL_VERTICAL || aEvent.GetWheelRotation() == 0 )
        return;

    // do not skip, or mpWindow would also zoom its hidden axes
    SmithZoomAt( aEvent.GetPosition(), aEvent.GetWheelRotation() > 0 ? 1.2 : 1.0 / 1.2 );
}


void SIM_VIEW::onSmithMagnify( wxMouseEvent& aEvent )
{
    if( !m_smithChart )
    {
        aEvent.Skip();
        return;
    }

    double factor = aEvent.GetMagnification() + 1.0;

    if( factor > 0.0 )
        SmithZoomAt( aEvent.GetPosition(), factor );
}


void SIM_VIEW::onSmithMiddleDown( wxMouseEvent& aEvent )
{
    // keep mpWindow's middle-button pan off the hidden axes
    if( !m_smithChart )
        aEvent.Skip();
}


void SIM_VIEW::onSmithLeftDown( wxMouseEvent& aEvent )
{
    // clear stale pan state so it cannot hijack a cursor grab
    m_smithPanning = false;
    m_smithLeftSkipped = false;

    wxPoint pos = aEvent.GetPosition();

    if( m_smithChart && !IsInsideInfoLayer( pos ) )
    {
        m_smithPanning = true;
        m_smithPanLast = pos;
        return;
    }

    // on a cursor or the legend, let mpWindow drag it, and remember that it saw the click
    // so the matching motions and release reach it too
    m_smithLeftSkipped = true;
    aEvent.Skip();
}


void SIM_VIEW::onSmithMotion( wxMouseEvent& aEvent )
{
    if( m_smithChart && aEvent.Dragging() )
    {
        if( aEvent.LeftIsDown() && !m_smithLeftSkipped )
        {
            // a left drag mpWindow did not see the start of, pan if one is active, and
            // swallow either way so mpWindow cannot rubber-band from a stale click point
            if( m_smithPanning )
            {
                wxPoint pos = aEvent.GetPosition();

                SmithPanBy( pos - m_smithPanLast );
                m_smithPanLast = pos;
            }

            return;
        }

        // keep mpWindow's middle-button pan off the hidden axes
        if( aEvent.MiddleIsDown() )
            return;
    }

    aEvent.Skip();
}


void SIM_VIEW::onSmithLeftUp( wxMouseEvent& aEvent )
{
    if( m_smithChart )
    {
        m_smithPanning = false;

        // a release mpWindow saw no click for would zoom the hidden axes to the rect
        // between its stale click point and this position
        if( !m_smithLeftSkipped )
            return;

        m_smithLeftSkipped = false;
    }

    aEvent.Skip();
}


void SIM_VIEW::onSmithDClick( wxMouseEvent& aEvent )
{
    wxPoint pos = aEvent.GetPosition();

    if( m_smithChart && !IsInsideInfoLayer( pos ) )
    {
        ResetSmithView();
        Refresh();
        return;
    }

    aEvent.Skip();
}


void SIM_VIEW::onSmithRightDown( wxMouseEvent& aEvent )
{
    // remember where the menu opened, for its zoom commands
    if( m_smithChart )
        m_smithMenuPos = aEvent.GetPosition();

    aEvent.Skip();
}


void SIM_VIEW::onSmithRightUp( wxMouseEvent& aEvent )
{
    if( !m_smithChart )
    {
        aEvent.Skip();
        return;
    }

    // no zoom history here, so grey out Undo/Redo and show the menu ourselves
    wxMenu* menu = GetPopupMenu();

    menu->Enable( mpID_ZOOM_UNDO, false );
    menu->Enable( mpID_ZOOM_REDO, false );

    PopupMenu( menu, aEvent.GetPosition() );
}


void SIM_VIEW::onSmithMenuCommand( wxCommandEvent& aEvent )
{
    if( !m_smithChart )
    {
        aEvent.Skip();
        return;
    }

    switch( aEvent.GetId() )
    {
    case mpID_ZOOM_IN: SmithZoomAt( m_smithMenuPos, 1.5 ); break;
    case mpID_ZOOM_OUT: SmithZoomAt( m_smithMenuPos, 1.0 / 1.5 ); break;

    case mpID_FIT:
        ResetSmithView();
        Refresh();
        break;

    case mpID_CENTER:
    {
        SMITH_VIEW view;

        if( getSmithView( view ) )
        {
            m_smithPan = view.ToGamma( m_smithMenuPos );
            Refresh();
        }

        break;
    }

    default: aEvent.Skip();
    }
}


SIM_PLOT_TAB::SIM_PLOT_TAB( const wxString& aSimCommand, wxWindow* parent ) :
        SIM_TAB( aSimCommand, parent ),
        m_dotted_cp( false ),
        m_syncingXView( false ),
        m_smithMode( false )
{
    m_mouseWheelActions = convertMouseWheelActions( SIM_MOUSE_WHEEL_ACTION_SET::GetMouseDefaults() );

    wxBoxSizer* outerSizer = new wxBoxSizer( wxVERTICAL );

    m_viewsSizer = new wxBoxSizer( wxVERTICAL );
    outerSizer->Add( m_viewsSizer, 1, wxEXPAND, 0 );

    wxBoxSizer* viewButtonsSizer = new wxBoxSizer( wxHORIZONTAL );

    STD_BITMAP_BUTTON* addViewButton = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap );
    addViewButton->SetBitmap( KiBitmapBundle( BITMAPS::small_plus ) );
    addViewButton->SetToolTip( _( "Add a new signal view" ) );

    STD_BITMAP_BUTTON* removeViewButton = new STD_BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap );
    removeViewButton->SetBitmap( KiBitmapBundle( BITMAPS::small_trash ) );
    removeViewButton->SetToolTip( _( "Remove the last signal view" ) );

    viewButtonsSizer->Add( addViewButton, 0, wxLEFT, 5 );
    viewButtonsSizer->Add( removeViewButton, 0, wxLEFT, 30 );

    addViewButton->Bind( wxEVT_BUTTON,
            [this]( wxCommandEvent& aEvent )
            {
                AddView();
            } );

    removeViewButton->Bind( wxEVT_BUTTON,
            [this]( wxCommandEvent& aEvent )
            {
                const std::vector<SIM_VIEW*>& views = GetViews();

                if( !views.empty() )
                    RemoveView( views.back() );
            } );

    outerSizer->Add( viewButtonsSizer, 0, wxEXPAND | wxTOP | wxBOTTOM, 3 );

    SetSizer( outerSizer );

    AddView();
}


SIM_PLOT_TAB::~SIM_PLOT_TAB()
{
    // ~mpWindow destroys all the added layers, so there is no need to destroy m_traces contents
}


void SIM_PLOT_TAB::ApplyPreferences( const SIM_PREFERENCES& aPrefs )
{
    m_mouseWheelActions = convertMouseWheelActions( aPrefs.mouse_wheel_actions );

    for( SIM_VIEW* view : m_views )
        view->SetMouseWheelActions( m_mouseWheelActions );
}


SIM_VIEW* SIM_PLOT_TAB::AddView()
{
    SIM_VIEW* view = new SIM_VIEW( this, this );

    view->LimitView( true );
    view->SetMargins( 30, 70, 45, 70 );
    view->SetMouseWheelActions( m_mouseWheelActions );

    view->updateAxes();

    // a mpInfoLegend displays the name of traces on the left top panel corner:
    mpInfoLegend* legend = new mpInfoLegend( wxRect( 0, 0, 200, 40 ), wxTRANSPARENT_BRUSH );
    legend->SetVisible( false );
    view->AddLayer( legend );
    view->m_legend = legend;
    view->m_lastLegendPosition = legend->GetPosition();

    view->EnableDoubleBuffer( true );
    view->SetSmithChart( m_smithMode );

    m_views.push_back( view );
    m_viewsSizer->Add( view, 1, wxALL | wxEXPAND, 1 );

    UpdatePlotColors();

    if( m_views.size() > 1 )
        SyncXView( m_views[0] );

    view->UpdateAll();
    Layout();

    wxQueueEvent( this, new wxCommandEvent( EVT_SIM_VIEWS_CHANGED ) );

    return view;
}


bool SIM_PLOT_TAB::RemoveView( SIM_VIEW* aView )
{
    if( m_views.size() <= 1 )
        return false;

    auto it = std::find( m_views.begin(), m_views.end(), aView );

    if( it == m_views.end() )
        return false;

    // The traces routed to this view lose their axes along with it, so drop them rather than
    // leaving them behind holding a destroyed view (and its destroyed axis layers).  Their
    // signals stay in the Signals grid, simply no longer plotted, and can be assigned to one
    // of the remaining views.
    std::vector<TRACE*> plottedHere;

    for( const auto& [name, trace] : m_traces )
    {
        trace->ClearYScaleViewIf( aView );

        if( trace->GetView() == aView )
            plottedHere.push_back( trace );
    }

    // DeleteTrace() erases from m_traces, so it cannot run while iterating it
    for( TRACE* trace : plottedHere )
        DeleteTrace( trace );

    m_viewsSizer->Detach( aView );
    m_views.erase( it );
    aView->Destroy();

    Layout();

    // Any Y-scale ranges that were merged with the removed view are now stale; recompute them
    // (and refresh) so the remaining views reflect only the traces that are still present.
    ResetScales( false );

    for( SIM_VIEW* view : m_views )
        view->UpdateAll();

    wxQueueEvent( this, new wxCommandEvent( EVT_SIM_VIEWS_CHANGED ) );

    return true;
}


int SIM_PLOT_TAB::GetViewIndex( SIM_VIEW* aView ) const
{
    for( size_t i = 0; i < m_views.size(); ++i )
    {
        if( m_views[i] == aView )
            return (int) i;
    }

    return -1;
}


void SIM_PLOT_TAB::SyncXView( SIM_VIEW* aSource )
{
    if( m_syncingXView || !aSource )
        return;

    m_syncingXView = true;

    double scaleX = aSource->GetScaleX();
    double pos = aSource->GetPosX();
    double desiredMin = aSource->GetDesiredXmin();
    double desiredMax = aSource->GetDesiredXmax();

    for( SIM_VIEW* view : m_views )
    {
        if( view == aSource )
            continue;

        view->SetScaleX( scaleX );
        view->SetXRange( pos, desiredMax, desiredMin );
        view->UpdateAll();
    }

    m_syncingXView = false;
}


wxString SIM_PLOT_TAB::GetLabelX() const
{
    SIM_VIEW* view = GetDefaultView();
    return view ? view->GetLabelX() : wxString( wxS( "" ) );
}


wxString SIM_PLOT_TAB::GetUnitsX() const
{
    SIM_VIEW* view = GetDefaultView();
    return view ? view->GetUnitsX() : wxString( wxS( "" ) );
}


wxString SIM_PLOT_TAB::GetUnitsForTrace( TRACE* aTrace ) const
{
    SIM_VIEW* view = aTrace->GetView() ? aTrace->GetView() : GetDefaultView();
    return view ? view->GetUnitsForTrace( aTrace ) : wxString( wxS( "" ) );
}


void SIM_PLOT_TAB::ShowGrid( bool aEnable )
{
    for( SIM_VIEW* view : m_views )
        view->ShowGrid( aEnable );
}


bool SIM_PLOT_TAB::IsGridShown() const
{
    SIM_VIEW* view = GetDefaultView();
    return view && view->IsGridShown();
}


void SIM_PLOT_TAB::ShowLegend( bool aEnable )
{
    for( SIM_VIEW* view : m_views )
        view->ShowLegend( aEnable );
}


bool SIM_PLOT_TAB::IsLegendShown() const
{
    SIM_VIEW* view = GetDefaultView();
    return view && view->IsLegendShown();
}


wxPoint SIM_PLOT_TAB::GetLegendPosition() const
{
    SIM_VIEW* view = GetDefaultView();
    return view ? view->GetLegendPosition() : wxPoint();
}


void SIM_PLOT_TAB::SetLegendPosition( const wxPoint& aPosition )
{
    if( SIM_VIEW* view = GetDefaultView() )
        view->SetLegendPosition( aPosition );

    m_LastLegendPosition = aPosition;
}


wxString SIM_PLOT_TAB::GetLabelY1() const
{
    SIM_VIEW* view = GetDefaultView();
    return view ? view->GetLabelY1() : wxString( wxS( "" ) );
}


wxString SIM_PLOT_TAB::GetLabelY2() const
{
    SIM_VIEW* view = GetDefaultView();
    return view ? view->GetLabelY2() : wxString( wxS( "" ) );
}


wxString SIM_PLOT_TAB::GetLabelY3() const
{
    SIM_VIEW* view = GetDefaultView();
    return view ? view->GetLabelY3() : wxString( wxS( "" ) );
}


wxString SIM_PLOT_TAB::GetUnitsY1() const
{
    SIM_VIEW* view = GetDefaultView();
    return view ? view->GetUnitsY1() : wxString( wxS( "" ) );
}


wxString SIM_PLOT_TAB::GetUnitsY2() const
{
    SIM_VIEW* view = GetDefaultView();
    return view ? view->GetUnitsY2() : wxString( wxS( "" ) );
}


wxString SIM_PLOT_TAB::GetUnitsY3() const
{
    SIM_VIEW* view = GetDefaultView();
    return view ? view->GetUnitsY3() : wxString( wxS( "" ) );
}


bool SIM_PLOT_TAB::GetY1Scale( double* aMin, double* aMax ) const
{
    SIM_VIEW* view = GetDefaultView();
    return view && view->GetY1Scale( aMin, aMax );
}


bool SIM_PLOT_TAB::GetY2Scale( double* aMin, double* aMax ) const
{
    SIM_VIEW* view = GetDefaultView();
    return view && view->GetY2Scale( aMin, aMax );
}


bool SIM_PLOT_TAB::GetY3Scale( double* aMin, double* aMax ) const
{
    SIM_VIEW* view = GetDefaultView();
    return view && view->GetY3Scale( aMin, aMax );
}


void SIM_PLOT_TAB::SetY1Scale( bool aLock, double aMin, double aMax )
{
    if( SIM_VIEW* view = GetDefaultView() )
        view->SetY1Scale( aLock, aMin, aMax );
}


void SIM_PLOT_TAB::SetY2Scale( bool aLock, double aMin, double aMax )
{
    if( SIM_VIEW* view = GetDefaultView() )
        view->SetY2Scale( aLock, aMin, aMax );
}


void SIM_PLOT_TAB::SetY3Scale( bool aLock, double aMin, double aMax )
{
    if( SIM_VIEW* view = GetDefaultView() )
        view->SetY3Scale( aLock, aMin, aMax );
}


void SIM_PLOT_TAB::EnsureThirdYAxisExists()
{
    if( SIM_VIEW* view = GetDefaultView() )
        view->EnsureThirdYAxisExists();
}


void SIM_PLOT_TAB::UpdatePlotColors()
{
    for( SIM_VIEW* view : m_views )
    {
        view->SetColourTheme( m_colors.GetPlotColor( SIM_PLOT_COLORS::COLOR_SET::BACKGROUND ),
                              m_colors.GetPlotColor( SIM_PLOT_COLORS::COLOR_SET::FOREGROUND ),
                              m_colors.GetPlotColor( SIM_PLOT_COLORS::COLOR_SET::AXIS ) );

        view->UpdateSmithGridColor();
        view->UpdateAll();
    }
}


void SIM_PLOT_TAB::SetSmithMode( bool aEnable )
{
    // only S-parameter tabs have a Smith view, a stale workbook cannot force one elsewhere
    if( aEnable && GetSimType() != ST_SP )
        return;

    if( m_smithMode == aEnable )
        return;

    m_smithMode = aEnable;

    for( SIM_VIEW* view : m_views )
        view->SetSmithChart( aEnable );
}


void SIM_PLOT_TAB::UpdateSmithReferenceImpedance()
{
    for( SIM_VIEW* view : m_views )
        view->UpdateSmithReferenceImpedance();
}


double SIM_PLOT_TAB::GetSmithZoom() const
{
    SIM_VIEW* view = GetDefaultView();
    return view ? view->GetSmithZoom() : 1.0;
}


wxRealPoint SIM_PLOT_TAB::GetSmithPan() const
{
    SIM_VIEW* view = GetDefaultView();
    return view ? view->GetSmithPan() : wxRealPoint( 0.0, 0.0 );
}


void SIM_PLOT_TAB::ResetSmithView()
{
    for( SIM_VIEW* view : m_views )
    {
        view->ResetSmithView();
        view->Refresh();
    }
}


void SIM_PLOT_TAB::SetSmithView( double aZoom, double aPanX, double aPanY )
{
    for( SIM_VIEW* view : m_views )
        view->SetSmithView( aZoom, aPanX, aPanY );
}


void SIM_PLOT_TAB::SmithZoomAt( const wxPoint& aPos, double aFactor )
{
    for( SIM_VIEW* view : m_views )
        view->SmithZoomAt( aPos, aFactor );
}


void SIM_PLOT_TAB::SmithPanBy( const wxPoint& aDelta )
{
    for( SIM_VIEW* view : m_views )
        view->SmithPanBy( aDelta );
}


void SIM_PLOT_TAB::OnLanguageChanged()
{
    for( SIM_VIEW* view : m_views )
    {
        view->updateAxes();
        view->UpdateAll();
    }
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
    m_sessionTraceColors[trace->GetName()] = trace->GetTraceColour();
}


TRACE* SIM_PLOT_TAB::GetOrAddTrace( const wxString& aVectorName, int aType, SIM_VIEW* aView )
{
    TRACE* trace = GetTrace( aVectorName, aType );

    if( !trace && aView )
    {
        aView->updateAxes( aType );

        if( GetSimType() == ST_TRAN || GetSimType() == ST_DC )
        {
            bool hasVoltageTraces = false;

            for( const auto& [id, candidate] : m_traces )
            {
                if( candidate->GetView() == aView && ( candidate->GetType() & SPT_VOLTAGE ) )
                {
                    hasVoltageTraces = true;
                    break;
                }
            }

            if( !hasVoltageTraces )
            {
                if( aView->m_axis_y2 )
                    aView->m_axis_y2->SetMasterScale( nullptr );

                if( aView->m_axis_y3 )
                    aView->m_axis_y3->SetMasterScale( nullptr );
            }
        }

        if( aType & SPT_SP_SMITH )
            trace = new SMITH_TRACE( aVectorName, (SIM_TRACE_TYPE) aType );
        else
            trace = new TRACE( aVectorName, (SIM_TRACE_TYPE) aType );

        if( m_sessionTraceColors.count( aVectorName ) )
            trace->SetTraceColour( m_sessionTraceColors[aVectorName] );
        else
            trace->SetTraceColour( m_colors.GenerateColor( m_sessionTraceColors ) );

        UpdateTraceStyle( trace );
        m_traces[getTraceId( aVectorName, aType )] = trace;

        trace->SetView( aView );
        aView->AddLayer( (mpLayer*) trace );
    }

    return trace;
}


void SIM_PLOT_TAB::SetTraceData( TRACE* trace, std::vector<double>& aX, std::vector<double>& aY, int aSweepCount,
                                 size_t aSweepSize, bool aIsMultiRun, const std::vector<wxString>& aMultiRunLabels )
{
    SIM_VIEW* view = trace->GetView();

    wxCHECK( view, /* void */ );

    // smith traces carry Re/Im of the reflection coefficient, not frequency
    bool smithTrace = ( trace->GetType() & SPT_SP_SMITH ) > 0;

    if( dynamic_cast<LOG_SCALE<mpScaleXLog>*>( view->m_axis_x ) && !smithTrace )
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
                pt = pt * 180.0 / M_PI; // convert to degrees
        }
        else
        {
            for( double& pt : aY )
                pt = MagnitudeToDb( pt ); // NaN where there is no signal
        }
    }

    trace->SetData( aX, aY );
    trace->SetSweepCount( aSweepCount );
    trace->SetSweepSize( aSweepSize );
    trace->SetIsMultiRun( aIsMultiRun );
    trace->SetMultiRunLabels( aMultiRunLabels );

    // Phase and currents on second Y axis, except for AC currents, those use the same axis as voltage
    if( smithTrace )
    {
        // drawn through the chart geometry, not the axis transforms
        trace->SetScale( nullptr, nullptr );
    }
    else if( ( trace->GetType() & SPT_AC_PHASE )
             || ( ( GetSimType() != ST_AC ) && ( trace->GetType() & SPT_CURRENT ) ) )
    {
        trace->SetScale( view->m_axis_x, view->m_axis_y2 );
    }
    else if( trace->GetType() & SPT_POWER )
    {
        trace->SetScale( view->m_axis_x, view->m_axis_y3 );
    }
    else
    {
        trace->SetScale( view->m_axis_x, view->m_axis_y1 );
    }

    for( auto& [cursorId, cursor] : trace->GetCursors() )
    {
        if( cursor )
            cursor->UpdateForNewData();
    }

    view->UpdateAxisVisibility();
}


void SIM_PLOT_TAB::DeleteTrace( TRACE* aTrace )
{
    for( const auto& [name, trace] : m_traces )
    {
        if( trace == aTrace )
        {
            m_traces.erase( name );
            break;
        }
    }

    if( SIM_VIEW* view = aTrace->GetView() )
    {
        for( const auto& [id, cursor] : aTrace->GetCursors() )
        {
            if( cursor )
                view->DelLayer( cursor, true );
        }

        view->DelLayer( aTrace, true, true );
        view->UpdateAxisVisibility();
        view->UpdateSmithReferenceImpedance();
    }

    ResetScales( false );
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
    SIM_VIEW* view = aTrace->GetView();

    wxCHECK( view, /* void */ );

    CURSOR* cursor;

    if( aTrace->GetType() & SPT_SP_SMITH )
    {
        SMITH_TRACE* smithTrace = static_cast<SMITH_TRACE*>( aTrace );

        cursor = new SMITH_CURSOR( smithTrace, this );

        // start somewhere on the locus, biased per cursor id like the rectangular case
        const std::vector<double>& freqs = smithTrace->GetFrequencies();

        if( !freqs.empty() )
            cursor->SetCoordX( freqs[freqs.size() * ( aCursorId == 1 ? 2 : 3 ) / 5] );
    }
    else
    {
        int width = view->GetXScreen() - view->GetMarginLeft() - view->GetMarginRight();
        int center = view->GetMarginLeft() + KiROUND( width * ( aCursorId == 1 ? 0.4 : 0.6 ) );

        cursor = new CURSOR( aTrace, this );

        cursor->SetX( center );
    }

    cursor->SetName( aSignalName );

    aTrace->SetCursor( aCursorId, cursor );
    view->AddLayer( cursor );

    // Notify the parent window about the changes
    wxQueueEvent( this, new wxCommandEvent( EVT_SIM_CURSOR_UPDATE ) );
}


void SIM_PLOT_TAB::DisableCursor( TRACE* aTrace, int aCursorId )
{
    if( CURSOR* cursor = aTrace->GetCursor( aCursorId ) )
    {
        aTrace->SetCursor( aCursorId, nullptr );

        if( SIM_VIEW* view = aTrace->GetView() )
            view->DelLayer( cursor, true );

        // Notify the parent window about the changes
        wxQueueEvent( this, new wxCommandEvent( EVT_SIM_CURSOR_UPDATE ) );
    }
}


void SIM_PLOT_TAB::ResetScales( bool aIncludeX )
{
    for( SIM_VIEW* view : m_views )
        view->ResetScales( aIncludeX );

    // Merge Y-axis auto-fit ranges for traces whose "Y Scale" has been explicitly linked to
    // another view, so that the linked axes end up sharing the same numeric range -- as if all
    // the involved traces were plotted together in a single view.
    std::map<std::pair<SIM_VIEW*, int>, std::vector<TRACE*>> groups;

    for( const auto& [name, trace] : m_traces )
    {
        if( SIM_VIEW* view = trace->GetView() )
            groups[{ trace->GetYScaleView(), view->GetAxisSlot( trace ) }].push_back( trace );
    }

    for( const auto& [key, traces] : groups )
    {
        std::vector<SIM_VIEW*> actualViews;

        for( TRACE* trace : traces )
        {
            SIM_VIEW* view = trace->GetView();

            if( std::find( actualViews.begin(), actualViews.end(), view ) == actualViews.end() )
                actualViews.push_back( view );
        }

        if( actualViews.size() <= 1 )
            continue; // All traces already share the same, single axis object.

        bool   haveRange = false;
        double minV = 0.0;
        double maxV = 0.0;

        for( TRACE* trace : traces )
        {
            double lo = trace->GetMinY();
            double hi = trace->GetMaxY();

            if( !haveRange )
            {
                minV = lo;
                maxV = hi;
                haveRange = true;
            }
            else
            {
                minV = std::min( minV, lo );
                maxV = std::max( maxV, hi );
            }
        }

        if( !haveRange )
            continue;

        for( SIM_VIEW* view : actualViews )
        {
            if( mpScaleY* axis = view->GetAxisBySlot( key.second ) )
                axis->SetDataRange( minV, maxV );
        }
    }
}


mpWindow* SIM_PLOT_TAB::GetPlotWin() const
{
    return GetDefaultView();
}


wxDEFINE_EVENT( EVT_SIM_CURSOR_UPDATE, wxCommandEvent );
wxDEFINE_EVENT( EVT_SIM_VIEWS_CHANGED, wxCommandEvent );
