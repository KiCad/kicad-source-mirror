/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 CERN
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

#ifndef __SIM_PLOT_PANEL_H
#define __SIM_PLOT_PANEL_H

#include <widgets/mathplot.h>
#include <map>
#include "sim_types.h"

class TRACE;

class CURSOR : public mpInfoLayer
{
public:
    CURSOR( const TRACE* aTrace )
        : mpInfoLayer( wxRect( 0, 0, DRAG_MARGIN, DRAG_MARGIN ), wxTRANSPARENT_BRUSH ),
        m_trace( aTrace ), m_updateRequired( false ), m_coords( 0.0, 0.0 ), m_window( nullptr )
    {
    }

    void Plot( wxDC& aDC, mpWindow& aWindow ) override;

    void Update()
    {
        m_updateRequired = true;
    }

    bool Inside( wxPoint& aPoint )
    {
        if( !m_window )
            return false;

        return ( std::abs( aPoint.x - m_window->x2p( m_coords.x ) ) <= DRAG_MARGIN )
            && ( std::abs( aPoint.y - m_window->y2p( m_coords.y ) ) <= DRAG_MARGIN );
    }

    void Move( wxPoint aDelta ) override
    {
        Update();
        mpInfoLayer::Move( aDelta );
    }

    void UpdateReference()
    {
        if( !m_window )
            return;

        m_reference.x = m_window->x2p( m_coords.x );
        m_reference.y = m_window->y2p( m_coords.y );
    }

    const wxRealPoint& GetCoords() const
    {
        return m_coords;
    }

private:
    const TRACE* m_trace;
    bool m_updateRequired;
    wxRealPoint m_coords;
    mpWindow* m_window;

    const int DRAG_MARGIN = 10;
};


class TRACE : public mpFXYVector
{
public:
    TRACE( const wxString& aName, const wxString& aSpiceName )
        : mpFXYVector( aName ), m_spiceName( aSpiceName ), m_cursor( nullptr )
    {
        SetContinuity( true );
        ShowName( false );
    }

    void SetData( const std::vector<double>& aXs, const std::vector<double>& aYs )
    {
        mpFXYVector::SetData( aXs, aYs );

        if( m_cursor )
            m_cursor->Update();
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

    bool HasCursor() const
    {
        return m_cursor != nullptr;
    }

    void SetCursor( CURSOR* aCursor )
    {
        m_cursor = aCursor;
    }

    CURSOR* GetCursor() const
    {
        return m_cursor;
    }

private:
    wxString m_spiceName;
    CURSOR* m_cursor;
};


class SIM_PLOT_PANEL : public mpWindow
{
public:
    SIM_PLOT_PANEL( SIM_TYPE aType, wxWindow* parent, wxWindowID id, const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize, long style = 0, const wxString& name = wxPanelNameStr );

    ~SIM_PLOT_PANEL();

    SIM_TYPE GetType() const
    {
        return m_type;
    }

    static bool IsPlottable( SIM_TYPE aSimType );

    wxString GetLabelX() const
    {
        return m_axis_x ? m_axis_x->GetName() : "";
    }

    wxString GetLabelY1() const
    {
        return m_axis_y1 ? m_axis_y1->GetName() : "";
    }

    wxString GetLabelY2() const
    {
        return m_axis_y2 ? m_axis_y2->GetName() : "";
    }

    bool AddTrace( const wxString& aSpiceName, const wxString& aName, int aPoints,
                    const double* aT, const double* aY, int aFlags );

    bool DeleteTrace( const wxString& aName );

    void DeleteAllTraces();

    bool IsShown( const wxString& aName ) const
    {
        return ( m_traces.count( aName ) != 0 );
    }

    const std::map<wxString, TRACE*>& GetTraces() const
    {
        return m_traces;
    }

    TRACE* GetTrace( const wxString& aName ) const
    {
        auto trace = m_traces.find( aName );

        return trace == m_traces.end() ? NULL : trace->second;
    }

    void ShowGrid( bool aEnable )
    {
        m_axis_x->SetTicks( !aEnable );
        m_axis_y1->SetTicks( !aEnable );
        m_axis_y2->SetTicks( !aEnable );
        UpdateAll();
    }

    bool IsGridShown() const
    {
        assert( m_axis_x->GetTicks() == m_axis_y1->GetTicks() );
        return !m_axis_x->GetTicks();
    }

    void ShowLegend( bool aEnable )
    {
        m_legend->SetVisible( aEnable );
        UpdateAll();
    }

    bool IsLegendShown() const
    {
        return m_legend->IsVisible();
    }

    void ShowCoords( bool aEnable )
    {
        m_coords->SetVisible( aEnable );
        UpdateAll();
    }

    bool IsCoordsShown() const
    {
        return m_coords->IsVisible();
    }

    bool HasCursorEnabled( const wxString& aName ) const;

    void EnableCursor( const wxString& aName, bool aEnable );

private:
    wxColour generateColor();

    unsigned int m_colorIdx;

    // Traces to be plotted
    std::map<wxString, TRACE*> m_traces;

    mpScaleX* m_axis_x;
    mpScaleY* m_axis_y1;
    mpScaleY* m_axis_y2;
    mpInfoLegend* m_legend;
    mpInfoCoords* m_coords;

    std::vector<mpLayer*> m_topLevel;

    const SIM_TYPE m_type;
};

wxDECLARE_EVENT( EVT_SIM_CURSOR_UPDATE, wxCommandEvent );

#endif
