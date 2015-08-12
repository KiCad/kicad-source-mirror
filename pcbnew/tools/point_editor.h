/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
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

#ifndef __POINT_EDITOR_H
#define __POINT_EDITOR_H

#include <boost/shared_ptr.hpp>

#include <tool/tool_interactive.h>
#include "edit_points.h"

class SELECTION_TOOL;

/**
 * Class POINT_EDITOR
 *
 * Tool that displays edit points allowing to modify items by dragging the points.
 */
class POINT_EDITOR : public TOOL_INTERACTIVE
{
public:
    POINT_EDITOR();

    /// @copydoc TOOL_INTERACTIVE::Reset()
    void Reset( RESET_REASON aReason );

    /// @copydoc TOOL_INTERACTIVE::Init()
    bool Init();

    /**
     * Function OnSelected()
     *
     * Change selection event handler.
     */
    int OnSelectionChange( const TOOL_EVENT& aEvent );

    ///> Sets up handlers for various events.
    void SetTransitions();

private:
    ///> Selection tool used for obtaining selected items
    SELECTION_TOOL* m_selectionTool;

    ///> Currently edited point, NULL if there is none.
    EDIT_POINT* m_editedPoint;

    ///> Original position for the current drag point.
    EDIT_POINT m_original;

    ///> Currently available edit points.
    boost::shared_ptr<EDIT_POINTS> m_editPoints;

    // Alternative constraint, enabled while a modifier key is held
    boost::shared_ptr<EDIT_CONSTRAINT<EDIT_POINT> > m_altConstraint;

    // EDIT_POINT for alternative constraint mode
    EDIT_POINT m_altConstrainer;

    ///> Updates item's points with edit points.
    void updateItem() const;

    ///> Applies the last changes to the edited item.
    void finishItem() const;

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

    ///> Adds a new edit point on a zone outline/line.
    void addCorner( const VECTOR2I& aPoint );

    ///> Removes a corner.
    void removeCorner( EDIT_POINT* aPoint );

    ///> Condition to display "Create corner" context menu entry.
    static bool addCornerCondition( const SELECTION& aSelection );

    ///> Condition to display "Remove corner" context menu entry.
    bool removeCornerCondition( const SELECTION& aSelection );
};

#endif
