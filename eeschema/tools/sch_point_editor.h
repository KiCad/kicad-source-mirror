/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 CERN
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

#ifndef SCH_POINT_EDITOR_H
#define SCH_POINT_EDITOR_H

#include <tools/sch_tool_base.h>
#include <tool/edit_points.h>
#include <tool/selection.h>

class SCH_SELECTION_TOOL;
class POINT_EDIT_BEHAVIOR;
class SCH_BASE_FRAME;
class SCH_COMMIT;

/**
 * Tool that displays edit points allowing to modify items by dragging the points.
 */
class SCH_POINT_EDITOR : public SCH_TOOL_BASE<SCH_BASE_FRAME>
{
public:
    SCH_POINT_EDITOR();

    /// @copydoc TOOL_INTERACTIVE::Reset()
    void Reset( RESET_REASON aReason ) override;

    /// @copydoc TOOL_INTERACTIVE::Init()
    bool Init() override;

    int Main( const TOOL_EVENT& aEvent );

    /**
     * Indicate the cursor is over an edit point.  Used to coordinate cursor shapes with
     * other tools.
     */
    bool HasPoint() { return m_editedPoint != nullptr; }

    ///< Set up handlers for various events.
    void setTransitions() override;

private:
    ///< Update item's points with edit points.
    void updateParentItem( bool aSnapToGrid, SCH_COMMIT& aCommit ) const;

    ///< When dragging a graphics edge, update pins too.
    void dragPinsOnEdge( const std::vector<SEG>& aOldEdges, const std::vector<VECTOR2I>& aMoveVecs,
                         int aUnit, SCH_COMMIT& aCommit ) const;

    ///< Update edit points with item's points.
    void updatePoints();

    ///< Update which point is being edited.
    void updateEditedPoint( const TOOL_EVENT& aEvent );

    ///< Clear references to the points
    int clearEditedPoints( const TOOL_EVENT& aEvent );

    ///< Set the current point being edited. NULL means none.
    void setEditedPoint( EDIT_POINT* aPoint );

    ///< Return true if \a aPoint is the currently modified point.
    inline bool isModified( const EDIT_POINT& aPoint ) const
    {
        return m_editedPoint == &aPoint;
    }

    inline int getEditedPointIndex() const
    {
        for( unsigned i = 0; i < m_editPoints->PointsSize(); ++i )
        {
            if( m_editedPoint == &m_editPoints->Point( i ) )
                return i;
        }

        return wxNOT_FOUND;
    }

    void pinEditedCorner( int minWidth, int minHeight, VECTOR2I& topLeft, VECTOR2I& topRight,
                          VECTOR2I& botLeft, VECTOR2I& botRight, EE_GRID_HELPER* aGrid ) const;

    bool addCornerCondition( const SELECTION& aSelection );
    bool removeCornerCondition( const SELECTION& aSelection );

    /// TOOL_ACTION handlers
    int addCorner( const TOOL_EVENT& aEvent );
    int removeCorner( const TOOL_EVENT& aEvent );
    int modifiedSelection( const TOOL_EVENT& aEvent );

private:
    void makePointsAndBehavior( EDA_ITEM* aItem );

    ///< Currently edited point, NULL if there is none.
    EDIT_POINT* m_editedPoint;

    ///< Re-entrancy guards
    bool m_inPointEditor;

    ///< Currently available edit points.
    std::shared_ptr<EDIT_POINTS> m_editPoints;

    ///< Current item-specific edit behavior.
    std::unique_ptr<POINT_EDIT_BEHAVIOR> m_editBehavior;
};

#endif  // SCH_POINT_EDITOR_H
