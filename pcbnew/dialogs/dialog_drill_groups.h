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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef DIALOG_DRILL_GROUPS_H
#define DIALOG_DRILL_GROUPS_H

#include <vector>

#include <dialog_drill_groups_base.h>
#include <board.h>
#include <drill/drill_chart_model.h>
#include <widgets/unit_binder.h>

class PCB_EDIT_FRAME;


/**
 * Modeless inspector for the board's drill groups.
 *
 * Modeless because its whole point is picking a row and seeing those holes on the canvas,
 * which a modal dialog cannot do. Edits go straight to the board through a commit rather
 * than being buffered until OK, since there is no OK.
 */
class DIALOG_DRILL_GROUPS : public DIALOG_DRILL_GROUPS_BASE, public BOARD_LISTENER
{
public:
    DIALOG_DRILL_GROUPS( PCB_EDIT_FRAME* aFrame );
    ~DIALOG_DRILL_GROUPS() override;

    /**
     * Rebuild from the board. Cheap enough to call whenever the drill model moves.
     */
    void Reload();

    void OnBoardItemAdded( BOARD& aBoard, BOARD_ITEM* aItem ) override { scheduleReload(); }
    void OnBoardItemsAdded( BOARD& aBoard, std::vector<BOARD_ITEM*>& aItems ) override
    {
        scheduleReload();
    }
    void OnBoardItemRemoved( BOARD& aBoard, BOARD_ITEM* aItem ) override { scheduleReload(); }
    void OnBoardItemsRemoved( BOARD& aBoard, std::vector<BOARD_ITEM*>& aItems ) override
    {
        scheduleReload();
    }
    void OnBoardItemChanged( BOARD& aBoard, BOARD_ITEM* aItem ) override { scheduleReload(); }
    void OnBoardItemsChanged( BOARD& aBoard, std::vector<BOARD_ITEM*>& aItems ) override
    {
        scheduleReload();
    }

private:
    void onFreezeChanged( wxCommandEvent& aEvent ) override;
    void onFlash( wxCommandEvent& aEvent ) override;
    void onClose( wxCommandEvent& aEvent ) override;

    void onGridSelect( wxGridEvent& aEvent );

    /**
     * Push the detail pane back into the profile for the given row.
     */
    void commitDetail( int aRow );

    /**
     * Coalesce a burst of board notifications into one rebuild, and keep the rebuild out
     * of the notification itself.
     */
    void scheduleReload();

    void showDetail( int aRow );

    const DRILL_CHART_GROUP* selectedGroup() const;

    PCB_EDIT_FRAME* m_frame;

    std::vector<DRILL_CHART_GROUP> m_groups;

    /**
     * Guards against the detail pane writing back while it is being filled in.
     */
    bool m_loading;

    /**
     * The row the detail pane is showing, so it can be committed before the selection moves.
     */
    int m_detailRow;

    bool m_reloadPending;

};

#endif // DIALOG_DRILL_GROUPS_H
