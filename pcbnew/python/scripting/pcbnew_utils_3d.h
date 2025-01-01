/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Alex Shvartzkop <dudesuchamazing@gmail.com>
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

#ifndef PCBNEW_UTILS_3D_H_
#define PCBNEW_UTILS_3D_H_

#include <wx/string.h>
#include <math/vector3.h>

class UTILS_BOX3D
{
    VECTOR3D m_min;
    VECTOR3D m_max;

public:
    VECTOR3D& Min();
    VECTOR3D& Max();
    VECTOR3D  GetCenter();
    VECTOR3D  GetSize();
};

class UTILS_STEP_MODEL
{
    struct DATA;
    DATA* m_data;

    ~UTILS_STEP_MODEL();

public:
    UTILS_BOX3D GetBoundingBox();
    void        Translate( double aX, double aY, double aZ );
    void        Scale( double aScale );
    bool        SaveSTEP( const wxString& aFileName );

    static UTILS_STEP_MODEL* LoadSTEP( const wxString& aFileName );
};

#endif // PCBNEW_UTILS_3D_H_