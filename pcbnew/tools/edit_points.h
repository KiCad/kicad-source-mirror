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

/**
 * Class EDIT_POINT_CONSTRAINT
 *
 * Allows to describe constraints between two points. After the constrained point is changed,
 * Apply() has to be called to fix its coordinates according to the implemented constraint.
 */
class EDIT_POINT_CONSTRAINT
{
public:
    /**
     * Constructor
     *
     * @param aConstrained is EDIT_POINT to which the constraint is applied.
     */
    EDIT_POINT_CONSTRAINT( EDIT_POINT& aConstrained ) : m_constrained( aConstrained ) {};

    virtual ~EDIT_POINT_CONSTRAINT() {};

    /**
     * Function Apply()
     *
     * Corrects coordinates of the constrained point.
     */
    virtual void Apply() = 0;

protected:
    EDIT_POINT& m_constrained;      ///< Point that is constrained by rules implemented by Apply()
};


/**
 * Class EDIT_POINT
 *
 * Represents a single point that can be used for modifying items. It is directly related to one
 * of points in a graphical item (e.g. vertex of a zone or center of a circle).
 */
class EDIT_POINT
{
public:
    /**
     * Constructor
     *
     * @param aPoint stores coordinates for EDIT_POINT.
     */
    EDIT_POINT( const VECTOR2I& aPoint ) :
        m_position( aPoint ), m_constraint( NULL ) {};

    virtual ~EDIT_POINT()
    {
        delete m_constraint;
    }

    /**
     * Function GetPosition()
     *
     * Returns coordinates of an EDIT_POINT. Note that it may be different than coordinates of
     * a graphical item that is bound to the EDIT_POINT.
     */
    virtual VECTOR2I GetPosition() const
    {
        return m_position;
    }

    /**
     * Function GetX()
     *
     * Returns X coordinate of an EDIT_POINT.
     */
    int GetX() const
    {
        return GetPosition().x;
    }

    /**
     * Function GetX()
     *
     * Returns Y coordinate of an EDIT_POINT.
     */
    int GetY() const
    {
        return GetPosition().y;
    }

    /**
     * Function SetPosition()
     *
     * Sets new coordinates for an EDIT_POINT. It does not change the coordinates of a graphical
     * item.
     * @param aPosition are new coordinates.
     */
    virtual void SetPosition( const VECTOR2I& aPosition )
    {
        m_position = aPosition;
    }

    /**
     * Function WithinPoint()
     *
     * Checks if given point is within a square centered in the EDIT_POINT position.
     * @param aPoint is point to be checked.
     * @param aSize is length of the square side.
     */
    bool WithinPoint( const VECTOR2I& aPoint, unsigned int aSize ) const;

    /**
     * Function SetConstraint()
     *
     * Sets a constraint for and EDIT_POINT.
     * @param aConstraint is the constraint to be set.
     */
    void SetConstraint( EDIT_POINT_CONSTRAINT* aConstraint )
    {
        if( m_constraint )
            delete m_constraint;

        m_constraint = aConstraint;
    }

    /**
     * Function GetConstraint()
     *
     * Returns the constraint imposed on an EDIT_POINT. If there are no constraints, NULL is
     * returned.
     */
    EDIT_POINT_CONSTRAINT* GetConstraint() const
    {
        return m_constraint;
    }

    /**
     * Function ClearConstraint()
     *
     * Removes previously set constraint.
     */
    void ClearConstraint()
    {
        delete m_constraint;
        m_constraint = NULL;
    }

    /**
     * Function IsConstrained()
     *
     * Checks if point is constrained.
     * @return True is point is constrained, false otherwise.
     */
    bool IsConstrained() const
    {
        return m_constraint != NULL;
    }

    /**
     * Function ApplyConstraint()
     *
     * Corrects coordinates of an EDIT_POINT by applying previously set constraint.
     */
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
    VECTOR2I m_position;                        ///< Position of EDIT_POINT
    EDIT_POINT_CONSTRAINT* m_constraint;        ///< Constraint for the point, NULL if none
};


/**
 * Class EDIT_LINE
 *
 * Represents a line connecting two EDIT_POINTs. That allows to move them both by dragging the
 * EDIT_POINT in the middle. As it uses references to EDIT_POINTs, all coordinates are
 * automatically synchronized.
 */
class EDIT_LINE : public EDIT_POINT
{
public:
    /**
     * Constructor
     *
     * @param aOrigin is the origin of EDIT_LINE.
     * @param aEnd is the end of EDIT_LINE.
     */
    EDIT_LINE( EDIT_POINT& aOrigin, EDIT_POINT& aEnd ) :
        EDIT_POINT( aOrigin.GetPosition() + ( aEnd.GetPosition() - aOrigin.GetPosition() ) / 2 ),
        m_origin( aOrigin ), m_end( aEnd )
    {
    }

    ///> @copydoc EDIT_POINT::GetPosition()
    virtual VECTOR2I GetPosition() const
    {
        return m_origin.GetPosition() + ( m_end.GetPosition() - m_origin.GetPosition() ) / 2;
    }

    ///> @copydoc EDIT_POINT::GetPosition()
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
    EDIT_POINT& m_origin;           ///< Origin point for a line
    EDIT_POINT& m_end;              ///< End point for a line
};


/**
 * Class EDIT_POINTS
 *
 * EDIT_POINTS is a VIEW_ITEM that manages EDIT_POINTs and EDIT_LINEs and draws them.
 */
class EDIT_POINTS : public EDA_ITEM
{
public:
    /**
     * Constructor.
     *
     * @param aParent is the item to which EDIT_POINTs are related.
     */
    EDIT_POINTS( EDA_ITEM* aParent );

    /**
     * Function FindPoint()
     *
     * Returns a point that is at given coordinates or NULL if there is no such point.
     * @param aLocation is the location for searched point.
     */
    EDIT_POINT* FindPoint( const VECTOR2I& aLocation );

    /**
     * Function GetParent()
     *
     * Returns parent of the EDIT_POINTS.
     */
    EDA_ITEM* GetParent() const
    {
        return m_parent;
    }

    /**
     * Function AddPoint()
     *
     * Adds an EDIT_POINT.
     * @param aPoint is the new point.
     */
    void AddPoint( const EDIT_POINT& aPoint )
    {
        m_points.push_back( aPoint );
    }

    /**
     * Function AddPoint()
     *
     * Adds an EDIT_POINT.
     * @param aPoint are coordinates of the new point.
     */
    void AddPoint( const VECTOR2I& aPoint )
    {
        AddPoint( EDIT_POINT( aPoint ) );
    }

    /**
     * Function AddLine()
     *
     * Adds an EDIT_LINE.
     * @param aLine is the new line.
     */
    void AddLine( const EDIT_LINE& aLine )
    {
        m_lines.push_back( aLine );
    }

    /**
     * Function AddLine()
     *
     * Adds an EDIT_LINE.
     * @param aOrigin is the origin for a new line.
     * @param aEnd is the end for a new line.
     */
    void AddLine( EDIT_POINT& aOrigin, EDIT_POINT& aEnd )
    {
        m_lines.push_back( EDIT_LINE( aOrigin, aEnd ) );
    }

    /**
     * Function Previous()
     *
     * Returns the point that is after the given point in the list.
     * @param aPoint is the point that is supposed to be preceding the searched point.
     * @return The point following aPoint in the list. If aPoint is the first in
     * the list, the last from the list will be returned. If there are no points at all, NULL
     * is returned.
     */
    EDIT_POINT* Previous( const EDIT_POINT& aPoint );

    /**
     * Function Next()
     *
     * Returns the point that is before the given point in the list.
     * @param aPoint is the point that is supposed to be following the searched point.
     * @return The point preceding aPoint in the list. If aPoint is the last in
     * the list, the first point from the list will be returned. If there are no points at all,
     * NULL is returned.
     */
    EDIT_POINT* Next( const EDIT_POINT& aPoint );

    EDIT_POINT& operator[]( unsigned int aIndex )
    {
        return m_points[aIndex];
    }

    const EDIT_POINT& operator[]( unsigned int aIndex ) const
    {
        return m_points[aIndex];
    }

    /**
     * Function Size()
     *
     * Returns number of stored points.
     */
    unsigned int Size() const
    {
        return m_points.size();
    }

    ///> @copydoc VIEW_ITEM::ViewBBox()
    virtual const BOX2I ViewBBox() const
    {
        return m_parent->ViewBBox();
    }

    ///> @copydoc VIEW_ITEM::ViewDraw()
    virtual void ViewDraw( int aLayer, KIGFX::GAL* aGal ) const;

    ///> @copydoc VIEW_ITEM::ViewGetLayers()
    virtual void ViewGetLayers( int aLayers[], int& aCount ) const
    {
        aCount = 1;
        aLayers[0] = ITEM_GAL_LAYER( GP_OVERLAY );
    }

    void Show( int x, std::ostream& st ) const
    {
    }

private:
    EDA_ITEM* m_parent;                 ///< Parent of the EDIT_POINTs
    std::deque<EDIT_POINT> m_points;    ///< EDIT_POINTs for modifying m_parent
    std::deque<EDIT_LINE> m_lines;      ///< EDIT_LINEs for modifying m_parent
};


/**
 * Class EPC_VERTICAL.
 *
 * EDIT_POINT_CONSTRAINT that imposes a constraint that two points have to have the same X coordinate.
 */
class EPC_VERTICAL : public EDIT_POINT_CONSTRAINT
{
public:
    /**
     * Constructor.
     *
     * @param aConstrained is the point that is put under constrain.
     * @param aConstrainer is the point that is the source of the constrain.
     */
    EPC_VERTICAL( EDIT_POINT& aConstrained, const EDIT_POINT& aConstrainer ) :
        EDIT_POINT_CONSTRAINT( aConstrained ), m_constrainer( aConstrainer )
    {}

    ///> @copydoc EDIT_POINT_CONSTRAINT::Apply()
    virtual void Apply()
    {
        VECTOR2I point = m_constrained.GetPosition();
        point.x = m_constrainer.GetPosition().x;
        m_constrained.SetPosition( point );
    }

private:
    const EDIT_POINT& m_constrainer;      ///< Point that imposes the constraint.
};


/**
 * Class EPC_HORIZONTAL.
 *
 * EDIT_POINT_CONSTRAINT that imposes a constraint that two points have to have the same Y coordinate.
 */
class EPC_HORIZONTAL : public EDIT_POINT_CONSTRAINT
{
public:
    /**
     * Constructor.
     *
     * @param aConstrained is the point that is put under constrain.
     * @param aConstrainer is the point that is the source of the constrain.
     */
    EPC_HORIZONTAL( EDIT_POINT& aConstrained, const EDIT_POINT& aConstrainer ) :
        EDIT_POINT_CONSTRAINT( aConstrained ), m_constrainer( aConstrainer )
    {}

    ///> @copydoc EDIT_POINT_CONSTRAINT::Apply()
    virtual void Apply()
    {
        VECTOR2I point = m_constrained.GetPosition();
        point.y = m_constrainer.GetPosition().y;
        m_constrained.SetPosition( point );
    }

private:
    const EDIT_POINT& m_constrainer;    ///< Point that imposes the constraint.
};


/**
 * Class EPC_45DEGREE
 *
 * EDIT_POINT_CONSTRAINT that imposes a constraint that two to be located at angle of 45 degree
 * multiplicity.
 */
class EPC_45DEGREE : public EDIT_POINT_CONSTRAINT
{
public:
    /**
     * Constructor.
     *
     * @param aConstrained is the point that is put under constrain.
     * @param aConstrainer is the point that is the source of the constrain.
     */
    EPC_45DEGREE( EDIT_POINT& aConstrained, const EDIT_POINT& aConstrainer ) :
        EDIT_POINT_CONSTRAINT( aConstrained ), m_constrainer( aConstrainer )
    {}

    ///> @copydoc EDIT_POINT_CONSTRAINT::Apply()
    virtual void Apply();

private:
    const EDIT_POINT& m_constrainer;    ///< Point that imposes the constraint.
};


/**
 * Class EPC_LINE
 *
 * EDIT_POINT_CONSTRAINT that imposes a constraint that a point has to lie on a line (determined
 * by 2 points).
 */
class EPC_LINE : public EDIT_POINT_CONSTRAINT
{
public:
    EPC_LINE( EDIT_POINT& aConstrained, EDIT_POINT& aConstrainer );

    ///> @copydoc EDIT_POINT_CONSTRAINT::Apply()
    virtual void Apply();

    /**
     * Function Update()
     * Updates line coefficients that make the constraining line.
     */
    void Update();

private:
    EDIT_POINT& m_constrainer;    ///< Point that imposes the constraint.
    double m_coefA, m_coefB;
};


/**
 * Class EPC_CIRCLE.
 *
 * EDIT_POINT_CONSTRAINT that imposes a constraint that a point has to lie on a circle.
 */
class EPC_CIRCLE : public EDIT_POINT_CONSTRAINT
{
public:
    /**
     * Constructor.
     *
     * @param aConstrained is the point that is put under constrain.
     * @parama aCenter is the point that is the center of the circle.
     * @parama aEnd is the point that decides on the radius of the circle.
     */
    EPC_CIRCLE( EDIT_POINT& aConstrained, const EDIT_POINT& aCenter, const EDIT_POINT& aEnd ) :
        EDIT_POINT_CONSTRAINT( aConstrained ), m_center( aCenter ), m_end( aEnd )
    {}

    ///> @copydoc EDIT_POINT_CONSTRAINT::Apply()
    virtual void Apply();

private:
    ///> Point that imposes the constraint (center of the circle).
    const EDIT_POINT& m_center;

    ///> Point that imposes the constraint (decides on the radius of the circle).
    const EDIT_POINT& m_end;
};

#endif /* EDIT_POINTS_H_ */
