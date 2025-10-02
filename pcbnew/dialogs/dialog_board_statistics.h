/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 Alexander Shuklin, jasuramme@gmail.com
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


#ifndef _DIALOG_BOARD_STATISTICS_H
#define _DIALOG_BOARD_STATISTICS_H


#include <board.h>
#include <footprint.h>
#include <pcb_track.h>
#include <dialog_board_statistics_base.h>
#include <board_statistics.h>
#include <board_statistics_report.h>
#include <pcb_base_frame.h>
#include <pcb_edit_frame.h>
#include <project.h>
#include <wx/datetime.h>

/**
 * Dialog to show common board info.
 */
class DIALOG_BOARD_STATISTICS : public DIALOG_BOARD_STATISTICS_BASE
{
public:
    DIALOG_BOARD_STATISTICS( PCB_EDIT_FRAME* aParentFrame );
    ~DIALOG_BOARD_STATISTICS();

    ///< Get data from the PCB board and print it to dialog
    bool TransferDataToWindow() override;

private:
    ///< Function to fill up all items types to be shown in the dialog.
    void refreshItemsTypes();

    ///< Get data from board.
    void getDataFromPCB();

    ///< Apply data to dialog widgets.
    void updateWidgets();

    ///< Update drills grid.
    void updateDrillGrid();

    void adjustDrillGridColumns();

    void checkboxClicked( wxCommandEvent& aEvent ) override;

    ///< Save board statistics to a file
    void saveReportClicked( wxCommandEvent& aEvent ) override;

    void drillGridSize( wxSizeEvent& aEvent ) override;

    void drillGridSort( wxGridEvent& aEvent );

    PCB_EDIT_FRAME* m_frame;

    BOARD_STATISTICS_DATA m_statsData;

    int m_startLayerColInitialSize;        ///< Width of the start layer column as calculated by
                                           ///<    the wxWidgets autosizing algorithm.
    int m_stopLayerColInitialSize;         ///< Width of the stop layer column.
};

#endif // __DIALOG_BOARD_STATISTICS_H
