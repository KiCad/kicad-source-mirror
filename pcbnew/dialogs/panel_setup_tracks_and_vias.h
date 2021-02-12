/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 KiCad Developers, see change_log.txt for contributors.
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


#ifndef PANEL_SETUP_TRACKS_AND_VIAS_H
#define PANEL_SETUP_TRACKS_AND_VIAS_H

#include <board.h>
#include <widgets/unit_binder.h>
#include <widgets/paged_dialog.h>

#include <panel_setup_tracks_and_vias_base.h>
#include "panel_setup_constraints.h"

class PCB_EDIT_FRAME;
class BOARD_DESIGN_SETTINGS;


class PANEL_SETUP_TRACKS_AND_VIAS : public PANEL_SETUP_TRACKS_AND_VIAS_BASE
{
private:
    PAGED_DIALOG*            m_Parent;
    PCB_EDIT_FRAME*          m_Frame;
    BOARD*                   m_Pcb;
    BOARD_DESIGN_SETTINGS*   m_BrdSettings;

    // We must validate against the current m_BrdSettings as they may have been
    // changed but not yet committed.  Fetch them from the constraints panel.
    PANEL_SETUP_CONSTRAINTS* m_ConstraintsPanel;

protected:
    void OnAddTrackWidthsClick( wxCommandEvent& event ) override;
    void OnRemoveTrackWidthsClick( wxCommandEvent& event ) override;
    void OnAddViaSizesClick( wxCommandEvent& event ) override;
    void OnRemoveViaSizesClick( wxCommandEvent& event ) override;
    void OnAddDiffPairsClick( wxCommandEvent& event ) override;
    void OnRemoveDiffPairsClick( wxCommandEvent& event ) override;

    void AppendTrackWidth( const int aWidth );
    void AppendViaSize( const int aSize, const int aDrill );
    void AppendDiffPairs( const int aWidth, const int aGap, const int aViaGap );

public:
    PANEL_SETUP_TRACKS_AND_VIAS( PAGED_DIALOG* aParent, PCB_EDIT_FRAME* aFrame,
                                 PANEL_SETUP_CONSTRAINTS* aConstraintsPanel );
    ~PANEL_SETUP_TRACKS_AND_VIAS() override;

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    bool Validate() override;

    void ImportSettingsFrom( BOARD* aBoard );
};

#endif //PANEL_SETUP_TRACKS_AND_VIAS_H
