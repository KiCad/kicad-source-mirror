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

#ifndef __SIM_PLOT_PANEL_H
#define __SIM_PLOT_PANEL_H

#include "sim_types.h"
#include <map>
#include <limits>
#include <widgets/mathplot.h>
#include <math/util.h>
#include <wx/colour.h>
#include <wx/sizer.h>
#include "sim_tab.h"
#include "smith_math.h"
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
            m_window( nullptr ),
            m_sweepIndex( 0 ),
            m_snapToNearest( false ),
            m_snapTargetY( 0.0 )
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

    void Move( wxPoint aDelta ) override;

    void UpdateReference() override;

    bool OnDoubleClick( const wxPoint& aPoint, mpWindow& aWindow ) override;

    const wxRealPoint& GetCoords() const
    {
        return m_coords;
    }

    virtual void SetCoordX( double aValue );

private:
    void doSetCoordX( double aValue );

protected:
    wxString getID();

protected:
    TRACE*       m_trace;
    bool         m_updateRequired;
    bool         m_updateRef;
    wxRealPoint  m_coords;
    mpWindow*    m_window;
    int          m_sweepIndex;
    bool         m_snapToNearest;
    double       m_snapTargetY;

    static constexpr int DRAG_MARGIN = 10;
};


class TRACE : public mpFXYVector
{
public:
    TRACE( const wxString& aName, SIM_TRACE_TYPE aType ) :
           mpFXYVector( aName ),
           m_type( aType ),
           m_isMultiRun( false )
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

    void SetIsMultiRun( bool aIsMultiRun ) { m_isMultiRun = aIsMultiRun; }
    bool IsMultiRun() const { return m_isMultiRun; }

    void SetMultiRunLabels( const std::vector<wxString>& aLabels ) { m_multiRunLabels = aLabels; }
    const std::vector<wxString>& GetMultiRunLabels() const { return m_multiRunLabels; }

protected:
    std::map<int, CURSOR*> m_cursors;       // No ownership; the mpWindow owns the CURSORs
    SIM_TRACE_TYPE         m_type;
    wxColour               m_traceColour;
    bool                   m_isMultiRun;
    std::vector<wxString>  m_multiRunLabels;
};


///< Overlay layer drawing the Smith chart grid (constant resistance and reactance circles)
class SMITH_GRID : public mpLayer
{
public:
    SMITH_GRID() { m_type = mpLAYER_AXIS; }

    void Plot( wxDC& aDC, mpWindow& aWindow ) override;

    bool HasBBox() const override { return false; }

    ///< Zero when no single reference applies, which labels the grid normalized.
    void SetReferenceImpedance( double aZ0 ) { m_z0 = aZ0; }

    void SetMixedReferences( bool aMixed ) { m_mixedReferences = aMixed; }

    ///< Chart placement including pan/zoom.
    static bool GetChartView( mpWindow& aWindow, double aZoom, const wxRealPoint& aPan, SMITH_VIEW& aView );

private:
    double m_z0 = 0.0;
    bool   m_mixedReferences = false;
};


///< Reflection coefficient locus, Re in X and Im in Y, drawn on the Smith chart
class SMITH_TRACE : public TRACE
{
public:
    SMITH_TRACE( const wxString& aName, SIM_TRACE_TYPE aType ) :
            TRACE( aName, aType )
    {
    }

    void Plot( wxDC& aDC, mpWindow& aWindow ) override;

    // drawn directly, keep the locus out of the axis auto-fit
    bool HasBBox() const override { return false; }

    void                       SetFrequencies( const std::vector<double>& aFreqs ) { m_frequencies = aFreqs; }
    const std::vector<double>& GetFrequencies() const { return m_frequencies; }

    ///< Zero until the response port resolves one, which leaves only normalized values readable.
    void   SetReferenceImpedance( double aZ0 ) { m_z0 = aZ0; }
    double GetReferenceImpedance() const { return m_z0; }

private:
    std::vector<double> m_frequencies;
    double              m_z0 = 0.0;
};


///< Cursor that snaps along a Smith chart locus, keyed by frequency.
///< m_coords holds ( frequency, gamma magnitude ) so the cursor grid and workbook still work.
class SMITH_CURSOR : public CURSOR
{
public:
    SMITH_CURSOR( SMITH_TRACE* aTrace, SIM_PLOT_TAB* aPlotTab ) :
            CURSOR( aTrace, aPlotTab ),
            m_index( -1 ),
            m_gamma( 0.0, 0.0 ),
            m_pendingFreq( false ),
            m_dragging( false )
    {
    }

    void Plot( wxDC& aDC, mpWindow& aWindow ) override;

    bool Inside( const wxPoint& aPoint ) const override;

    void Move( wxPoint aDelta ) override;

    void UpdateReference() override;

    void SetCoordX( double aValue ) override;

private:
    void snapToIndex( int aIndex );
    void snapToFrequency( double aFreq );

private:
    int         m_index;
    wxRealPoint m_gamma;
    bool        m_pendingFreq; // a saved frequency waiting for the sim data to load
    bool        m_dragging;    // the pending update comes from a drag, not a data refresh
};


///< Trace hidden while the tab is in Smith mode, kept so leaving the mode restores it.
struct SMITH_STASHED_TRACE
{
    wxString vectorName;
    wxString displayName;
    int      baseType;
};


///< Cursor recorded when entering Smith mode, so leaving restores it to its original trace.
struct SMITH_STASHED_CURSOR
{
    int      id;
    wxString vectorName;
    int      baseType;
    int      subType; // the SP subtype bit the cursor lived on before Smith mode
    double   frequency;
};


class SIM_PLOT_TAB : public SIM_TAB
{
public:
    SIM_PLOT_TAB( const wxString& aSimCommand, wxWindow* parent );

    virtual ~SIM_PLOT_TAB();

    void ApplyPreferences( const SIM_PREFERENCES& aPrefs ) override
    {
        m_plotWin->SetMouseWheelActions( convertMouseWheelActions( aPrefs.mouse_wheel_actions ) );
    }

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
        m_LastLegendPosition = aPosition;
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

    void SetSmithMode( bool aEnable );
    bool IsSmithMode() const { return m_smithMode; }

    ///< Refresh the grid z0 from the shown Smith traces.
    void UpdateSmithReferenceImpedance();

    double             GetSmithZoom() const { return m_smithZoom; }
    const wxRealPoint& GetSmithPan() const { return m_smithPan; }

    void ResetSmithView()
    {
        m_smithZoom = 1.0;
        m_smithPan = wxRealPoint( 0.0, 0.0 );
    }

    ///< Restore a saved view, values are validated and clamped.
    void SetSmithView( double aZoom, double aPanX, double aPanY )
    {
        m_smithZoom = std::isfinite( aZoom ) ? std::clamp( aZoom, 1.0, 50.0 ) : 1.0;
        m_smithPan.x = std::isfinite( aPanX ) ? std::clamp( aPanX, -100.0, 100.0 ) : 0.0;
        m_smithPan.y = std::isfinite( aPanY ) ? std::clamp( aPanY, -100.0, 100.0 ) : 0.0;
    }

    void SmithZoomAt( const wxPoint& aPos, double aFactor );
    void SmithPanBy( const wxPoint& aDelta );

    ///< Traces and cursors set aside while in Smith mode, restored when leaving it.
    std::vector<SMITH_STASHED_TRACE>&  SmithStashedTraces() { return m_smithStashedTraces; }
    std::vector<SMITH_STASHED_CURSOR>& SmithStashedCursors() { return m_smithStashedCursors; }

    ///< Turn on/off the cursor for a particular trace.
    void EnableCursor( TRACE* aTrace, int aCursorId, const wxString& aSignalName );
    void DisableCursor( TRACE* aTrace, int aCursorId );

    ///< Reset scale ranges to fit the current traces.
    void ResetScales( bool aIncludeX );

    ///< Update trace line style
    void UpdateTraceStyle( TRACE* trace );

    ///< Update plot colors
    void UpdatePlotColors();

    void OnLanguageChanged() override;

    ///< Getter for math plot window
    mpWindow* GetPlotWin() const { return m_plotWin; }

    TRACE* GetOrAddTrace( const wxString& aVectorName, int aType );

    void SetTraceData( TRACE* aTrace, std::vector<double>& aX, std::vector<double>& aY,
                       int aSweepCount, size_t aSweepSize, bool aIsMultiRun = false,
                       const std::vector<wxString>& aMultiRunLabels = {} );

    bool DeleteTrace( const wxString& aVectorName, int aTraceType );
    void DeleteTrace( TRACE* aTrace );

    std::vector<std::pair<wxString, wxString>>& Measurements() { return m_measurements; }

    void EnsureThirdYAxisExists();

public:
    wxPoint m_LastLegendPosition;

private:
    static mpWindow::MouseWheelActionSet
    convertMouseWheelActions( const SIM_MOUSE_WHEEL_ACTION_SET& s )
    {
        static_assert( static_cast<unsigned>( mpWindow::MouseWheelAction::COUNT )
                               == static_cast<unsigned>( SIM_MOUSE_WHEEL_ACTION::COUNT ),
                       "mpWindow::MouseWheelAction enum must match SIM_MOUSE_WHEEL_ACTION" );

        using A = mpWindow::MouseWheelAction;
        mpWindow::MouseWheelActionSet m;
        m.verticalUnmodified = static_cast<A>( s.vertical_unmodified );
        m.verticalWithCtrl   = static_cast<A>( s.vertical_with_ctrl );
        m.verticalWithShift  = static_cast<A>( s.vertical_with_shift );
        m.verticalWithAlt    = static_cast<A>( s.vertical_with_alt );
        m.horizontal         = static_cast<A>( s.horizontal );

        return m;
    }

    wxString getTraceId( const wxString& aVectorName, int aType ) const
    {
        return wxString::Format( wxS( "%s%d" ), aVectorName, aType & SPT_Y_AXIS_MASK );
    }

    ///< @brief Construct the plot axes for DC simulation plot.
    void prepareDCAxes( int aNewTraceType );

    ///< Create/Ensure axes are available for plotting
    void updateAxes( int aNewTraceType = SIM_TRACE_TYPE::SPT_UNKNOWN );

    void UpdateAxisVisibility();

    void onSmithMouseWheel( wxMouseEvent& aEvent );
    void onSmithMagnify( wxMouseEvent& aEvent );
    void onSmithMiddleDown( wxMouseEvent& aEvent );
    void onSmithLeftDown( wxMouseEvent& aEvent );
    void onSmithMotion( wxMouseEvent& aEvent );
    void onSmithLeftUp( wxMouseEvent& aEvent );
    void onSmithDClick( wxMouseEvent& aEvent );
    void onSmithRightDown( wxMouseEvent& aEvent );
    void onSmithRightUp( wxMouseEvent& aEvent );
    void onSmithMenuCommand( wxCommandEvent& aEvent );

    bool getSmithView( SMITH_VIEW& aView ) const;

private:
    SIM_PLOT_COLORS              m_colors;
    std::map<wxString, wxColour> m_sessionTraceColors;

    // Top-level plot window
    mpWindow*                    m_plotWin;
    wxBoxSizer*                  m_sizer;

    // Traces to be plotted
    std::map<wxString, TRACE*>   m_traces;

    mpScaleXBase*                m_axis_x;
    mpScaleY*                    m_axis_y1;
    mpScaleY*                    m_axis_y2;
    mpScaleY*                    m_axis_y3;
    mpInfoLegend*                m_legend;
    SMITH_GRID*                  m_smithGrid;

    bool                         m_dotted_cp;
    bool                         m_smithMode;
    double                       m_smithZoom;
    wxRealPoint                  m_smithPan;
    bool                         m_smithPanning;
    bool                         m_smithLeftSkipped;
    wxPoint                      m_smithPanLast;
    wxPoint                      m_smithMenuPos;

    std::vector<SMITH_STASHED_TRACE>  m_smithStashedTraces;
    std::vector<SMITH_STASHED_CURSOR> m_smithStashedCursors;

    // Measurements (and their format strings)
    std::vector<std::pair<wxString, wxString>> m_measurements;
};

wxDECLARE_EVENT( EVT_SIM_CURSOR_UPDATE, wxCommandEvent );

#endif
