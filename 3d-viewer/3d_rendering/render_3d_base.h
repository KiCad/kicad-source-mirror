/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2016 Mario Luzeiro <mrluzeiro@ua.pt>
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

/**
 * @file render_3d_base.h
 */

#ifndef RENDER_3D_BASE_H
#define RENDER_3D_BASE_H


#include <pcb_base_frame.h>
#include "3d_canvas/board_adapter.h"
#include <reporter.h>

#include <widgets/busy_indicator.h>

/**
 *  This is a base class to hold data and functions for render targets.
 */
class RENDER_3D_BASE
{
public:
    explicit RENDER_3D_BASE( BOARD_ADAPTER& aBoardAdapter, CAMERA& aCamera );

    virtual ~RENDER_3D_BASE() = 0;

    /**
     * Before each render, the canvas will tell the render what is the size of its windows,
     * so render can take actions if it changed.
     *
     * @param aSize the current size of the render window
     */
    virtual void SetCurWindowSize( const wxSize& aSize ) = 0;

    /**
     * Redraw the view.
     *
     * @param aIsMoving if the user is moving the scene, it should be render in preview mode.
     * @param aStatusReporter a pointer to the status progress reporter.
     * @return true if the render would like to redraw again.
     */
    virtual bool Redraw( bool aIsMoving, REPORTER* aStatusReporter = nullptr,
                         REPORTER* aWarningReporter = nullptr ) = 0;

    /**
     * @todo This must be reviewed to add flags to improve specific render.
     */
    void ReloadRequest() { m_reloadRequested = true; }

    /**
     * Query if there is a pending reload request.
     *
     * @return true if it wants to reload, false if there is no reload pending
     */
    bool IsReloadRequestPending() const { return m_reloadRequested; }

    /**
     * Give the interface the time (in ms) that it should wait for editing or movements before
     * (this works for display preview mode).
     *
     * @return a value in milliseconds
     */
    virtual int GetWaitForEditingTimeOut() = 0;

    /**
     * Set a new busy indicator factory.
     *
     * When set, this factory will be used to generate busy indicators when
     * suitable. If not set, no busy indicator will be used.
     */
    void SetBusyIndicatorFactory( BUSY_INDICATOR::FACTORY aNewFactory );

protected:
    /**
     * Return a created busy indicator, if a factory has been set, else a null pointer.
     */
    std::unique_ptr<BUSY_INDICATOR> CreateBusyIndicator() const;

    /// Settings reference in use for this render.
    BOARD_ADAPTER& m_boardAdapter;

    CAMERA&        m_camera;

    /// Flag if the canvas specific for this render was already initialized.
    bool m_canvasInitialized;

    /// @todo This must be reviewed in order to flag change types.
    bool m_reloadRequested;

    /// The window size that this camera is working.
    wxSize m_windowSize;

    /**
     *  Trace mask used to enable or disable the trace output of this class.
     *  The debug output can be turned on by setting the WXTRACE environment variable to
     *  "KI_TRACE_3D_RENDER".  See the wxWidgets documentation on wxLogTrace for
     *  more information.
     */
    static const wxChar* m_logTrace;

private:
    /// Factory that returns a suitable busy indicator for the context.
    BUSY_INDICATOR::FACTORY m_busyIndicatorFactory;
};

#endif // RENDER_3D_BASE_H
