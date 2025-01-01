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

#ifndef RENDER_3D_RAYTRACE_RAM_H
#define RENDER_3D_RAYTRACE_RAM_H

#include "render_3d_raytrace_base.h"


class RENDER_3D_RAYTRACE_RAM : public RENDER_3D_RAYTRACE_BASE
{
public:
    // TODO: Take into account board thickness so that the camera won't move inside of the board
    // when facing it perpendicularly.
    static constexpr float MIN_DISTANCE_IU = 4 * PCB_IU_PER_MM;

    explicit RENDER_3D_RAYTRACE_RAM( BOARD_ADAPTER& aAdapter, CAMERA& aCamera );

    ~RENDER_3D_RAYTRACE_RAM();

    uint8_t* GetBuffer();
    wxSize   GetRealBufferSize();

    void SetCurWindowSize( const wxSize& aSize ) override;
    bool Redraw( bool aIsMoving, REPORTER* aStatusReporter, REPORTER* aWarningReporter ) override;

private:
    void initPbo() override;
    void deletePbo() override;

    uint8_t* m_outputBuffer;
    uint32_t m_pboDataSize;
};


#endif // RENDER_3D_RAYTRACE_RAM_H
