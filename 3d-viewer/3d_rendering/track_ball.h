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
 * @file track_ball.h
 * @brief Declaration for a track ball camera
 */

#ifndef TRACK_BALL_H
#define TRACK_BALL_H

#include <gal/3d/camera.h>


class TRACK_BALL : public CAMERA
{
public:
    explicit TRACK_BALL( float aInitialDistance );
    explicit TRACK_BALL( SFVEC3F aInitPos, SFVEC3F aLookat, PROJECTION_TYPE aProjectionType );

    virtual ~TRACK_BALL()
    {
    }

    void Drag( const wxPoint& aNewMousePosition ) override;

    void Pan( const wxPoint& aNewMousePosition ) override;

    void Pan( const SFVEC3F& aDeltaOffsetInc ) override;

    void Pan_T1( const SFVEC3F& aDeltaOffsetInc ) override;

    void Reset_T1() override;

    void SetT0_and_T1_current_T() override;

    void Interpolate( float t ) override;

private:
    void initQuat();

    /**
     *  interpolate quaternions of the trackball
     */
    double m_quat_t0[4];
    double m_quat_t1[4];
};

#endif // TRACK_BALL_H
