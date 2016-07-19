/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2016 Mario Luzeiro <mrluzeiro@ua.pt>
 * Copyright (C) 1992-2016 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file  c3d_render_base.h
 * @brief
 */

#ifndef C3D_RENDER_BASE_H
#define C3D_RENDER_BASE_H


#include <wxBasePcbFrame.h>
#include "../3d_canvas/cinfo3d_visu.h"
#include <reporter.h>

/**
 *  This is a base class to hold data and functions for render targets.
 */
class C3D_RENDER_BASE
{


    // Operations
public:

    explicit C3D_RENDER_BASE( CINFO3D_VISU &aSettings );

    virtual ~C3D_RENDER_BASE() = 0;

    /**
     * @brief SetCurWindowSize - Before each render, the canvas will tell the
     * render what is the size of its windows, so render can take actions if it
     * changed.
     * @param aSize: the current size of the render window
     */
    virtual void SetCurWindowSize( const wxSize &aSize ) = 0;

    /**
     * @brief Redraw - Ask to redraw the view
     * @param aIsMoving: if the user is moving the scene, it should be render in
     * preview mode
     * @param aStatusTextReporter: a pointer to the status progress reporter
     * @return it will return true if the render would like to redraw again
     */
    virtual bool Redraw( bool aIsMoving, REPORTER *aStatusTextReporter = NULL ) = 0;

    /**
     * @brief ReloadRequest - !TODO: this must be reviewed to add flags to
     * improve specific render
     */
    void ReloadRequest() { m_reloadRequested = true; }

    /**
     * @brief IsReloadRequestPending - Query if there is a pending reload request
     * @return true if it wants to reload, false if there is no reload pending
     */
    bool IsReloadRequestPending() const { return m_reloadRequested; }

    /**
     * @brief GetWaitForEditingTimeOut - Give the interface the time (in ms)
     * that it should wait for editing or movements before
     * (this works for display preview mode)
     * @return a value in miliseconds
     */
    virtual int GetWaitForEditingTimeOut() = 0;

    // Attributes

protected:

    /// settings refrence in use for this render
    CINFO3D_VISU &m_settings;

    /// flag if the opengl specific for this render was already initialized
    bool m_is_opengl_initialized;

    /// !TODO: this must be reviewed in order to flag change types
    bool m_reloadRequested;

    /// The window size that this camera is working.
    wxSize m_windowSize;

    /**
     *  Trace mask used to enable or disable the trace output of this class.
     *  The debug output can be turned on by setting the WXTRACE environment variable to
     *  "KI_TRACE_3D_RENDER".  See the wxWidgets documentation on wxLogTrace for
     *  more information.
     */
    static const wxChar *m_logTrace;
};

#endif // C3D_RENDER_BASE_H
