/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 1992-2020 Kicad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file pcb_calculator.h
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


/* Class PCB_CALCULATOR_FRAME_BASE
This is the main frame for this application
*/
class PCB_CALCULATOR_FRAME : public PCB_CALCULATOR_FRAME_BASE
{
public:
    REGULATOR_LIST m_RegulatorList;      // the list of known regulator

private:
    bool m_RegulatorListChanged; // set to true when m_RegulatorList
                                 // was modified, and the corresponging file
                                 // must be rewritten

    enum                     // Which dimension is controlling the track
    {                        // width / current calculations:
        TW_MASTER_CURRENT,   // the maximum current,
        TW_MASTER_EXT_WIDTH, // the external trace width,
        TW_MASTER_INT_WIDTH  // or the internal trace width?
    } m_TWMode;

    bool m_TWNested; // Used to stop events caused by setting the answers.

    enum TRANSLINE_TYPE_ID m_currTransLineType;
    TRANSLINE*             m_currTransLine; // a pointer to the active transline
    // List of translines: ordered like in dialog menu list
    std::vector<TRANSLINE_IDENT*> m_transline_list;

    ATTENUATOR* m_currAttenuator;
    // List ofattenuators: ordered like in dialog menu list
    std::vector<ATTENUATOR*> m_attenuator_list;
    wxString                 m_lastSelectedRegulatorName; // last regulator name selected

    int                      m_lastNotebookPage;
    int                      m_lastRadioButton;

public:
    PCB_CALCULATOR_FRAME( KIWAY* aKiway, wxWindow* aParent );
    ~PCB_CALCULATOR_FRAME();

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
     * force the standard extension of the file (.pcbcalc)
     * @param aFilename = the full filename, with or without extension
     */
    void SetDataFilename( const wxString& aFilename );

    // Trace width / maximum current capability calculations.
    /**
     * Function TW_Init
     * Read config and init dialog widgets values
     */
    void TW_Init();

    /**
     * E-Series Resistor calculator Panel
     * Called on calculator start to display markdown formula explanations
     */
    void ES_Init( void );

    /**
     * Called on calculate button and executes all E-series calculations
     *
     */

    void OnCalculateESeries( wxCommandEvent& event ) override;

    /**
     * Radio Buttons to select the E-serie for the resistor calculator
     * @param event contains the radio button state
     */
    void OnESeriesSelection( wxCommandEvent& event ) override;

    /**
     * Function TW_WriteConfig
     * Write Track width parameters in config
     */
    void TW_WriteConfig();

    /**
     * Function OnTWParametersChanged
     * Called when the user changes the general parameters (i.e., anything that
     * is not one of the controlling values). This update the calculations.
     */
    void OnTWParametersChanged( wxCommandEvent& event ) override;

    /**
     * Function OnTWCalculateFromCurrent
     * Called when the user changes the desired maximum current. This sets the
     * current as the controlling value and performs the calculations.
     */
    void OnTWCalculateFromCurrent( wxCommandEvent& event ) override;

    /**
     * Function OnTWCalculateFromExtWidth
     * Called when the user changes the desired external trace width. This sets
     * the external width as the controlling value and performs the calculations.
     */
    void OnTWCalculateFromExtWidth( wxCommandEvent& event ) override;

    /**
     * Function OnTWCalculateFromIntWidth
     * Called when the user changes the desired internal trace width. This sets
     * the internal width as the controlling value and performs the calculations.
     */
    void OnTWCalculateFromIntWidth( wxCommandEvent& event ) override;

    /**
     * Function OnTWResetButtonClick
     * Called when the user clicks the reset button. This sets
     * the parameters to their default values.
     */
    void OnTWResetButtonClick( wxCommandEvent& event ) override;

    /**
     * Function TWCalculateWidth
     * Calculate track width required based on given current and temperature rise.
     */
    double TWCalculateWidth( double aCurrent, double aThickness, double aDeltaT_C,
                             bool aUseInternalLayer );

    /**
     * Function TWCalculateCurrent
     * Calculate maximum current based on given width and temperature rise.
     */
    double TWCalculateCurrent( double aWidth, double aThickness, double aDeltaT_C,
                               bool aUseInternalLayer );

    /**
     * Function TWDisplayValues
     * Displays the results of a calculation (including resulting values such
     * as the resistance and power loss).
     */
    void TWDisplayValues( double aCurrent, double aExtWidth, double aIntWidth,
                          double aExtThickness, double aIntThickness );

    /**
     * Function TWUpdateModeDisplay
     * Updates the fields to show whether the maximum current, external trace
     * width, or internal trace width is currently the controlling parameter.
     */
    void TWUpdateModeDisplay();

    // Via size calculations

    /**
     * Function VS_Init
     * Read config and init dialog widgets values
     */
    void VS_Init();

    /**
     * Function VS_WriteConfig
     * Write Via Size parameters in config
     */
    void VS_WriteConfig();

    /**
     * Function OnViaCalculate
     * Called when the user changes any value in the via calcultor.
     */
    void OnViaCalculate( wxCommandEvent& event ) override;

    /**
     * Function OnViaEpsilonR_Button
     */
    void OnViaEpsilonR_Button( wxCommandEvent& event ) override;

    /**
     * Function OnViaRho_Button
     */
    void OnViaRho_Button( wxCommandEvent& event ) override;

    /**
     * Update the Error message in Via calculation panel
     */
    void onUpdateViaCalcErrorText( wxUpdateUIEvent& event ) override;

    /**
    * Function OnViaResetButtonClick
    * Called when the user clicks the reset button. This sets
    * the parameters to their default values.
    */
    void OnViaResetButtonClick( wxCommandEvent& event ) override;

    /**
     * Function VSDisplayValues
     * Displays the results of the calculation.
     */
    void VSDisplayValues( double aViaResistance, double aVoltageDrop, double aPowerLoss,
                          double aEstimatedAmpacity, double aThermalResistance,
                          double aCapacitance, double aTimeDegradation, double aInductance,
                          double aReactance );

    // Electrical spacing panel:
    void OnElectricalSpacingUnitsSelection( wxCommandEvent& event ) override;
    void OnElectricalSpacingRefresh( wxCommandEvent& event ) override;
    void ElectricalSpacingUpdateData( double aUnitScale );

    // Transline functions:
    /**
     * Function OnTranslineSelection
     * Called on new transmission line selection
    */
    void OnTranslineSelection( wxCommandEvent& event ) override;

    /**
     * Function OnTransLineResetButtonClick
     * Called when the user clicks the reset button. This sets
     * the parameters to their default values.
    */
    void OnTransLineResetButtonClick( wxCommandEvent& event ) override;

    /**
     * Function OnTranslineAnalyse
     * Run a new analyse for the current transline with current parameters
     * and displays the electrical parameters
     */
    void OnTranslineAnalyse( wxCommandEvent& event ) override;

    /**
     * Function OnTranslineSynthetize
     * Run a new synthezis for the current transline with current parameters
     * and displays the geometrical parameters
     */
    void OnTranslineSynthetize( wxCommandEvent& event ) override;

    /**
     * Function OnTranslineEpsilonR_Button
     * Shows a list of current relative dielectric constant(Er)
     * and set the selected value in main dialog frame
     */
    void OnTranslineEpsilonR_Button( wxCommandEvent& event ) override;

    /**
     * Function OnTranslineTanD_Button
     * Shows a list of current dielectric loss factor (tangent delta)
     * and set the selected value in main dialog frame
     */
    void OnTranslineTanD_Button( wxCommandEvent& event ) override;

    /**
     * Function OnTranslineRho_Button
     * Shows a list of current Specific resistance list (rho)
     * and set the selected value in main dialog frame
     */
    void OnTranslineRho_Button( wxCommandEvent& event ) override;

    /**
     * Function TranslineTypeSelection
     * Must be called after selection of a new transline.
     * Update all values, labels and tool tips of parameters needed
     * by the new transline
     * Irrelevant parameters texts are blanked.
     * @param aType = the TRANSLINE_TYPE_ID of the new selected transline
    */
    void TranslineTypeSelection( enum TRANSLINE_TYPE_ID aType );

    /**
     * Function TransfDlgDataToTranslineParams
     * Read values entered in dialog frame, and transfert these
     * values in current transline parameters, converted in normalized units
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
     * Function RegulatorPageUpdate:
     * Update the regulator page dialog display:
     * enable the current regulator drawings and the formula used for calculations
     */
    void RegulatorPageUpdate();

    /**
     * Function SelectLastSelectedRegulator
     * select in choice box the last selected regulator
     * (name in m_lastSelectedRegulatorName)
     * and update the displayed values.
     * if m_lastSelectedRegulatorName is empty, just calls
     * RegulatorPageUpdate()
     */
    void SelectLastSelectedRegulator();

    void RegulatorsSolve();

    /**
     * Write regulators parameters in config
     * @param aCfg is the config settings
     */
    void Regulators_WriteConfig( PCB_CALCULATOR_SETTINGS* aCfg );

public:
    // Read/write params values and results

    /**
     * Function SetPrmValue
     * Read/write params values and results
     * @param aPrmId = param id to write
     * @param aValue = valmue to write
     */
    void SetPrmValue( enum PRMS_ID aPrmId, double aValue );

    /**
     * Function SetResult
     * Puts the text into the given result line.
     * @param aLineNumber = the line (0 to 5) wher to display the text
     * @param aText = the text to display
     */
    void SetResult( int aLineNumber, const wxString& aText );

    /** Function SetPrgmBgColor
     *  Set the background color of a parameter
     *  @param aPrmId = param id to set
     *  @param aCol = new color
     */
    void SetPrmBgColor( enum PRMS_ID aPrmId, const KIGFX::COLOR4D* aCol );
    /**
     * Function GetPrmValue
     * Returns a param value.
     * @param aPrmId = param id to write
     * @return the value always in normalized unit (meter, Hz, Ohm, radian)
     */
    double GetPrmValue( enum PRMS_ID aPrmId );

    /**
     * Function IsPrmSelected
     * @return true if the param aPrmId is selected
     * Has meaning only for params that have a radio button
     */
    bool IsPrmSelected( enum PRMS_ID aPrmId );

    // Board classes panel:
    void OnBoardClassesUnitsSelection( wxCommandEvent& event ) override;
    void BoardClassesUpdateData( double aUnitScale );

    // Calculator doesn't host a tool framework
    wxWindow* GetToolCanvas() const override
    {
        return nullptr;
    }
};


extern const wxString DataFileNameExt;

#endif // PCB_CALCULATOR_H
