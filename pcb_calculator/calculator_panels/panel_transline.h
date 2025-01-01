/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef PANEL_TRANSLINE_H
#define PANEL_TRANSLINE_H

#include "panel_transline_base.h"
#include <transline/transline.h>
#include "transline_ident.h"

class PCB_CALCULATOR_SETTINGS;


class PANEL_TRANSLINE : public PANEL_TRANSLINE_BASE
{
public:
    PANEL_TRANSLINE( wxWindow* parent, wxWindowID id = wxID_ANY,
                     const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize,
                     long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );
    ~PANEL_TRANSLINE();

    // Methods from CALCULATOR_PANEL that must be overridden
    void LoadSettings( PCB_CALCULATOR_SETTINGS* aCfg ) override;
    void SaveSettings( PCB_CALCULATOR_SETTINGS* aCfg ) override;
    void ThemeChanged() override;

    // Accessors:
    wxRadioBox*       GetTranslineSelector() { return m_TranslineSelection; }
    TRANSLINE_TYPE_ID GetCurrTransLineType() { return m_currTransLineType; }

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

private:
    TRANSLINE*                    m_currTransLine;
    std::vector<TRANSLINE_IDENT*> m_transline_list;

    enum TRANSLINE_TYPE_ID        m_currTransLineType;
};

#endif
