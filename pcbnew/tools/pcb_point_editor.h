/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2017 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef PCB_POINT_EDITOR_H
#define PCB_POINT_EDITOR_H

#include <tool/tool_interactive.h>
#include "tool/edit_points.h"
#include <pcbnew_settings.h>
#include <status_popup.h>

#include <memory>

namespace KIGFX { namespace PREVIEW { class ANGLE_ITEM; } }


class PCB_SELECTION_TOOL;
class POINT_EDIT_BEHAVIOR;
class RECT_RADIUS_TEXT_ITEM;
class SHAPE_POLY_SET;

/**
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
     * Change selection event handler.
     */
    int OnSelectionChange( const TOOL_EVENT& aEvent );

    /**
     * Indicate the cursor is over an edit point.
     *
     * Used to coordinate cursor shapes with other tools.
     */
    bool HasPoint()    { return m_editedPoint != nullptr; }
    bool HasMidpoint() { return HasPoint() && dynamic_cast<EDIT_LINE*>( m_editedPoint ); }
    bool HasCorner()   { return HasPoint() && !HasMidpoint() && ( !m_editPoints || m_editPoints->GetParent()->Type() != PCB_GROUP_T ); }

    /**
     * Check if a corner can be added to the given item (zones, polys, segments, arcs).
     */
    static bool CanAddCorner( const EDA_ITEM& aItem );

    /**
     * Check if a corner of the given item can be chamfered (zones, polys only).
     */
    static bool CanChamferCorner( const EDA_ITEM& aItem );

    /**
     * Condition to display "Remove Corner" context menu entry.
     */
    bool CanRemoveCorner( const SELECTION& aSelection );

private:
    ///< Set up handlers for various events.
    void setTransitions() override;

    std::shared_ptr<EDIT_POINTS> makePoints( EDA_ITEM* aItem );

    ///< Update item's points with edit points.
    void updateItem( BOARD_COMMIT& aCommit );

    ///< Update edit points with item's points.
    void updatePoints();

    ///< Update which point is being edited.
    void updateEditedPoint( const TOOL_EVENT& aEvent );

    ///< Set the current point being edited. NULL means none.
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

    ///< Return true if aPoint is the currently modified point.
    inline bool isModified( const EDIT_POINT& aPoint ) const
    {
        return m_editedPoint == &aPoint;
    }

    ///< Set up an alternative constraint (typically enabled upon a modifier key being pressed).
    void setAltConstraint( bool aEnabled );

    ///< Return a point that should be used as a constrainer for 45 degrees mode.
    EDIT_POINT get45DegConstrainer() const;

    /// TOOL_ACTION handlers
    int movePoint( const TOOL_EVENT& aEvent );
    int addCorner( const TOOL_EVENT& aEvent );
    int removeCorner( const TOOL_EVENT& aEvent );
    int chamferCorner( const TOOL_EVENT& aEvent );
    int modifiedSelection( const TOOL_EVENT& aEvent );

    ///< Change the edit method for arcs.
    int changeArcEditMode( const TOOL_EVENT& aEvent );

private:
    PCB_BASE_FRAME*               m_frame;
    PCB_SELECTION_TOOL*           m_selectionTool;
    std::shared_ptr<EDIT_POINTS>  m_editPoints;
    std::unique_ptr<KIGFX::PREVIEW::ANGLE_ITEM> m_angleItem;

    EDIT_POINT*                   m_editedPoint;
    EDIT_POINT*                   m_hoveredPoint;

    EDIT_POINT                    m_original;   ///< Original pos for the current drag point.

    ARC_EDIT_MODE                 m_arcEditMode;

    PCB_SELECTION                 m_preview;
    RECT_RADIUS_TEXT_ITEM*        m_radiusHelper;

    // Alternative constraint, enabled while a modifier key is held
    std::shared_ptr<EDIT_CONSTRAINT<EDIT_POINT>> m_altConstraint;
    EDIT_POINT                                   m_altConstrainer;

    bool                          m_inPointEditorTool; // Re-entrancy guard

    VECTOR2I                      m_angleSnapPos;
    VECTOR2I                      m_stickyDisplacement;
    bool                          m_angleSnapActive;

    // This handles the edit process for a specific tpye of item (not
    // just C++ type, because PCB_SHAPE is one type that has many subtypes)
    std::unique_ptr<POINT_EDIT_BEHAVIOR> m_editorBehavior;

    static const unsigned int COORDS_PADDING; // Padding from coordinates limits for this tool
};

#endif
