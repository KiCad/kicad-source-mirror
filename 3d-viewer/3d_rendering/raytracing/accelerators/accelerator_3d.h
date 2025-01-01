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
 * @file accelerator_3d.h
 */

#ifndef _ACCELERATOR_3D_H_
#define _ACCELERATOR_3D_H_

#include "container_3d.h"
#include "../raypacket.h"


class ACCELERATOR_3D
{
public:
    ACCELERATOR_3D( );
    virtual ~ACCELERATOR_3D();

    virtual bool Intersect( const RAY& aRay, HITINFO& aHitInfo ) const = 0;

    virtual bool Intersect( const RAY& aRay, HITINFO& aHitInfo,
                            unsigned int aAccNodeInfo ) const = 0;

    virtual bool Intersect( const RAYPACKET& aRayPacket,
                            HITINFO_PACKET* aHitInfoPacket ) const = 0;

    virtual bool IntersectP( const RAY& aRay, float aMaxDistance ) const = 0;

protected:
    BBOX_3D m_bbox;
};

#endif // _ACCELERATOR_3D_H_
