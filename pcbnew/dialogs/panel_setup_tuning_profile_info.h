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

#ifndef PANEL_SETUP_TUNING_PROFILE_INFO_H
#define PANEL_SETUP_TUNING_PROFILE_INFO_H

#include <dialogs/panel_setup_tuning_profile_info_base.h>
#include <project/tuning_profiles.h>
#include <transline_calculations/coupled_microstrip.h>
#include <transline_calculations/coupled_stripline.h>
#include <transline_calculations/microstrip.h>
#include <transline_calculations/stripline.h>
#include <widgets/unit_binder.h>
#include <board_stackup_manager/board_stackup.h>
#include <layer_ids.h>

class PANEL_SETUP_TUNING_PROFILES;

class PANEL_SETUP_TUNING_PROFILE_INFO : public PANEL_SETUP_TUNING_PROFILE_INFO_BASE
{
public:
    PANEL_SETUP_TUNING_PROFILE_INFO( wxWindow* aParentWindow, PANEL_SETUP_TUNING_PROFILES* parentPanel );

    ~PANEL_SETUP_TUNING_PROFILE_INFO() override;

    /// Updates the displayed layer names in all grids
    void UpdateLayerNames();

    /// Loads the given profile in to the panel
    void LoadProfile( const TUNING_PROFILE& aProfile );

    /// Saves the panel to the given profile
    TUNING_PROFILE GetProfile() const;

    /// Updates the parent notebook control
    void OnProfileNameChanged( wxCommandEvent& event ) override;

    /// Changes between Single and Differential profiles
    void OnChangeProfileType( wxCommandEvent& event ) override;

    /// Adds a row to the track propagation grid
    void OnAddTrackRow( wxCommandEvent& event ) override;

    /// Removes a row from the track propagation grid
    void OnRemoveTrackRow( wxCommandEvent& event ) override;

    /// Adds a via override row
    void OnAddViaOverride( wxCommandEvent& event ) override;

    /// Removes a via override row
    void OnRemoveViaOverride( wxCommandEvent& event ) override;

    /// Gets the name of this profile
    wxString GetProfileName() const;

    /// Validate this panel's data
    bool ValidateProfile( size_t aPageIndex );

private:
    enum TRACK_GRID_COLS
    {
        TRACK_GRID_SIGNAL_LAYER = 0,
        TRACK_GRID_TOP_REFERENCE,
        TRACK_GRID_BOTTOM_REFERENCE,
        TRACK_GRID_TRACK_WIDTH,
        TRACK_GRID_TRACK_GAP,
        TRACK_GRID_DELAY
    };

    enum VIA_GRID_COLS
    {
        VIA_GRID_SIGNAL_LAYER_FROM = 0,
        VIA_GRID_SIGNAL_LAYER_TO,
        VIA_GRID_VIA_LAYER_FROM,
        VIA_GRID_VIA_LAYER_TO,
        VIA_GRID_DELAY
    };

    enum class CalculationType
    {
        WIDTH,
        GAP,
        DELAY
    };

    struct CALCULATION_RESULT
    {
        CALCULATION_RESULT() = default;
        explicit CALCULATION_RESULT( const wxString& errorMsg ) :
                ErrorMsg{ errorMsg }
        {
        }
        explicit CALCULATION_RESULT( const int aWidth, const int aDelay ) :
                OK{ true },
                Width{ aWidth },
                Delay{ aDelay }
        {
        }
        explicit CALCULATION_RESULT( const int aWidth, const int aDiffPairGap, const int aDelay ) :
                OK{ true },
                Width{ aWidth },
                DiffPairGap{ aDiffPairGap },
                Delay{ aDelay }
        {
        }

        bool     OK{ false };
        int      Width{ 0 };
        int      DiffPairGap{ 0 };
        int      Delay{ 0 };
        wxString ErrorMsg;
    };

    struct CALCULATION_BOARD_PARAMETERS
    {
        double DielectricConstant{ 0 };
        double TopDielectricLayerThickness{ 0 };
        double BottomDielectricLayerThickness{ 0 };
        double SignalLayerThickness{ 0 };
        double LossTangent{ 0 };
    };

    /// Initialises all controls on the panel
    void initPanel();

    /// Set up the widths of all grid columns
    void setColumnWidths();

    /// Gets the board parameters for microstrip calculations
    /// @parameter aRow The grid row to calculate board parameters for
    std::pair<CALCULATION_BOARD_PARAMETERS, CALCULATION_RESULT>
    getMicrostripBoardParameters( int aRow, const EDA_IU_SCALE& aScale );

    /// Gets the board parameters for stripline calculations
    /// @parameter aRow The grid row to calculate board parameters for
    std::pair<CALCULATION_BOARD_PARAMETERS, CALCULATION_RESULT>
    getStriplineBoardParameters( int aRow, const EDA_IU_SCALE& aScale );

    /// Calculates the track width or delay for the given propagation grid row
    /// @returns pair of (width, unit propagation delay) in IU
    CALCULATION_RESULT calculateSingleMicrostrip( const int aRow, CalculationType aCalculationType );

    /// Calculates the track width or delay for the given propagation grid row
    /// @returns pair of (width, unit propagation delay) in IU
    CALCULATION_RESULT calculateSingleStripline( const int aRow, CalculationType aCalculationType );

    /// Calculates the track width, pair gap, or delay for the given propagation grid row
    /// @returns tuple of (width, diff pair gap, unit propagation delay) in IU
    CALCULATION_RESULT calculateDifferentialMicrostrip( int aRow, CalculationType aCalculationType );

    /// Calculates the track width, pair gap, or delay for the given propagation grid row
    /// @returns tuple of (width, diff pair gap, unit propagation delay) in IU
    CALCULATION_RESULT calculateDifferentialStripline( int aRow, CalculationType aCalculationType );

    /// Calculate the effective skin depth for the given parameters
    static double calculateSkinDepth( double aFreq, double aMurc, double aSigma );

    /// Gets the index in to the layer list for the given layer.
    /// @returns -1 if not found
    static int getStackupLayerId( const std::vector<BOARD_STACKUP_ITEM*>& aLayerList, PCB_LAYER_ID aPcbLayerId );

    struct DIELECTRIC_INFO
    {
        double Height{ 0.0 };
        double E_r{ 0.0 };
        double Loss_Tangent{ 0.0 };
    };

    /**
     * Calculates the geometric average of the dielectric material properties. Note: This is a poor approximation as the
     * electric field distribution is not equal across the dielectrics. However, it will do as an approximation before
     * we have a field solver integrated.
     *
     * @returns DIELECTRIC_INFO contining calculated values
     */
    static DIELECTRIC_INFO
    calculateAverageDielectricConstants( const std::vector<BOARD_STACKUP_ITEM*>& aStackupLayerList,
                                         const std::vector<int>&                 dielectricLayerStackupIds,
                                         const EDA_IU_SCALE&                     aIuScale );

    /// Gets the dielectric layers for dielectrics between the two given copper layer IDs
    void getDielectricLayers( const std::vector<BOARD_STACKUP_ITEM*>& aStackupLayerList, int aSignalLayerId,
                              int aReferenceLayerId, std::vector<int>& aDielectricLayerStackupIds );

    /// Gets the target impedance for the profile
    double getTargetImpedance() const;

    /// Calculates the required track parameters for the given track parameters grid row and col
    void calculateTrackParametersForCell( int aRow, int aCol );

    /// Sets the panel display for the given tuning type
    void onChangeProfileType( TUNING_PROFILE::PROFILE_TYPE aType ) const;

    /// The parent setup panel
    PANEL_SETUP_TUNING_PROFILES* m_parentPanel;

    /// Units for global via propagation unit delay
    UNIT_BINDER m_viaPropagationUnits;

    /// Calculator for single microstrip parameters
    MICROSTRIP m_microstripCalc;

    /// Calculator for single stripline parameters
    STRIPLINE m_striplineCalc;

    /// Calculator for coupled (differential) microstrip parameters
    COUPLED_MICROSTRIP m_coupledMicrostripCalc;

    /// Calculator for coupled (differential) stripline parameters
    COUPLED_STRIPLINE m_coupledStriplineCalc;

    // Electrical resistivity or specific electrical resistance of copper (ohm*meter)
    static constexpr double RHO = 1.72e-8;
};


#endif //PANEL_SETUP_TUNING_PROFILE_INFO_H
