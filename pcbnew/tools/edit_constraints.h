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

#ifndef EDIT_CONSTRAINTS_H_
#define EDIT_CONSTRAINTS_H_

#include <math/vector2d.h>
#include <boost/function.hpp>

class EDIT_POINT;
class EDIT_LINE;
class EDIT_POINTS;

/**
 * Class EDIT_CONSTRAINT
 *
 * Allows to describe constraints between two edit handles. After the constrained handle is changed,
 * Apply() has to be called to fix its coordinates according to the implemented constraint.
 */
template<class EDIT_TYPE>
class EDIT_CONSTRAINT
{
public:
    /**
     * Constructor
     *
     * @param aConstrained is EDIT_POINT to which the constraint is applied.
     */
    EDIT_CONSTRAINT( EDIT_TYPE& aConstrained ) : m_constrained( aConstrained ) {};

    virtual ~EDIT_CONSTRAINT() {};

    /**
     * Function Apply()
     *
     * Corrects coordinates of the constrained edit handle.
     */
    virtual void Apply( EDIT_TYPE& aHandle ) = 0;

    /**
     * Function Apply()
     *
     * Corrects coordinates of the constrained edit handle.
     */
    void Apply()
    {
        Apply( m_constrained );
    }

protected:
    EDIT_TYPE& m_constrained;      ///< Point that is constrained by rules implemented by Apply()
};


/**
 * Class EC_VERTICAL.
 *
 * EDIT_CONSTRAINT that imposes a constraint that two points have to have the same X coordinate.
 */
class EC_VERTICAL : public EDIT_CONSTRAINT<EDIT_POINT>
{
public:
    /**
     * Constructor.
     *
     * @param aConstrained is the point that is put under constrain.
     * @param aConstrainer is the point that is the source of the constrain.
     */
    EC_VERTICAL( EDIT_POINT& aConstrained, const EDIT_POINT& aConstrainer ) :
        EDIT_CONSTRAINT<EDIT_POINT>( aConstrained ), m_constrainer( aConstrainer )
    {}

    ///> @copydoc EDIT_CONSTRAINT::Apply()
    virtual void Apply( EDIT_POINT& aHandle );

private:
    const EDIT_POINT& m_constrainer;      ///< Point that imposes the constraint.
};


/**
 * Class EC_HORIZONTAL.
 *
 * EDIT_CONSTRAINT that imposes a constraint that two points have to have the same Y coordinate.
 */
class EC_HORIZONTAL : public EDIT_CONSTRAINT<EDIT_POINT>
{
public:
    /**
     * Constructor.
     *
     * @param aConstrained is the point that is put under constrain.
     * @param aConstrainer is the point that is the source of the constrain.
     */
    EC_HORIZONTAL( EDIT_POINT& aConstrained, const EDIT_POINT& aConstrainer ) :
        EDIT_CONSTRAINT<EDIT_POINT>( aConstrained ), m_constrainer( aConstrainer )
    {}

    ///> @copydoc EDIT_CONSTRAINT::Apply()
    virtual void Apply( EDIT_POINT& aHandle );

private:
    const EDIT_POINT& m_constrainer;    ///< Point that imposes the constraint.
};


/**
 * Class EC_45DEGREE
 *
 * EDIT_CONSTRAINT that imposes a constraint that two points have to be located at angle of 45
 * degree multiplicity.
 */
class EC_45DEGREE : public EDIT_CONSTRAINT<EDIT_POINT>
{
public:
    /**
     * Constructor.
     *
     * @param aConstrained is the point that is put under constrain.
     * @param aConstrainer is the point that is the source of the constrain.
     */
    EC_45DEGREE( EDIT_POINT& aConstrained, const EDIT_POINT& aConstrainer ) :
        EDIT_CONSTRAINT<EDIT_POINT>( aConstrained ), m_constrainer( aConstrainer )
    {}

    ///> @copydoc EDIT_CONSTRAINT::Apply()
    virtual void Apply( EDIT_POINT& aHandle );

private:
    const EDIT_POINT& m_constrainer;    ///< Point that imposes the constraint.
};


/**
 * Class EC_LINE
 *
 * EDIT_CONSTRAINT that imposes a constraint that a point has to lie on a line (determined
 * by 2 points).
 */
class EC_LINE : public EDIT_CONSTRAINT<EDIT_POINT>
{
public:
    EC_LINE( EDIT_POINT& aConstrained, const EDIT_POINT& aConstrainer );

    ///> @copydoc EDIT_CONSTRAINT::Apply()
    virtual void Apply( EDIT_POINT& aHandle );

private:
    const EDIT_POINT& m_constrainer;    ///< Point that imposes the constraint.
    VECTOR2I m_line;                    ///< Vector representing the constraining line.
};


/**
 * Class EC_CIRCLE.
 *
 * EDIT_CONSTRAINT that imposes a constraint that a point has to lie on a circle.
 */
class EC_CIRCLE : public EDIT_CONSTRAINT<EDIT_POINT>
{
public:
    /**
     * Constructor.
     *
     * @param aConstrained is the point that is put under constrain.
     * @parama aCenter is the point that is the center of the circle.
     * @parama aEnd is the point that decides on the radius of the circle.
     */
    EC_CIRCLE( EDIT_POINT& aConstrained, const EDIT_POINT& aCenter, const EDIT_POINT& aEnd ) :
        EDIT_CONSTRAINT<EDIT_POINT>( aConstrained ), m_center( aCenter ), m_end( aEnd )
    {}

    ///> @copydoc EDIT_CONSTRAINT::Apply()
    virtual void Apply( EDIT_POINT& aHandle );

private:
    ///> Point that imposes the constraint (center of the circle).
    const EDIT_POINT& m_center;

    ///> Point that imposes the constraint (decides on the radius of the circle).
    const EDIT_POINT& m_end;
};


/**
 * Class EC_CONVERGING
 *
 * EDIT_CONSTRAINT for 3 segments: dragged and two adjacent ones, enforcing to keep their slopes
 * and allows only to change ending points. Applied to zones.
 */
class EC_CONVERGING : public EDIT_CONSTRAINT<EDIT_LINE>
{
public:
    EC_CONVERGING( EDIT_LINE& aLine, EDIT_POINTS& aPoints );

    virtual ~EC_CONVERGING();

    ///> @copydoc EDIT_CONSTRAINT::Apply()
    virtual void Apply( EDIT_LINE& aHandle );

private:
    ///> Constraint for origin side segment.
    EDIT_CONSTRAINT<EDIT_POINT>* m_originSideConstraint;

    ///> Constraint for end side segment.
    EDIT_CONSTRAINT<EDIT_POINT>* m_endSideConstraint;

    ///> Additional constriant, applied when at least two points are collinear. It is a pointer to
    ///> m_[origin/end]SideConstraint, so it should not be freed.
    EDIT_CONSTRAINT<EDIT_POINT>* m_colinearConstraint;

    ///> EDIT_POINTS instance that stores currently modified lines.
    EDIT_POINTS& m_editPoints;

    ///> Vector that represents the initial direction of the dragged segment.
    VECTOR2I m_draggedVector;
};


/**
 * Class EC_SNAPLINE
 *
 * EDIT_CONSTRAINT for a EDIT_LINE, one of the ends is snapped to a spot determined by a
 * transform function passed as parameter (e.g. it can be snapped to a grid), instead of having
 * the line center snapped to a point.
 */
class EC_SNAPLINE : public EDIT_CONSTRAINT<EDIT_LINE>
{
public:
    ///> Typedef for a function that determines snapping point.
    typedef boost::function<VECTOR2D (const VECTOR2D&)> V2D_TRANSFORM_FUN;

    EC_SNAPLINE( EDIT_LINE& aLine, V2D_TRANSFORM_FUN aSnapFun );

    virtual ~EC_SNAPLINE()
    {}

    ///> @copydoc EDIT_CONSTRAINT::Apply()
    virtual void Apply( EDIT_LINE& aHandle );

private:
    ///> Function that determines snapping point.
    V2D_TRANSFORM_FUN m_snapFun;
};

#endif /* EDIT_CONSTRAINTS_H_ */
