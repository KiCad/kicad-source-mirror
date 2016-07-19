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
 * @file  c3d_render_base.cpp
 * @brief implements the initialization of the base class
 */


#include "c3d_render_base.h"


/**
 *  Trace mask used to enable or disable the trace output of this class.
 *  The debug output can be turned on by setting the WXTRACE environment variable to
 *  "KI_TRACE_3D_RENDER".  See the wxWidgets documentation on wxLogTrace for
 *  more information.
 */
const wxChar * C3D_RENDER_BASE::m_logTrace = wxT( "KI_TRACE_3D_RENDER" );


C3D_RENDER_BASE::C3D_RENDER_BASE(CINFO3D_VISU &aSettings) :
                                  m_settings( aSettings )
{
    wxLogTrace( m_logTrace, wxT( "C3D_RENDER_BASE::C3D_RENDER_BASE" ) );
    m_is_opengl_initialized = false;
    m_windowSize            = wxSize( -1, -1 );
    m_reloadRequested       = true;
}


C3D_RENDER_BASE::~C3D_RENDER_BASE()
{
}

