/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2014 CERN
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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

#ifndef EDIT_POINTS_H_
#define EDIT_POINTS_H_

#include <vector>
#include <list>
#include <math/box2.h>

#include <base_struct.h>
#include <layers_id_colors_and_visibility.h>

class EDIT_POINT;

class EDIT_POINT_CONSTRAINT
{
public:
    EDIT_POINT_CONSTRAINT( EDIT_POINT& aConstrained ) : m_constrained( aConstrained ) {};
    virtual ~EDIT_POINT_CONSTRAINT() {};

    virtual void Apply() = 0;

protected:
    EDIT_POINT& m_constrained;
};


// TODO docs
class EDIT_POINT
{
public:
    EDIT_POINT( const VECTOR2I& aPoint ) :
        m_point( aPoint ), m_constraint( NULL ) {};

    ~EDIT_POINT()
    {
        delete m_constraint;
    }

    const VECTOR2I& GetPosition() const
    {
        return m_point;
    }

    void SetPosition( const VECTOR2I& aPosition )
    {
        m_point = aPosition;
    }

    bool WithinPoint( const VECTOR2I& aPoint, unsigned int aSize ) const
    {
        VECTOR2I topLeft = GetPosition() - aSize;
        VECTOR2I bottomRight = GetPosition() + aSize;

        return ( aPoint.x > topLeft.x && aPoint.y > topLeft.y &&
                 aPoint.x < bottomRight.x && aPoint.y < bottomRight.y );
    }

    void SetConstraint( EDIT_POINT_CONSTRAINT* aConstraint )
    {
        m_constraint = aConstraint;
    }

    void ClearConstraint( EDIT_POINT_CONSTRAINT* aConstraint )
    {
        delete m_constraint;
        m_constraint = NULL;
    }

    EDIT_POINT_CONSTRAINT* GetConstraint() const
    {
        return m_constraint;
    }

    void ApplyConstraint()
    {
        if( m_constraint )
            m_constraint->Apply();
    }

    ///> Single point size in pixels
    static const int POINT_SIZE = 10;

private:
    VECTOR2I m_point;
    EDIT_POINT_CONSTRAINT* m_constraint;
};


class EDIT_POINTS : public EDA_ITEM
{
public:
    EDIT_POINTS( EDA_ITEM* aParent );
    ~EDIT_POINTS();

    /**
     * Function FindPoint
     * Returns a point that is at given coordinates or NULL if there is no such point.
     * @param aLocation is the location for searched point.
     */
    EDIT_POINT* FindPoint( const VECTOR2I& aLocation );

    EDA_ITEM* GetParent() const
    {
        return m_parent;
    }

    void Add( const EDIT_POINT& aPoint )
    {
        m_points.push_back( aPoint );
    }

    void Add( const VECTOR2I& aPoint )
    {
        m_points.push_back( EDIT_POINT( aPoint ) );
    }

    EDIT_POINT& operator[]( unsigned int aIndex )
    {
        return m_points[aIndex];
    }

    const EDIT_POINT& operator[]( unsigned int aIndex ) const
    {
        return m_points[aIndex];
    }

    unsigned int Size() const
    {
        return m_points.size();
    }

    virtual const BOX2I ViewBBox() const
    {
        return m_parent->ViewBBox();
    }

    virtual void ViewDraw( int aLayer, KIGFX::GAL* aGal ) const;

    virtual void ViewGetLayers( int aLayers[], int& aCount ) const
    {
        aCount = 1;
        aLayers[0] = ITEM_GAL_LAYER( GP_OVERLAY );
    }

    void Show( int x, std::ostream& st ) const
    {
    }

private:
    EDA_ITEM* m_parent;
    std::deque<EDIT_POINT> m_points;
};


class EPC_VERTICAL : public EDIT_POINT_CONSTRAINT
{
public:
    EPC_VERTICAL( EDIT_POINT& aConstrained, EDIT_POINT& aConstrainer ) :
        EDIT_POINT_CONSTRAINT( aConstrained ), m_constrainer( aConstrainer )
    {}

    virtual ~EPC_VERTICAL() {};

    virtual void Apply()
    {
        VECTOR2I point = m_constrained.GetPosition();
        point.x = m_constrainer.GetPosition().x;
        m_constrained.SetPosition( point );
    }

    virtual std::list<EDIT_POINT*> GetConstrainers() const
    {
        return std::list<EDIT_POINT*>( 1, &m_constrainer );
    }

private:
    EDIT_POINT& m_constrainer;
};


class EPC_HORIZONTAL : public EDIT_POINT_CONSTRAINT
{
public:
    EPC_HORIZONTAL( EDIT_POINT& aConstrained, const EDIT_POINT& aConstrainer ) :
        EDIT_POINT_CONSTRAINT( aConstrained ), m_constrainer( aConstrainer )
    {}

    virtual ~EPC_HORIZONTAL() {};

    virtual void Apply()
    {
        VECTOR2I point = m_constrained.GetPosition();
        point.y = m_constrainer.GetPosition().y;
        m_constrained.SetPosition( point );
    }

private:
    const EDIT_POINT& m_constrainer;
};


class EPC_CIRCLE : public EDIT_POINT_CONSTRAINT
{
public:
    EPC_CIRCLE( EDIT_POINT& aConstrained, const EDIT_POINT& aCenter, const EDIT_POINT& aEnd ) :
        EDIT_POINT_CONSTRAINT( aConstrained ), m_center( aCenter ), m_end( aEnd )
    {}

    virtual ~EPC_CIRCLE() {};

    virtual void Apply()
    {
        VECTOR2I centerToEnd = m_end.GetPosition() - m_center.GetPosition();
        VECTOR2I centerToPoint = m_constrained.GetPosition() - m_center.GetPosition();

        int radius = centerToEnd.EuclideanNorm();
        double angle = centerToPoint.Angle();

        VECTOR2I newLine( radius, 0 );
        newLine = newLine.Rotate( angle );

        m_constrained.SetPosition( m_center.GetPosition() + newLine );
    }

private:
    const EDIT_POINT& m_center;
    const EDIT_POINT& m_end;
};

#endif /* EDIT_POINTS_H_ */
