/**
 * @file pcb_calculator.h
 */

#ifndef  PCB_CALCULATOR_H
#define  PCB_CALCULATOR_H

#include <pcb_calculator_frame_base.h>

#include <transline.h>          // Included for SUBST_PRMS_ID definition.
#include <transline_ident.h>
#include <attenuator_classes.h>
#include <class_regulator_data.h>

extern const wxString PcbCalcDataFileExt;

/* Class PCB_CALCULATOR_FRAME_BASE
This is the main frame for this application
*/
class PCB_CALCULATOR_FRAME : public PCB_CALCULATOR_FRAME_BASE
{
public:
    REGULATOR_LIST m_RegulatorList;      // the list of known regulator

private:
    bool m_RegulatorListChanged;        // set to true when m_RegulatorList
                                        // was modified, and the corresponging file
                                        // must be rewritten
    wxSize          m_FrameSize;
    wxPoint         m_FramePos;
    wxConfigBase*   m_Config;
    enum TRANSLINE_TYPE_ID m_currTransLineType;
    TRANSLINE * m_currTransLine;        // a pointer to the active transline
    // List of translines: ordered like in dialog menu list
    std::vector <TRANSLINE_IDENT *> m_transline_list;
    ATTENUATOR * m_currAttenuator;
    // List ofattenuators: ordered like in dialog menu list
    std::vector <ATTENUATOR *> m_attenuator_list;
    wxString m_lastSelectedRegulatorName;   // last regulator name selected


public:
    PCB_CALCULATOR_FRAME( KIWAY* aKiway, wxWindow* aParent );
    ~PCB_CALCULATOR_FRAME();

private:

    // Event handlers
    void OnClosePcbCalc( wxCloseEvent& event );

    // These 3 functions are called by the OnPaint event, to draw
    // icons that show the current item on the specific panels
    void OnPaintTranslinePanel( wxPaintEvent& event );
    void OnPaintAttenuatorPanel( wxPaintEvent& event );
    void OnPaintAttFormulaPanel( wxPaintEvent& event );

    // Config read-write
    void ReadConfig();
    void WriteConfig();

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
    void SetDataFilename( const wxString & aFilename);

    // tracks width versus current functions:
    /**
     * Function OnTWCalculateButt
     * Called by clicking on the calculate button
     */
    void OnTWCalculateButt( wxCommandEvent& event );

    /**
     * Function TW_Init
     * Read config and init dialog widgets values
     */
    void TW_Init();

    /**
     * Function TW_WriteConfig
     * Write Track width prameters in config
     */
    void TW_WriteConfig();

    /**
     * Function TWCalculate
     * Performs track caracteristics values calculations.
     */
    double TWCalculate( double aCurrent, double aThickness, double aDeltaT_C,
                    bool aUseInternalLayer );

    // Electrical spacing panel:
    void OnElectricalSpacingUnitsSelection( wxCommandEvent& event );
    void OnElectricalSpacingRefresh( wxCommandEvent& event );
    void ElectricalSpacingUpdateData( double aUnitScale );

    // Transline functions:
    /**
     * Function OnTranslineSelection
     * Called on new transmission line selection
    */
    void OnTranslineSelection( wxCommandEvent& event );

    /**
     * Function OnTranslineAnalyse
     * Run a new analyse for the current transline with current parameters
     * and displays the electrical parmeters
     */
    void OnTranslineAnalyse( wxCommandEvent& event );

    /**
     * Function OnTranslineSynthetize
     * Run a new synthezis for the current transline with current parameters
     * and displays the geometrical parmeters
     */
    void OnTranslineSynthetize( wxCommandEvent& event );

    /**
     * Function OnTranslineEpsilonR_Button
     * Shows a list of current relative dielectric constant(Er)
     * and set the selected value in main dialog frame
     */
    void OnTranslineEpsilonR_Button( wxCommandEvent& event );

    /**
     * Function OnTranslineTanD_Button
     * Shows a list of current dielectric loss factor (tangent delta)
     * and set the selected value in main dialog frame
     */
    void OnTranslineTanD_Button( wxCommandEvent& event );

    /**
     * Function OnTranslineRho_Button
     * Shows a list of current Specific resistance list (rho)
     * and set the selected value in main dialog frame
     */
    void OnTranslineRho_Button( wxCommandEvent& event );

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
    void OnToleranceSelection( wxCommandEvent& event );
    void ToleranceSelection( int aSelection );

    // Attenuators Panel
    void OnAttenuatorSelection( wxCommandEvent& event );
    void SetAttenuator( unsigned aIdx );
    void OnCalculateAttenuator( wxCommandEvent& event );
    void TransfPanelDataToAttenuator();
    void TransfAttenuatorDataToPanel();
    void TransfAttenuatorResultsToPanel();

    // Regulators Panel
    void OnRegulatorCalcButtonClick( wxCommandEvent& event );
    void OnRegulTypeSelection( wxCommandEvent& event );
    void OnRegulatorSelection( wxCommandEvent& event );
    void OnDataFileSelection( wxCommandEvent& event );
    void OnAddRegulator( wxCommandEvent& event );
    void OnEditRegulator( wxCommandEvent& event );
    void OnRemoveRegulator( wxCommandEvent& event );

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
    void SetResult( int aLineNumber, const wxString & aText );

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
    bool   IsPrmSelected( enum PRMS_ID aPrmId );

    // Board classes panel:
    void OnBoardClassesUnitsSelection( wxCommandEvent& event );
    void BoardClassesUpdateData( double aUnitScale );

};


extern const wxString DataFileNameExt;

#endif  // PCB_CALCULATOR_H
