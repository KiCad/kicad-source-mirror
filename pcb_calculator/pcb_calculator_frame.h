/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 1992-2021 Kicad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef PCB_CALCULATOR_H
#define PCB_CALCULATOR_H

#include "transline/transline.h" // Included for SUBST_PRMS_ID definition.
#include "transline_ident.h"
#include "pcb_calculator_frame_base.h"

#include "attenuators/attenuator_classes.h"
#include "class_regulator_data.h"

extern const wxString PcbCalcDataFileExt;

class APP_SETTINGS_BASE;
class KIWAY;
class PCB_CALCULATOR_SETTINGS;


/**
 * PCB calculator the main frame.
 */
class PCB_CALCULATOR_FRAME : public PCB_CALCULATOR_FRAME_BASE
{
public:
    PCB_CALCULATOR_FRAME( KIWAY* aKiway, wxWindow* aParent );
    ~PCB_CALCULATOR_FRAME();

    /**
     * Read/write params values and results.
     *
     * @param aPrmId is the parameter id to write.
     * @param aValue is the value to write.
     */
    void SetPrmValue( enum PRMS_ID aPrmId, double aValue );

    /**
     * Put the text into the given result line.
     *
     * @param aLineNumber is the line (0 to 5) where to display the text.
     * @param aText is the text to display.
     */
    void SetResult( int aLineNumber, const wxString& aText );

    /**
     *  Set the background color of a parameter.
     *
     *  @param aPrmId is the parameter id to set.
     *  @param aCol is the new color.
     */
    void SetPrmBgColor( enum PRMS_ID aPrmId, const KIGFX::COLOR4D* aCol );

    /**
     * Return a param value.
     *
     * @param aPrmId is the parameter id to write.
     * @return the value always in normalized unit (meter, Hz, Ohm, radian).
     */
    double GetPrmValue( enum PRMS_ID aPrmId ) const;

    /**
     * @return true if the parameter aPrmId is selected.
     */
    bool IsPrmSelected( enum PRMS_ID aPrmId ) const;

    // Board classes panel:
    void OnBoardClassesUnitsSelection( wxCommandEvent& event ) override;
    void BoardClassesUpdateData( double aUnitScale );

    // Calculator doesn't host a tool framework
    wxWindow* GetToolCanvas() const override
    {
        return nullptr;
    }

private:
    // Event handlers
    void OnClosePcbCalc( wxCloseEvent& event ) override;

    void OnUpdateUI( wxUpdateUIEvent& event ) override;

    // Config read-write, virtual from EDA_BASE_FRAME
    void LoadSettings( APP_SETTINGS_BASE* aCfg ) override;
    void SaveSettings( APP_SETTINGS_BASE* aCfg ) override;

    // R/W data files:
    bool ReadDataFile();
    bool WriteDataFile();

    /**
     * @return the full filename of the selected pcb_calculator data file
     */
    const wxString GetDataFilename();

    /**
     * Initialize the full filename of the selected pcb_calculator data file
     * force the standard extension of the file (.pcbcalc).
     *
     * @param aFilename is the full filename, with or without extension.
     */
    void SetDataFilename( const wxString& aFilename );

    /**
     * Panel-specific initializers
     */
    void initTrackWidthPanel();
    void initESeriesPanel();
    void initColorCodePanel();
    void initViaSizePanel();

    /**
     * Called on calculate button and executes all E-series calculations
     */
    void OnCalculateESeries( wxCommandEvent& event ) override;

    /**
     * Radio Buttons to select the E-serie for the resistor calculator.
     *
     * @param event contains the radio button state.
     */
    void OnESeriesSelection( wxCommandEvent& event ) override;

    /**
     * Write track width parameters in configuration.
     */
    void writeTrackWidthConfig();

    /**
     * Update the calculations the user changes the general parameters.
     */
    void OnTWParametersChanged( wxCommandEvent& event ) override;

    /**
     * Update the calculations when the user changes the desired maximum current.
     */
    void OnTWCalculateFromCurrent( wxCommandEvent& event ) override;

    /**
     * Update the calculations when the user changes the desired external trace width.
     */
    void OnTWCalculateFromExtWidth( wxCommandEvent& event ) override;

    /**
     * Update the calculations when the user changes the desired internal trace width.
     */
    void OnTWCalculateFromIntWidth( wxCommandEvent& event ) override;

    /**
     * Update the calculations when the user clicks the reset button.
     */
    void OnTWResetButtonClick( wxCommandEvent& event ) override;

    /**
     * Calculate track width required based on given current and temperature rise.
     */
    double TWCalculateWidth( double aCurrent, double aThickness, double aDeltaT_C,
                             bool aUseInternalLayer );

    /**
     * Calculate maximum current based on given width and temperature rise.
     */
    double TWCalculateCurrent( double aWidth, double aThickness, double aDeltaT_C,
                               bool aUseInternalLayer );

    /**
     * Display the results of a calculation (including resulting values such
     * as the resistance and power loss).
     */
    void TWDisplayValues( double aCurrent, double aExtWidth, double aIntWidth,
                          double aExtThickness, double aIntThickness );

    /**
     * Update the fields to show whether the maximum current, external trace
     * width, or internal trace width is currently the controlling parameter.
     */
    void TWUpdateModeDisplay();

    /**
     * Write via size parameters in configuration.
     */
    void writeViaSizeConfig();

    /**
     * Called when the user changes any value in the via calculator.
     */
    void OnViaCalculate( wxCommandEvent& event ) override;

    void OnViaEpsilonR_Button( wxCommandEvent& event ) override;

    void OnViaRho_Button( wxCommandEvent& event ) override;

    /**
     * Update the Error message in via calculation panel.
     */
    void onUpdateViaCalcErrorText( wxUpdateUIEvent& event ) override;

    /**
     * Called when the user clicks the reset button; sets the parameters to their default values.
     */
    void OnViaResetButtonClick( wxCommandEvent& event ) override;

    /**
     * Display the results of the calculation.
     */
    void VSDisplayValues( double aViaResistance, double aVoltageDrop, double aPowerLoss,
                          double aEstimatedAmpacity, double aThermalResistance,
                          double aCapacitance, double aTimeDegradation, double aInductance,
                          double aReactance );

    // Electrical spacing panel:
    void OnElectricalSpacingUnitsSelection( wxCommandEvent& event ) override;
    void OnElectricalSpacingRefresh( wxCommandEvent& event ) override;
    void ElectricalSpacingUpdateData( double aUnitScale );

    /**
     * Called on new transmission line selection.
     */
    void OnTranslineSelection( wxCommandEvent& event ) override;

    /**
     * Called when the user clicks the reset button; sets the parameters to their default values.
     */
    void OnTransLineResetButtonClick( wxCommandEvent& event ) override;

    /**
     * Run a new analyze for the current transline with current parameters and displays the
     * electrical parameters.
     */
    void OnTranslineAnalyse( wxCommandEvent& event ) override;

    /**
     * Run a new synthesis for the current transline with current parameters and displays the
     * geometrical parameters.
     */
    void OnTranslineSynthetize( wxCommandEvent& event ) override;

    /**
     * Shows a list of current relative dielectric constant(Er) and set the selected value in
     * main dialog frame.
     */
    void OnTranslineEpsilonR_Button( wxCommandEvent& event ) override;

    /**
     * Show a list of current dielectric loss factor (tangent delta) and set the selected value
     * in main dialog frame.
     */
    void OnTranslineTanD_Button( wxCommandEvent& event ) override;

    /**
     * Show a list of current Specific resistance list (rho) and set the selected value in main
     * dialog frame.
     */
    void OnTranslineRho_Button( wxCommandEvent& event ) override;

    /**
     * Must be called after selection of a new transline.
     *
     * Update all values, labels and tool tips of parameters needed by the new transline;
     * irrelevant parameters are blanked.
     *
     * @param aType is the #TRANSLINE_TYPE_ID of the new selected transmission line.
    */
    void TranslineTypeSelection( enum TRANSLINE_TYPE_ID aType );

    /**
     * Read values entered in dialog frame, and transfer these values in current transline
     * parameters, converted in normalized units.
     */
    void TransfDlgDataToTranslineParams();

    // Color Code panel
    void OnToleranceSelection( wxCommandEvent& event ) override;
    void ToleranceSelection( int aSelection );

    // Attenuators Panel
    void OnAttenuatorSelection( wxCommandEvent& event ) override;
    void SetAttenuator( unsigned aIdx );
    void OnCalculateAttenuator( wxCommandEvent& event ) override;
    void TransfPanelDataToAttenuator();
    void TransfAttenuatorDataToPanel();
    void TransfAttenuatorResultsToPanel();

    // Regulators Panel
    void OnRegulatorCalcButtonClick( wxCommandEvent& event ) override;
    void OnRegulatorResetButtonClick( wxCommandEvent& event ) override;
    void OnRegulTypeSelection( wxCommandEvent& event ) override;
    void OnRegulatorSelection( wxCommandEvent& event ) override;
    void OnDataFileSelection( wxCommandEvent& event ) override;
    void OnAddRegulator( wxCommandEvent& event ) override;
    void OnEditRegulator( wxCommandEvent& event ) override;
    void OnRemoveRegulator( wxCommandEvent& event ) override;

    /**
     * Update the regulator page dialog display.
     *
     * Enable the current regulator drawings and the formula used for calculations.
     */
    void RegulatorPageUpdate();

    /**
     * If m_lastSelectedRegulatorName is empty, just calls RegulatorPageUpdate()
     */
    void SelectLastSelectedRegulator();

    void RegulatorsSolve();

    /**
     * Write regulators parameters in configuration.
     *
     * @param aCfg is the configuration settings.
     */
    void Regulators_WriteConfig( PCB_CALCULATOR_SETTINGS* aCfg );

public:
    REGULATOR_LIST m_RegulatorList;      // the list of known regulator

private:
    bool m_RegulatorListChanged; // Set when m_RegulatorList is modified and the corresponding file
                                 // must be rewritten

    enum                         // Which dimension is controlling the track width / current
    {                            // calculations:
        TW_MASTER_CURRENT,       //   the maximum current,
        TW_MASTER_EXT_WIDTH,     //   the external trace width,
        TW_MASTER_INT_WIDTH      //   or the internal trace width?
    } m_TWMode;

    bool                          m_TWNested; // Used to stop events caused by setting the answers.

    enum TRANSLINE_TYPE_ID        m_currTransLineType;
    TRANSLINE*                    m_currTransLine;
    std::vector<TRANSLINE_IDENT*> m_transline_list;

    ATTENUATOR*                   m_currAttenuator;
    std::vector<ATTENUATOR*>      m_attenuator_list;

    wxString                      m_lastSelectedRegulatorName;

    wxBitmap*                     m_ccValueNamesBitmap;
    wxBitmap*                     m_ccValuesBitmap;
    wxBitmap*                     m_ccMultipliersBitmap;
    wxBitmap*                     m_ccTolerancesBitmap;

    int                           m_lastNotebookPage;
    bool                          m_macHack;
};


extern const wxString DataFileNameExt;

#endif // PCB_CALCULATOR_H
