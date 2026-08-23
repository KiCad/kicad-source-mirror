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
#include <vector>
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
class SIM_VIEW;
class TRACE;

/**
 *
 * The SIMULATOR_FRAME holds the main user-interface for running simulations.
 *
 * It contains a workbook with multiple tabs, each tab holding a SIM_PLOT_TAB, a specific
 * simulation command (.TRAN, .AC, etc.), and simulation settings (save all currents, etc.).
 *
 * Each SIM_PLOT_TAB holds one or more SIM_VIEWs, stacked vertically.  All the views of a tab
 * share the same (synchronized) X axis, but each has its own independent Y axis scaling, so
 * signals with very different magnitudes can be routed to separate views.  Each TRACE belongs
 * to exactly one SIM_VIEW (or none, if the signal isn't currently plotted).  While internally
 * each TRACE can have multiple cursors, the GUI supports only two cursors (and a differential
 * cursor) for each plot.
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

    virtual void UpdateForNewData() { SetCoordX( m_coords.x ); }

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
            m_isMultiRun( false ),
            m_view( nullptr ),
            m_yScaleView( nullptr )
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

    ///< The SIM_VIEW this trace is currently plotted on (nullptr if not plotted anywhere).
    SIM_VIEW* GetView() const { return m_view; }
    void      SetView( SIM_VIEW* aView ) { m_view = aView; }

    ///< The view whose Y-axis scale this trace's axis should match (defaults to its own view).
    ///< When set to a view other than its own, this trace's Y-axis range is merged with that
    ///< of every other trace targeting the same view/axis, as if they were all plotted together.
    SIM_VIEW* GetYScaleView() const { return m_yScaleView ? m_yScaleView : m_view; }
    void      SetYScaleView( SIM_VIEW* aView ) { m_yScaleView = ( aView == m_view ) ? nullptr : aView; }

    ///< True if this trace's Y-axis scale hasn't been explicitly linked to another view (i.e.
    ///< it defaults to its own view's scale).
    bool IsYScaleDefault() const { return m_yScaleView == nullptr; }

    ///< Clears an explicit Y-scale link if it pointed at a view that no longer exists.
    void ClearYScaleViewIf( SIM_VIEW* aView )
    {
        if( m_yScaleView == aView )
            m_yScaleView = nullptr;
    }

protected:
    std::map<int, CURSOR*> m_cursors;       // No ownership; the mpWindow owns the CURSORs
    SIM_TRACE_TYPE         m_type;
    wxColour               m_traceColour;
    bool                   m_isMultiRun;
    std::vector<wxString>  m_multiRunLabels;
    SIM_VIEW*              m_view;       // No ownership; the SIM_PLOT_TAB owns the SIM_VIEWs
    SIM_VIEW*              m_yScaleView; // No ownership; nullptr means "same as m_view"
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
            m_requestFreq( 0.0 ),
            m_pendingFreq( false ),
            m_dragging( false )
    {
    }

    void Plot( wxDC& aDC, mpWindow& aWindow ) override;

    bool Inside( const wxPoint& aPoint ) const override;

    void Move( wxPoint aDelta ) override;

    void UpdateReference() override;

    void SetCoordX( double aValue ) override;

    void UpdateForNewData() override;

    const wxRealPoint& GetGamma() const { return m_gamma; }

private:
    void snapToIndex( int aIndex );
    void snapToFrequency( double aFreq );

private:
    int         m_index;
    wxRealPoint m_gamma;
    double      m_requestFreq;
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


/**
 * A single stacked plot area within a SIM_PLOT_TAB.
 *
 * All the SIM_VIEWs of a tab share the same (synchronized) X axis range, but each has its own
 * independent set of Y axes (voltage/current/power), legend, and grid/data-range state.
 */
class SIM_VIEW : public mpWindow
{
public:
    SIM_VIEW( SIM_PLOT_TAB* aPlotTab, wxWindow* aParent );

    ///< Mirrors this view's X range onto every other view of the same tab.
    void OnXViewChanged() override;

    ///< Directly set this view's X range (used to mirror another view's zoom/pan onto this one).
    void SetXRange( double aPos, double aDesiredMax, double aDesiredMin )
    {
        SetXView( aPos, aDesiredMax, aDesiredMin );
    }

    SIM_PLOT_TAB* GetPlotTab() const { return m_plotTab; }

    wxString GetLabelX() const { return m_axis_x ? m_axis_x->GetName() : wxString( wxS( "" ) ); }

    wxString GetLabelY1() const { return m_axis_y1 ? m_axis_y1->GetName() : wxString( wxS( "" ) ); }

    wxString GetLabelY2() const { return m_axis_y2 ? m_axis_y2->GetName() : wxString( wxS( "" ) ); }

    wxString GetLabelY3() const { return m_axis_y3 ? m_axis_y3->GetName() : wxString( wxS( "" ) ); }

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

    ///< Get the display units (e.g. "V", "A", "dB") for a given trace plotted on this view.
    wxString GetUnitsForTrace( TRACE* aTrace ) const;

    ///< Get the Y-axis slot (1, 2 or 3) a trace of this type is plotted on.
    int GetAxisSlot( TRACE* aTrace ) const;

    ///< Get the Y-axis scale object for a given slot (1, 2 or 3), or nullptr if not created yet.
    mpScaleY* GetAxisBySlot( int aSlot ) const;

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

        UpdateAll();
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
        UpdateAll();
    }

    bool IsLegendShown() const { return m_legend->IsVisible(); }

    wxPoint GetLegendPosition() const { return m_legend->GetPosition(); }

    void SetLegendPosition( const wxPoint& aPosition )
    {
        m_legend->Move( aPosition );
        m_legend->UpdateReference();
        m_lastLegendPosition = aPosition;
    }

    ///< Reset the Y (and, if requested, X) scale ranges to fit this view's own traces.
    void ResetScales( bool aIncludeX );

    ///< Create/Ensure axes are available for plotting.
    void updateAxes( int aNewTraceType = SIM_TRACE_TYPE::SPT_UNKNOWN );

    void UpdateAxisVisibility();

    void EnsureThirdYAxisExists();

    // ---- Smith chart --------------------------------------------------------------------

    ///< Show/hide this view's Smith chart overlay.  The mode itself is owned by the tab; each
    ///< view keeps its own grid layer and pan/zoom so the stacked charts stay independent.
    void SetSmithChart( bool aEnable );

    bool IsSmithChart() const { return m_smithChart; }

    ///< Refresh the grid z0 from the Smith traces shown on this view.
    void UpdateSmithReferenceImpedance();

    ///< Re-read the Smith grid pen from the tab's color theme.
    void UpdateSmithGridColor();

    double             GetSmithZoom() const { return m_smithZoom; }
    const wxRealPoint& GetSmithPan() const { return m_smithPan; }

    void ResetSmithView()
    {
        m_smithZoom = 1.0;
        m_smithPan = wxRealPoint( 0.0, 0.0 );
    }

    void SetSmithView( double aZoom, double aPanX, double aPanY )
    {
        m_smithZoom = std::isfinite( aZoom ) ? std::clamp( aZoom, 1.0, 50.0 ) : 1.0;
        m_smithPan.x = std::isfinite( aPanX ) ? std::clamp( aPanX, -100.0, 100.0 ) : 0.0;
        m_smithPan.y = std::isfinite( aPanY ) ? std::clamp( aPanY, -100.0, 100.0 ) : 0.0;
    }

    void SmithZoomAt( const wxPoint& aPos, double aFactor );
    void SmithPanBy( const wxPoint& aDelta );

public:
    wxPoint m_lastLegendPosition;

    mpScaleXBase* m_axis_x;
    mpScaleY*     m_axis_y1;
    mpScaleY*     m_axis_y2;
    mpScaleY*     m_axis_y3;
    mpInfoLegend* m_legend;

private:
    ///< @brief Construct the plot axes for DC simulation plot.
    void prepareDCAxes( int aNewTraceType );

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
    SIM_PLOT_TAB* m_plotTab;

    SMITH_GRID* m_smithGrid;

    bool        m_smithChart;
    double      m_smithZoom;
    wxRealPoint m_smithPan;
    bool        m_smithPanning;
    bool        m_smithLeftSkipped;
    wxPoint     m_smithPanLast;
    wxPoint     m_smithMenuPos;
};


class SIM_PLOT_TAB : public SIM_TAB
{
public:
    SIM_PLOT_TAB( const wxString& aSimCommand, wxWindow* parent );

    virtual ~SIM_PLOT_TAB();

    void ApplyPreferences( const SIM_PREFERENCES& aPrefs ) override;

    // ---- View management ----------------------------------------------------------------

    const std::vector<SIM_VIEW*>& GetViews() const { return m_views; }

    SIM_VIEW* GetView( int aIndex ) const
    {
        return ( aIndex >= 0 && aIndex < (int) m_views.size() ) ? m_views[aIndex] : nullptr;
    }

    int GetViewIndex( SIM_VIEW* aView ) const;

    int GetViewCount() const { return (int) m_views.size(); }

    ///< The view new traces default to, and legacy (single-view) settings apply to.
    SIM_VIEW* GetDefaultView() const { return m_views.empty() ? nullptr : m_views[0]; }

    ///< Add a new (empty) view, stacked below the existing ones.
    SIM_VIEW* AddView();

    ///< Remove a view.  Any traces plotted on it are unplotted (not deleted from the grid).
    ///< The last remaining view cannot be removed.
    bool RemoveView( SIM_VIEW* aView );

    ///< Mirror aSource's current X range onto every other view of this tab.
    void SyncXView( SIM_VIEW* aSource );

    // ---- Tab-wide accessors (identical across views; delegate to the default view) -------

    wxString GetLabelX() const;
    wxString GetUnitsX() const;

    ///< Get the display units (e.g. "V", "A", "dB") for a given trace's own view.
    wxString GetUnitsForTrace( TRACE* aTrace ) const;

    // ---- Settings shared/fanned out across all views --------------------------------------

    void ShowGrid( bool aEnable );
    bool IsGridShown() const;

    void    ShowLegend( bool aEnable );
    bool    IsLegendShown() const;
    wxPoint GetLegendPosition() const;
    void    SetLegendPosition( const wxPoint& aPosition );

    // ---- Default-view convenience (used by the Analysis Properties dialog) ----------------

    wxString GetLabelY1() const;
    wxString GetLabelY2() const;
    wxString GetLabelY3() const;
    wxString GetUnitsY1() const;
    wxString GetUnitsY2() const;
    wxString GetUnitsY3() const;

    bool GetY1Scale( double* aMin, double* aMax ) const;
    bool GetY2Scale( double* aMin, double* aMax ) const;
    bool GetY3Scale( double* aMin, double* aMax ) const;
    void SetY1Scale( bool aLock, double aMin, double aMax );
    void SetY2Scale( bool aLock, double aMin, double aMax );
    void SetY3Scale( bool aLock, double aMin, double aMax );

    void EnsureThirdYAxisExists();

    /**
     * Draw secondary signal traces (current or phase) with dotted lines
     */
    void SetDottedSecondary( bool aEnable )
    {
        m_dotted_cp = aEnable;

        for( const auto& [name, trace] : m_traces )
            UpdateTraceStyle( trace );

        for( SIM_VIEW* view : m_views )
            view->UpdateAll();
    }

    bool GetDottedSecondary() const { return m_dotted_cp; }

    ///< Smith mode is tab-wide: every view of the tab shows a Smith chart while it is on.
    void SetSmithMode( bool aEnable );
    bool IsSmithMode() const { return m_smithMode; }

    ///< Refresh the grid z0 from the shown Smith traces, on every view.
    void UpdateSmithReferenceImpedance();

    ///< The saved/restored pan and zoom track the default view.
    double      GetSmithZoom() const;
    wxRealPoint GetSmithPan() const;

    void ResetSmithView();

    ///< Restore a saved view, values are validated and clamped.
    void SetSmithView( double aZoom, double aPanX, double aPanY );

    void SmithZoomAt( const wxPoint& aPos, double aFactor );
    void SmithPanBy( const wxPoint& aDelta );

    ///< Traces and cursors set aside while in Smith mode, restored when leaving it.
    std::vector<SMITH_STASHED_TRACE>&  SmithStashedTraces() { return m_smithStashedTraces; }
    std::vector<SMITH_STASHED_CURSOR>& SmithStashedCursors() { return m_smithStashedCursors; }

    ///< Turn on/off the cursor for a particular trace.
    void EnableCursor( TRACE* aTrace, int aCursorId, const wxString& aSignalName );
    void DisableCursor( TRACE* aTrace, int aCursorId );

    ///< Reset scale ranges to fit the current traces, on every view.
    void ResetScales( bool aIncludeX );

    ///< Update trace line style
    void UpdateTraceStyle( TRACE* trace );

    ///< Update plot colors
    void UpdatePlotColors();

    wxColour GetPlotColor( SIM_PLOT_COLORS::COLOR_SET aColorId ) { return m_colors.GetPlotColor( aColorId ); }

    void OnLanguageChanged() override;

    ///< Getter for the default view's math plot window (back-compat for zoom undo/redo, export).
    mpWindow* GetPlotWin() const;

    ///< Get (or create, on aView) the TRACE for a signal.  If the trace already exists (on any
    ///< view), it is returned unchanged -- callers wanting to move an existing trace to a
    ///< different view should DeleteTrace() it first.
    TRACE* GetOrAddTrace( const wxString& aVectorName, int aType, SIM_VIEW* aView );

    void SetTraceData( TRACE* aTrace, std::vector<double>& aX, std::vector<double>& aY, int aSweepCount,
                       size_t aSweepSize, bool aIsMultiRun = false, const std::vector<wxString>& aMultiRunLabels = {} );

    bool DeleteTrace( const wxString& aVectorName, int aTraceType );
    void DeleteTrace( TRACE* aTrace );

    const std::map<wxString, TRACE*>& GetTraces() const { return m_traces; }

    TRACE* GetTrace( const wxString& aVecName, int aType ) const
    {
        auto trace = m_traces.find( getTraceId( aVecName, aType ) );

        return trace == m_traces.end() ? nullptr : trace->second;
    }

    std::vector<std::pair<wxString, wxString>>& Measurements() { return m_measurements; }

public:
    wxPoint m_LastLegendPosition;

private:
    static mpWindow::MouseWheelActionSet convertMouseWheelActions( const SIM_MOUSE_WHEEL_ACTION_SET& s )
    {
        static_assert( static_cast<unsigned>( mpWindow::MouseWheelAction::COUNT )
                               == static_cast<unsigned>( SIM_MOUSE_WHEEL_ACTION::COUNT ),
                       "mpWindow::MouseWheelAction enum must match SIM_MOUSE_WHEEL_ACTION" );

        using A = mpWindow::MouseWheelAction;
        mpWindow::MouseWheelActionSet m;
        m.verticalUnmodified = static_cast<A>( s.vertical_unmodified );
        m.verticalWithCtrl = static_cast<A>( s.vertical_with_ctrl );
        m.verticalWithShift = static_cast<A>( s.vertical_with_shift );
        m.verticalWithAlt = static_cast<A>( s.vertical_with_alt );
        m.horizontal = static_cast<A>( s.horizontal );

        return m;
    }

    wxString getTraceId( const wxString& aVectorName, int aType ) const
    {
        return wxString::Format( wxS( "%s%d" ), aVectorName, aType & SPT_Y_AXIS_MASK );
    }

private:
    SIM_PLOT_COLORS               m_colors;
    std::map<wxString, wxColour>  m_sessionTraceColors;

    wxBoxSizer*                   m_viewsSizer;      // Holds all the stacked SIM_VIEWs

    // Stacked plot views (owned via wx parent-child, except when explicitly removed)
    std::vector<SIM_VIEW*>        m_views;

    // Cached so newly-added views can be initialized with the current preferences
    mpWindow::MouseWheelActionSet m_mouseWheelActions;

    // Traces to be plotted; each knows which view (if any) it's currently plotted on
    std::map<wxString, TRACE*>    m_traces;

    bool m_dotted_cp;

    // Re-entrancy guard for SyncXView()
    bool m_syncingXView;

    // Smith-chart mode is a property of the whole tab; each SIM_VIEW carries its own grid
    // layer and pan/zoom state.
    bool m_smithMode;

    std::vector<SMITH_STASHED_TRACE>  m_smithStashedTraces;
    std::vector<SMITH_STASHED_CURSOR> m_smithStashedCursors;

    // Measurements (and their format strings)
    std::vector<std::pair<wxString, wxString>> m_measurements;
};

wxDECLARE_EVENT( EVT_SIM_CURSOR_UPDATE, wxCommandEvent );

///< Fired whenever a view is added to or removed from a SIM_PLOT_TAB, so the Signals grid can
///< refresh its "Plot" column dropdown choices.
wxDECLARE_EVENT( EVT_SIM_VIEWS_CHANGED, wxCommandEvent );

#endif
