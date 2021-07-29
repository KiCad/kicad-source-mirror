/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2014-2017 CERN
 * Copyright (C) 2020-2021 KiCad Developers, see AUTHORS.txt for contributors.
 *
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

#include <eda_item.h>
#include <layer_ids.h>

#include <list>
#include <deque>
#include <memory>

#include "edit_constraints.h"
#include <view/view.h>


/**
 * Represent a single point that can be used for modifying items.
 *
 * It is directly related to one of points in a graphical item (e.g. vertex of a zone or
 * center of a circle).
 */
class EDIT_POINT
{
public:
    /**
     * @param aPoint stores coordinates for EDIT_POINT.
     */
    EDIT_POINT( const VECTOR2I& aPoint, std::pair<EDA_ITEM*, int> aConnected = { nullptr, 0 } ) :
            m_position( aPoint ),
            m_isActive( false ),
            m_isHover( false ),
            m_gridConstraint( SNAP_TO_GRID ),
            m_connected( aConnected )
    {
    }

    virtual ~EDIT_POINT() {}

    /**
     * Return coordinates of an EDIT_POINT.
     *
     * @note It may be different than coordinates of a graphical item that is bound to the
     *       EDIT_POINT.
     */
    virtual VECTOR2I GetPosition() const
    {
        return m_position;
    }

    /**
     * Return a connected item record comprising an EDA_ITEM* and a STARTPOINT/ENDPOINT flag.
     */
    virtual std::pair<EDA_ITEM*, int> GetConnected() const
    {
        return m_connected;
    }

    /**
     * Return X coordinate of an EDIT_POINT.
     */
    inline int GetX() const
    {
        return GetPosition().x;
    }

    /**
     * Return Y coordinate of an EDIT_POINT.
     */
    inline int GetY() const
    {
        return GetPosition().y;
    }

    /**
     * Set new coordinates for an EDIT_POINT.
     *
     * It does not change the coordinates of a graphical item.
     *
     * @param aPosition are new coordinates.
     */
    virtual void SetPosition( const VECTOR2I& aPosition )
    {
        m_position = aPosition;
    }

    virtual void SetPosition( int x, int y )
    {
        m_position.x = x;
        m_position.y = y;
    }

    /**
     * Check if given point is within a square centered in the EDIT_POINT position.
     *
     * @param aPoint is point to be checked.
     * @param aSize is length of the square side.
     */
    bool WithinPoint( const VECTOR2I& aPoint, unsigned int aSize ) const;

    /**
     * Set a constraint for and EDIT_POINT.
     *
     * @param aConstraint is the constraint to be set.
     */
    void SetConstraint( EDIT_CONSTRAINT<EDIT_POINT>* aConstraint )
    {
        m_constraint.reset( aConstraint );
    }

    /**
     * Return the constraint imposed on an EDIT_POINT. If there are no constraints, NULL is
     * returned.
     */
    EDIT_CONSTRAINT<EDIT_POINT>* GetConstraint() const
    {
        return m_constraint.get();
    }

    /**
     * Remove previously set constraint.
     */
    inline void ClearConstraint()
    {
        m_constraint.reset();
    }

    /**
     * Check if point is constrained.
     *
     * @return True is point is constrained, false otherwise.
     */
    inline bool IsConstrained() const
    {
        return m_constraint != nullptr;
    }

    /**
     * Correct coordinates of an EDIT_POINT by applying previously set constraint.
     */
    virtual void ApplyConstraint()
    {
        if( m_constraint )
            m_constraint->Apply();
    }

    bool IsActive() const { return m_isActive; }
    void SetActive( bool aActive = true ) { m_isActive = aActive; }

    bool IsHover() const { return m_isHover; }
    void SetHover( bool aHover = true ) { m_isHover = aHover; }

    GRID_CONSTRAINT_TYPE GetGridConstraint() const { return m_gridConstraint; }
    void SetGridConstraint( GRID_CONSTRAINT_TYPE aConstraint ) { m_gridConstraint = aConstraint; }

    bool operator==( const EDIT_POINT& aOther ) const
    {
        return m_position == aOther.m_position;
    }

    ///< Single point size in pixels
    static const int POINT_SIZE = 10;

    ///< Border size when not hovering
    static const int BORDER_SIZE = 2;

    ///< Border size when hovering
    static const int HOVER_SIZE = 5;

private:
    VECTOR2I             m_position;        ///< Position of EDIT_POINT.
    bool                 m_isActive;        ///< True if this point is being manipulated.
    bool                 m_isHover;         ///< True if this point is being hovered over.
    GRID_CONSTRAINT_TYPE m_gridConstraint;  ///< Describe the grid snapping behavior.

    ///< An optional connected item record used to mimic polyLine behavior with individual
    ///< line segments.
    std::pair<EDA_ITEM*, int>                     m_connected;

    ///< Constraint for the point, NULL if none
    std::shared_ptr<EDIT_CONSTRAINT<EDIT_POINT> > m_constraint;
};


/**
 * Represent a line connecting two EDIT_POINTs.
 *
 * This allows one to move them both by dragging the EDIT_POINT in the middle.  It uses
 * references to EDIT_POINTs, all coordinates are automatically synchronized.
 */
class EDIT_LINE : public EDIT_POINT
{
public:
    /**
     * @param aOrigin is the origin of EDIT_LINE.
     * @param aEnd is the end of EDIT_LINE.
     */
    EDIT_LINE( EDIT_POINT& aOrigin, EDIT_POINT& aEnd ) :
        EDIT_POINT( aOrigin.GetPosition() + ( aEnd.GetPosition() - aOrigin.GetPosition() ) / 2 ),
        m_origin( aOrigin ),
        m_end( aEnd )
    {
        SetGridConstraint( SNAP_BY_GRID );
    }

    ///< @copydoc EDIT_POINT::GetPosition()
    virtual VECTOR2I GetPosition() const override
    {
        return ( m_origin.GetPosition() + m_end.GetPosition() ) / 2;
    }

    ///< @copydoc EDIT_POINT::GetPosition()
    virtual void SetPosition( const VECTOR2I& aPosition ) override
    {
        VECTOR2I difference = aPosition - GetPosition();

        m_origin.SetPosition( m_origin.GetPosition() + difference );
        m_end.SetPosition( m_end.GetPosition() + difference );
    }

    ///< @copydoc EDIT_POINT::ApplyConstraint()
    virtual void ApplyConstraint() override
    {
        if( m_constraint )
            m_constraint->Apply();

        m_origin.ApplyConstraint();
        m_end.ApplyConstraint();
    }

    /**
     * Set a constraint for and EDIT_POINT.
     *
     * @param aConstraint is the constraint to be set.
     */
    void SetConstraint( EDIT_CONSTRAINT<EDIT_LINE>* aConstraint )
    {
        m_constraint.reset( aConstraint );
    }

    /**
     * Return the constraint imposed on an EDIT_POINT. If there are no constraints, NULL is
     * returned.
     */
    EDIT_CONSTRAINT<EDIT_LINE>* GetConstraint() const
    {
        return m_constraint.get();
    }

    /**
     * Return the origin EDIT_POINT.
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
     * Return the end EDIT_POINT.
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

    ///< Constraint for the point, NULL if none
    std::shared_ptr<EDIT_CONSTRAINT<EDIT_LINE> > m_constraint;
};


/**
 * EDIT_POINTS is a #VIEW_ITEM that manages EDIT_POINTs and EDIT_LINEs and draws them.
 */
class EDIT_POINTS : public EDA_ITEM
{
public:
    /**
     * @param aParent is the item to which EDIT_POINTs are related.
     */
    EDIT_POINTS( EDA_ITEM* aParent );

    /**
     * Return a point that is at given coordinates or NULL if there is no such point.
     *
     * @param aLocation is the location for searched point.
     */
    EDIT_POINT* FindPoint( const VECTOR2I& aLocation, KIGFX::VIEW *aView );

    /**
     * Return parent of the EDIT_POINTS.
     */
    EDA_ITEM* GetParent() const
    {
        return m_parent;
    }

    /**
     * Add an EDIT_POINT.
     *
     * @param aPoint is the new point.
     */
    void AddPoint( const EDIT_POINT& aPoint )
    {
        m_points.push_back( aPoint );
    }

    /**
     * Add an EDIT_POINT.
     *
     * @param aPoint are coordinates of the new point.
     */
    void AddPoint( const VECTOR2I& aPoint, std::pair<EDA_ITEM*, int> aConnected = { nullptr, 0 } )
    {
        AddPoint( EDIT_POINT( aPoint, aConnected ) );
    }

    /**
     * Adds an EDIT_LINE.
     *
     * @param aLine is the new line.
     */
    void AddLine( const EDIT_LINE& aLine )
    {
        m_lines.push_back( aLine );
    }

    /**
     * Adds an EDIT_LINE.
     *
     * @param aOrigin is the origin for a new line.
     * @param aEnd is the end for a new line.
     */
    void AddLine( EDIT_POINT& aOrigin, EDIT_POINT& aEnd )
    {
        m_lines.emplace_back( aOrigin, aEnd );
    }

    /**
     * Adds a break, indicating the end of a contour.
     */
    void AddBreak()
    {
        assert( m_points.size() > 0 );
        m_contours.push_back( m_points.size() - 1 );
    }

    /**
     * Return index of the contour origin for a point with given index.
     *
     * @param aPointIdx is the index of point for which the contour origin is searched.
     * @return Index of the contour origin point.
     */
    int GetContourStartIdx( int aPointIdx ) const;

    /**
     * Return index of the contour finish for a point with given index.
     *
     * @param aPointIdx is the index of point for which the contour finish is searched.
     * @return Index of the contour finish point.
     */
    int GetContourEndIdx( int aPointIdx ) const;

    /**
     * Check if a point with given index is a contour origin.
     *
     * @param aPointIdx is the index of the point to be checked.
     * @return True if the point is an origin of a contour.
     */
    bool IsContourStart( int aPointIdx ) const;

    /**
     * Check is a point with given index is a contour finish.
     *
     * @param aPointIdx is the index of the point to be checked.
     * @return True if the point is a finish of a contour.
     */
    bool IsContourEnd( int aPointIdx ) const;

    /**
     * Return the point that is after the given point in the list.
     *
     * @param aPoint is the point that is supposed to be preceding the searched point.
     * @param aTraverseContours decides if in case of breaks should we return to the origin
     *                          of contour or continue with the next contour.
     * @return The point following aPoint in the list. If aPoint is the first in
     *         the list, the last from the list will be returned. If there are no points at
     *         all, NULL is returned.
     */
    EDIT_POINT* Previous( const EDIT_POINT& aPoint, bool aTraverseContours = true );

    EDIT_LINE* Previous( const EDIT_LINE& aLine );

    /**
     * Return the point that is before the given point in the list.
     *
     * @param aPoint is the point that is supposed to be following the searched point.
     * @param aTraverseContours decides if in case of breaks should we return to the origin
     *                          of contour or continue with the next contour.
     * @return The point preceding aPoint in the list. If aPoint is the last in the list, the
     *         first point from the list will be returned. If there are no points at all,
     *         NULL is returned.
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
     * Return number of stored EDIT_POINTs.
     */
    unsigned int PointsSize() const
    {
        return m_points.size();
    }

    /**
     * Return number of stored EDIT_LINEs.
     */
    unsigned int LinesSize() const
    {
        return m_lines.size();
    }

    ///< @copydoc VIEW_ITEM::ViewBBox()
    virtual const BOX2I ViewBBox() const override;

    ///< @copydoc VIEW_ITEM::ViewDraw()
    virtual void ViewDraw( int aLayer, KIGFX::VIEW* aView ) const override;

    ///< @copydoc VIEW_ITEM::ViewGetLayers()
    virtual void ViewGetLayers( int aLayers[], int& aCount ) const override
    {
        aCount = 1;
        aLayers[0] = LAYER_GP_OVERLAY ;
    }

#if defined(DEBUG)
    void Show( int x, std::ostream& st ) const override
    {
    }
#endif

    /**
     * Get the class name.
     *
     * @return string "EDIT_POINTS".
     */
    virtual wxString GetClass() const override
    {
        return wxT( "EDIT_POINTS" );
    }

private:
    EDA_ITEM* m_parent;                 ///< Parent of the EDIT_POINTs.
    std::deque<EDIT_POINT> m_points;    ///< EDIT_POINTs for modifying m_parent.
    std::deque<EDIT_LINE> m_lines;      ///< EDIT_LINEs for modifying m_parent.
    std::list<int> m_contours;          ///< Indices of end contour points.
    bool m_allowPoints;                 ///< If false, only allow editing of EDIT_LINES.
};

#endif /* EDIT_POINTS_H_ */
