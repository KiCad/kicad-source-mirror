/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014-2016 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
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

#ifndef _COMMON_TOOLS_H
#define _COMMON_TOOLS_H

#include <tool/tool_interactive.h>

class EDA_DRAW_FRAME;

/**
 * Handles action that are shared between different applications.
 */
class COMMON_TOOLS : public TOOL_INTERACTIVE
{
public:
    COMMON_TOOLS();

    ~COMMON_TOOLS() override { }

    /// @copydoc TOOL_BASE::Reset()
    void Reset( RESET_REASON aReason ) override;

    int SelectionTool( const TOOL_EVENT& aEvent );

    // View controls
    int ZoomRedraw( const TOOL_EVENT& aEvent );
    int ZoomInOut( const TOOL_EVENT& aEvent );
    int ZoomInOutCenter( const TOOL_EVENT& aEvent );
    int ZoomCenter( const TOOL_EVENT& aEvent );
    int ZoomFitScreen( const TOOL_EVENT& aEvent );
    int ZoomFitObjects( const TOOL_EVENT& aEvent );
    int ZoomFitSelection( const TOOL_EVENT& aEvent );
    int ZoomPreset( const TOOL_EVENT& aEvent );

    int CenterContents( const TOOL_EVENT& aEvent );
    int CenterSelection( const TOOL_EVENT& aEvent );

    int PanControl( const TOOL_EVENT& aEvent );

    // Cursor control
    int CursorControl( const TOOL_EVENT& aEvent );
    int ToggleCursor( const TOOL_EVENT& aEvent );
    int CursorSmallCrosshairs( const TOOL_EVENT& aEvent );
    int CursorFullCrosshairs( const TOOL_EVENT& aEvent );
    int Cursor45Crosshairs( const TOOL_EVENT& aEvent );

    int ToggleBoundingBoxes( const TOOL_EVENT& aEvent );

    // Units control
    int SwitchUnits( const TOOL_EVENT& aEvent );
    int ToggleUnits( const TOOL_EVENT& aEvent );
    int TogglePolarCoords( const TOOL_EVENT& aEvent );
    int ResetLocalCoords( const TOOL_EVENT& aEvent );

    void SetLastUnits( EDA_UNITS aUnit );
    EDA_UNITS GetLastMetricUnits() { return m_metricUnit; }
    EDA_UNITS GetLastImperialUnits() { return m_imperialUnit; }

    // Grid control
    int GridNext( const TOOL_EVENT& aEvent );
    int GridPrev( const TOOL_EVENT& aEvent );
    int GridPreset( const TOOL_EVENT& aEvent );
    int GridFast1( const TOOL_EVENT& aEvent );
    int GridFast2( const TOOL_EVENT& aEvent );
    int GridFastCycle( const TOOL_EVENT& aEvent );
    int ToggleGrid( const TOOL_EVENT& aEvent );
    int ToggleGridOverrides( const TOOL_EVENT& aEvent );
    int GridProperties( const TOOL_EVENT& aEvent );
    int GridPreset( int idx, bool aFromHotkey );
    int GridOrigin( const TOOL_EVENT& aEvent );
    int OnGridChanged( bool aFromHotkey );

    const std::vector<VECTOR2I> Grids() const { return m_grids; }

private:
    /**
     * The set of "Zoom to Fit" types that can be performed.
     */
    enum ZOOM_FIT_TYPE_T
    {
        ZOOM_FIT_ALL,       ///< Zoom to fall all items in view INCLUDING page and border
        ZOOM_FIT_OBJECTS,   ///< Zoom to fit all items in view EXCLUDING page and border
        ZOOM_FIT_SELECTION, ///< Zoom to fit selected items in view
    };

    enum class CENTER_TYPE
    {
        CENTER_CONTENTS,
        CENTER_SELECTION,
    };

    ///< Sets up handlers for various events.
    void setTransitions() override;

    ///< Pointer to the currently used edit frame.
    EDA_DRAW_FRAME* m_frame;

    int doZoomInOut( bool aDirection, bool aCenterOnCursor );

    ///< Note: idx == 0 is Auto; idx == 1 is first entry in zoomList
    int doZoomToPreset( int idx, bool aCenterOnCursor );

    int doZoomFit( ZOOM_FIT_TYPE_T aFitType );

    int doCenter( CENTER_TYPE aCenterType );

    std::vector<VECTOR2I> m_grids;  ///< Grids from #APP_SETTINGS converted to internal units
                                    ///< and with the user grid appended.

    // The last used units in each system (used for toggling between metric and imperial)
    EDA_UNITS m_imperialUnit;
    EDA_UNITS m_metricUnit;
};

#endif
