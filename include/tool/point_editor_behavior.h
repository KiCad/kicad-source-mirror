/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
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


#pragma once

#include <wx/debug.h>

#include <settings/app_settings.h>
#include <eda_shape.h>
#include <tool/edit_points.h>
#include <view/view_controls.h>

class SHAPE_POLY_SET;

/**
 * A helper class interface to manage the edit points for a single item.
 * Create one ofthese, and it will provide a way to keep a list of points updated.
 *
 * For the moment this is implemented such that it mutates an external #EDIT_POINTS object,
 * but it might be able to also own the points.
 */
class POINT_EDIT_BEHAVIOR
{
public:
    virtual ~POINT_EDIT_BEHAVIOR() = default;

    /**
     * Construct the initial set of edit points for the item
     * and append to the given list.
     *
     * @param aPoints The list of edit points to fill.
     */
    virtual void MakePoints( EDIT_POINTS& aPoints ) = 0;

    /**
     * Update the list of the edit points for the item.
     *
     * Be very careful not to overrun the list of points - this class knows how big they are
     * because it made them in the first place.
     *
     * If item has changed such that that number of points needs to change, this method has to
     * handle that (probably by clearing the list and refilling it).
     *
     * If the behavior itself must change (for instance, a rectangle is non-cardinallly rotated
     * to a polygon), the method should return false.
     *
     * @param aPoints The list of edit points to update.
     */
    virtual bool UpdatePoints( EDIT_POINTS& aPoints ) = 0;

    /**
     * Finalize the edit operation. (optional)
     *
     * This is called once, after the user has finished editing a point (e.g. released the
     * mouse button).
     *
     * @param aPoints The final positions of the edit points.
     * @param aCommit The commit object to use to modify the item.
     */
    virtual void FinalizeItem( EDIT_POINTS& aPoints, COMMIT& aCommit ) {};

    /**
     * Update the item with the new positions of the edit points.
     *
     * This method should all commit and add to the update list anything that is NOT the
     * parent item of the EDIT_POINTs. For example, connected lines, parent tables, etc. The
     * item itself is already handled (most behaviors don't need more than that).
     *
     * @param aEditedPoint The point that was dragged.
     *                     You can use this to check by address which point to update.
     * @param aPoints The new positions of the edit points.
     * @param aCommit The commit object to use to modify the item.
     * @param aUpdatedItems The list of items that were updated by the edit (not only the
     *                      item that was being edited, but also any other items that were
     *                      affected, e.g. by being conneted to the edited item).
     */
    virtual void UpdateItem( const EDIT_POINT& aEditedPoint, EDIT_POINTS& aPoints, COMMIT& aCommit,
                             std::vector<EDA_ITEM*>& aUpdatedItems ) = 0;

    /**
     * Get the 45-degree constrainer for the item, when the given point is moved.
     * Return std::nullopt if not, and the caller can decide.
     *
     * If you want to actively disable constraining, return the aEditedPoint
     * position.
     */
    virtual OPT_VECTOR2I Get45DegreeConstrainer( const EDIT_POINT& aEditedPoint,
                                                 EDIT_POINTS&      aPoints ) const
    {
        // By default, no constrainer is defined and the caller must decide
        return std::nullopt;
    }

protected:
    /**
     * Checks if two points are the same instance - which means the point is being edited.
     */
    static bool isModified( const EDIT_POINT& aEditedPoint, const EDIT_POINT& aPoint )
    {
        return &aEditedPoint == &aPoint;
    }
};

// Helper macros to check the number of points in the edit points object
// Still a bug, but at least it won't segfault if the number of points is wrong
#define CHECK_POINT_COUNT( aPoints, aExpected ) \
    wxCHECK( aPoints.PointsSize() == aExpected, /* void */ )
#define CHECK_POINT_COUNT_GE( aPoints, aExpected ) \
    wxCHECK( aPoints.PointsSize() >= aExpected, /* void */ )


/**
 * Class that implements "standard" polygon editing behavior.
 *
 * You still need to implement the POINT_EDIT_BEHAVIOR interface (in particular, you may
 * need to construct a poly set from or apply the poly set to an actual object) but you can
 * use the helper methods in this class to do the actual work.
 */
class POLYGON_POINT_EDIT_BEHAVIOR : public POINT_EDIT_BEHAVIOR
{
public:
    POLYGON_POINT_EDIT_BEHAVIOR( SHAPE_POLY_SET& aPolygon ) :
            m_polygon( aPolygon )
    {}

    /**
     * Build the edit points for the given polygon outline.
     */
    static void BuildForPolyOutline( EDIT_POINTS& aPoints, const SHAPE_POLY_SET& aOutline );

    /**
     * Update the edit points with the current polygon outline.
     *
     * If the point sizes differ, the points are rebuilt entirely (in-place)
     */
    static void UpdatePointsFromOutline( const SHAPE_POLY_SET& aOutline, EDIT_POINTS& aPoints );

    /**
     * Update the polygon outline with the new positions of the edit points.
     */
    static void UpdateOutlineFromPoints( SHAPE_POLY_SET& aOutline, const EDIT_POINT& aEditedPoint,
                                         EDIT_POINTS& aPoints );

    void MakePoints( EDIT_POINTS& aPoints ) override
    {
        BuildForPolyOutline( aPoints, m_polygon );
    }

    bool UpdatePoints( EDIT_POINTS& aPoints ) override
    {
        UpdatePointsFromOutline( m_polygon, aPoints );
        return true;
    }

    void UpdateItem( const EDIT_POINT& aEditedPoint, EDIT_POINTS& aPoints, COMMIT& aCommit,
                     std::vector<EDA_ITEM*>& aUpdatedItems ) override
    {
        UpdateOutlineFromPoints( m_polygon, aEditedPoint, aPoints );
    }

    void FinalizeItem( EDIT_POINTS& aPoints, COMMIT& aCommit ) override;

private:
    SHAPE_POLY_SET& m_polygon;
};


/**
 * "Standard" polygon editing behavior for EDA_SHAPE polygons.
 *
 * As long as updating the EDA_SHAPE's SHAPE_POLY_SET in-place is enough,
 * this will do the job.
 */
class EDA_POLYGON_POINT_EDIT_BEHAVIOR : public POLYGON_POINT_EDIT_BEHAVIOR
{
public:
    // Editing the underlying polygon shape in-place is enough
    EDA_POLYGON_POINT_EDIT_BEHAVIOR( EDA_SHAPE& aPolygon ) :
            POLYGON_POINT_EDIT_BEHAVIOR( aPolygon.GetPolyShape() )
    {
        wxASSERT( aPolygon.GetShape() == SHAPE_T::POLY );
    }

    void UpdateItem( const EDIT_POINT& aEditedPoint, EDIT_POINTS& aPoints, COMMIT& aCommit,
                     std::vector<EDA_ITEM*>& aUpdatedItems ) override
    {
        POLYGON_POINT_EDIT_BEHAVIOR::UpdateItem( aEditedPoint, aPoints, aCommit, aUpdatedItems );
    }
};


/**
 * "Standard" segment editing behavior for EDA_SHAPE segments.
 */
class EDA_SEGMENT_POINT_EDIT_BEHAVIOR : public POINT_EDIT_BEHAVIOR
{
public:
    EDA_SEGMENT_POINT_EDIT_BEHAVIOR( EDA_SHAPE& aSegment ) :
            m_segment( aSegment )
    {
        wxASSERT( aSegment.GetShape() == SHAPE_T::SEGMENT );
    }

    void MakePoints( EDIT_POINTS& aPoints ) override;

    bool UpdatePoints( EDIT_POINTS& aPoints ) override;

    void UpdateItem( const EDIT_POINT& aEditedPoint, EDIT_POINTS& aPoints, COMMIT& aCommit,
                     std::vector<EDA_ITEM*>& aUpdatedItems ) override;

    OPT_VECTOR2I Get45DegreeConstrainer( const EDIT_POINT& aEditedPoint,
                                         EDIT_POINTS&      aPoints ) const override;

protected:
    enum SEGMENT_POINTS
    {
        SEGMENT_START,
        SEGMENT_END,

        SEGMENT_MAX_POINTS,
    };

private:
    EDA_SHAPE& m_segment;
};


/**
 * "Standard" circle editing behavior for EDA_SHAPE circles.
 */
class EDA_CIRCLE_POINT_EDIT_BEHAVIOR : public POINT_EDIT_BEHAVIOR
{
public:
    EDA_CIRCLE_POINT_EDIT_BEHAVIOR( EDA_SHAPE& aCircle ) :
            m_circle( aCircle )
    {
        wxASSERT( aCircle.GetShape() == SHAPE_T::CIRCLE );
    }

    void MakePoints( EDIT_POINTS& aPoints ) override;

    bool UpdatePoints( EDIT_POINTS& aPoints ) override;

    void UpdateItem( const EDIT_POINT& aEditedPoint, EDIT_POINTS& aPoints, COMMIT& aCommit,
                     std::vector<EDA_ITEM*>& aUpdatedItems ) override;

    OPT_VECTOR2I Get45DegreeConstrainer( const EDIT_POINT& aEditedPoint,
                                         EDIT_POINTS&      aPoints ) const override;

protected:
    enum CIRCLE_POINTS
    {
        CIRC_CENTER,
        CIRC_END,

        CIRC_MAX_POINTS,
    };

private:
    EDA_SHAPE& m_circle;
};


/**
 * "Standard" bezier editing behavior for EDA_SHAPE beziers.
 */
class EDA_BEZIER_POINT_EDIT_BEHAVIOR : public POINT_EDIT_BEHAVIOR
{
public:
    EDA_BEZIER_POINT_EDIT_BEHAVIOR( EDA_SHAPE& aBezier, int aMaxError ) :
            m_bezier( aBezier ),
            m_maxError( aMaxError )
    {
        wxASSERT( aBezier.GetShape() == SHAPE_T::BEZIER );
    }

    void MakePoints( EDIT_POINTS& aPoints ) override;

    bool UpdatePoints( EDIT_POINTS& aPoints ) override;

    void UpdateItem( const EDIT_POINT& aEditedPoint, EDIT_POINTS& aPoints, COMMIT& aCommit,
                     std::vector<EDA_ITEM*>& aUpdatedItems ) override;

protected:
    enum BEZIER_POINTS
    {
        BEZIER_START,
        BEZIER_CTRL_PT1,
        BEZIER_CTRL_PT2,
        BEZIER_END,

        BEZIER_MAX_POINTS
    };

private:
    EDA_SHAPE& m_bezier;
    int        m_maxError;
};


/**
 * "Standard" table-cell editing behavior.
 *
 * This works over the #EDA_SHAPE basis of a SCH/PCB_TABLECELL.
 * The cells and tables themselves aren't (yet) polymorphic, so the implmentation
 * has to provide UpdateItem() to handle the actual update.
 */
class EDA_TABLECELL_POINT_EDIT_BEHAVIOR : public POINT_EDIT_BEHAVIOR
{
public:
    EDA_TABLECELL_POINT_EDIT_BEHAVIOR( EDA_SHAPE& aCell ) :
            m_cell( aCell )
    {
        // Point editor only supports cardinally-rotated table cells.
        wxASSERT( aCell.GetShape() == SHAPE_T::RECTANGLE );
    }

    void MakePoints( EDIT_POINTS& aPoints ) override;

    bool UpdatePoints( EDIT_POINTS& aPoints ) override;

protected:
    enum TABLECELL_POINTS
    {
        COL_WIDTH,
        ROW_HEIGHT,

        TABLECELL_MAX_POINTS,
    };

private:
    EDA_SHAPE& m_cell;
};


/**
 * "Standard" arc editing behavior.
 */
class EDA_ARC_POINT_EDIT_BEHAVIOR : public POINT_EDIT_BEHAVIOR
{
public:
    EDA_ARC_POINT_EDIT_BEHAVIOR( EDA_SHAPE& aArc, const ARC_EDIT_MODE& aArcEditMode,
                                 KIGFX::VIEW_CONTROLS& aViewContols );

    void MakePoints( EDIT_POINTS& aPoints ) override;

    bool UpdatePoints( EDIT_POINTS& aPoints ) override;

    void UpdateItem( const EDIT_POINT& aEditedPoint, EDIT_POINTS& aPoints, COMMIT& aCommit,
                     std::vector<EDA_ITEM*>& aUpdatedItems ) override;

    OPT_VECTOR2I Get45DegreeConstrainer( const EDIT_POINT& aEditedPoint, EDIT_POINTS& aPoints ) const override;

private:
    enum ARC_POINTS
    {
        ARC_START,
        ARC_MID,
        ARC_END,
        ARC_CENTER,
    };

    EDA_SHAPE& m_arc;
    // The arc edit mode, which is injected from the editor
    const ARC_EDIT_MODE&  m_arcEditMode;
    KIGFX::VIEW_CONTROLS& m_viewControls;
};


ARC_EDIT_MODE IncrementArcEditMode( ARC_EDIT_MODE aMode );
