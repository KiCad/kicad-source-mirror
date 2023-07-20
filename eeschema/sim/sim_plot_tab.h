/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016-2023 CERN
 * Copyright (C) 2016-2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef __SIM_PLOT_PANEL_H
#define __SIM_PLOT_PANEL_H

#include "sim_types.h"
#include <map>
#include <widgets/mathplot.h>
#include <wx/colour.h>
#include <wx/sizer.h>
#include "sim_tab.h"
#include "sim_plot_colors.h"

class SIMULATOR_FRAME;
class SIM_PLOT_TAB;
class TRACE;

/**
 *
 * The SIMULATOR_FRAME holds the main user-interface for running simulations.
 *
 * It contains a workbook with multiple tabs, each tab holding a SIM_PLOT_TAB, a specific
 * simulation command (.TRAN, .AC, etc.), and simulation settings (save all currents, etc.).
 *
 * Each plot can have multiple TRACEs.  While internally each TRACE can have multiple cursors,
 * the GUI supports only two cursors (and a differential cursor) for each plot.
 *
 * TRACEs are identified by a signal (V(OUT), I(R2), etc.) and a type (SPT_VOLTAGE, SPT_AC_PHASE,
 * etc.).
 *
 * The simulator outputs simple signals in a vector of the same name.  Complex signals (such as
 * V(OUT) / V(IN)) are stored in vectors of the format "user%d".
 *
 */


///< Cursor attached to a trace to follow its values:
class CURSOR : public mpInfoLayer
{
public:
    CURSOR( TRACE* aTrace, SIM_PLOT_TAB* aPlotTab ) :
            mpInfoLayer( wxRect( 0, 0, DRAG_MARGIN, DRAG_MARGIN ), wxTRANSPARENT_BRUSH ),
            m_trace( aTrace ),
            m_updateRequired( true ),
            m_updateRef( false ),
            m_coords( 0.0, 0.0 ),
            m_window( nullptr )
    {
    }

    void Plot( wxDC& aDC, mpWindow& aWindow ) override;

    void SetX( int aX )
    {
        m_reference.x = 0;
        m_updateRef = true;
        Move( wxPoint( aX, 0 ) );
    }

    void Update()
    {
        m_updateRequired = true;
    }

    bool Inside( const wxPoint& aPoint ) const override;

    void Move( wxPoint aDelta ) override
    {
        Update();
        mpInfoLayer::Move( aDelta );
    }

    void UpdateReference() override;

    const wxRealPoint& GetCoords() const
    {
        return m_coords;
    }

    void SetCoordX( double aValue );

private:
    void doSetCoordX( double aValue );

    wxString getID();

private:
    TRACE*       m_trace;
    bool         m_updateRequired;
    bool         m_updateRef;
    wxRealPoint  m_coords;
    mpWindow*    m_window;

    static constexpr int DRAG_MARGIN = 10;
};


class TRACE : public mpFXYVector
{
public:
    TRACE( const wxString& aName, SIM_TRACE_TYPE aType ) :
           mpFXYVector( aName ),
           m_type( aType )
    {
        SetContinuity( true );
        ShowName( false );
        SetName( aName );
    }

    void SetName( const wxString& aName ) override
    {
        for( auto& [ idx, cursor ] : m_cursors )
        {
            if( cursor )
                cursor->SetName( aName );
        }

        mpFXYVector::SetName( aName );

        if( m_type & SPT_AC_GAIN )
            m_displayName = aName + _( " (gain)" );
        else if( m_type & SPT_AC_PHASE )
            m_displayName = aName + _( " (phase)" );
        else
            m_displayName = aName;
    }

    /**
     * Assigns new data set for the trace. aX and aY need to have the same length.
     *
     * @param aX are the X axis values.
     * @param aY are the Y axis values.
     */
    void SetData( const std::vector<double>& aX, const std::vector<double>& aY ) override
    {
        for( auto& [ idx, cursor ] : m_cursors )
        {
            if( cursor )
                cursor->Update();
        }

        mpFXYVector::SetData( aX, aY );
    }

    const std::vector<double>& GetDataX() const { return m_xs; }
    const std::vector<double>& GetDataY() const { return m_ys; }

    bool HasCursor( int aCursorId ) { return m_cursors[ aCursorId ] != nullptr; }

    void SetCursor( int aCursorId, CURSOR* aCursor ) { m_cursors[ aCursorId ] = aCursor; }
    CURSOR* GetCursor( int aCursorId ) { return m_cursors[ aCursorId ]; }
    std::map<int, CURSOR*>& GetCursors() { return m_cursors; }

    SIM_TRACE_TYPE GetType() const { return m_type; }

    void SetTraceColour( const wxColour& aColour ) { m_traceColour = aColour; }
    wxColour GetTraceColour() const { return m_traceColour; }

protected:
    std::map<int, CURSOR*> m_cursors;       // No ownership; the mpWindow owns the CURSORs
    SIM_TRACE_TYPE         m_type;
    wxColour               m_traceColour;
};


class SIM_PLOT_TAB : public SIM_TAB
{
public:
    SIM_PLOT_TAB( const wxString& aSimCommand, wxWindow* parent );

    virtual ~SIM_PLOT_TAB();

    wxString GetLabelX() const
    {
        return m_axis_x ? m_axis_x->GetName() : wxString( wxS( "" ) );
    }

    wxString GetLabelY1() const
    {
        return m_axis_y1 ? m_axis_y1->GetName() : wxString( wxS( "" ) );
    }

    wxString GetLabelY2() const
    {
        return m_axis_y2 ? m_axis_y2->GetName() : wxString( wxS( "" ) );
    }

    wxString GetLabelY3() const
    {
        return m_axis_y3 ? m_axis_y3->GetName() : wxString( wxS( "" ) );
    }

    bool GetY1Scale( double* aMin, double* aMax ) const
    {
        if( m_axis_y1 )
            return m_axis_y1->GetAxisMinMax( aMin, aMax );

        return false;
    }

    bool GetY2Scale( double* aMin, double* aMax ) const
    {
        if( m_axis_y2 )
            return m_axis_y2->GetAxisMinMax( aMin, aMax );

        return false;
    }

    bool GetY3Scale( double* aMin, double* aMax ) const
    {
        if( m_axis_y3 )
            return m_axis_y3->GetAxisMinMax( aMin, aMax );

        return false;
    }

    void SetY1Scale( bool aLock, double aMin, double aMax );
    void SetY2Scale( bool aLock, double aMin, double aMax );
    void SetY3Scale( bool aLock, double aMin, double aMax );

    wxString GetUnitsX() const;
    wxString GetUnitsY1() const;
    wxString GetUnitsY2() const;
    wxString GetUnitsY3() const;

    const std::map<wxString, TRACE*>& GetTraces() const
    {
        return m_traces;
    }

    TRACE* GetTrace( const wxString& aVecName, int aType ) const
    {
        auto trace = m_traces.find( getTraceId( aVecName, aType ) );

        return trace == m_traces.end() ? nullptr : trace->second;
    }

    void ShowGrid( bool aEnable )
    {
        if( m_axis_x )
            m_axis_x->SetTicks( !aEnable );

        if( m_axis_y1 )
            m_axis_y1->SetTicks( !aEnable );

        if( m_axis_y2 )
            m_axis_y2->SetTicks( !aEnable );

        if( m_axis_y3 )
            m_axis_y3->SetTicks( !aEnable );

        m_plotWin->UpdateAll();
    }

    bool IsGridShown() const
    {
        if( !m_axis_x || !m_axis_y1 )
            return false;

        assert( m_axis_x->GetTicks() == m_axis_y1->GetTicks() );
        return !m_axis_x->GetTicks();
    }

    void ShowLegend( bool aEnable )
    {
        m_legend->SetVisible( aEnable );
        m_plotWin->UpdateAll();
    }

    bool IsLegendShown() const
    {
        return m_legend->IsVisible();
    }

    wxPoint GetLegendPosition() const
    {
        return m_legend->GetPosition();
    }

    void SetLegendPosition( const wxPoint& aPosition )
    {
        m_legend->Move( aPosition );
        m_legend->UpdateReference();
    }

    /**
     * Draw secondary signal traces (current or phase) with dotted lines
     */
    void SetDottedSecondary( bool aEnable )
    {
        m_dotted_cp = aEnable;

        for( const auto& [ name, trace ] : m_traces )
            UpdateTraceStyle( trace );

        m_plotWin->UpdateAll();
    }

    bool GetDottedSecondary() const
    {
        return m_dotted_cp;
    }

    ///< Toggle cursor for a particular trace.
    void EnableCursor( const wxString& aVectorName, int aType, int aCursorId, bool aEnable,
                       const wxString& aSignalName );

    ///< Reset scale ranges to fit the current traces.
    void ResetScales( bool aIncludeX );

    ///< Update trace line style
    void UpdateTraceStyle( TRACE* trace );

    ///< Update plot colors
    void UpdatePlotColors();

    void OnLanguageChanged() override;

    ///< Getter for math plot window
    mpWindow* GetPlotWin() const { return m_plotWin; }

    TRACE* AddTrace( const wxString& aVectorName, int aType );

    void SetTraceData( TRACE* aTrace, std::vector<double>& aX, std::vector<double>& aY );

    bool DeleteTrace( const wxString& aVectorName, int aTraceType );
    void DeleteTrace( TRACE* aTrace );

    std::vector<std::pair<wxString, wxString>>& Measurements() { return m_measurements; }

private:
    wxString getTraceId( const wxString& aVectorName, int aType ) const
    {
        return wxString::Format( wxS( "%s%d" ), aVectorName, aType & SPT_Y_AXIS_MASK );
    }

    ///< @brief Construct the plot axes for DC simulation plot.
    void prepareDCAxes( int aNewTraceType );

    ///< Create/Ensure axes are available for plotting
    void updateAxes( int aNewTraceType = SIM_TRACE_TYPE::SPT_UNKNOWN );

private:
    SIM_PLOT_COLORS            m_colors;

    // Top-level plot window
    mpWindow*                  m_plotWin;
    wxBoxSizer*                m_sizer;

    // Traces to be plotted
    std::map<wxString, TRACE*> m_traces;

    mpScaleXBase*              m_axis_x;
    mpScaleY*                  m_axis_y1;
    mpScaleY*                  m_axis_y2;
    mpScaleY*                  m_axis_y3;
    mpInfoLegend*              m_legend;

    bool                       m_dotted_cp;

    // Measurements (and their format strings)
    std::vector<std::pair<wxString, wxString>> m_measurements;
};

wxDECLARE_EVENT( EVT_SIM_CURSOR_UPDATE, wxCommandEvent );

#endif
