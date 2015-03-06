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

#include <boost/shared_ptr.hpp>

#include <base_struct.h>
#include <layers_id_colors_and_visibility.h>

#include "edit_constraints.h"

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
        m_position( aPoint ) {};

    virtual ~EDIT_POINT() {}

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
     * Function GetY()
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
    void SetConstraint( EDIT_CONSTRAINT<EDIT_POINT>* aConstraint )
    {
        m_constraint.reset( aConstraint );
    }

    /**
     * Function GetConstraint()
     *
     * Returns the constraint imposed on an EDIT_POINT. If there are no constraints, NULL is
     * returned.
     */
    EDIT_CONSTRAINT<EDIT_POINT>* GetConstraint() const
    {
        return m_constraint.get();
    }

    /**
     * Function ClearConstraint()
     *
     * Removes previously set constraint.
     */
    void ClearConstraint()
    {
        m_constraint.reset();
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
    virtual void ApplyConstraint()
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

private:
    ///> Position of EDIT_POINT
    VECTOR2I m_position;

    ///> Constraint for the point, NULL if none
    boost::shared_ptr<EDIT_CONSTRAINT<EDIT_POINT> > m_constraint;
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
        return ( m_origin.GetPosition() + m_end.GetPosition() ) / 2;
    }

    ///> @copydoc EDIT_POINT::GetPosition()
    virtual void SetPosition( const VECTOR2I& aPosition )
    {
        VECTOR2I difference = aPosition - GetPosition();

        m_origin.SetPosition( m_origin.GetPosition() + difference );
        m_end.SetPosition( m_end.GetPosition() + difference );
    }

    ///> @copydoc EDIT_POINT::ApplyConstraint()
    virtual void ApplyConstraint()
    {
        m_origin.ApplyConstraint();
        m_end.ApplyConstraint();

        if( m_constraint )
            m_constraint->Apply();
    }

    /**
     * Function SetConstraint()
     *
     * Sets a constraint for and EDIT_POINT.
     * @param aConstraint is the constraint to be set.
     */
    void SetConstraint( EDIT_CONSTRAINT<EDIT_LINE>* aConstraint )
    {
        m_constraint.reset( aConstraint );
    }

    /**
     * Function GetConstraint()
     *
     * Returns the constraint imposed on an EDIT_POINT. If there are no constraints, NULL is
     * returned.
     */
    EDIT_CONSTRAINT<EDIT_LINE>* GetConstraint() const
    {
        return m_constraint.get();
    }

    /**
     * Function GetOrigin()
     *
     * Returns the origin EDIT_POINT.
     */
    EDIT_POINT& GetOrigin()
    {
        return m_origin;
    }

    const EDIT_POINT& GetOrigin() const
    {
        return m_origin;
    }

    /**
     * Function GetEnd()
     *
     * Returns the end EDIT_POINT.
     */
    EDIT_POINT& GetEnd()
    {
        return m_end;
    }

    const EDIT_POINT& GetEnd() const
    {
        return m_end;
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

    ///> Constraint for the point, NULL if none
    boost::shared_ptr<EDIT_CONSTRAINT<EDIT_LINE> > m_constraint;
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
     * Function AddBreak()
     *
     * Adds a break, indicating the end of a contour.
     */
    void AddBreak()
    {
        assert( m_points.size() > 0 );
        m_contours.push_back( m_points.size() - 1 );
    }

    /**
     * Function GetContourStartIdx()
     *
     * Returns index of the contour origin for a point with given index.
     * @param aPointIdx is the index of point for which the contour origin is searched.
     * @return Index of the contour origin point.
     */
    int GetContourStartIdx( int aPointIdx ) const;

    /**
     * Function GetContourEndIdx()
     *
     * Returns index of the contour finish for a point with given index.
     * @param aPointIdx is the index of point for which the contour finish is searched.
     * @return Index of the contour finish point.
     */
    int GetContourEndIdx( int aPointIdx ) const;

    /**
     * Function IsContourStart()
     *
     * Checks is a point with given index is a contour origin.
     * @param aPointIdx is the index of the point to be checked.
     * @return True if the point is an origin of a contour.
     */
    bool IsContourStart( int aPointIdx ) const;

    /**
     * Function IsContourEnd()
     *
     * Checks is a point with given index is a contour finish.
     * @param aPointIdx is the index of the point to be checked.
     * @return True if the point is a finish of a contour.
     */
    bool IsContourEnd( int aPointIdx ) const;

    /**
     * Function Previous()
     *
     * Returns the point that is after the given point in the list.
     * @param aPoint is the point that is supposed to be preceding the searched point.
     * @param aTraverseContours decides if in case of breaks should we return to the origin
     * of contour or continue with the next contour.
     * @return The point following aPoint in the list. If aPoint is the first in
     * the list, the last from the list will be returned. If there are no points at all, NULL
     * is returned.
     */
    EDIT_POINT* Previous( const EDIT_POINT& aPoint, bool aTraverseContours = true );

    EDIT_LINE* Previous( const EDIT_LINE& aLine );

    /**
     * Function Next()
     *
     * Returns the point that is before the given point in the list.
     * @param aPoint is the point that is supposed to be following the searched point.
     * @param aTraverseContours decides if in case of breaks should we return to the origin
     * of contour or continue with the next contour.
     * @return The point preceding aPoint in the list. If aPoint is the last in
     * the list, the first point from the list will be returned. If there are no points at all,
     * NULL is returned.
     */
    EDIT_POINT* Next( const EDIT_POINT& aPoint, bool aTraverseContours = true );

    EDIT_LINE* Next( const EDIT_LINE& aLine );

    EDIT_POINT& Point( unsigned int aIndex )
    {
        return m_points[aIndex];
    }

    const EDIT_POINT& Point( unsigned int aIndex ) const
    {
        return m_points[aIndex];
    }

    EDIT_LINE& Line( unsigned int aIndex )
    {
        return m_lines[aIndex];
    }

    const EDIT_LINE& Line( unsigned int aIndex ) const
    {
        return m_lines[aIndex];
    }

    /**
     * Function PointsSize()
     *
     * Returns number of stored EDIT_POINTs.
     */
    unsigned int PointsSize() const
    {
        return m_points.size();
    }

    /**
     * Function LinesSize()
     *
     * Returns number of stored EDIT_LINEs.
     */
    unsigned int LinesSize() const
    {
        return m_lines.size();
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

#if defined(DEBUG)
    void Show( int x, std::ostream& st ) const
    {
    }
#endif

    /** Get class name
     * @return  string "EDIT_POINTS"
     */
    virtual wxString GetClass() const
    {
        return wxT( "EDIT_POINTS" );
    }

private:
    EDA_ITEM* m_parent;                 ///< Parent of the EDIT_POINTs
    std::deque<EDIT_POINT> m_points;    ///< EDIT_POINTs for modifying m_parent
    std::deque<EDIT_LINE> m_lines;      ///< EDIT_LINEs for modifying m_parent
    std::list<int> m_contours;          ///< Indices of end contour points
};

#endif /* EDIT_POINTS_H_ */
