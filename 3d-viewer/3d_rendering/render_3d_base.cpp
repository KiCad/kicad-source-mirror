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
 * @file render_3d_base.cpp
 * @brief implements the initialization of the base class
 */


#include "render_3d_base.h"
#include <wx/log.h>


/**
 *  Trace mask used to enable or disable the trace output of this class.
 *
 *  The debug output can be turned on by setting the WXTRACE environment variable to
 *  "KI_TRACE_3D_RENDER".  See the wxWidgets documentation on wxLogTrace for
 *  more information.
 *
 * @ingroup trace_env_vars
 */
const wxChar* RENDER_3D_BASE::m_logTrace = wxT( "KI_TRACE_3D_RENDER" );


RENDER_3D_BASE::RENDER_3D_BASE( BOARD_ADAPTER& aBoardAdapter, CAMERA& aCamera ) :
        m_boardAdapter( aBoardAdapter ),
        m_camera( aCamera )
{
    wxLogTrace( m_logTrace, wxT( "RENDER_3D_BASE::RENDER_3D_BASE" ) );
    m_canvasInitialized     = false;
    m_windowSize            = wxSize( -1, -1 );
    m_reloadRequested       = true;
}


RENDER_3D_BASE::~RENDER_3D_BASE()
{
}


void RENDER_3D_BASE::SetBusyIndicatorFactory( BUSY_INDICATOR::FACTORY aNewFactory )
{
    m_busyIndicatorFactory = aNewFactory;
}


std::unique_ptr<BUSY_INDICATOR> RENDER_3D_BASE::CreateBusyIndicator() const
{
    std::unique_ptr<BUSY_INDICATOR> busy;

    if( m_busyIndicatorFactory )
        busy = m_busyIndicatorFactory();

    return busy;
}
