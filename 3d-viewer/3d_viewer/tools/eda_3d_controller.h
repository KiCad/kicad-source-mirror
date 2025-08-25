/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#pragma once

#include <tool/tool_interactive.h>

class EDA_3D_VIEWER_FRAME;
class BOARD_ADAPTER;


/**
 * Handle view actions for various 3D canvases.
 */

class EDA_3D_CONTROLLER : public TOOL_INTERACTIVE
{
public:
    EDA_3D_CONTROLLER() :
            TOOL_INTERACTIVE( "3DViewer.Control" ),
            m_canvas( nullptr ),
            m_boardAdapter( nullptr ),
            m_camera( nullptr ),
            m_rotationIncrement( 10.0 )
    { }

    ~EDA_3D_CONTROLLER() override { }

    /// @copydoc TOOL_INTERACTIVE::Init()
    bool Init() override;

    /// @copydoc TOOL_BASE::Reset()
    void Reset( RESET_REASON aReason ) override;

    int UpdateMenu( const TOOL_EVENT& aEvent );

    int Main( const TOOL_EVENT& aEvent );

    /**
     * Set the increment used by the RotateView actions.
     *
     * @param aRotIncrement is the rotation increment in degrees
     */
    void SetRotationIncrement( double aRotIncrement ) { m_rotationIncrement = aRotIncrement; }
    double GetRotationIncrement() { return m_rotationIncrement; }

    // View controls
    int ZoomRedraw( const TOOL_EVENT& aEvent );
    int ZoomInOut( const TOOL_EVENT& aEvent );
    int ZoomInOutCenter( const TOOL_EVENT& aEvent );
    int ZoomFitScreen( const TOOL_EVENT& aEvent );

    int PanControl( const TOOL_EVENT& aEvent );
    int ViewControl( const TOOL_EVENT& aEvent );
    int RotateView( const TOOL_EVENT& aEvent );

    int On3DGridSelection( const TOOL_EVENT& aEvent );

    int SetMaterial( const TOOL_EVENT& aEvent );

    int ToggleOrtho( const TOOL_EVENT& aEvent );
    int ToggleVisibility( const TOOL_EVENT& aEvent );
    int ToggleLayersManager( const TOOL_EVENT& aEvent );

    int ReloadBoard( const TOOL_EVENT& aEvent );
    int ToggleRaytracing( const TOOL_EVENT& aEvent );
    int ExportImage( const TOOL_EVENT& aEvent );

private:
    /// Set up handlers for various events.
    void setTransitions() override;

    int doZoomInOut( bool aDirection, bool aCenterOnCursor );

private:
    EDA_3D_CANVAS* m_canvas;
    BOARD_ADAPTER* m_boardAdapter;
    CAMERA*        m_camera;
    double         m_rotationIncrement;     ///< Rotation increment for the rotate actions (degrees)
};
