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

#ifndef PANEL_SETUP_TUNING_PROFILES_H
#define PANEL_SETUP_TUNING_PROFILES_H

#include <memory>

#include <board.h>
#include <dialog_shim.h>
#include <dialogs/panel_setup_tuning_profiles_base.h>
#include <project/tuning_profiles.h>

class PANEL_SETUP_TUNING_PROFILE_INFO;

class PANEL_SETUP_TUNING_PROFILES final : public PANEL_SETUP_TUNING_PROFILES_BASE
{
public:
    friend class PANEL_SETUP_TUNING_PROFILE_INFO;

    PANEL_SETUP_TUNING_PROFILES( wxWindow* aParentWindow, PCB_EDIT_FRAME* aFrame, BOARD* aBoard,
                                 std::shared_ptr<TUNING_PROFILES> aTimeDomainParameters );
    ~PANEL_SETUP_TUNING_PROFILES() override;

    /**
     * Called when switching to this tab to make sure that any changes to the copper layer count
     * made on the physical stackup page are reflected here
     * @param aNumCopperLayers is the number of copper layers in the board
     */
    void SyncCopperLayers( int aNumCopperLayers );

    /// Load parameter data from the settings object
    bool TransferDataToWindow() override;

    /// Save parameter data to the settings object
    bool TransferDataFromWindow() override;

    /// Returns all configured tuning profile names. Used by the netclass setup panel
    std::vector<wxString> GetDelayProfileNames() const;

    /// Load configuration from the given settings object
    void ImportSettingsFrom( const std::shared_ptr<TUNING_PROFILES>& aOtherParameters );

    /// Adds a new tuning profile entry to the tuning profile grid
    void OnAddTuningProfileClick( wxCommandEvent& event ) override;

    /// Removes a tuning profile entry from the tuning profile grid
    void OnRemoveTuningProfileClick( wxCommandEvent& event ) override;

    /// Validates all data
    bool Validate() override;

    /// Update the notebook display of the name for a given panel
    void UpdateProfileName( PANEL_SETUP_TUNING_PROFILE_INFO* panel, wxString newName ) const;

private:
    /// The parameters object to load / save data from / to
    std::shared_ptr<TUNING_PROFILES> m_tuningProfileParameters;

    /// The parent dialog
    DIALOG_SHIM* m_dlg;

    /// The active edit frame
    PCB_EDIT_FRAME* m_frame;

    /// The current board
    BOARD* m_board;

    std::unique_ptr<UNITS_PROVIDER> m_unitsProvider;

    // Layer / index lookups
    std::map<PCB_LAYER_ID, int> m_copperLayerIdsToIndex;
    std::map<int, PCB_LAYER_ID> m_copperIndexToLayerId;
    std::vector<wxString>       m_layerNames;

    // We cache these in case the names change on the layers panel
    std::map<wxString, PCB_LAYER_ID> m_layerNamesToIDs;
    std::map<wxString, PCB_LAYER_ID> m_prevLayerNamesToIDs;
};


#endif //PANEL_SETUP_TUNING_PROFILES_H
