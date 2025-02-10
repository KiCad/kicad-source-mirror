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


#ifndef PANEL_SETUP_TIME_DOMAIN_PARAMETERS_H
#define PANEL_SETUP_TIME_DOMAIN_PARAMETERS_H

#include <utility>

#include <board.h>
#include <dialogs/panel_setup_time_domain_parameters_base.h>
#include <project/time_domain_parameters.h>

class NET_SETTINGS;


class PANEL_SETUP_TIME_DOMAIN_PARAMETERS : public PANEL_SETUP_TIME_DOMAIN_PARAMETERS_BASE
{
public:
    PANEL_SETUP_TIME_DOMAIN_PARAMETERS(
            wxWindow* aParentWindow, PCB_EDIT_FRAME* aFrame, BOARD* aBoard,
            std::shared_ptr<TIME_DOMAIN_PARAMETERS> aTimeDomainParameters );
    ~PANEL_SETUP_TIME_DOMAIN_PARAMETERS() override;

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
    void ImportSettingsFrom( const std::shared_ptr<TIME_DOMAIN_PARAMETERS>& aOtherParameters );

private:
    enum PROFILE_GRID_REQUIRED_COLS
    {
        PROFILE_GRID_PROFILE_NAME = 0,
        PROFILE_GRID_VIA_PROP_DELAY,
        PROFILE_GRID_NUM_REQUIRED_COLS
    };

    enum VIA_GRID_REQUIRED_COLS
    {
        VIA_GRID_PROFILE_NAME = 0,
        VIA_GRID_SIGNAL_LAYER_FROM,
        VIA_GRID_SIGNAL_LAYER_TO,
        VIA_GRID_VIA_LAYER_FROM,
        VIA_GRID_VIA_LAYER_TO,
        VIA_GRID_DELAY
    };

    /// Optimise grid columns to fit titles and content
    void setColumnWidths();

    /// Updates the via override tuning profile name dropdown lists
    /// Updates entries if aOldName and aNewName are passed
    void updateViaProfileNamesEditor( const wxString& aOldName = wxEmptyString,
                                      const wxString& aNewName = wxEmptyString ) const;

    /// Update the dynamic (per-layer) columns in the tuning profiles grid
    void updateProfileGridColumns();

    /// Update the dynamic (per-layer) columns in the via overrides grid
    void updateViaGridColumns();

    /// Adds a new tuning profile entry to the tuning profile grid
    void OnAddDelayProfileClick( wxCommandEvent& event ) override;

    /// Removes a tuning profile entry from the tuning profile grid
    void OnRemoveDelayProfileClick( wxCommandEvent& event ) override;

    /// Adds a new via override profile entry to the via overrides grid
    void OnAddViaOverrideClick( wxCommandEvent& event ) override;

    /// Removes a via override profile entry from the via overrides grid
    void OnRemoveViaOverrideClick( wxCommandEvent& event ) override;

    /// Validates a tuning profile row data
    void OnDelayProfileGridCellChanging( wxGridEvent& event );

    /// Validates a tuning profile name (checks for not empty and not duplicated)
    bool validateDelayProfileName( int aRow, const wxString& aName, bool focusFirst = true );

    /// Validates all data
    bool Validate() override;

    /// Validates all via override rows
    bool validateViaRows();

    /// Adds a tuning profile row with the given persisted parameters
    void addProfileRow( const TIME_DOMAIN_TUNING_PROFILE& aDelayProfile );

    /// Gets a tuning profile row as a set of persistable parameters
    TIME_DOMAIN_TUNING_PROFILE getProfileRow( int aRow );

    /// Adds a via override row with the given persisted parameters
    void addViaRow( const wxString& aProfileName, const TUNING_PROFILE_VIA_OVERRIDE_ENTRY& aViaOverrideEntry ) const;

    /// Gets a via override row as a set of persistable parameters
    TUNING_PROFILE_VIA_OVERRIDE_ENTRY getViaRow( int aRow );

    /// Gets the profile name for the given profile grid row
    wxString getProfileNameForProfileGridRow( int aRow ) const;

    /// The parameters object to load / save data from / to
    std::shared_ptr<TIME_DOMAIN_PARAMETERS> m_timeDomainParameters;

    /// The active edit frame
    PCB_EDIT_FRAME* m_frame;

    /// The current board
    BOARD* m_board;

    std::unique_ptr<UNITS_PROVIDER> m_unitsProvider;

    // Layer / column lookups
    std::map<PCB_LAYER_ID, int> m_copperLayerIdsToColumns;
    std::map<int, PCB_LAYER_ID> m_copperColumnsToLayerId;
    std::vector<wxString>       m_layerNames;

    // We cache these in case the names change on the layers panel
    std::map<wxString, PCB_LAYER_ID> m_layerNamesToIDs;
    std::map<wxString, PCB_LAYER_ID> m_prevLayerNamesToIDs;
};

#endif // PANEL_SETUP_TIME_DOMAIN_PARAMETERS_H
