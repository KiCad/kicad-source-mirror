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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef PCB_POINT_EDITOR_H
#define PCB_POINT_EDITOR_H

#include <tool/tool_interactive.h>
#include <tools/pcb_selection.h>
#include <tools/pcb_handle_key.h>
#include "tool/edit_points.h"
#include <pcbnew_settings.h>
#include <status_popup.h>

#include <functional>
#include <memory>
#include <set>
#include <unordered_set>

namespace KIGFX { namespace PREVIEW { class ANGLE_ITEM; } }


class PCB_SELECTION_TOOL;
class POINT_EDIT_BEHAVIOR;
class RECT_RADIUS_TEXT_ITEM;
class SHAPE_POLY_SET;
class BOARD_CONSTRAINT_DRAG_SESSION;
class BLOCK_DRAG_SESSION;

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
     * True when the cursor sits on a handle the user can pick.
     *
     * The selection tool asks this so its long press menu does not open on a click
     * the point editor is about to use.
     */
    bool HasBlockHandle() const;

    /// True while handles are shown for the members of a multi-selection or group.
    bool HasBlockHandles() const { return !m_blockHandles.empty(); }

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
    /// Set up handlers for various events.
    void setTransitions() override;

    std::shared_ptr<EDIT_POINTS> makePoints( EDA_ITEM* aItem );

    /// Create the point edit behavior for an item. Sets aRejected for item types that
    /// should show no handles at all.
    std::unique_ptr<POINT_EDIT_BEHAVIOR> makeBehavior( EDA_ITEM* aItem, bool& aRejected );

    /// Whether the user has turned on anchor point handles for multi-selections.
    bool blockAnchorEditingEnabled() const;

    /// Build the handles shown for every reshapeable item in a multi-selection or group.
    void buildBlockHandles( const PCB_SELECTION& aSelection );

    /// Take those handles off the view and throw them away.
    void clearBlockHandles();

    /// Move those handles back onto their items after the geometry changed.
    void updateBlockHandlePoints();

    /// Highlight the handle under the cursor, if there is one.
    void updateBlockHandleHover( const VECTOR2I& aPosition );

    /// Gap to leave between the scaling box handles and the shapes' own handles, in
    /// board units for the current zoom.
    int bboxHandleInflation() const;

    /// The handle collections the user can pick from.
    std::vector<EDIT_POINTS*> pickableCollections();

    /// Pick the handle under aPosition. Returns false when there is no handle to pick.
    bool pickHandleAt( const VECTOR2I& aPosition, bool aAccumulate );

    /// Unpick everything and redraw.
    void clearPickedHandles();

    /// Replace the picked set and repaint the handles to match it.
    void setPickedHandles( std::set<PCB_HANDLE_KEY> aPicked );

    /// Paint the picked handles, dropping keys whose handle has gone.
    void applyPickedFlags();

    /// Update item's points with edit points.
    void updateItem( BOARD_COMMIT& aCommit );

    /// Update edit points with item's points.
    void updatePoints();

    /// Update which point is being edited.
    void updateEditedPoint( const TOOL_EVENT& aEvent );

    /// Set the current point being edited. NULL means none.
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

    /// Return true if aPoint is the currently modified point.
    inline bool isModified( const EDIT_POINT& aPoint ) const
    {
        return m_editedPoint == &aPoint;
    }

    /// Set up an alternative constraint (typically enabled upon a modifier key being pressed).
    void setAltConstraint( bool aEnabled );

    /// Return a point that should be used as a constrainer for 45 degrees mode.
    EDIT_POINT get45DegConstrainer() const;

    /// TOOL_ACTION handlers
    int movePoint( const TOOL_EVENT& aEvent );
    int addCorner( const TOOL_EVENT& aEvent );
    int removeCorner( const TOOL_EVENT& aEvent );
    int chamferCorner( const TOOL_EVENT& aEvent );
    int modifiedSelection( const TOOL_EVENT& aEvent );

    /// Change the edit method for arcs.
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
    std::unique_ptr<EDIT_RELATION> m_altConstraint;
    EDIT_POINT                                   m_altConstrainer;

    bool                          m_inPointEditorTool; // Re-entrancy guard

    VECTOR2I                      m_angleSnapPos;
    VECTOR2I                      m_stickyDisplacement;
    bool                          m_angleSnapActive;

    // This handles the edit process for a specific tpye of item (not
    // just C++ type, because PCB_SHAPE is one type that has many subtypes)
    std::unique_ptr<POINT_EDIT_BEHAVIOR> m_editorBehavior;
    std::shared_ptr<BOARD_CONSTRAINT_DRAG_SESSION> m_constraintDragSession;

    // Handles shown for the reshapeable members of a multi-selection or group, one
    // collection per item
    struct BLOCK_HANDLE_ENTRY
    {
        std::unique_ptr<POINT_EDIT_BEHAVIOR> behavior;
        std::shared_ptr<EDIT_POINTS>         points;
    };

    std::vector<BLOCK_HANDLE_ENTRY> m_blockHandles;
    EDIT_POINT*                     m_blockHoveredPoint;
    EDIT_POINTS*                    m_blockHoveredOwner;
    std::set<PCB_HANDLE_KEY>        m_pickedHandles;

    // The selection with each group replaced by its members, kept in selection order
    // so a refusal message always names the same item
    std::vector<BOARD_ITEM*> m_expandedSelection;

    // Undoes a block drag that is still running. Set only while OnSelectionChange is
    // parked in its loop, because it reads that call's locals. Reset() calls it before
    // it clears any collection, so the drag is undone while the items still exist
    std::function<void()> m_blockDragAbort;

    static const unsigned int COORDS_PADDING; // Padding from coordinates limits for this tool

    // Runs the block drag gesture. It works on the handle collections, the picked set and
    // the expanded selection this tool builds, and runs its area picks through its Wait()
    friend class BLOCK_DRAG_SESSION;
};

#endif
