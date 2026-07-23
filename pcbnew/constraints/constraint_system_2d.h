/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef CONSTRAINT_SYSTEM_2D_H_
#define CONSTRAINT_SYSTEM_2D_H_

#include <cstddef>
#include <deque>
#include <memory>
#include <vector>

#include <math/vector2d.h>


namespace GCS
{
class System;
}


class CONSTRAINT_SYSTEM_2D
{
public:
    using SNAPSHOT = std::vector<double>;

    CONSTRAINT_SYSTEM_2D();
    ~CONSTRAINT_SYSTEM_2D();

    CONSTRAINT_SYSTEM_2D( const CONSTRAINT_SYSTEM_2D& ) = delete;
    CONSTRAINT_SYSTEM_2D& operator=( const CONSTRAINT_SYSTEM_2D& ) = delete;

    void SetCoordinateFrame( const VECTOR2I& aOrigin, double aScale );

    double NormalizeX( int aIU ) const;
    double NormalizeY( int aIU ) const;
    double DenormalizeX( double aNormalized ) const;
    double DenormalizeY( double aNormalized ) const;

    int                 AddParameter( double aValue );
    double&             Parameter( int aIndex );
    const double&       Parameter( int aIndex ) const;
    double*             ParameterPointer( int aIndex );
    std::size_t         ParameterCount() const { return m_parameters.size(); }
    std::deque<double>& ParameterStorage() { return m_parameters; }

    SNAPSHOT Snapshot() const;
    bool     Restore( const SNAPSHOT& aSnapshot );
    bool     RestorePrefix( const SNAPSHOT& aSnapshot );
    void     Clear();

    GCS::System&       Solver();
    const GCS::System& Solver() const;

private:
    std::unique_ptr<GCS::System> m_solver;
    std::deque<double>           m_parameters;
    double                       m_originX = 0.0;
    double                       m_originY = 0.0;
    double                       m_scale = 1.0;
    double                       m_inverseScale = 1.0;
};

#endif
