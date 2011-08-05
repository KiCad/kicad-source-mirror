/**
 * @file pcb_calculator.h
 */

#ifndef  PCB_CALCULATOR_H
#define  PCB_CALCULATOR_H

#include "pcb_calculator_frame_base.h"

#include "transline.h"          // Included for SUBST_PRMS_ID definition.
#include "transline_ident.h"
#include "attenuator_classes.h"

/* Class PCB_CALCULATOR_FRAME_BASE
This is the main frame for this application
*/
class PCB_CALCULATOR_FRAME : public PCB_CALCULATOR_FRAME_BASE
{
private:
    wxSize m_FrameSize;
    wxPoint m_FramePos;
    wxConfig * m_Config;
    enum transline_type_id m_currTransLineType;
    TRANSLINE * m_currTransLine;        // a pointer to the active transline
    // List of translines: ordered like in dialog menu list
    std::vector <TRANSLINE_IDENT *> m_transline_list;
    ATTENUATOR * m_currAttenuator;
    // List ofattenuators: ordered like in dialog menu list
    std::vector <ATTENUATOR *> m_attenuator_list;


public:
    PCB_CALCULATOR_FRAME( wxWindow * parent = NULL );
    ~PCB_CALCULATOR_FRAME();

private:

    // Event handlers
    // These 3 functions are called by the OnPaint event, to draw
    // icons that show the current item on the specific panels
	void OnPaintTranslinePanel( wxPaintEvent& event );
	void OnPaintAttenuatorPanel( wxPaintEvent& event );
	void OnPaintAttFormulaPanel( wxPaintEvent& event );

    // Config read-write
    void ReadConfig();
    void WriteConfig();

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
     * @param aType = the transline_type_id of the new selected transline
    */
	void TranslineTypeSelection( enum transline_type_id aType );

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

#endif  // PCB_CALCULATOR_H
