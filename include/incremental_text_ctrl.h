/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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

#ifndef INCREMENTAL_TEXT_CTRL__H_
#define INCREMENTAL_TEXT_CTRL__H_

#include <wx/wx.h>
#include <wx/spinbutt.h>

#include <functional>

/**
 * Class that governs a textual control holding a number that can
 * be incremented/decremented according to some scheme (often just
 * a constant step).
 */
class INCREMENTAL_TEXT_CTRL
{
public:

    /**
     * A callable object type that can be used to provide a step
     * value. Client can provide one of these to use for implementing
     * non-linear stepping, or stepping based on external parameters,
     * such as unit selection.
     *
     * @param aUp true if the next step is upwards
     * @param aCurrVal the current value of the control
     */
    using STEP_FUNCTION = std::function<double(bool aUp, double aCurrVal)>;

    INCREMENTAL_TEXT_CTRL();

    virtual ~INCREMENTAL_TEXT_CTRL() {}

    /**
     * Set the value of the text control, but obey the limits
     * currently set.
     *
     * @param aValue the control value to set
     */
    void SetValue( double aValue );

    /**
     * Get the current value of the control
     */
    double GetValue();

    /**
     * Function SetStep()
     *
     * Set the stepping parameters of the control. The range is
     * enforced by not allowing the scroll to exceed it, and on
     * loss of focus, the control is also clamped to the range.
     *
     * @param aMin the minium value allowed
     * @param aMax the maximum value allows
     * @param aNewFunc the step function used to calculate the next step
     */
    void SetStep( double aMin, double aMax, STEP_FUNCTION aNewFunc );

    /**
     * Function SetStep()
     *
     * Shortcut method to set step parameters when the step is constant
     *
     * @param aMin the minium value allowed
     * @param aMax the maximum value allows
     * @param aStep the constant step size
     */
    void SetStep( double aMin, double aMax, double aStep )
    {
        SetStep( aMin, aMax,
            [aStep] ( bool aUp, double aCurrent ) { return aUp ? aStep : -aStep; } );
    }

    /**
     * Set the number of decimal places to display
     */
    void SetPrecision( int aPrecision );

protected:

    /**
     * Increment the control by the given amount
     */
    void incrementCtrlBy( double aInc);

    /**
     * Single increment up or down by one step
     */
    void incrementCtrl( bool aUp );

    /**
     * Update the text control value with the current value,
     * clamping to limits as needed
     */
    void updateTextValue();

    /*
     * Implementation-specific interfaces
     */

    /**
     * Set the text control string value after an increment.
     */
    virtual void setTextCtrl( const wxString& aVal ) = 0;

    /**
     * @return the current string value of the text control
     */
    virtual wxString getCtrlText() const = 0;

private:

    double m_minVal;
    double m_maxVal;

    ///< Current value of the control
    double m_currentValue;

    ///< Precision to display
    int m_precision;

    ///< The function used to determine the step
    STEP_FUNCTION m_stepFunc;
};


/**
 * Class SPIN_INCREMENTING_TEXT_CTRL
 *
 * An incrementable text control, with WX spin buttons for clickable
 * control.
 */
class SPIN_INCREMENTAL_TEXT_CTRL: public INCREMENTAL_TEXT_CTRL
{
public:

    /**
     * Constructor
     *
     * @param aSpinBtn the spin button to control the value
     * @param aTextCtrl the text control that will display the value
     */
    SPIN_INCREMENTAL_TEXT_CTRL( wxSpinButton& aSpinBtn,
                                wxTextCtrl& aTextCtrl );

    ~SPIN_INCREMENTAL_TEXT_CTRL();

protected:

    ///> @copydoc INCREMENTAL_TEXT_CTRL::setTextCtrl()
    void setTextCtrl( const wxString& val ) override;

    ///> @copydoc INCREMENTAL_TEXT_CTRL::getCtrlText()
    wxString getCtrlText() const override;

private:

    void onFocusLoss( wxFocusEvent& aEvent );

    wxSpinButton& m_spinBtn;
    wxTextCtrl& m_textCtrl;
};

#endif /* INCREMENTAL_TEXT_CTRL__H_ */
