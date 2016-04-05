/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Mario Luzeiro <mrluzeiro@ua.pt>
 * Copyright (C) 1992-2015 KiCad Developers, see AUTHORS.txt for contributors.
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
#include "3d_cache/3d_cache.h"

/**
 *  This is a base class to hold data and functions for render targets.
 */
class C3D_RENDER_BASE
{


    // Operations
public:

    C3D_RENDER_BASE( CINFO3D_VISU &aSettings,
                     S3D_CACHE *a3DModelManager );

    virtual ~C3D_RENDER_BASE() = 0;

    virtual void SetCurWindowSize( const wxSize &aSize ) = 0;

    virtual void Redraw( bool aIsMoving ) = 0;

    void ReloadRequest() { m_reloadRequested = true; }

    // Attributes

protected:
    CINFO3D_VISU &m_settings;
    S3D_CACHE *m_3d_model_manager;

    bool m_is_opengl_initialized;
    bool m_reloadRequested;

    /**
     *  The window size that this camera is working.
     */
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
