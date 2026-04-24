/*
 * This program source code file is part of KiCad, a free EDA CAD application.
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
 */

#ifndef BOARD_TEXT_VAR_ADAPTER_H_
#define BOARD_TEXT_VAR_ADAPTER_H_

#include <board.h>
#include <text_var_dependency.h>

class FOOTPRINT;


/**
 * Bridges BOARD's listener stream into the generic TEXT_VAR_TRACKER.
 *
 * Responsibilities:
 *   - Register any EDA_TEXT-bearing item added to the BOARD so its `${...}`
 *     references become graph edges.
 *   - On composite updates, re-register changed text items (their source text
 *     may have been edited) and fan out invalidations for cross-refs that
 *     changed FOOTPRINTs could source (`${REFDES:FIELD}`).
 *   - Unregister removed items.
 *
 * The adapter is owned by BOARD and added to its listener list in the BOARD
 * ctor. Lifetime is tied to the BOARD, so no explicit remove is needed on
 * destruction — BOARD's listener vector is torn down by its own destructor.
 */
class BOARD_TEXT_VAR_ADAPTER : public BOARD_LISTENER
{
public:
    explicit BOARD_TEXT_VAR_ADAPTER( BOARD& aBoard );
    ~BOARD_TEXT_VAR_ADAPTER() override = default;

    TEXT_VAR_TRACKER&       Tracker()       { return m_tracker; }
    const TEXT_VAR_TRACKER& Tracker() const { return m_tracker; }

    void OnBoardItemAdded( BOARD& aBoard, BOARD_ITEM* aItem ) override;
    void OnBoardItemsAdded( BOARD& aBoard, std::vector<BOARD_ITEM*>& aItems ) override;
    void OnBoardItemRemoved( BOARD& aBoard, BOARD_ITEM* aItem ) override;
    void OnBoardItemsRemoved( BOARD& aBoard, std::vector<BOARD_ITEM*>& aItems ) override;
    void OnBoardItemChanged( BOARD& aBoard, BOARD_ITEM* aItem ) override;
    void OnBoardItemsChanged( BOARD& aBoard, std::vector<BOARD_ITEM*>& aItems ) override;
    void OnBoardCompositeUpdate( BOARD&                     aBoard,
                                 std::vector<BOARD_ITEM*>&  aAdded,
                                 std::vector<BOARD_ITEM*>&  aRemoved,
                                 std::vector<BOARD_ITEM*>&  aChanged ) override;

    /**
     * Scan the whole board and register every text-bearing item. Called from
     * the BOARD ctor so dependencies are correct from the start, and exposed
     * for use after bulk loads (board open, import) that bypass per-item
     * listener callbacks.
     */
    void RebuildIndex();

    /**
     * Return the keys @p aItem could source as a cross-reference target. For
     * a FOOTPRINT, these are `${REFDES:FIELD}` keys — one per named field.
     */
    std::vector<TEXT_VAR_REF_KEY> ExtractSourceKeys( EDA_ITEM* aItem ) const;

private:
    void registerItem( BOARD_ITEM* aItem );
    void unregisterItem( BOARD_ITEM* aItem );

    BOARD&           m_board;
    TEXT_VAR_TRACKER m_tracker;
};


#endif // BOARD_TEXT_VAR_ADAPTER_H_
