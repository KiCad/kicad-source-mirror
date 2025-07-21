/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 CERN
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

#ifndef ALIGN_DISTRIBUTE_TOOL_H_
#define ALIGN_DISTRIBUTE_TOOL_H_

#include <tool/tool_interactive.h>
#include <tools/pcb_selection.h>
#include <board_item.h>
#include <pcb_base_frame.h>


class PCB_SELECTION_TOOL;
class BOARD_COMMIT;

class ALIGN_DISTRIBUTE_TOOL : public TOOL_INTERACTIVE
{
public:
    ALIGN_DISTRIBUTE_TOOL();
    virtual ~ALIGN_DISTRIBUTE_TOOL();

    /// @copydoc TOOL_INTERACTIVE::Reset()
    void Reset( RESET_REASON aReason ) override {}

    /// @copydoc TOOL_INTERACTIVE::Init()
    bool Init() override;

    /**
     * Set Y coordinate of the selected items to the value of the top-most selected item Y
     * coordinate.
     */
    int AlignTop( const TOOL_EVENT& aEvent );

    /**
     * Sets Y coordinate of the selected items to the value of the bottom-most selected item Y
     * coordinate.
     */
    int AlignBottom( const TOOL_EVENT& aEvent );

    /**
     * Sets X coordinate of the selected items to the value of the left-most selected item X
     * coordinate.
     */
    int AlignLeft( const TOOL_EVENT& aEvent );

    /**
     * Sets X coordinate of the selected items to the value of the right-most selected item X
     * coordinate.
     */
    int AlignRight( const TOOL_EVENT& aEvent );

    /**
     * Set the x coordinate of the midpoint of each of the selected items to the value of the
     * x coordinate of the center of the middle selected item.
     */
    int AlignCenterX( const TOOL_EVENT& aEvent );

    /**
     * Set the y coordinate of the midpoint of each of the selected items to the value of the
     * y coordinate of the center of the middle selected item.
     */
    int AlignCenterY( const TOOL_EVENT& aEvent );

    /**
     * Distribute the selected items in some way
     */
    int DistributeItems( const TOOL_EVENT& aEvent );

    ///< Set up handlers for various events.
    void setTransitions() override;

private:
    /**
     * Populate two vectors with the sorted selection and sorted locked items.
     *
     * Returns the size of aItemsToAlign()
     */
    template< typename T >
    size_t GetSelections( std::vector<std::pair<BOARD_ITEM*, BOX2I>>& aItemsToAlign,
                          std::vector<std::pair<BOARD_ITEM*, BOX2I>>& aLockedItems,
                          T aCompare );

    template< typename T >
    int selectTarget( std::vector<std::pair<BOARD_ITEM*, BOX2I>>& aItems,
                      std::vector<std::pair<BOARD_ITEM*, BOX2I>>& aLocked, T aGetValue );

    /**
     * Sets X coordinate of the selected items to the value of the left-most selected item
     * X coordinate.
     *
     * @note Uses the bounding box of items, which do not get mirrored even when
     *       the view is mirrored!
     */
    int doAlignLeft();

    /**
     * Align selected items using the right edge of their bounding boxes to the right-most item.
     *
     * @note Uses the bounding box of items, which do not get mirrored even when
     *       the view is mirrored!
     */
    int doAlignRight();

    /**
     * Distribute selected items using an even spacing between the centers of their bounding
     * boxes.
     *
     * @note Using the centers of bounding box of items can give unsatisfactory visual results
     *       since items of differing widths will be placed with different gaps. Is only used
     *       if items overlap
     */
    void doDistributeCenters( bool aIsXAxis, std::vector<std::pair<BOARD_ITEM*, BOX2I>>& aItems,
                              BOARD_COMMIT& aCommit ) const;

    /**
     * Distributes selected items using an even spacing between their bounding boxe
     * in the x or y axis.
     *
     * @note If the total item widths exceed the available space, the overlaps will be
     *       distributed evenly.
     */
    void doDistributeGaps( bool aIsXAxis, std::vector<std::pair<BOARD_ITEM*, BOX2I>>& aItems,
                           BOARD_COMMIT& aCommit ) const;

private:
    PCB_SELECTION_TOOL*  m_selectionTool;
    CONDITIONAL_MENU*    m_placementMenu;
    PCB_BASE_FRAME*      m_frame;
};

#endif /* ALIGN_DISTRIBUTE_TOOL_H_ */
