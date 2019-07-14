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

#ifndef POINT_EDITOR_H
#define POINT_EDITOR_H

#include <tool/tool_interactive.h>
#include "tool/edit_points.h"
#include <status_popup.h>

#include <memory>


class SELECTION_TOOL;
class SHAPE_POLY_SET;

/**
 * Class POINT_EDITOR
 *
 * Tool that displays edit points allowing to modify items by dragging the points.
 */
class POINT_EDITOR : public PCB_TOOL_BASE
{
public:
    POINT_EDITOR();

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

    ///> Sets up handlers for various events.
    void setTransitions() override;

private:
    ///> Selection tool used for obtaining selected items
    SELECTION_TOOL* m_selectionTool;

    ///> Currently edited point, NULL if there is none.
    EDIT_POINT* m_editedPoint;

    ///> Original position for the current drag point.
    EDIT_POINT m_original;

    ///> Currently available edit points.
    std::shared_ptr<EDIT_POINTS> m_editPoints;

    // Alternative constraint, enabled while a modifier key is held
    std::shared_ptr<EDIT_CONSTRAINT<EDIT_POINT> > m_altConstraint;

    // EDIT_POINT for alternative constraint mode
    EDIT_POINT m_altConstrainer;

    // Flag indicating whether the selected zone needs to be refilled
    bool m_refill;

    std::unique_ptr<STATUS_TEXT_POPUP> m_statusPopup;

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
};

#endif
