/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Cirilo Bernardo <cirilo.bernardo@gmail.com>
 * Copyright (C) 2015 Mario Luzeiro <mrluzeiro@ua.pt>
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 2004 Jean-Pierre Charras, jp.charras at wanadoo.fr
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
 * @file 3d_info.h
 * defines the basic data associated with a single 3D model.
 */

#ifndef INFO_3D_H
#define INFO_3D_H

#include <wx/string.h>
#include <plugins/3dapi/sg_base.h>

class FP_3DMODEL;

struct S3D_INFO
{
    S3D_INFO();
    S3D_INFO( const FP_3DMODEL& aModel );

    SGPOINT m_Scale;        ///< scaling factors for the 3D footprint shape
    SGPOINT m_Rotation;     ///< an X,Y,Z rotation (unit = degrees) for the 3D shape
    SGPOINT m_Offset;       ///< an offset (unit = inch) for the 3D shape
    wxString m_Filename;    ///< The 3D shape filename in 3D library
};

#endif // INFO_3D_H
