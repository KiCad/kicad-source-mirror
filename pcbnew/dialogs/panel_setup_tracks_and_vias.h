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
public:
    PANEL_SETUP_TRACKS_AND_VIAS( wxWindow* aParentWindow, PCB_EDIT_FRAME* aFrame );
    ~PANEL_SETUP_TRACKS_AND_VIAS() override;

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    bool Validate() override;

    void ImportSettingsFrom( BOARD* aBoard );

protected:
    void OnAddTrackWidthsClick( wxCommandEvent& event ) override;
    void OnSortTrackWidthsClick( wxCommandEvent& event ) override;
    void OnRemoveTrackWidthsClick( wxCommandEvent& event ) override;
    void OnAddViaSizesClick( wxCommandEvent& event ) override;
    void OnSortViaSizesClick( wxCommandEvent& event ) override;
    void OnRemoveViaSizesClick( wxCommandEvent& event ) override;
    void OnAddDiffPairsClick( wxCommandEvent& event ) override;
    void OnSortDiffPairsClick( wxCommandEvent& event ) override;
    void OnRemoveDiffPairsClick( wxCommandEvent& event ) override;

    void onUnitsChanged( wxCommandEvent& aEvent );

    void AppendTrackWidth( int aWidth );
    void AppendViaSize( int aSize, int aDrill );
    void AppendDiffPairs( int aWidth, int aGap, int aViaGap );

    bool commitPendingChanges( bool aQuietMode = false );

private:
    PCB_EDIT_FRAME*          m_Frame;
    BOARD*                   m_Pcb;
    BOARD_DESIGN_SETTINGS*   m_BrdSettings;
};

#endif //PANEL_SETUP_TRACKS_AND_VIAS_H
