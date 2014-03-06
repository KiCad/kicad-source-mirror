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
        m_position( aPoint ), m_constraint( NULL ) {};

    virtual ~EDIT_POINT()
    {
        delete m_constraint;
    }

    virtual VECTOR2I GetPosition() const
    {
        return m_position;
    }

    virtual void SetPosition( const VECTOR2I& aPosition )
    {
        m_position = aPosition;
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
        if( m_constraint )
            delete m_constraint;

        m_constraint = aConstraint;
    }

    void ClearConstraint()
    {
        delete m_constraint;
        m_constraint = NULL;
    }

    EDIT_POINT_CONSTRAINT* GetConstraint() const
    {
        return m_constraint;
    }

    bool IsConstrained() const
    {
        return m_constraint != NULL;
    }

    void ApplyConstraint()
    {
        if( m_constraint )
            m_constraint->Apply();
    }

    bool operator==( const EDIT_POINT& aOther ) const
    {
        return m_position == aOther.m_position;
    }

    ///> Single point size in pixels
    static const int POINT_SIZE = 10;

protected:
    VECTOR2I m_position;
    EDIT_POINT_CONSTRAINT* m_constraint;
};


class EDIT_LINE : public EDIT_POINT
{
public:
    EDIT_LINE( EDIT_POINT& aOrigin, EDIT_POINT& aEnd ) :
        EDIT_POINT( aOrigin.GetPosition() + ( aEnd.GetPosition() - aOrigin.GetPosition() ) / 2 ),
        m_origin( aOrigin ), m_end( aEnd )
    {
    }

    virtual VECTOR2I GetPosition() const
    {
        return m_origin.GetPosition() + ( m_end.GetPosition() - m_origin.GetPosition() ) / 2;
    }

    virtual void SetPosition( const VECTOR2I& aPosition )
    {
        VECTOR2I difference = aPosition - GetPosition();

        m_origin.SetPosition( m_origin.GetPosition() + difference );
        m_end.SetPosition( m_end.GetPosition() + difference );
    }

    bool operator==( const EDIT_POINT& aOther ) const
    {
        return GetPosition() == aOther.GetPosition();
    }

    bool operator==( const EDIT_LINE& aOther ) const
    {
        return m_origin == aOther.m_origin && m_end == aOther.m_end;
    }

private:
    EDIT_POINT& m_origin;
    EDIT_POINT& m_end;
};


class EDIT_POINTS : public EDA_ITEM
{
public:
    EDIT_POINTS( EDA_ITEM* aParent );

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

    void AddPoint( const EDIT_POINT& aPoint )
    {
        m_points.push_back( aPoint );
    }

    void AddPoint( const VECTOR2I& aPoint )
    {
        AddPoint( EDIT_POINT( aPoint ) );
    }

    void AddLine( const EDIT_LINE& aLine )
    {
        m_lines.push_back( aLine );
    }

    void AddLine( EDIT_POINT& aOrigin, EDIT_POINT& aEnd )
    {
        m_lines.push_back( EDIT_LINE( aOrigin, aEnd ) );
    }

    EDIT_POINT* Previous( const EDIT_POINT& aPoint )
    {
        for( unsigned int i = 0; i < m_points.size(); ++i )
        {
            if( m_points[i] == aPoint )
            {
                if( i == 0 )
                    return &m_points[m_points.size() - 1];
                else
                    return &m_points[i - 1];
            }
        }

        for( unsigned int i = 0; i < m_lines.size(); ++i )
        {
            if( m_lines[i] == aPoint )
            {
                if( i == 0 )
                    return &m_lines[m_lines.size() - 1];
                else
                    return &m_lines[i - 1];
            }
        }

        return NULL;
    }

    EDIT_POINT* Next( const EDIT_POINT& aPoint )
    {
        for( unsigned int i = 0; i < m_points.size(); ++i )
        {
            if( m_points[i] == aPoint )
            {
                if( i == m_points.size() - 1 )
                    return &m_points[0];
                else
                    return &m_points[i + 1];
            }
        }

        for( unsigned int i = 0; i < m_lines.size(); ++i )
        {
            if( m_lines[i] == aPoint )
            {
                if( i == m_lines.size() - 1 )
                    return &m_lines[0];
                else
                    return &m_lines[i + 1];
            }
        }

        return NULL;
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
    std::deque<EDIT_LINE> m_lines;
};


class EPC_VERTICAL : public EDIT_POINT_CONSTRAINT
{
public:
    EPC_VERTICAL( EDIT_POINT& aConstrained, EDIT_POINT& aConstrainer ) :
        EDIT_POINT_CONSTRAINT( aConstrained ), m_constrainer( aConstrainer )
    {}

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

    virtual void Apply()
    {
        VECTOR2I point = m_constrained.GetPosition();
        point.y = m_constrainer.GetPosition().y;
        m_constrained.SetPosition( point );
    }

private:
    const EDIT_POINT& m_constrainer;
};


class EPC_45DEGREE : public EDIT_POINT_CONSTRAINT
{
public:
    EPC_45DEGREE ( EDIT_POINT& aConstrained, const EDIT_POINT& aConstrainer ) :
        EDIT_POINT_CONSTRAINT( aConstrained ), m_constrainer( aConstrainer )
    {}

    virtual void Apply()
    {
        // Current line vector
        VECTOR2I lineVector( m_constrained.GetPosition() - m_constrainer.GetPosition() );
        double angle = lineVector.Angle();

        // Find the closest angle, which is a multiple of 45 degrees
        double newAngle = round( angle / ( M_PI / 4.0 ) ) * M_PI / 4.0;
        VECTOR2I newLineVector = lineVector.Rotate( newAngle - angle );

        m_constrained.SetPosition( m_constrainer.GetPosition() + newLineVector );
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
