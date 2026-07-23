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

#ifndef EDIT_RELATIONS_H_
#define EDIT_RELATIONS_H_

#include <math/vector2d.h>

#include <memory>

class EDIT_LINE;
class EDIT_POINT;
class EDIT_POINTS;
class GRID_HELPER;


enum class EDIT_RELATION_KIND
{
    SAME_X,
    SAME_Y,
    ANGLE_STEP_45,
    ANGLE_STEP_90,
    POINT_ON_LINE,
    POINT_ON_CIRCLE,
    PERPENDICULAR_TRANSLATION
};


enum class POLYGON_LINE_MODE
{
    CONVERGING,
    FIXED_LENGTH
};


enum GRID_CONSTRAINT_TYPE
{
    IGNORE_GRID,
    SNAP_TO_GRID,
    SNAP_BY_GRID
};


enum SNAP_CONSTRAINT_TYPE
{
    IGNORE_SNAPS,
    OBJECT_LAYERS,
    ALL_LAYERS
};


class EDIT_RELATION
{
public:
    static std::unique_ptr<EDIT_RELATION> SameX( const EDIT_POINT& aReference );
    static std::unique_ptr<EDIT_RELATION> SameY( const EDIT_POINT& aReference );
    static std::unique_ptr<EDIT_RELATION> Angle45( const EDIT_POINT& aReference );
    static std::unique_ptr<EDIT_RELATION> Angle90( const EDIT_POINT& aReference );
    static std::unique_ptr<EDIT_RELATION> PointOnLine( const EDIT_POINT& aConstrained, const EDIT_POINT& aReference );
    static std::unique_ptr<EDIT_RELATION> PointOnCircle( const EDIT_POINT& aCenter, const EDIT_POINT& aEnd );
    static std::unique_ptr<EDIT_RELATION> PerpendicularTranslation( const EDIT_LINE& aLine );

    EDIT_RELATION_KIND Kind() const { return m_kind; }
    const EDIT_POINT*  Reference() const { return m_reference; }
    const EDIT_POINT*  SecondaryReference() const { return m_secondaryReference; }
    const VECTOR2I&    Origin() const { return m_origin; }
    const VECTOR2I&    Direction() const { return m_direction; }

    void Apply( EDIT_POINT& aHandle, const GRID_HELPER& aGrid ) const;
    void Apply( EDIT_LINE& aHandle, const GRID_HELPER& aGrid ) const;

private:
    explicit EDIT_RELATION( EDIT_RELATION_KIND aKind ) :
            m_kind( aKind )
    {
    }

    static std::unique_ptr<EDIT_RELATION> make( EDIT_RELATION_KIND aKind, const EDIT_POINT& aReference );

    EDIT_RELATION_KIND m_kind;
    const EDIT_POINT*  m_reference = nullptr;
    const EDIT_POINT*  m_secondaryReference = nullptr;
    VECTOR2I           m_origin;
    VECTOR2I           m_direction;
};


class POLYGON_EDGE_DRAG_POLICY
{
public:
    POLYGON_EDGE_DRAG_POLICY( EDIT_LINE& aLine, EDIT_POINTS& aPoints,
                              POLYGON_LINE_MODE aMode = POLYGON_LINE_MODE::CONVERGING );

    void Apply( EDIT_LINE& aHandle, const GRID_HELPER& aGrid );

    POLYGON_LINE_MODE GetMode() const { return m_mode; }
    void              SetMode( POLYGON_LINE_MODE aMode ) { m_mode = aMode; }

    const VECTOR2I& GetOriginalCenter() const { return m_originalCenter; }
    const VECTOR2I& GetPerpVector() const { return m_perpVector; }

private:
    void applyConverging( EDIT_LINE& aHandle );
    void applyFixedLength( EDIT_LINE& aHandle );

    POLYGON_LINE_MODE m_mode;
    EDIT_POINTS&      m_editPoints;
    VECTOR2I          m_originSideDirection;
    VECTOR2I          m_endSideDirection;
    VECTOR2I          m_draggedVector;
    VECTOR2I          m_originalCenter;
    VECTOR2I          m_perpVector;
    double            m_halfLength;
    bool              m_originCollinear;
    bool              m_endCollinear;
    EDIT_POINT*       m_prevOrigin;
    EDIT_POINT*       m_nextEnd;
    VECTOR2I          m_convergencePoint;
};

#endif
