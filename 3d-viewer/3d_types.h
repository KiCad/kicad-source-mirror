/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Mario Luzeiro <mrluzeiro@gmail.com>
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
 * @file 3d_types.h
 */

#ifndef _3D_TYPES_H_
#define _3D_TYPES_H_

#define GLM_FORCE_RADIANS
#include <gal/opengl/glm/glm.hpp>
#include <base_units.h>     // for IU_PER_MILS


/**
 * @note For historical reasons the 3D modeling unit is 0.1 inch
 * 1 3Dunit = 2.54 mm = 0.1 inch = 100 mils
 */
#define UNITS3D_TO_UNITSPCB (IU_PER_MILS * 100)


/**
 * scaling factor for 3D shape offset ( S3D_MASTER::m_MatPosition member )
 * Was in inches in legacy version, and, due to a mistake, still in inches
 * in .kicad_pcb files (which are using mm)
 * so this scaling convert file units (inch) to 3D units (0.1 inch), only
 * for S3D_MASTER::m_MatPosition parameter
 */
#define SCALE_3D_CONV 10


// S3D_VERTEX manages a opengl 3D coordinate (3 float numbers: x,y,z coordinates)
// float are widely used in opengl functions.
// they are used here in coordinates which are also used in opengl functions.
#define S3D_VERTEX glm::vec3


// S3DPOINT manages a set of 3 double values (x,y,z )
// It is used for values which are not directly used in opengl functions.
// It is used in dialogs, or when reading/writing files for instance
class S3DPOINT
{
public:
    double x, y, z;

public:
    S3DPOINT()
    {
        x = y = z = 0.0;
    }

    S3DPOINT( double px, double py, double pz)
    {
        x = px;
        y = py;
        z = pz;
    }
};


/**
 * Class S3DPOINT_VALUE_CTRL
 * displays a S3DPOINT for editing (in dialogs).  A S3DPOINT is a triplet of values
 * Values can be scale, rotation, offset...
 */
class S3DPOINT_VALUE_CTRL
{
private:
    wxTextCtrl*   m_XValueCtrl, * m_YValueCtrl, * m_ZValueCtrl;

public:
    S3DPOINT_VALUE_CTRL( wxWindow* parent, wxBoxSizer* BoxSizer );

    ~S3DPOINT_VALUE_CTRL();

    /**
     * Function GetValue
     * @return the 3D point in internal units.
     */
    S3DPOINT   GetValue();
    void       SetValue( S3DPOINT a3Dpoint );
    void       Enable( bool enbl );
    void       SetToolTip( const wxString& text );
};

#endif // 3D_TYPES_H
