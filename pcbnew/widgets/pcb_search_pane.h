/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <board.h>
#include <widgets/search_pane.h>

class PCB_EDIT_FRAME;

class PCB_SEARCH_PANE : public SEARCH_PANE, public BOARD_LISTENER
{
public:
    PCB_SEARCH_PANE( PCB_EDIT_FRAME* aFrame );
    virtual ~PCB_SEARCH_PANE();

    virtual void OnBoardItemAdded( BOARD& aBoard, BOARD_ITEM* aBoardItem ) override;
    virtual void OnBoardItemsAdded( BOARD& aBoard, std::vector<BOARD_ITEM*>& aBoardItems ) override;
    virtual void OnBoardItemRemoved( BOARD& aBoard, BOARD_ITEM* aBoardItem ) override;
    virtual void OnBoardItemsRemoved( BOARD& aBoard, std::vector<BOARD_ITEM*>& aBoardItems ) override;
    virtual void OnBoardNetSettingsChanged( BOARD& aBoard ) override;
    virtual void OnBoardItemChanged( BOARD& aBoard, BOARD_ITEM* aBoardItem ) override;
    virtual void OnBoardItemsChanged( BOARD& aBoard, std::vector<BOARD_ITEM*>& aBoardItems ) override;
    virtual void OnBoardHighlightNetChanged( BOARD& aBoard ) override;
    virtual void OnBoardRatsnestChanged( BOARD& aBoard ) override;
    virtual void OnBoardCompositeUpdate( BOARD& aBoard, std::vector<BOARD_ITEM*>& aAddedItems,
                                         std::vector<BOARD_ITEM*>& aRemovedItems,
                                         std::vector<BOARD_ITEM*>& aChangedItems ) override;

private:
    void onUnitsChanged( wxCommandEvent& event );
    void onBoardChanging( wxCommandEvent& event );
    void onBoardChanged( wxCommandEvent& event );

private:
    PCB_EDIT_FRAME* m_pcbFrame;
    BOARD*          m_brd;
};
