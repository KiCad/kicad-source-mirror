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
 * @file  ctrack_ball.h
 * @brief Declaration for a track ball camera
 */

#ifndef CTRACK_BALL_H
#define CTRACK_BALL_H

#include "ccamera.h"


class CTRACK_BALL : public CCAMERA
{

 public:

    explicit CTRACK_BALL( float aRangeScale );

    void Drag( const wxPoint &aNewMousePosition ) override;

    void Pan( const wxPoint &aNewMousePosition ) override;

    void Pan( const SFVEC3F &aDeltaOffsetInc ) override;

    void Pan_T1( const SFVEC3F &aDeltaOffsetInc ) override;

    void SetLookAtPos( const SFVEC3F &aLookAtPos ) override;

    void Reset() override;

    void Reset_T1() override;

    void SetT0_and_T1_current_T() override;

    void Interpolate( float t ) override;

 private:

    /**
     *  quarternion of the trackball
     */
    double m_quat[4];
    double m_quat_t0[4];
    double m_quat_t1[4];
};

#endif // CTRACK_BALL_H
