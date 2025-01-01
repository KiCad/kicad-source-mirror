/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2020 Mario Luzeiro <mrluzeiro@ua.pt>
 * Copyright (C) 2024 Alex Shvartzkop <dudesuchamazing@gmail.com>
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

#include "render_3d_raytrace_ram.h"
#include <wx/log.h>


RENDER_3D_RAYTRACE_RAM::RENDER_3D_RAYTRACE_RAM( BOARD_ADAPTER& aAdapter, CAMERA& aCamera ) :
        RENDER_3D_RAYTRACE_BASE( aAdapter, aCamera ),
    m_outputBuffer( nullptr ),
    m_pboDataSize( 0 )
{
}


RENDER_3D_RAYTRACE_RAM::~RENDER_3D_RAYTRACE_RAM()
{
    deletePbo();
}


uint8_t* RENDER_3D_RAYTRACE_RAM::GetBuffer()
{
    return m_outputBuffer;
}


wxSize RENDER_3D_RAYTRACE_RAM::GetRealBufferSize()
{
    return wxSize( m_realBufferSize.x, m_realBufferSize.y );
}


void RENDER_3D_RAYTRACE_RAM::deletePbo()
{
    delete[] m_outputBuffer;
    m_outputBuffer = nullptr;
}


void RENDER_3D_RAYTRACE_RAM::SetCurWindowSize( const wxSize& aSize )
{
    if( m_windowSize != aSize )
    {
        m_windowSize = aSize;

        initPbo();
    }
}


bool RENDER_3D_RAYTRACE_RAM::Redraw( bool aIsMoving, REPORTER* aStatusReporter,
                                     REPORTER* aWarningReporter )
{
    bool requestRedraw = false;

    // Initialize openGL if need
    if( !m_canvasInitialized )
    {
        m_canvasInitialized = true;

        //aIsMoving = true;
        requestRedraw = true;

        // It will assign the first time the windows size, so it will now
        // revert to preview mode the first time the Redraw is called
        m_oldWindowsSize = m_windowSize;
        initializeBlockPositions();
    }

    std::unique_ptr<BUSY_INDICATOR> busy = CreateBusyIndicator();

    // Reload board if it was requested
    if( m_reloadRequested )
    {
        if( aStatusReporter )
            aStatusReporter->Report( _( "Loading..." ) );

        //aIsMoving = true;
        requestRedraw = true;
        Reload( aStatusReporter, aWarningReporter, false );
    }


    // Recalculate constants if windows size was changed
    if( m_windowSize != m_oldWindowsSize )
    {
        m_oldWindowsSize = m_windowSize;
        aIsMoving = true;
        requestRedraw = true;

        initializeBlockPositions();
    }

    const bool was_camera_changed = m_camera.ParametersChanged();

    if( requestRedraw || aIsMoving || was_camera_changed )
        m_renderState = RT_RENDER_STATE_MAX; // Set to an invalid state,
                                             // so it will restart again latter

    // This will only render if need, otherwise it will redraw the PBO on the screen again
    if( aIsMoving || was_camera_changed )
    {
        // Set head light (camera view light) with the opposite direction of the camera
        if( m_cameraLight )
            m_cameraLight->SetDirection( -m_camera.GetDir() );

        if( m_outputBuffer )
        {
            renderPreview( m_outputBuffer );
        }
    }
    else
    {
        if( m_renderState != RT_RENDER_STATE_FINISH )
        {
            if( m_outputBuffer )
            {
                render( m_outputBuffer, aStatusReporter );

                if( m_renderState != RT_RENDER_STATE_FINISH )
                    requestRedraw = true;
            }
        }
    }

    return requestRedraw;
}


void RENDER_3D_RAYTRACE_RAM::initPbo()
{
    deletePbo();

    m_pboDataSize = m_realBufferSize.x * m_realBufferSize.y * 4;
    m_outputBuffer = new uint8_t[m_pboDataSize]();
}
