/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 CERN
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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

#ifndef __SIM_PLOT_PANEL_H
#define __SIM_PLOT_PANEL_H

#include <widgets/mathplot.h>
#include <map>

class TRACE : public mpFXYVector
{
public:
    TRACE( const wxString& aName, const wxString& aSpiceName )
        : mpFXYVector( aName ), m_spiceName( aSpiceName )
    {
        SetContinuity( true );
        ShowName( false );
    }

    const wxString& GetSpiceName() const
    {
        return m_spiceName;
    }

    const std::vector<double>& GetDataX() const
    {
        return m_xs;
    }

    const std::vector<double>& GetDataY() const
    {
        return m_ys;
    }

private:
    wxString m_spiceName;
};

class CURSOR : public mpInfoLayer
{
public:
    CURSOR( const TRACE* aTrace, mpWindow* aWindow )
        : mpInfoLayer( wxRect( 0, 0, DRAG_MARGIN, DRAG_MARGIN ), wxTRANSPARENT_BRUSH ),
        m_trace( aTrace ), m_moved( false ), m_coords( 0.0, 0.0 ), m_window( aWindow )
    {
    }

    void Plot( wxDC& aDC, mpWindow& aWindow ) override;

    bool Inside( wxPoint& aPoint )
    {
        return ( std::abs( aPoint.x - m_window->x2p( m_coords.x ) ) <= DRAG_MARGIN )
            && ( std::abs( aPoint.y - m_window->y2p( m_coords.y ) ) <= DRAG_MARGIN );
    }

    void Move( wxPoint aDelta ) override
    {
        m_moved = true;
        mpInfoLayer::Move( aDelta );
    }

    void UpdateReference()
    {
        m_reference.x = m_window->x2p( m_coords.x );
        m_reference.y = m_window->y2p( m_coords.y );
    }

    const wxRealPoint& GetCoords() const
    {
        return m_coords;
    }

private:
    const TRACE* m_trace;
    bool m_moved;
    wxRealPoint m_coords;
    mpWindow* m_window;

    const int DRAG_MARGIN = 10;
};

class SIM_PLOT_PANEL : public mpWindow
{
public:
    SIM_PLOT_PANEL( wxWindow* parent, wxWindowID id, const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize, long style = 0, const wxString& name = wxPanelNameStr );

    ~SIM_PLOT_PANEL();

    bool AddTrace( const wxString& aSpiceName, const wxString& aName, int aPoints,
                    const double* aT, const double* aY, int aFlags );

    bool DeleteTrace( const wxString& aName );

    bool IsShown( const wxString& aName ) const
    {
        return ( m_traces.count( aName ) != 0 );
    }

    void DeleteAllTraces();

    const std::map<wxString, TRACE*>& GetTraces() const
    {
        return m_traces;
    }

    void ShowGrid( bool aEnable = true );

    bool IsGridShown() const;

private:
    wxColour generateColor();

    unsigned int m_colorIdx;

    // Traces to be plotted
    std::map<wxString, TRACE*> m_traces;

    mpScaleX* m_axis_x;
    mpScaleY* m_axis_y;
    mpInfoLegend* m_legend;
    //mpInfoCoords* m_coords;
};

#endif
