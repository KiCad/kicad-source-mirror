/////////////////////////////////////////////////////////////////////////////
// Name:            mathplot.cpp
// Purpose:         Framework for plotting in wxWindows
// Original Author: David Schalig
// Maintainer:      Davide Rondini
// Contributors:    Jose Luis Blanco, Val Greene, Maciej Suminski, Tomasz Wlostowski
// Created:         21/07/2003
// Last edit:       2024
// Copyright:       (c) David Schalig, Davide Rondini
// Copyright        (c) 2021-2024 KiCad Developers, see AUTHORS.txt for contributors.
// Licence:         wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#include <wx/window.h>

// Comment out for release operation:
// (Added by J.L.Blanco, Aug 2007)
//#define MATHPLOT_DO_LOGGING

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include "wx/object.h"
#include "wx/font.h"
#include "wx/colour.h"
#include "wx/sizer.h"
#include "wx/intl.h"
#include "wx/dcclient.h"
#include "wx/cursor.h"
#include "gal/cursors.h"
#endif

#include <widgets/mathplot.h>
#include <wx/graphics.h>
#include <wx/image.h>

#include <cmath>
#include <cstdio>   // used only for debug
#include <ctime>    // used for representation of x axes involving date
#include <set>

// Memory leak debugging
#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// Legend margins
#define mpLEGEND_MARGIN     5
#define mpLEGEND_LINEWIDTH  10

// See doxygen comments.
double mpWindow::zoomIncrementalFactor = 1.1;

// -----------------------------------------------------------------------------
// mpLayer
// -----------------------------------------------------------------------------

IMPLEMENT_ABSTRACT_CLASS( mpLayer, wxObject )

mpLayer::mpLayer() :
        m_type( mpLAYER_UNDEF )
{
    SetPen( (wxPen&) *wxBLACK_PEN );
    SetFont( (wxFont&) *wxNORMAL_FONT );
    m_continuous = false;   // Default
    m_showName = true;      // Default
    m_visible = true;
}


// -----------------------------------------------------------------------------
// mpInfoLayer
// -----------------------------------------------------------------------------
IMPLEMENT_DYNAMIC_CLASS( mpInfoLayer, mpLayer )

mpInfoLayer::mpInfoLayer()
{
    m_dim = wxRect( 0, 0, 1, 1 );
    m_brush = *wxTRANSPARENT_BRUSH;
    m_reference.x = 0; m_reference.y = 0;
    m_winX  = 1;    // parent->GetScrX();
    m_winY  = 1;    // parent->GetScrY();
    m_type  = mpLAYER_INFO;
}


mpInfoLayer::mpInfoLayer( wxRect rect, const wxBrush* brush ) :
        m_dim( rect )
{
    m_brush = *brush;
    m_reference.x   = rect.x;
    m_reference.y   = rect.y;
    m_winX  = 1;    // parent->GetScrX();
    m_winY  = 1;    // parent->GetScrY();
    m_type  = mpLAYER_INFO;
}


mpInfoLayer::~mpInfoLayer()
{
}


bool mpInfoLayer::Inside( const wxPoint& point ) const
{
    return m_dim.Contains( point );
}


bool mpInfoLayer::OnDoubleClick( const wxPoint& point, mpWindow& w )
{
    return false;
}


void mpInfoLayer::Move( wxPoint delta )
{
    m_dim.SetX( m_reference.x + delta.x );
    m_dim.SetY( m_reference.y + delta.y );
}


void mpInfoLayer::UpdateReference()
{
    m_reference.x   = m_dim.x;
    m_reference.y   = m_dim.y;
}


void mpInfoLayer::Plot( wxDC& dc, mpWindow& w )
{
    if( m_visible )
    {
        // Adjust relative position inside the window
        int scrx    = w.GetScrX();
        int scry    = w.GetScrY();

        // Avoid dividing by 0
        if( scrx == 0 )
            scrx = 1;

        if( scry == 0 )
            scry = 1;

        if( ( m_winX != scrx ) || ( m_winY != scry ) )
        {
            if( m_winX > 1 )
                m_dim.x = (int) floor( (double) ( m_dim.x * scrx / m_winX ) );

            if( m_winY > 1 )
            {
                m_dim.y = (int) floor( (double) ( m_dim.y * scry / m_winY ) );
                UpdateReference();
            }

            // Finally update window size
            m_winX  = scrx;
            m_winY  = scry;
        }

        dc.SetPen( m_pen );
        dc.SetBrush( m_brush );
        dc.DrawRectangle( m_dim.x, m_dim.y, m_dim.width, m_dim.height );
    }
}


wxPoint mpInfoLayer::GetPosition() const
{
    return m_dim.GetPosition();
}


wxSize mpInfoLayer::GetSize() const
{
    return m_dim.GetSize();
}


mpInfoLegend::mpInfoLegend() :
        mpInfoLayer()
{
}


mpInfoLegend::mpInfoLegend( wxRect rect, const wxBrush* brush ) :
        mpInfoLayer( rect, brush )
{
}


mpInfoLegend::~mpInfoLegend()
{
}


void mpInfoLegend::Plot( wxDC& dc, mpWindow& w )
{
    if( m_visible )
    {
        // Adjust relative position inside the window
        int scrx    = w.GetScrX();
        int scry    = w.GetScrY();

        if( m_winX != scrx || m_winY != scry )
        {
            if( m_winX > 1 )
                m_dim.x = (int) floor( (double) ( m_dim.x * scrx / m_winX ) );

            if( m_winY > 1 )
            {
                m_dim.y = (int) floor( (double) ( m_dim.y * scry / m_winY ) );
                UpdateReference();
            }

            // Finally update window size
            m_winX  = scrx;
            m_winY  = scry;
        }

        dc.SetBrush( m_brush );
        dc.SetFont( m_font );

        const int baseWidth = mpLEGEND_MARGIN * 2 + mpLEGEND_LINEWIDTH;
        int       textX = baseWidth, textY = mpLEGEND_MARGIN;
        int       plotCount = 0;
        int       posY = 0;
        int       tmpX = 0;
        int       tmpY = 0;
        mpLayer*  layer = nullptr;
        wxPen     lpen;
        wxString  label;

        for( unsigned int p = 0; p < w.CountAllLayers(); p++ )
        {
            layer = w.GetLayer( p );

            if( layer->GetLayerType() == mpLAYER_PLOT && layer->IsVisible() )
            {
                label = layer->GetDisplayName();
                dc.GetTextExtent( label, &tmpX, &tmpY );
                textX = ( textX > tmpX + baseWidth ) ? textX : tmpX + baseWidth + mpLEGEND_MARGIN;
                textY += tmpY;
            }
        }

        dc.SetPen( m_pen );
        dc.SetBrush( m_brush );
        m_dim.width = textX;

        if( textY != mpLEGEND_MARGIN )    // Don't draw any thing if there are no visible layers
        {
            textY += mpLEGEND_MARGIN;
            m_dim.height = textY;
            dc.DrawRectangle( m_dim.x, m_dim.y, m_dim.width, m_dim.height );

            for( unsigned int p2 = 0; p2 < w.CountAllLayers(); p2++ )
            {
                layer = w.GetLayer( p2 );

                if( layer->GetLayerType() == mpLAYER_PLOT && layer->IsVisible() )
                {
                    label   = layer->GetDisplayName();
                    lpen    = layer->GetPen();
                    dc.GetTextExtent( label, &tmpX, &tmpY );
                    dc.SetPen( lpen );
                    posY = m_dim.y + mpLEGEND_MARGIN + plotCount * tmpY + (tmpY >> 1);
                    dc.DrawLine( m_dim.x + mpLEGEND_MARGIN,                      // X start coord
                                 posY,                                           // Y start coord
                                 m_dim.x + mpLEGEND_LINEWIDTH + mpLEGEND_MARGIN, // X end coord
                                 posY );
                    dc.DrawText( label,
                                 m_dim.x + baseWidth,
                                 m_dim.y + mpLEGEND_MARGIN + plotCount * tmpY );
                    plotCount++;
                }
            }
        }
    }
}


// -----------------------------------------------------------------------------
// mpLayer implementations - functions
// -----------------------------------------------------------------------------

IMPLEMENT_ABSTRACT_CLASS( mpFX, mpLayer )

mpFX::mpFX( const wxString& name, int flags )
{
    SetName( name );
    m_flags = flags;
    m_type  = mpLAYER_PLOT;
}


void mpFX::Plot( wxDC& dc, mpWindow& w )
{
    if( m_visible )
    {
        dc.SetPen( m_pen );

        wxCoord startPx = w.GetMarginLeft();
        wxCoord endPx   = w.GetScrX() - w.GetMarginRight();
        wxCoord minYpx  = w.GetMarginTop();
        wxCoord maxYpx  = w.GetScrY() - w.GetMarginBottom();

        wxCoord iy = 0;

        if( m_pen.GetWidth() <= 1 )
        {
            for( wxCoord i = startPx; i < endPx; ++i )
            {
                iy = w.y2p( GetY( w.p2x( i ) ) );

                // Draw the point only if you can draw outside margins or if the point is
                // inside margins
                if( ( iy >= minYpx ) && ( iy <= maxYpx ) )
                    dc.DrawPoint( i, iy );
            }
        }
        else
        {
            for( wxCoord i = startPx; i < endPx; ++i )
            {
                iy = w.y2p( GetY( w.p2x( i ) ) );

                // Draw the point only if you can draw outside margins or if the point is
                // inside margins
                if( iy >= minYpx && iy <= maxYpx )
                    dc.DrawLine( i, iy, i, iy );
            }
        }

        if( !m_name.IsEmpty() && m_showName )
        {
            dc.SetFont( m_font );

            wxCoord tx, ty;
            dc.GetTextExtent( m_name, &tx, &ty );

            if( ( m_flags & mpALIGNMASK ) == mpALIGN_RIGHT )
                tx = ( w.GetScrX() - tx ) - w.GetMarginRight() - 8;
            else if( ( m_flags & mpALIGNMASK ) == mpALIGN_CENTER )
                tx = ( ( w.GetScrX() - w.GetMarginRight() - w.GetMarginLeft() - tx ) / 2 )
                     + w.GetMarginLeft();
            else
                tx = w.GetMarginLeft() + 8;

            dc.DrawText( m_name, tx, w.y2p( GetY( w.p2x( tx ) ) ) );
        }
    }
}


IMPLEMENT_ABSTRACT_CLASS( mpFY, mpLayer )


mpFY::mpFY( const wxString& name, int flags )
{
    SetName( name );
    m_flags = flags;
    m_type  = mpLAYER_PLOT;
}


void mpFY::Plot( wxDC& dc, mpWindow& w )
{
    if( m_visible )
    {
        dc.SetPen( m_pen );

        wxCoord i, ix;

        wxCoord startPx = w.GetMarginLeft();
        wxCoord endPx   = w.GetScrX() - w.GetMarginRight();
        wxCoord minYpx  = w.GetMarginTop();
        wxCoord maxYpx  = w.GetScrY() - w.GetMarginBottom();

        if( m_pen.GetWidth() <= 1 )
        {
            for( i = minYpx; i < maxYpx; ++i )
            {
                ix = w.x2p( GetX( w.p2y( i ) ) );

                if( ( ix >= startPx ) && ( ix <= endPx ) )
                    dc.DrawPoint( ix, i );
            }
        }
        else
        {
            for( i = 0; i< w.GetScrY(); ++i )
            {
                ix = w.x2p( GetX( w.p2y( i ) ) );

                if( ( ix >= startPx ) && ( ix <= endPx ) )
                    dc.DrawLine( ix, i, ix, i );
            }
        }

        if( !m_name.IsEmpty() && m_showName )
        {
            dc.SetFont( m_font );

            wxCoord tx, ty;
            dc.GetTextExtent( m_name, &tx, &ty );

            if( ( m_flags & mpALIGNMASK ) == mpALIGN_TOP )
                ty = w.GetMarginTop() + 8;
            else if( ( m_flags & mpALIGNMASK ) == mpALIGN_CENTER )
                ty = ( ( w.GetScrY() - w.GetMarginTop() - w.GetMarginBottom() - ty ) / 2 )
                     + w.GetMarginTop();
            else
                ty = w.GetScrY() - 8 - ty - w.GetMarginBottom();

            dc.DrawText( m_name, w.x2p( GetX( w.p2y( ty ) ) ), ty );
        }
    }
}


IMPLEMENT_ABSTRACT_CLASS( mpFXY, mpLayer )


mpFXY::mpFXY( const wxString& name, int flags )
{
    SetName( name );
    m_flags     = flags;
    m_type      = mpLAYER_PLOT;
    m_scaleX    = nullptr;
    m_scaleY    = nullptr;

    // Avoid not initialized members:
    maxDrawX = minDrawX = maxDrawY = minDrawY = 0;
}


void mpFXY::UpdateViewBoundary( wxCoord xnew, wxCoord ynew )
{
    // Keep track of how many points have been drawn and the bounding box
    maxDrawX    = (xnew > maxDrawX) ? xnew : maxDrawX;
    minDrawX    = (xnew < minDrawX) ? xnew : minDrawX;
    maxDrawY    = (maxDrawY > ynew) ? maxDrawY : ynew;
    minDrawY    = (minDrawY < ynew) ? minDrawY : ynew;
    // drawnPoints++;
}


void mpFXY::Plot( wxDC& dc, mpWindow& w )
{
    // If trace doesn't have any data yet then it won't have any scale set.  In any case, there's
    // nothing to plot.
    if( !GetCount() )
        return;

    wxCHECK_RET( m_scaleX, wxS( "X scale was not set" ) );
    wxCHECK_RET( m_scaleY, wxS( "Y scale was not set" ) );

    if( !m_visible )
        return;

    wxCoord startPx = w.GetMarginLeft();
    wxCoord endPx   = w.GetScrX() - w.GetMarginRight();
    wxCoord minYpx  = w.GetMarginTop();
    wxCoord maxYpx  = w.GetScrY() - w.GetMarginBottom();

    // Check for a collapsed window before we try to allocate a negative number of points
    if( endPx <= startPx || minYpx >= maxYpx )
        return;

    dc.SetPen( m_pen );

    double x, y;
    // Do this to reset the counters to evaluate bounding box for label positioning
    Rewind();
    GetNextXY( x, y );
    maxDrawX = x;
    minDrawX = x;
    maxDrawY = y;
    minDrawY = y;
    // drawnPoints = 0;
    Rewind();

    dc.SetClippingRegion( startPx, minYpx, endPx - startPx + 1, maxYpx - minYpx + 1 );

    if( !m_continuous )
    {
        bool first = true;
        wxCoord ix = 0;
        std::set<wxCoord> ys;

        while( GetNextXY( x, y ) )
        {
            double px = m_scaleX->TransformToPlot( x );
            double py = m_scaleY->TransformToPlot( y );
            wxCoord newX = w.x2p( px );

            if( first )
            {
                ix = newX;
                first = false;
            }

            if( newX == ix )    // continue until a new X coordinate is reached
            {
                // collect all unique points
                ys.insert( w.y2p( py ) );
                continue;
            }

            for( auto& iy: ys )
            {
                if( ( ix >= startPx ) && ( ix <= endPx ) && ( iy >= minYpx ) && ( iy <= maxYpx ) )
                {
                    // for some reason DrawPoint does not use the current pen, so we use
                    // DrawLine for fat pens
                    if( m_pen.GetWidth() <= 1 )
                        dc.DrawPoint( ix, iy );
                    else
                        dc.DrawLine( ix, iy, ix, iy );

                    UpdateViewBoundary( ix, iy );
                }
            }

            ys.clear();
            ix = newX;
            ys.insert( w.y2p( py ) );
        }
    }
    else for( int sweep = 0; sweep < GetSweepCount(); ++sweep )
    {
        SetSweepWindow( sweep );

        int     count = 0;
        int     x0 = 0;         // X position of merged current vertical line
        int     ymin0 = 0;      // y min coord of merged current vertical line
        int     ymax0 = 0;      // y max coord of merged current vertical line
        int     dupx0 = 0;      // count of currently merged vertical lines
        wxPoint line_start;     // starting point of the current line to draw

        // A buffer to store coordinates of lines to draw
        std::vector<wxPoint>pointList;
        pointList.reserve( ( endPx - startPx ) * 2 );

        double nextX;
        double nextY;
        bool hasNext = GetNextXY( nextX, nextY );
        bool offRight = false;

        // Note: we can use dc.DrawLines() only for a reasonable number or points (<10,000),
        // because at least on Windows dc.DrawLines() can hang for a lot of points.  Note that
        // this includes the intermediate points when drawing dotted lines.

        // Our first-pass optimization is to exclude points outside the view, and aggregate all
        // contiguous y values found at a single x value into a vertical line.
        while( hasNext )
        {
            x = nextX;
            y = nextY;
            hasNext = GetNextXY( nextX, nextY );

            double px = m_scaleX->TransformToPlot( x );
            double py = m_scaleY->TransformToPlot( y );

            wxCoord x1 = w.x2p( px );
            wxCoord y1 = w.y2p( py );

            // Note that we can't start *right* at the edge of the view because we need to
            // interpolate between two points, one of which might be outside the view.
            // Note: x1 is a value truncated from px by w.x2p(). So to be sure the first point
            // is drawn, the x1 low limit is startPx-1 in plot coordinates
            if( x1 < startPx-1 )
            {
                wxCoord nextX1 = w.x2p( m_scaleX->TransformToPlot( nextX ) );

                if( nextX1 < startPx-1 )
                    continue;
            }
            else if( x1 > endPx )
            {
                if( offRight )
                    continue;
                else
                    offRight = true;
            }

            if( !count || line_start.x != x1 )
            {
                // We've aggregated a bunch of y values with a shared x value, so we need to draw
                // a vertical line.  However, short vertical segments spoil anti-aliasing on
                // Retina displays, so only draw them if they're "significant" (the user should
                // zoom in if they need a more accurate picture).
                if( count && dupx0 > 1 && abs( ymax0 - ymin0 ) > 2 )
                    dc.DrawLine( x0, ymin0, x0, ymax0 );

                x0 = x1;
                ymin0 = ymax0 = y1;
                dupx0 = 0;

                pointList.emplace_back( wxPoint( x1, y1 ) );

                line_start.x = x1;
                line_start.y = y1;
                count++;
            }
            else
            {
                ymin0 = std::min( ymin0, y1 );
                ymax0 = std::max( ymax0, y1 );
                x0 = x1;
                dupx0++;
            }
        }

        if( pointList.size() > 1 )
        {
            // Second pass optimization is to merge horizontal segments.  This improves the look
            // of dotted lines, keeps the point count down, and it's easy.
            //
            // This pass also includes a final protection to keep MSW from hanging by chunking to
            // a size it can handle.
            std::vector<wxPoint> drawPoints;
            drawPoints.reserve( ( endPx - startPx ) * 2 );

#ifdef __WXMSW__
            int chunkSize = 10000;
#else
            int chunkSize = 100000;
#endif
            wxPenStyle penStyle = dc.GetPen().GetStyle();
            bool isSolidPen = ( penStyle == wxPENSTYLE_SOLID
                                || penStyle == wxPENSTYLE_TRANSPARENT );

            if( !isSolidPen )
                chunkSize /= 500;

            drawPoints.push_back( pointList[0] );   // push the first point in list

            for( size_t ii = 1; ii < pointList.size()-1; ii++ )
            {
                // Skip intermediate points between the first point and the last point of the
                // segment candidate. This optimization merges horizontal line segments, which
                // breaks non-solid pen styles by altering segment lengths.
                if( isSolidPen
                    && drawPoints.back().y == pointList[ii].y
                    && drawPoints.back().y == pointList[ii+1].y )
                {
                    continue;
                }
                else
                {
                    drawPoints.push_back( pointList[ii] );

                    if( (int) drawPoints.size() > chunkSize )
                    {
                        dc.DrawLines( (int) drawPoints.size(), &drawPoints[0] );
                        drawPoints.clear();

                        // Restart the line with the current point
                        drawPoints.push_back( pointList[ii] );
                    }
                }
            }

            // push the last point to draw in list
            if( drawPoints.back() != pointList.back() )
                drawPoints.push_back( pointList.back() );

            dc.DrawLines( (int) drawPoints.size(), &drawPoints[0] );
        }
    }

    if( !m_name.IsEmpty() && m_showName )
    {
        dc.SetFont( m_font );

        wxCoord tx, ty;
        dc.GetTextExtent( m_name, &tx, &ty );

        if( ( m_flags & mpALIGNMASK ) == mpALIGN_NW )
        {
            tx  = minDrawX + 8;
            ty  = maxDrawY + 8;
        }
        else if( ( m_flags & mpALIGNMASK ) == mpALIGN_NE )
        {
            tx  = maxDrawX - tx - 8;
            ty  = maxDrawY + 8;
        }
        else if( ( m_flags & mpALIGNMASK ) == mpALIGN_SE )
        {
            tx  = maxDrawX - tx - 8;
            ty  = minDrawY - ty - 8;
        }
        else
        {
            // mpALIGN_SW
            tx  = minDrawX + 8;
            ty  = minDrawY - ty - 8;
        }

        dc.DrawText( m_name, tx, ty );
    }

    dc.DestroyClippingRegion();
}


// -----------------------------------------------------------------------------
// mpLayer implementations - furniture (scales, ...)
// -----------------------------------------------------------------------------

#define mpLN10 2.3025850929940456840179914546844

void mpScaleX::recalculateTicks( wxDC& dc, mpWindow& w )
{
    double minV, maxV, minVvis, maxVvis;

    GetDataRange( minV, maxV );
    getVisibleDataRange( w, minVvis, maxVvis );

    m_absVisibleMaxV = std::max( std::abs( minVvis ), std::abs( maxVvis ) );

    m_tickValues.clear();
    m_tickLabels.clear();

    double  minErr = 1000000000000.0;
    double  bestStep = 1.0;
    int     m_scrX = w.GetXScreen();

    for( int i = 10; i <= 20; i += 2 )
    {
        double  curr_step    = fabs( maxVvis - minVvis ) / (double) i;
        double  base    = pow( 10, floor( log10( curr_step ) ) );
        double  stepInt = floor( curr_step / base ) * base;
        double  err = fabs( curr_step - stepInt );

        if( err < minErr )
        {
            minErr = err;
            bestStep = stepInt;
        }
    }

    double numberSteps = floor( ( maxVvis - minVvis ) / bestStep );

    // Half the number of ticks according to window size.
    // The value 96 is used to have only 4 ticks when m_scrX is 268.
    // For each 96 device context units, is possible to add a new tick.
    while( numberSteps - 2.0 >= m_scrX/96.0 )
    {
        bestStep *= 2;
        numberSteps = floor( ( maxVvis - minVvis ) / bestStep );
    }

    double v = floor( minVvis / bestStep ) * bestStep;
    double zeroOffset = 100000000.0;

    while( v < maxVvis )
    {
        m_tickValues.push_back( v );

        if( fabs( v ) < zeroOffset )
            zeroOffset = fabs( v );

        v += bestStep;
    }

    if( zeroOffset <= bestStep )
    {
        for( double& t : m_tickValues )
            t -= zeroOffset;
    }

    for( double t : m_tickValues )
        m_tickLabels.emplace_back( t );

    updateTickLabels( dc, w );
}


mpScaleBase::mpScaleBase()
{
    m_rangeSet = false;
    m_axisLocked = false;
    m_axisMin = 0;
    m_axisMax = 0;
    m_nameFlags = mpALIGN_BORDER_BOTTOM;

    // initialize these members mainly to avoid not initialized values
    m_offset = 0.0;
    m_scale = 1.0;
    m_absVisibleMaxV = 0.0;
    m_flags = 0;            // Flag for axis alignment
    m_ticks = true;         // Flag to toggle between ticks or grid
    m_minV = 0.0;
    m_maxV = 0.0;
    m_maxLabelHeight = 1;
    m_maxLabelWidth = 1;
}


void mpScaleBase::computeLabelExtents( wxDC& dc, mpWindow& w )
{
    m_maxLabelHeight    = 0;
    m_maxLabelWidth     = 0;

    for( const TICK_LABEL& tickLabel : m_tickLabels )
    {
        int            tx, ty;
        const wxString s = tickLabel.label;

        dc.GetTextExtent( s, &tx, &ty );
        m_maxLabelHeight = std::max( ty, m_maxLabelHeight );
        m_maxLabelWidth  = std::max( tx, m_maxLabelWidth );
    }
}


void mpScaleBase::updateTickLabels( wxDC& dc, mpWindow& w )
{
    formatLabels();
    computeLabelExtents( dc, w );
}


void mpScaleY::getVisibleDataRange( mpWindow& w, double& minV, double& maxV )
{
    wxCoord minYpx  = w.GetMarginTop();
    wxCoord maxYpx  = w.GetScrY() - w.GetMarginBottom();

    double  pymin   = w.p2y( minYpx );
    double  pymax   = w.p2y( maxYpx );

    minV    = TransformFromPlot( pymax );
    maxV    = TransformFromPlot( pymin );
}


void mpScaleY::computeSlaveTicks( mpWindow& w )
{
    // No need for slave ticks when there aren't 2 main ticks for them to go between
    if( m_masterScale->m_tickValues.size() < 2 )
        return;

    m_tickValues.clear();
    m_tickLabels.clear();

    double  p0 = m_masterScale->TransformToPlot( m_masterScale->m_tickValues[0] );
    double  p1 = m_masterScale->TransformToPlot( m_masterScale->m_tickValues[1] );

    m_scale = 1.0 / ( m_maxV - m_minV );
    m_offset = -m_minV;

    double y_slave0 = p0 / m_scale;
    double y_slave1 = p1 / m_scale;

    double  dy_slave = ( y_slave1 - y_slave0 );
    double  exponent = floor( log10( dy_slave ) );
    double  base = dy_slave / pow( 10.0, exponent );

    double dy_scaled = ceil( 2.0 * base ) / 2.0 * pow( 10.0, exponent );

    double minvv, maxvv;

    getVisibleDataRange( w, minvv, maxvv );

    minvv = floor( minvv / dy_scaled ) * dy_scaled;

    m_scale = 1.0 / ( m_maxV - m_minV );
    m_scale *= dy_slave / dy_scaled;

    m_offset = p0 / m_scale - minvv;

    m_tickValues.clear();

    m_absVisibleMaxV = 0;

    for( double tickValue : m_masterScale->m_tickValues )
    {
        double m = TransformFromPlot( m_masterScale->TransformToPlot( tickValue ) );
        m_tickValues.push_back( m );
        m_tickLabels.emplace_back( m );
        m_absVisibleMaxV = std::max( m_absVisibleMaxV, fabs( m ) );
    }
}


void mpScaleY::recalculateTicks( wxDC& dc, mpWindow& w )
{
    double minVvis, maxVvis;

    if( m_axisLocked )
    {
        minVvis = m_axisMin;
        maxVvis = m_axisMax;
        m_offset = -m_axisMin;
        m_scale = 1.0 / ( m_axisMax - m_axisMin );
    }
    else if( m_masterScale )
    {
        computeSlaveTicks( w );
        updateTickLabels( dc, w );

        return;
    }
    else
    {
        getVisibleDataRange( w, minVvis, maxVvis );
    }

    m_absVisibleMaxV = std::max( std::abs( minVvis ), std::abs( maxVvis ) );
    m_tickValues.clear();
    m_tickLabels.clear();

    double  minErr = 1000000000000.0;
    double  bestStep = 1.0;
    int     m_scrY = w.GetYScreen();

    for( int i = 10; i <= 20; i += 2 )
    {
        double  curr_step = fabs( maxVvis - minVvis ) / (double) i;
        double  base = pow( 10, floor( log10( curr_step ) ) );
        double  stepInt = floor( curr_step / base ) * base;
        double  err = fabs( curr_step - stepInt );

        if( err< minErr )
        {
            minErr = err;
            bestStep = stepInt;
        }
    }

    double numberSteps = floor( ( maxVvis - minVvis ) / bestStep );

    // Half the number of ticks according to window size.
    // For each 32 device context units, is possible to add a new tick.
    while( numberSteps >= m_scrY / 32.0 )
    {
        bestStep *= 2;
        numberSteps = floor( ( maxVvis - minVvis ) / bestStep );
    }

    double v = floor( minVvis / bestStep ) * bestStep;
    double zeroOffset = 100000000.0;
    const  int iterLimit = 1000;
    int    i = 0;

    while( v <= maxVvis && i < iterLimit )
    {
        m_tickValues.push_back( v );

        if( fabs( v ) < zeroOffset )
            zeroOffset = fabs( v );

        v += bestStep;
        i++;
    }

    // something weird happened...
    if( i == iterLimit )
        m_tickValues.clear();

    if( zeroOffset <= bestStep )
    {
        for( double& t : m_tickValues )
            t -= zeroOffset;
    }

    for( double t : m_tickValues )
        m_tickLabels.emplace_back( t );

    updateTickLabels( dc, w );
}


void mpScaleXBase::getVisibleDataRange( mpWindow& w, double& minV, double& maxV )
{
    wxCoord startPx = w.GetMarginLeft();
    wxCoord endPx = w.GetScrX() - w.GetMarginRight();
    double  pxmin   = w.p2x( startPx );
    double  pxmax   = w.p2x( endPx );

    minV    = TransformFromPlot( pxmin );
    maxV    = TransformFromPlot( pxmax );
}


void mpScaleXLog::recalculateTicks( wxDC& dc, mpWindow& w )
{
    double minV, maxV, minVvis, maxVvis;

    GetDataRange( minV, maxV );
    getVisibleDataRange( w, minVvis, maxVvis );

    // double decades = log( maxV / minV ) / log(10);
    double  minDecade   = pow( 10, floor( log10( minV ) ) );
    double  maxDecade   = pow( 10, ceil( log10( maxV ) ) );
    double visibleDecades = log( maxVvis / minVvis ) / log( 10 );
    double  step = 10.0;
    int     m_scrX = w.GetXScreen();

    double d;

    m_tickValues.clear();
    m_tickLabels.clear();

    if( minDecade == 0.0 )
        return;

    // Half the number of ticks according to window size.
    // The value 96 is used to have only 4 ticks when m_scrX is 268.
    // For each 96 device context units, is possible to add a new tick.
    while( visibleDecades - 2 >= m_scrX / 96.0 )
    {
        step *= 10.0;
        visibleDecades = log( maxVvis / minVvis ) / log( step );

        if( !std::isfinite( visibleDecades ) )
            break;
    }

    for( d = minDecade; d<=maxDecade; d *= step )
    {
        m_tickLabels.emplace_back( d );

        for( double dd = d; dd < d * step; dd += d )
        {
            if( visibleDecades < 2 )
                m_tickLabels.emplace_back( dd );

            m_tickValues.push_back( dd );
        }
    }

    updateTickLabels( dc, w );
}


IMPLEMENT_ABSTRACT_CLASS( mpScaleXBase, mpLayer )
IMPLEMENT_DYNAMIC_CLASS( mpScaleX, mpScaleXBase )
IMPLEMENT_DYNAMIC_CLASS( mpScaleXLog, mpScaleXBase )


mpScaleXBase::mpScaleXBase( const wxString& name, int flags, bool ticks, unsigned int type )
{
    SetName( name );
    SetFont( (wxFont&) *wxSMALL_FONT );
    SetPen( (wxPen&) *wxGREY_PEN );
    m_flags = flags;
    m_ticks = ticks;
    m_type = mpLAYER_AXIS;
}


mpScaleX::mpScaleX( const wxString& name, int flags, bool ticks, unsigned int type ) :
    mpScaleXBase( name, flags, ticks, type )
{
}


mpScaleXLog::mpScaleXLog( const wxString& name, int flags, bool ticks, unsigned int type ) :
    mpScaleXBase( name, flags, ticks, type )
{
}


void mpScaleXBase::Plot( wxDC& dc, mpWindow& w )
{
    int tx, ty;

    m_offset    = -m_minV;
    m_scale     = 1.0 / ( m_maxV - m_minV );

    recalculateTicks( dc, w );

    if( m_visible )
    {
        dc.SetPen( m_pen );
        dc.SetFont( m_font );
        int orgy = 0;

        const int extend = w.GetScrX();    ///2;

        if( m_flags == mpALIGN_CENTER )
            orgy = w.y2p( 0 );

        if( m_flags == mpALIGN_TOP )
            orgy = w.GetMarginTop();

        if( m_flags == mpALIGN_BOTTOM )
            orgy = w.GetScrY() - w.GetMarginBottom();

        if( m_flags == mpALIGN_BORDER_BOTTOM )
            orgy = w.GetScrY() - 1;

        if( m_flags == mpALIGN_BORDER_TOP )
            orgy = 1;

        wxCoord startPx = w.GetMarginLeft();
        wxCoord endPx   = w.GetScrX() - w.GetMarginRight();
        wxCoord minYpx  = w.GetMarginTop();
        wxCoord maxYpx  = w.GetScrY() - w.GetMarginBottom();

        // int tmp=-65535;

        // Control labels height to decide where to put axis name (below labels or on top of axis).
        int labelH = m_maxLabelHeight;

        // int maxExtent = tc.MaxLabelWidth();
        for( double tp : m_tickValues )
        {
            double    px = TransformToPlot( tp );
            const int p = (int) ( ( px - w.GetPosX() ) * w.GetScaleX() );

            if( p >= startPx && p <= endPx )
            {
                if( m_ticks )    // draw axis ticks
                {
                    if( m_flags == mpALIGN_BORDER_BOTTOM )
                        dc.DrawLine( p, orgy, p, orgy - 4 );
                    else
                        dc.DrawLine( p, orgy, p, orgy + 4 );
                }
                else     // draw grid dotted lines
                {
                    m_pen.SetStyle( wxPENSTYLE_DOT );
                    dc.SetPen( m_pen );

                    if( m_flags == mpALIGN_BOTTOM )
                    {
                        m_pen.SetStyle( wxPENSTYLE_DOT );
                        dc.SetPen( m_pen );
                        dc.DrawLine( p, orgy + 4, p, minYpx );
                        m_pen.SetStyle( wxPENSTYLE_SOLID );
                        dc.SetPen( m_pen );
                        dc.DrawLine( p, orgy + 4, p, orgy - 4 );
                    }
                    else
                    {
                        if( m_flags == mpALIGN_TOP )
                            dc.DrawLine( p, orgy - 4, p, maxYpx );
                        else
                            dc.DrawLine( p, minYpx, p, maxYpx );
                    }

                    m_pen.SetStyle( wxPENSTYLE_SOLID );
                    dc.SetPen( m_pen );
                }
            }
        }

        m_pen.SetStyle( wxPENSTYLE_SOLID );
        dc.SetPen( m_pen );
        dc.DrawLine( startPx, minYpx, endPx, minYpx );
        dc.DrawLine( startPx, maxYpx, endPx, maxYpx );

        // Actually draw labels, taking care of not overlapping them, and distributing them
        // regularly
        for( const TICK_LABEL& tickLabel : m_tickLabels )
        {
            if( !tickLabel.visible )
                continue;

            double    px = TransformToPlot( tickLabel.pos );
            const int p = (int) ( ( px - w.GetPosX() ) * w.GetScaleX() );

            if( ( p >= startPx ) && ( p <= endPx ) )
            {
                // Write ticks labels in s string
                wxString s = tickLabel.label;

                dc.GetTextExtent( s, &tx, &ty );

                if( ( m_flags == mpALIGN_BORDER_BOTTOM ) || ( m_flags == mpALIGN_TOP ) )
                    dc.DrawText( s, p - tx / 2, orgy - 4 - ty );
                else
                    dc.DrawText( s, p - tx / 2, orgy + 4 );
            }
        }

        // Draw axis name
        dc.GetTextExtent( m_name, &tx, &ty );

        switch( m_nameFlags )
        {
        case mpALIGN_BORDER_BOTTOM:
            dc.DrawText( m_name, extend - tx - 4, orgy - 8 - ty - labelH );
            break;

        case mpALIGN_BOTTOM:
            dc.DrawText( m_name, ( endPx + startPx ) / 2 - tx / 2, orgy + 6 + labelH );
            break;

        case mpALIGN_CENTER:
            dc.DrawText( m_name, extend - tx - 4, orgy - 4 - ty );
            break;

        case mpALIGN_TOP:
            if( w.GetMarginTop() > (ty + labelH + 8) )
                dc.DrawText( m_name, ( endPx - startPx - tx ) >> 1, orgy - 6 - ty - labelH );
            else
                dc.DrawText( m_name, extend - tx - 4, orgy + 4 );

            break;

        case mpALIGN_BORDER_TOP:
            dc.DrawText( m_name, extend - tx - 4, orgy + 6 + labelH );
            break;

        default:
            break;
        }
    }
}


IMPLEMENT_DYNAMIC_CLASS( mpScaleY, mpLayer )


mpScaleY::mpScaleY( const wxString& name, int flags, bool ticks )
{
    SetName( name );
    SetFont( (wxFont&) *wxSMALL_FONT );
    SetPen( (wxPen&) *wxGREY_PEN );
    m_flags = flags;
    m_ticks = ticks;
    m_type  = mpLAYER_AXIS;
    m_masterScale = nullptr;
    m_nameFlags = mpALIGN_BORDER_LEFT;
}


void mpScaleY::Plot( wxDC& dc, mpWindow& w )
{
    m_offset    = -m_minV;
    m_scale     = 1.0 / ( m_maxV - m_minV );

    recalculateTicks( dc, w );

    if( m_visible )
    {
        dc.SetPen( m_pen );
        dc.SetFont( m_font );

        int orgx = 0;

        if( m_flags == mpALIGN_CENTER )
            orgx = w.x2p( 0 );

        if( m_flags == mpALIGN_LEFT )
            orgx = w.GetMarginLeft();

        if( m_flags == mpALIGN_RIGHT )
            orgx = w.GetScrX() - w.GetMarginRight();

        if( m_flags == mpALIGN_FAR_RIGHT )
            orgx = w.GetScrX() - ( w.GetMarginRight() / 2 );

        if( m_flags == mpALIGN_BORDER_RIGHT )
            orgx = w.GetScrX() - 1;

        if( m_flags == mpALIGN_BORDER_LEFT )
            orgx = 1;

        wxCoord endPx   = w.GetScrX() - w.GetMarginRight();
        wxCoord minYpx  = w.GetMarginTop();
        wxCoord maxYpx  = w.GetScrY() - w.GetMarginBottom();

        // Draw line
        dc.DrawLine( orgx, minYpx, orgx, maxYpx );

        wxCoord  tx, ty;
        wxString s;
        wxString fmt;

        int labelW = 0;

        // Before staring cycle, calculate label height
        int labelHeight = 0;
        s.Printf( fmt, 0 );
        dc.GetTextExtent( s, &tx, &labelHeight );

        for( double tp : m_tickValues )
        {
            double    py = TransformToPlot( tp );
            const int p = (int) ( ( w.GetPosY() - py ) * w.GetScaleY() );

            if( p >= minYpx && p <= maxYpx )
            {
                if( m_ticks )    // Draw axis ticks
                {
                    if( m_flags == mpALIGN_BORDER_LEFT )
                        dc.DrawLine( orgx, p, orgx + 4, p );
                    else
                        dc.DrawLine( orgx - 4, p, orgx, p );
                }
                else
                {
                    dc.DrawLine( orgx - 4, p, orgx + 4, p );

                    m_pen.SetStyle( wxPENSTYLE_DOT );
                    dc.SetPen( m_pen );

                    dc.DrawLine( orgx - 4, p, endPx, p );

                    m_pen.SetStyle( wxPENSTYLE_SOLID );
                    dc.SetPen( m_pen );
                }

                // Print ticks labels
            }
        }

        for( const TICK_LABEL& tickLabel : m_tickLabels )
        {
            double    py = TransformToPlot( tickLabel.pos );
            const int p = (int) ( ( w.GetPosY() - py ) * w.GetScaleY() );

            if( !tickLabel.visible )
                continue;

            if( p >= minYpx && p <= maxYpx )
            {
                s = tickLabel.label;
                dc.GetTextExtent( s, &tx, &ty );

                if( m_flags == mpALIGN_BORDER_LEFT || m_flags == mpALIGN_RIGHT
                  || m_flags == mpALIGN_FAR_RIGHT )
                    dc.DrawText( s, orgx + 4, p - ty / 2 );
                else
                    dc.DrawText( s, orgx - 4 - tx, p - ty / 2 );    // ( s, orgx+4, p-ty/2);
            }
        }

        // Draw axis name
        dc.GetTextExtent( m_name, &tx, &ty );

        switch( m_nameFlags )
        {
        case mpALIGN_BORDER_LEFT:
            dc.DrawText( m_name, labelW + 8, 4 );
            break;

        case mpALIGN_LEFT:
            dc.DrawText( m_name, orgx - ( tx / 2 ), minYpx - ty - 4 );
            break;

        case mpALIGN_CENTER:
            dc.DrawText( m_name, orgx + 4, 4 );
            break;

        case mpALIGN_RIGHT:
        case mpALIGN_FAR_RIGHT:
            dc.DrawText( m_name, orgx - ( tx / 2 ), minYpx - ty - 4 );
            break;

        case mpALIGN_BORDER_RIGHT:
            dc.DrawText( m_name, orgx - 6 - tx - labelW, 4 );
            break;

        default:
            break;
        }
    }
}


// -----------------------------------------------------------------------------
// mpWindow
// -----------------------------------------------------------------------------

IMPLEMENT_DYNAMIC_CLASS( mpWindow, wxWindow )

BEGIN_EVENT_TABLE( mpWindow, wxWindow )
EVT_PAINT( mpWindow::OnPaint )
EVT_SIZE( mpWindow::OnSize )

EVT_MIDDLE_DOWN( mpWindow::OnMouseMiddleDown )  // JLB
EVT_RIGHT_UP( mpWindow::OnShowPopupMenu )
EVT_MOUSEWHEEL( mpWindow::onMouseWheel )        // JLB
EVT_MAGNIFY( mpWindow::onMagnify )
EVT_MOTION( mpWindow::onMouseMove )             // JLB
EVT_LEFT_DOWN( mpWindow::onMouseLeftDown )
EVT_LEFT_DCLICK( mpWindow::onMouseLeftDClick )
EVT_LEFT_UP( mpWindow::onMouseLeftRelease )

EVT_MENU( mpID_CENTER, mpWindow::OnCenter )
EVT_MENU( mpID_FIT, mpWindow::OnFit )
EVT_MENU( mpID_ZOOM_IN, mpWindow::onZoomIn )
EVT_MENU( mpID_ZOOM_OUT, mpWindow::onZoomOut )
EVT_MENU( mpID_ZOOM_UNDO, mpWindow::onZoomUndo )
EVT_MENU( mpID_ZOOM_REDO, mpWindow::onZoomRedo )
END_EVENT_TABLE()


mpWindow::mpWindow() : mpWindow( DelegatingContructorTag() )
{
    initializeGraphicsContext();
}


mpWindow::mpWindow( wxWindow* parent, wxWindowID id ) :
        mpWindow( DelegatingContructorTag(), parent, id, wxDefaultPosition, wxDefaultSize, 0,
                  wxT( "mathplot" ) )
{
    m_popmenu.Append( mpID_ZOOM_UNDO, _( "Undo Last Zoom" ),
                      _( "Return zoom to level prior to last zoom action" ) );
    m_popmenu.Append( mpID_ZOOM_REDO, _( "Redo Last Zoom" ),
                      _( "Return zoom to level prior to last zoom undo" ) );
    m_popmenu.AppendSeparator();
    m_popmenu.Append( mpID_ZOOM_IN, _( "Zoom In" ), _( "Zoom in plot view." ) );
    m_popmenu.Append( mpID_ZOOM_OUT, _( "Zoom Out" ), _( "Zoom out plot view." ) );
    m_popmenu.Append( mpID_CENTER, _( "Center on Cursor" ),
                      _( "Center plot view to this position" ) );
    m_popmenu.Append( mpID_FIT, _( "Fit on Screen" ), _( "Set plot view to show all items" ) );

    m_layers.clear();
    SetBackgroundColour( *wxWHITE );
    m_bgColour  = *wxWHITE;
    m_fgColour  = *wxBLACK;

    SetSizeHints( 128, 128 );

    // J.L.Blanco: Eliminates the "flick" with the double buffer.
    SetBackgroundStyle( wxBG_STYLE_CUSTOM );

    initializeGraphicsContext();
    UpdateAll();
}


mpWindow::~mpWindow()
{
    // Free all the layers:
    DelAllLayers( true, false );

    delete m_buff_bmp;
    m_buff_bmp = nullptr;
}


// Mouse handler, for detecting when the user drag with the right button or just "clicks" for
// the menu.
// JLB
void mpWindow::OnMouseMiddleDown( wxMouseEvent& event )
{
    m_mouseMClick.x = event.GetX();
    m_mouseMClick.y = event.GetY();
}


void mpWindow::onMagnify( wxMouseEvent& event )
{
    if( !m_enableMouseNavigation )
    {
        event.Skip();
        return;
    }

    float   zoom = event.GetMagnification() + 1.0f;
    wxPoint pos( event.GetX(), event.GetY() );

    if( zoom > 1.0f )
        ZoomIn( pos, zoom );
    else if( zoom < 1.0f )
        ZoomOut( pos, 1.0f / zoom );
}


// Process mouse wheel events
// JLB
void mpWindow::onMouseWheel( wxMouseEvent& event )
{
    if( !m_enableMouseNavigation )
    {
        event.Skip();
        return;
    }

    const wxMouseWheelAxis axis = event.GetWheelAxis();
    const int              modifiers = event.GetModifiers();
    MouseWheelAction       action = MouseWheelAction::NONE;

    if( axis == wxMOUSE_WHEEL_HORIZONTAL )
    {
        action = m_mouseWheelActions.horizontal;
    }
    else if( modifiers == wxMOD_NONE )
    {
        action = m_mouseWheelActions.verticalUnmodified;
    }
    else if( modifiers == wxMOD_CONTROL )
    {
        action = m_mouseWheelActions.verticalWithCtrl;
    }
    else if( modifiers == wxMOD_SHIFT )
    {
        action = m_mouseWheelActions.verticalWithShift;
    }
    else if( modifiers == wxMOD_ALT )
    {
        action = m_mouseWheelActions.verticalWithAlt;
    }
    else
    {
        event.Skip();
        return;
    }

    PerformMouseWheelAction( event, action );
}


// If the user "drags" with the right button pressed, do "pan"
// JLB
void mpWindow::onMouseMove( wxMouseEvent& event )
{
    if( !m_enableMouseNavigation )
    {
        event.Skip();
        return;
    }

    wxCursor cursor = wxCURSOR_MAGNIFIER;

    if( event.m_middleDown )
    {
        cursor = wxCURSOR_ARROW;

        // The change:
        int Ax  = m_mouseMClick.x - event.GetX();
        int Ay  = m_mouseMClick.y - event.GetY();

        // For the next event, use relative to this coordinates.
        m_mouseMClick.x = event.GetX();
        m_mouseMClick.y = event.GetY();

        if( Ax )
        {
            double  Ax_units = Ax / m_scaleX;
            SetXView( m_posX + Ax_units, m_desiredXmax + Ax_units, m_desiredXmin + Ax_units );
        }

        if( Ay )
        {
            double  Ay_units = -Ay / m_scaleY;
            SetYView( m_posY + Ay_units, m_desiredYmax + Ay_units, m_desiredYmin + Ay_units );
        }

        if( Ax || Ay )
             UpdateAll();
    }
    else if( event.m_leftDown )
    {
        if( m_movingInfoLayer )
        {
            if( dynamic_cast<mpInfoLegend*>( m_movingInfoLayer ) )
                cursor = wxCURSOR_SIZING;
            else
                cursor = wxCURSOR_SIZEWE;

            wxPoint moveVector( event.GetX() - m_mouseLClick.x, event.GetY() - m_mouseLClick.y );
            m_movingInfoLayer->Move( moveVector );
            m_zooming = false;
        }
        else
        {
            cursor = wxCURSOR_MAGNIFIER;

            wxClientDC dc( this );
            wxPen pen( m_fgColour, 1, wxPENSTYLE_DOT );
            dc.SetPen( pen );
            dc.SetBrush( *wxTRANSPARENT_BRUSH );
            dc.DrawRectangle( m_mouseLClick.x, m_mouseLClick.y,
                              event.GetX() - m_mouseLClick.x, event.GetY() - m_mouseLClick.y );
            m_zooming = true;
            m_zoomRect.x      = m_mouseLClick.x;
            m_zoomRect.y      = m_mouseLClick.y;
            m_zoomRect.width  = event.GetX() - m_mouseLClick.x;
            m_zoomRect.height = event.GetY() - m_mouseLClick.y;
        }

        UpdateAll();
    }
    else
    {
        for( mpLayer* layer : m_layers)
        {
            if( layer->IsInfo() && layer->IsVisible() )
            {
                mpInfoLayer* infoLayer = (mpInfoLayer*) layer;

                if( infoLayer->Inside( event.GetPosition() ) )
                {
                    if( dynamic_cast<mpInfoLegend*>( infoLayer ) )
                        cursor = wxCURSOR_SIZING;
                    else
                        cursor = wxCURSOR_SIZEWE;
                }
            }
        }
    }

    SetCursor( cursor );

    event.Skip();
}


void mpWindow::onMouseLeftDown( wxMouseEvent& event )
{
    m_mouseLClick.x = event.GetX();
    m_mouseLClick.y = event.GetY();
    m_zooming = true;
    wxPoint pointClicked = event.GetPosition();
    m_movingInfoLayer = IsInsideInfoLayer( pointClicked );

    event.Skip();
}


void mpWindow::onMouseLeftDClick( wxMouseEvent& event )
{
    wxPoint pointClicked = event.GetPosition();

    if( mpInfoLayer* infoLayer = IsInsideInfoLayer( pointClicked ) )
    {
        if( infoLayer->OnDoubleClick( pointClicked, *this ) )
        {
            UpdateAll();
            return;
        }
    }

    event.Skip();
}


void mpWindow::onMouseLeftRelease( wxMouseEvent& event )
{
    wxPoint release( event.GetX(), event.GetY() );
    wxPoint press( m_mouseLClick.x, m_mouseLClick.y );

    m_zooming = false;

    if( m_movingInfoLayer != nullptr )
    {
        m_movingInfoLayer->UpdateReference();
        m_movingInfoLayer = nullptr;
    }
    else
    {
        if( release != press )
            ZoomRect( press, release );
    }

    event.Skip();
}


void mpWindow::Fit()
{
    if( UpdateBBox() )
        Fit( m_minX, m_maxX, m_minY, m_maxY );
}


// JL
void mpWindow::Fit( double xMin, double xMax, double yMin, double yMax, const wxCoord* printSizeX,
                    const wxCoord* printSizeY, wxOrientation directions )
{
    const bool isPrinting = printSizeX != nullptr && printSizeY != nullptr;

    // Save desired borders:
    double newDesiredXmin = xMin;
    double newDesiredXmax = xMax;
    double newDesiredYmin = yMin;
    double newDesiredYmax = yMax;

    // Provide a gap between the extrema of the curve and the top/bottom edges of the
    // plot area. Not to be confused with the left/right/top/bottom margins outside the plot area.
    const double xGap = fabs( xMax - xMin ) * m_leftRightPlotGapFactor;
    const double yGap = fabs( yMax - yMin ) * m_topBottomPlotGapFactor;
    xMin -= xGap;
    xMax += xGap;
    yMin -= yGap;
    yMax += yGap;

    int newScrX = m_scrX;
    int newScrY = m_scrY;

    if( isPrinting )
    {
        // Printer:
        newScrX = *printSizeX;
        newScrY = *printSizeY;
    }
    else
    {
        // Normal case (screen):
        GetClientSize( &newScrX, &newScrY );
    }

    // Compute the width/height in pixels for the plot area.
    const int plotScreenWidth  = newScrX - m_marginLeft - m_marginRight;
    const int plotScreenHeight = newScrY - m_marginTop  - m_marginBottom;

    // Adjust scale so that desired X/Y span plus extra gap fits in the plot area
    double desiredSpanX = xMax - xMin;
    double desiredSpanY = yMax - yMin;
    double newScaleX = ( desiredSpanX != 0 ) ? double( plotScreenWidth )  / desiredSpanX : 1;
    double newScaleY = ( desiredSpanY != 0 ) ? double( plotScreenHeight ) / desiredSpanY : 1;

    // Adjust corner coordinates:
    // Upstream's aspect lock code has been removed, so no need to account for centering.
    double newPosX = xMin - ( m_marginLeft / newScaleX );
    double newPosY = yMax + ( m_marginTop  / newScaleY );

    // Commit above changes to member variables only if enabled for their respective dimension.
    if( ( ( directions & wxHORIZONTAL ) != 0 ) || isPrinting )
    {
        // Don't commit the passed desired bounds when printing
        if( !isPrinting )
        {
            m_desiredXmin = newDesiredXmin;
            m_desiredXmax = newDesiredXmax;
        }

        m_scrX = newScrX;
        m_scaleX = newScaleX;
        m_posX = newPosX;
    }

    if( ( ( directions & wxVERTICAL ) != 0 ) || isPrinting )
    {
        // Don't commit the passed desired bounds when printing
        if( !isPrinting )
        {
            m_desiredYmin = newDesiredYmin;
            m_desiredYmax = newDesiredYmax;
        }

        m_scrY = newScrY;
        m_scaleY = newScaleY;
        m_posY = newPosY;
    }

    // It is VERY IMPORTANT to NOT call Refresh if we are drawing to the printer!!
    // Otherwise, the DC dimensions will be those of the window instead of the printer device
    // The caller wanting to print should perform another Fit() afterwards to restore this
    // object's state.
    if( !isPrinting )
        UpdateAll();
}


void mpWindow::AdjustLimitedView( wxOrientation directions )
{
    if( !m_enableLimitedView )
        return;

    // The m_desired* members are expressed in plot coordinates.
    // They should be clamped against their respective m_minX, m_maxX, m_minY, m_maxY limits.

    if( ( directions & wxHORIZONTAL ) != 0 )
    {
        if( m_desiredXmin < m_minX )
        {
            double diff = m_minX - m_desiredXmin;
            m_posX += diff;
            m_desiredXmax += diff;
            m_desiredXmin = m_minX;
        }

        if( m_desiredXmax > m_maxX )
        {
            double diff = m_desiredXmax - m_maxX;
            m_posX -= diff;
            m_desiredXmin -= diff;
            m_desiredXmax = m_maxX;
        }
    }

    if( ( directions & wxVERTICAL ) != 0 )
    {
        if( m_desiredYmin < m_minY )
        {
            double diff = m_minY - m_desiredYmin;
            m_posY += diff;
            m_desiredYmax += diff;
            m_desiredYmin = m_minY;
        }

        if( m_desiredYmax > m_maxY )
        {
            double diff = m_desiredYmax - m_maxY;
            m_posY -= diff;
            m_desiredYmin -= diff;
            m_desiredYmax = m_maxY;
        }
    }
}


bool mpWindow::SetXView( double pos, double desiredMax, double desiredMin )
{
    // TODO (ecorm): Investigate X scale flickering when panning at minimum zoom level
    // Possible cause: When AdjustLimitedView subtracts the out-of-bound delta, it does not
    // revert back to the exact same original coordinates due to floating point rounding errors.
    m_posX = pos;
    m_desiredXmax   = desiredMax;
    m_desiredXmin   = desiredMin;
    AdjustLimitedView( wxHORIZONTAL );

    return true;
}


bool mpWindow::SetYView( double pos, double desiredMax, double desiredMin )
{
    m_posY = pos;
    m_desiredYmax   = desiredMax;
    m_desiredYmin   = desiredMin;
    AdjustLimitedView( wxVERTICAL );

    return true;
}


void mpWindow::ZoomIn( const wxPoint& centerPoint )
{
    ZoomIn( centerPoint, zoomIncrementalFactor, wxBOTH );
}


void mpWindow::ZoomIn( const wxPoint& centerPoint, double zoomFactor, wxOrientation directions )
{
    DoZoom( centerPoint, zoomFactor, directions );
}


void mpWindow::ZoomOut( const wxPoint& centerPoint )
{
    ZoomOut( centerPoint, zoomIncrementalFactor, wxBOTH );
}


void mpWindow::ZoomOut( const wxPoint& centerPoint, double zoomFactor, wxOrientation directions )
{
    if( zoomFactor == 0 )
        zoomFactor = 1.0;

    DoZoom( centerPoint, 1.0 / zoomFactor, directions );
}


void mpWindow::ZoomRect( wxPoint p0, wxPoint p1 )
{
    pushZoomUndo( { { m_desiredXmin, m_desiredXmax, m_desiredYmin, m_desiredYmax } } );

    // Constrain given rectangle to plot area
    const int pMinX = m_marginLeft;
    const int pMaxX = m_scrX - m_marginRight;
    const int pMinY = m_marginTop;
    const int pMaxY = m_scrY - m_marginBottom;
    p0.x = std::max( p0.x, pMinX );
    p0.x = std::min( p0.x, pMaxX );
    p0.y = std::max( p0.y, pMinY );
    p0.y = std::min( p0.y, pMaxY );
    p1.x = std::max( p1.x, pMinX );
    p1.x = std::min( p1.x, pMaxX );
    p1.y = std::max( p1.y, pMinY );
    p1.y = std::min( p1.y, pMaxY );

    // Compute the 2 corners in graph coordinates:
    double  p0x = p2x( p0.x );
    double  p0y = p2y( p0.y );
    double  p1x = p2x( p1.x );
    double  p1y = p2y( p1.y );

    // Order them:
    double  zoom_x_min = p0x<p1x ? p0x : p1x;
    double  zoom_x_max = p0x>p1x ? p0x : p1x;
    double  zoom_y_min = p0y<p1y ? p0y : p1y;
    double  zoom_y_max = p0y>p1y ? p0y : p1y;

    if( m_yLocked )
    {
        zoom_y_min = m_desiredYmin;
        zoom_y_max = m_desiredYmax;
    }

    Fit( zoom_x_min, zoom_x_max, zoom_y_min, zoom_y_max );

    // Even with the input rectangle constrained to the plot area, it's still possible for the
    // resulting view to exceed limits when a portion of the gap is grabbed.
    AdjustLimitedView();

    // These additional checks are needed because AdjustLimitedView only adjusts the position
    // and not the scale.
    wxOrientation directionsNeedingRefitting = ViewNeedsRefitting( wxBOTH );

    if( directionsNeedingRefitting != 0 )
        Fit( m_minX, m_maxX, m_minY, m_maxY, nullptr, nullptr, directionsNeedingRefitting );
}


void mpWindow::pushZoomUndo( const std::array<double, 4>& aZoom )
{
    m_undoZoomStack.push( aZoom );

    while( !m_redoZoomStack.empty() )
        m_redoZoomStack.pop();
}


void mpWindow::ZoomUndo()
{
    if( m_undoZoomStack.size() )
    {
        m_redoZoomStack.push( { { m_desiredXmin, m_desiredXmax, m_desiredYmin, m_desiredYmax } } );

        std::array<double, 4> zoom = m_undoZoomStack.top();
        m_undoZoomStack.pop();

        Fit( zoom[0], zoom[1], zoom[2], zoom[3] );
        AdjustLimitedView();
    }
}


void mpWindow::ZoomRedo()
{
    if( m_redoZoomStack.size() )
    {
        m_undoZoomStack.push( { { m_desiredXmin, m_desiredXmax, m_desiredYmin, m_desiredYmax } } );

        std::array<double, 4> zoom = m_redoZoomStack.top();
        m_redoZoomStack.pop();

        Fit( zoom[0], zoom[1], zoom[2], zoom[3] );
        AdjustLimitedView();
    }
}


void mpWindow::OnShowPopupMenu( wxMouseEvent& event )
{
    m_clickedX  = event.GetX();
    m_clickedY  = event.GetY();

    m_popmenu.Enable( mpID_ZOOM_UNDO, !m_undoZoomStack.empty() );
    m_popmenu.Enable( mpID_ZOOM_REDO, !m_redoZoomStack.empty() );

    PopupMenu( &m_popmenu, event.GetX(), event.GetY() );
}


void mpWindow::OnFit( wxCommandEvent& WXUNUSED( event ) )
{
    pushZoomUndo( { { m_desiredXmin, m_desiredXmax, m_desiredYmin, m_desiredYmax } } );

    Fit();
}


void mpWindow::OnCenter( wxCommandEvent& WXUNUSED( event ) )
{
    GetClientSize( &m_scrX, &m_scrY );
    int centerX = ( m_scrX - m_marginLeft - m_marginRight ) / 2;
    int centerY = ( m_scrY - m_marginTop - m_marginBottom ) / 2;
    SetPos( p2x( m_clickedX - centerX ), p2y( m_clickedY - centerY ) );
}


mpWindow::MouseWheelActionSet mpWindow::defaultMouseWheelActions()
{
    MouseWheelActionSet actions;
    actions.verticalUnmodified = MouseWheelAction::ZOOM;
    actions.verticalWithCtrl   = MouseWheelAction::PAN_LEFT_RIGHT;
    actions.verticalWithShift  = MouseWheelAction::PAN_UP_DOWN;
    actions.verticalWithAlt    = MouseWheelAction::NONE;
    actions.horizontal         = MouseWheelAction::NONE;
    return actions;
}


void mpWindow::onZoomIn( wxCommandEvent& WXUNUSED( event ) )
{
    ZoomIn( wxPoint( m_mouseMClick.x, m_mouseMClick.y ) );
}


void mpWindow::onZoomOut( wxCommandEvent& WXUNUSED( event ) )
{
    ZoomOut();
}


void mpWindow::onZoomUndo( wxCommandEvent& WXUNUSED( event ) )
{
    ZoomUndo();
}


void mpWindow::onZoomRedo( wxCommandEvent& WXUNUSED( event ) )
{
    ZoomRedo();
}


void mpWindow::OnSize( wxSizeEvent& WXUNUSED( event ) )
{
    // Try to fit again with the new window size:
    Fit( m_desiredXmin, m_desiredXmax, m_desiredYmin, m_desiredYmax );
}


bool mpWindow::AddLayer( mpLayer* layer, bool refreshDisplay )
{
    if( layer )
    {
        m_layers.push_back( layer );

        if( refreshDisplay )
            UpdateAll();

        return true;
    }

    return false;
}


bool mpWindow::DelLayer( mpLayer* layer, bool alsoDeleteObject, bool refreshDisplay )
{
    wxLayerList::iterator layIt;

    for( layIt = m_layers.begin(); layIt != m_layers.end(); layIt++ )
    {
        if( *layIt == layer )
        {
            // Also delete the object?
            if( alsoDeleteObject )
                delete *layIt;

            m_layers.erase( layIt );    // this deleted the reference only

            if( refreshDisplay )
                UpdateAll();

            return true;
        }
    }

    return false;
}


void mpWindow::DelAllLayers( bool alsoDeleteObject, bool refreshDisplay )
{
    while( m_layers.size()>0 )
    {
        // Also delete the object?
        if( alsoDeleteObject )
            delete m_layers[0];

        m_layers.erase( m_layers.begin() );    // this deleted the reference only
    }

    if( refreshDisplay )
        UpdateAll();
}


void mpWindow::OnPaint( wxPaintEvent& WXUNUSED( event ) )
{
    wxPaintDC paintDC( this );

    paintDC.GetSize( &m_scrX, &m_scrY );    // This is the size of the visible area only!

    // Selects direct or buffered draw:
    wxDC* targetDC = &paintDC;

    // J.L.Blanco @ Aug 2007: Added double buffer support
    if( m_enableDoubleBuffer )
    {
        if( m_last_lx != m_scrX || m_last_ly != m_scrY )
        {
            delete m_buff_bmp;
            m_buff_bmp = new wxBitmap( m_scrX, m_scrY );
            m_buff_dc.SelectObject( *m_buff_bmp );
            m_last_lx   = m_scrX;
            m_last_ly   = m_scrY;
        }

        targetDC = &m_buff_dc;
    }

    if( wxGraphicsContext* ctx = targetDC->GetGraphicsContext() )
    {
        if( !ctx->SetInterpolationQuality( wxINTERPOLATION_BEST ) )
            if( !ctx->SetInterpolationQuality( wxINTERPOLATION_GOOD ) )
                ctx->SetInterpolationQuality( wxINTERPOLATION_FAST );

        ctx->SetAntialiasMode( wxANTIALIAS_DEFAULT );
    }

    // Draw background:
    targetDC->SetPen( *wxTRANSPARENT_PEN );
    wxBrush brush( GetBackgroundColour() );
    targetDC->SetBrush( brush );
    targetDC->SetTextForeground( m_fgColour );
    targetDC->DrawRectangle( 0, 0, m_scrX, m_scrY );

    // Draw all the layers:
    for( mpLayer* layer : m_layers )
        layer->Plot( *targetDC, *this );

    if( m_zooming )
    {
        wxPen pen( m_fgColour, 1, wxPENSTYLE_DOT );
        targetDC->SetPen( pen );
        targetDC->SetBrush( *wxTRANSPARENT_BRUSH );
        targetDC->DrawRectangle( m_zoomRect );
    }

    // If doublebuffer, draw now to the window:
    if( m_enableDoubleBuffer )
        paintDC.Blit( 0, 0, m_scrX, m_scrY, targetDC, 0, 0 );
}


void mpWindow::DoZoom( const wxPoint& centerPoint, double zoomFactor, wxOrientation directions )
{
    if( m_yLocked )
    {
        if( directions == wxVERTICAL )
            return;

        directions = wxHORIZONTAL;
    }

    const bool horizontally = ( directions & wxHORIZONTAL ) != 0;
    const bool vertically   = ( directions & wxVERTICAL )   != 0;

    pushZoomUndo( { { m_desiredXmin, m_desiredXmax, m_desiredYmin, m_desiredYmax } } );

    // Preserve the position of the clicked point:
    wxPoint c( centerPoint );
    if( c == wxDefaultPosition )
    {
        GetClientSize( &m_scrX, &m_scrY );
        c.x = ( m_scrX - m_marginLeft - m_marginRight ) / 2 + m_marginLeft;
        c.y = ( m_scrY - m_marginTop - m_marginBottom ) / 2 + m_marginTop;
    }
    else
    {
        c.x = std::max( c.x, m_marginLeft );
        c.x = std::min( c.x, m_scrX - m_marginRight );
        c.y = std::max( c.y, m_marginTop );
        c.y = std::min( c.y, m_scrY - m_marginBottom );
    }

    // Zoom in/out:
    const double MAX_SCALE = 1e6;
    const double newScaleX = horizontally ? ( m_scaleX * zoomFactor ) : m_scaleX;
    const double newScaleY = vertically   ? ( m_scaleY * zoomFactor ) : m_scaleY;

    // Baaaaad things happen when you zoom in too much..
    if( newScaleX > MAX_SCALE || newScaleY > MAX_SCALE )
        return;

    if( horizontally )
    {
        // Transform the clicked X point to layer coordinates:
        const double prior_layer_x = p2x( c.x );

        // Adjust the new X scale and plot X origin:
        m_scaleX = newScaleX;
        m_posX = prior_layer_x - c.x / newScaleX;

        // Recompute the desired X view extents:
        RecomputeDesiredX( m_desiredXmin, m_desiredXmax );
    }

    if( vertically )
    {
        // Transform the clicked Y point to layer coordinates:
        const double prior_layer_y = p2y( c.y );

        // Adjust the new Y scale and plot Y origin:
        m_scaleY = newScaleY;
        m_posY = prior_layer_y + c.y / newScaleY;

        // Recompute the desired Y view extents:
        RecomputeDesiredY( m_desiredYmin, m_desiredYmax );
    }

    AdjustLimitedView( directions );

    if( zoomFactor < 1.0 )
    {
        // These additional checks are needed because AdjustLimitedView only adjusts the position
        // and not the scale.
        wxOrientation directionsNeedingRefitting = ViewNeedsRefitting( directions );

        // If the view is still out-of-limits after AdjustLimitedView is called, perform a Fit
        // along the offending dimension(s).
        if( directionsNeedingRefitting != 0 )
            Fit( m_minX, m_maxX, m_minY, m_maxY, nullptr, nullptr, directionsNeedingRefitting );
    }

    UpdateAll();
}


void mpWindow::RecomputeDesiredX( double& min, double& max )
{
    const int    plotScreenWidth = m_scrX - m_marginLeft - m_marginRight;
    const double plotSpanX = plotScreenWidth / m_scaleX;
    const double desiredSpanX = plotSpanX / ( 2 * m_leftRightPlotGapFactor + 1 );
    const double xGap = desiredSpanX * m_leftRightPlotGapFactor;
    min = m_posX + ( m_marginLeft / m_scaleX ) + xGap;
    max = m_desiredXmin + desiredSpanX;
}


void mpWindow::RecomputeDesiredY( double& min, double& max )
{
    const int    plotScreenHeight = m_scrY - m_marginTop - m_marginBottom;
    const double plotSpanY = plotScreenHeight / m_scaleY;
    const double desiredSpanY = plotSpanY / ( 2 * m_topBottomPlotGapFactor + 1 );
    const double yGap = desiredSpanY * m_topBottomPlotGapFactor;
    max = m_posY - ( m_marginTop / m_scaleY ) - yGap;
    min = m_desiredYmax - desiredSpanY;
}


wxOrientation mpWindow::ViewNeedsRefitting( wxOrientation directions ) const
{
    if( !m_enableLimitedView )
        return static_cast<wxOrientation>( 0 );

    // Allow a gap between the extrema of the curve and the edges of the plot area. Not to be
    // confused with the left/right/top/bottom margins outside the plot area.
    const double xGap = fabs( m_maxX - m_minX ) * m_leftRightPlotGapFactor;
    const double yGap = fabs( m_maxY - m_minY ) * m_topBottomPlotGapFactor;

    wxOrientation result = {};

    if( ( directions & wxHORIZONTAL ) != 0 )
    {
        if( ( m_desiredXmax > m_maxX + xGap ) || ( m_desiredXmin < m_minX - xGap ) )
            result = static_cast<wxOrientation>( result | wxHORIZONTAL );
    }

    if( ( directions & wxVERTICAL ) != 0 )
    {
        if( ( m_desiredYmax > m_maxY + yGap ) || ( m_desiredYmin < m_minY - yGap ) )
            result = static_cast<wxOrientation>( result | wxVERTICAL );
    }

    return result;
}


void mpWindow::PerformMouseWheelAction( wxMouseEvent& event, MouseWheelAction action )
{
    const int     change = event.GetWheelRotation();
    const double  changeUnitsX = change / m_scaleX;
    const double  changeUnitsY = change / m_scaleY;
    const wxPoint clickPt( event.GetX(), event.GetY() );

    switch( action )
    {
    case MouseWheelAction::NONE: break;

    case MouseWheelAction::PAN_LEFT_RIGHT:
        SetXView( m_posX + changeUnitsX, m_desiredXmax + changeUnitsX,
                  m_desiredXmin + changeUnitsX );
        UpdateAll();
        break;

    case MouseWheelAction::PAN_RIGHT_LEFT:
        SetXView( m_posX - changeUnitsX, m_desiredXmax - changeUnitsX,
                  m_desiredXmin - changeUnitsX );
        UpdateAll();
        break;

    case MouseWheelAction::PAN_UP_DOWN:
        if( !m_yLocked )
        {
            SetYView( m_posY + changeUnitsY, m_desiredYmax + changeUnitsY,
                      m_desiredYmin + changeUnitsY );
            UpdateAll();
        }

        break;

    case MouseWheelAction::ZOOM:
        if( event.GetWheelRotation() > 0 )
            ZoomIn( clickPt );
        else
            ZoomOut( clickPt );

        break;

    case MouseWheelAction::ZOOM_HORIZONTALLY:
        if( event.GetWheelRotation() > 0 )
            ZoomIn( clickPt, zoomIncrementalFactor, wxHORIZONTAL );
        else
            ZoomOut( clickPt, zoomIncrementalFactor, wxHORIZONTAL );

        break;

    case MouseWheelAction::ZOOM_VERTICALLY:
        if( event.GetWheelRotation() > 0 )
            ZoomIn( clickPt, zoomIncrementalFactor, wxVERTICAL );
        else
            ZoomOut( clickPt, zoomIncrementalFactor, wxVERTICAL );

        break;

    default:
        break;
    }
}


bool mpWindow::UpdateBBox()
{
    m_minX  = 0.0;
    m_maxX  = 1.0;
    m_minY  = 0.0;
    m_maxY  = 1.0;

    return true;
}


void mpWindow::UpdateAll()
{
    UpdateBBox();
    Refresh( false );
}


void mpWindow::SetScaleX( double scaleX )
{
    if( scaleX != 0 )
        m_scaleX = scaleX;

    UpdateAll();
}


// New methods implemented by Davide Rondini

mpLayer* mpWindow::GetLayer( int position ) const
{
    if( ( position >= (int) m_layers.size() ) || position < 0 )
        return nullptr;

    return m_layers[position];
}


const mpLayer* mpWindow::GetLayerByName( const wxString& name ) const
{
    for( const mpLayer* layer : m_layers )
    {
        if( !layer->GetName().Cmp( name ) )
            return layer;
    }

    return nullptr; // Not found
}


void mpWindow::GetBoundingBox( double* bbox ) const
{
    bbox[0] = m_minX;
    bbox[1] = m_maxX;
    bbox[2] = m_minY;
    bbox[3] = m_maxY;
}


bool mpWindow::SaveScreenshot( wxImage& aImage, wxSize aImageSize, bool aFit )
{
    int sizeX, sizeY;

    if( aImageSize == wxDefaultSize )
    {
        sizeX = m_scrX;
        sizeY = m_scrY;
    }
    else
    {
        sizeX = aImageSize.x;
        sizeY = aImageSize.y;
        SetScr( sizeX, sizeY );
    }

    wxBitmap screenBuffer( sizeX, sizeY );
    wxMemoryDC screenDC;
    screenDC.SelectObject( screenBuffer );
    screenDC.SetPen( *wxWHITE_PEN );
    screenDC.SetTextForeground( m_fgColour );
    wxBrush brush( GetBackgroundColour() );
    screenDC.SetBrush( brush );
    screenDC.DrawRectangle( 0, 0, sizeX, sizeY );

    if( aFit )
        Fit( m_minX, m_maxX, m_minY, m_maxY, &sizeX, &sizeY );
    else
        Fit( m_desiredXmin, m_desiredXmax, m_desiredYmin, m_desiredYmax, &sizeX, &sizeY );

    // Draw all the layers:
    for( mpLayer* layer : m_layers )
        layer->Plot( screenDC, *this );

    if( aImageSize != wxDefaultSize )
    {
        // Restore dimensions
        int bk_scrX = m_scrX;
        int bk_scrY = m_scrY;
        SetScr( bk_scrX, bk_scrY );
        Fit( m_desiredXmin, m_desiredXmax, m_desiredYmin, m_desiredYmax, &bk_scrX, &bk_scrY );
        UpdateAll();
    }

    // Once drawing is complete, actually save screen shot
    aImage = screenBuffer.ConvertToImage();

    return true;
}


void mpWindow::SetMargins( int top, int right, int bottom, int left )
{
    m_marginTop    = top;
    m_marginRight  = right;
    m_marginBottom = bottom;
    m_marginLeft   = left;
}


mpInfoLayer* mpWindow::IsInsideInfoLayer( wxPoint& point )
{
    for( mpLayer* layer : m_layers )
    {
        if( layer->IsInfo() )
        {
            mpInfoLayer* tmpLyr = static_cast<mpInfoLayer*>( layer );

            if( tmpLyr->Inside( point ) )
                return tmpLyr;
        }
    }

    return nullptr;
}


void mpWindow::SetLayerVisible( const wxString& name, bool viewable )
{
    if( mpLayer* lx = GetLayerByName( name ) )
    {
        lx->SetVisible( viewable );
        UpdateAll();
    }
}


bool mpWindow::IsLayerVisible( const wxString& name ) const
{
    if( const mpLayer* lx = GetLayerByName( name ) )
        return lx->IsVisible();

    return false;
}


void mpWindow::SetLayerVisible( const unsigned int position, bool viewable )
{
    if( mpLayer* lx = GetLayer( position ) )
    {
        lx->SetVisible( viewable );
        UpdateAll();
    }
}


bool mpWindow::IsLayerVisible( unsigned int position ) const
{
    if( const mpLayer* lx = GetLayer( position ) )
        return lx->IsVisible();

    return false;
}


void mpWindow::SetColourTheme( const wxColour& bgColour, const wxColour& drawColour,
                               const wxColour& axesColour )
{
    SetBackgroundColour( bgColour );
    SetForegroundColour( drawColour );
    m_bgColour  = bgColour;
    m_fgColour  = drawColour;
    m_axColour  = axesColour;

    // Cycle between layers to set colours and properties to them
    for( mpLayer* layer : m_layers )
    {
        if( layer->GetLayerType() == mpLAYER_AXIS )
        {
            // Get the old pen to modify only colour, not style or width.
            wxPen axisPen = layer->GetPen();
            axisPen.SetColour( axesColour );
            layer->SetPen( axisPen );
        }

        if( layer->GetLayerType() == mpLAYER_INFO )
        {
            // Get the old pen to modify only colour, not style or width.
            wxPen infoPen = layer->GetPen();
            infoPen.SetColour( drawColour );
            layer->SetPen( infoPen );
        }
    }
}


template <typename... Ts>
mpWindow::mpWindow( DelegatingContructorTag, Ts&&... windowArgs ) :
        wxWindow( std::forward<Ts>( windowArgs )... ),
        m_minX( 0.0 ),
        m_maxX( 0.0 ),
        m_minY( 0.0 ),
        m_maxY( 0.0 ),
        m_scaleX( 1.0 ),
        m_scaleY( 1.0 ),
        m_posX( 0.0 ),
        m_posY( 0.0 ),
        m_scrX( 64 ),
        m_scrY( 64 ),
        m_clickedX( 0 ),
        m_clickedY( 0 ),
        m_yLocked( false ),
        m_desiredXmin( 0.0 ),
        m_desiredXmax( 1.0 ),
        m_desiredYmin( 0.0 ),
        m_desiredYmax( 1.0 ),
        m_topBottomPlotGapFactor( 0.03 ),
        m_leftRightPlotGapFactor( 0.0 ),
        m_marginTop( 0 ),
        m_marginRight( 0 ),
        m_marginBottom( 0 ),
        m_marginLeft( 0 ),
        m_last_lx( 0 ),
        m_last_ly( 0 ),
        m_buff_bmp( nullptr ),
        m_enableDoubleBuffer( false ),
        m_enableMouseNavigation( true ),
        m_enableLimitedView( false ),
        m_mouseWheelActions( defaultMouseWheelActions() ),
        m_movingInfoLayer( nullptr ),
        m_zooming( false )
{}


void mpWindow::initializeGraphicsContext()
{
    if( wxGraphicsContext* ctx = m_buff_dc.GetGraphicsContext() )
    {
        if( !ctx->SetInterpolationQuality( wxINTERPOLATION_BEST )
            || !ctx->SetInterpolationQuality( wxINTERPOLATION_GOOD ) )
        {
            ctx->SetInterpolationQuality( wxINTERPOLATION_FAST );
        }

        ctx->SetAntialiasMode( wxANTIALIAS_DEFAULT );
    }
}


// -----------------------------------------------------------------------------
// mpFXYVector implementation - by Jose Luis Blanco (AGO-2007)
// -----------------------------------------------------------------------------

IMPLEMENT_DYNAMIC_CLASS( mpFXYVector, mpFXY )


mpFXYVector::mpFXYVector( const wxString& name, int flags ) :
        mpFXY( name, flags )
{
    m_index = 0;
    m_sweepWindow = 0;
    m_minX  = -1;
    m_maxX  = 1;
    m_minY  = -1;
    m_maxY  = 1;
    m_type  = mpLAYER_PLOT;
}


double mpScaleX::TransformToPlot( double x ) const
{
    return ( x + m_offset ) * m_scale;
}


double mpScaleX::TransformFromPlot( double xplot ) const
{
    return xplot / m_scale - m_offset;
}


double mpScaleY::TransformToPlot( double x ) const
{
    return ( x + m_offset ) * m_scale;
}


double mpScaleY::TransformFromPlot( double xplot ) const
{
    return xplot / m_scale - m_offset;
}


double mpScaleXLog::TransformToPlot( double x ) const
{
    double  xlogmin = log10( m_minV );
    double  xlogmax = log10( m_maxV );

    return ( log10( x ) - xlogmin ) / ( xlogmax - xlogmin );
}


double mpScaleXLog::TransformFromPlot( double xplot ) const
{
    double  xlogmin = log10( m_minV );
    double  xlogmax = log10( m_maxV );

    return pow( 10.0, xplot * ( xlogmax - xlogmin ) + xlogmin );
}


void mpFXYVector::Rewind()
{
    m_index = 0;
    m_sweepWindow = std::numeric_limits<size_t>::max();
}


void mpFXYVector::SetSweepWindow( int aSweepIdx )
{
    m_index = aSweepIdx * m_sweepSize;
    m_sweepWindow = ( aSweepIdx + 1 ) * m_sweepSize;
}


bool mpFXYVector::GetNextXY( double& x, double& y )
{
    if( m_index >= m_xs.size() || m_index >= m_sweepWindow )
    {
        return false;
    }
    else
    {
        x = m_xs[m_index];
        y = m_ys[m_index++];
        return m_index <= m_xs.size() && m_index <= m_sweepWindow;
    }
}


void mpFXYVector::Clear()
{
    m_xs.clear();
    m_ys.clear();
}


void mpFXYVector::SetData( const std::vector<double>& xs, const std::vector<double>& ys )
{
    // Check if the data vectors are of the same size
    if( xs.size() != ys.size() )
        return;

    // Copy the data:
    m_xs    = xs;
    m_ys    = ys;

    // Update internal variables for the bounding box.
    if( xs.size() > 0 )
    {
        m_minX  = xs[0];
        m_maxX  = xs[0];
        m_minY  = ys[0];
        m_maxY  = ys[0];

        for( const double x : xs )
        {
            if( x < m_minX )
                m_minX = x;

            if( x > m_maxX )
                m_maxX = x;
        }

        for( const double y : ys )
        {
            if( y < m_minY )
                m_minY = y;

            if( y > m_maxY )
                m_maxY = y;
        }
    }
    else
    {
        m_minX  = 0;
        m_maxX  = 0;
        m_minY  = 0;
        m_maxY  = 0;
    }
}


void mpFXY::SetScale( mpScaleBase* scaleX, mpScaleBase* scaleY )
{
    m_scaleX    = scaleX;
    m_scaleY    = scaleY;

    UpdateScales();
}


void mpFXY::UpdateScales()
{
    if( m_scaleX )
        m_scaleX->ExtendDataRange( GetMinX(), GetMaxX() );

    if( m_scaleY )
        m_scaleY->ExtendDataRange( GetMinY(), GetMaxY() );
}


double mpFXY::s2x( double plotCoordX ) const
{
    return m_scaleX ? m_scaleX->TransformFromPlot( plotCoordX ) : plotCoordX;
}


double mpFXY::s2y( double plotCoordY ) const
{
    return m_scaleY ? m_scaleY->TransformFromPlot( plotCoordY ) : plotCoordY;
}


double mpFXY::x2s( double x ) const
{
    return m_scaleX ? m_scaleX->TransformToPlot( x ) : x;
}


double mpFXY::y2s( double y ) const
{
    return m_scaleY ? m_scaleY->TransformToPlot( y ) : y;
}
