/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2017 CERN
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

#ifndef PCB_POINT_EDITOR_H
#define PCB_POINT_EDITOR_H

#include <tool/tool_interactive.h>
#include "tool/edit_points.h"
#include <status_popup.h>

#include <memory>


class PCB_SELECTION_TOOL;
class SHAPE_POLY_SET;

/**
 * PCB_POINT_EDITOR
 *
 * Tool that displays edit points allowing to modify items by dragging the points.
 */
class PCB_POINT_EDITOR : public PCB_TOOL_BASE
{
public:
    PCB_POINT_EDITOR();

    /// @copydoc TOOL_INTERACTIVE::Reset()
    void Reset( RESET_REASON aReason ) override;

    /// @copydoc TOOL_INTERACTIVE::Init()
    bool Init() override;

    /**
     * Function OnSelected()
     *
     * Change selection event handler.
     */
    int OnSelectionChange( const TOOL_EVENT& aEvent );

    /**
     * Indicates the cursor is over an edit point.  Used to coordinate cursor shapes with
     * other tools.
     */
    bool HasPoint() { return m_editedPoint != nullptr; }

private:
    ///> Sets up handlers for various events.
    void setTransitions() override;

    void buildForPolyOutline( std::shared_ptr<EDIT_POINTS> points, const SHAPE_POLY_SET* aOutline );

    std::shared_ptr<EDIT_POINTS> makePoints( EDA_ITEM* aItem );

    ///> Updates item's points with edit points.
    void updateItem() const;

    ///> Applies the last changes to the edited item.
    void finishItem();

    /**
     * Validates a polygon and displays a popup warning if invalid.
     * @param aModified is the polygon to be checked.
     * @return True if polygon is valid.
     */
    bool validatePolygon( SHAPE_POLY_SET& aModified ) const;

    ///> Updates edit points with item's points.
    void updatePoints();

    ///> Updates which point is being edited.
    void updateEditedPoint( const TOOL_EVENT& aEvent );

    ///> Sets the current point being edited. NULL means none.
    void setEditedPoint( EDIT_POINT* aPoint );

    inline int getEditedPointIndex() const
    {
        for( unsigned i = 0; i < m_editPoints->PointsSize(); ++i )
        {
            if( m_editedPoint == &m_editPoints->Point( i ) )
                return i;
        }

        return wxNOT_FOUND;
    }

    ///> Returns true if aPoint is the currently modified point.
    inline bool isModified( const EDIT_POINT& aPoint ) const
    {
        return m_editedPoint == &aPoint;
    }

    ///> Sets up an alternative constraint (typically enabled upon a modifier key being pressed).
    void setAltConstraint( bool aEnabled );

    ///> Returns a point that should be used as a constrainer for 45 degrees mode.
    EDIT_POINT get45DegConstrainer() const;

    ///> Condition to display "Create corner" context menu entry.
    static bool addCornerCondition( const SELECTION& aSelection );

    ///> Determine if the tool can currently add a corner to the given item
    static bool canAddCorner( const EDA_ITEM& aItem );

    ///> Condition to display "Remove corner" context menu entry.
    bool removeCornerCondition( const SELECTION& aSelection );

    /// TOOL_ACTION handlers
    int addCorner( const TOOL_EVENT& aEvent );
    int removeCorner( const TOOL_EVENT& aEvent );
    int modifiedSelection( const TOOL_EVENT& aEvent );

    /**
     * Move an end point of the arc, while keeping the same subtended angle.
     */
    void editArcEndpointKeepShape( PCB_SHAPE* aArc, VECTOR2I aStart, VECTOR2I aMid, VECTOR2I aEnd,
                                   const VECTOR2I aCursor ) const;

    /**
     * Move an end point of the arc around the circumference.
     */
    void editArcEndpointKeepCenter( PCB_SHAPE* aArc, VECTOR2I aCenter, VECTOR2I aStart,
                                    VECTOR2I aMid, VECTOR2I aEnd, const VECTOR2I aCursor ) const;

    /**
     * Move the mid point of the arc, while keeping the two endpoints.
     */
    void editArcMidKeepEndpoints( PCB_SHAPE* aArc, VECTOR2I aStart, VECTOR2I aEnd,
                                  const VECTOR2I aCursor ) const;

    /**
     * Move the mid point of the arc, while keeping the angle.
     */
    void editArcMidKeepCenter( PCB_SHAPE* aArc, VECTOR2I aCenter, VECTOR2I aStart, VECTOR2I aMid,
                               VECTOR2I aEnd, const VECTOR2I aCursor ) const;

    ///> Change the edit method to an alternative method ( currently, arcs only )
    int changeEditMethod( const TOOL_EVENT& aEvent );

private:
    PCB_SELECTION_TOOL*                m_selectionTool;
    std::unique_ptr<STATUS_TEXT_POPUP> m_statusPopup;
    std::shared_ptr<EDIT_POINTS>       m_editPoints;

    EDIT_POINT*         m_editedPoint;
    EDIT_POINT*         m_hoveredPoint;

    EDIT_POINT          m_original;        ///> Original position for the current drag point.

    bool                m_refill;
    bool                m_altEditMethod;

    // Alternative constraint, enabled while a modifier key is held
    std::shared_ptr<EDIT_CONSTRAINT<EDIT_POINT>> m_altConstraint;
    EDIT_POINT                                   m_altConstrainer;
};

#endif
