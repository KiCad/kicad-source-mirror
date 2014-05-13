/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014  CERN
 * Author: Maciej Suminski <maciej.suminski@cern.ch>
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

#ifndef WXUNITTEXT_H_
#define WXUNITTEXT_H_

#include <common.h>
#include <wx/spinbutt.h>

namespace boost
{
    template <class T>
    class optional;
}
class wxTextCtrl;
class wxSpinButton;
class wxStaticText;

class WX_UNIT_TEXT : public wxPanel
{
public:
    /**
     * Constructor.
     * @param aParent is the parent window.
     * @param aLabel is the label displayed next to the text input control.
     * @param aValue is the initial value for the control.
     * @param aStep is the step size when using spin buttons.
     */
    WX_UNIT_TEXT( wxWindow* aParent, const wxString& aLabel = _( "Size:" ),
                  double aValue = 0.0, double aStep = 0.1 );

    virtual ~WX_UNIT_TEXT();

    /**
     * Function SetUnits
     * Changes the units used by the control.
     * @param aUnits is the new unit to be used.
     * @param aConvert decides if the current value should be converted to the value in new units
     * or should it stay the same.
     */
    void SetUnits( EDA_UNITS_T aUnits, bool aConvert = false );

    /**
     * Function SetValue
     * Sets new value for the control.
     * @param aValue is the new value.
     */
    virtual void SetValue( double aValue );

    /**
     * Function GetValue
     * Returns the current value using specified units (if currently used units are different, then
     * they are converted first).
     * @param aUnits is the wanted unit.
     */
    //virtual double GetValue( EDA_UNITS_T aUnits ) const;

    /**
     * Function GetValue
     * Returns the current value in currently used units.
     */
    virtual boost::optional<double> GetValue() const;

    /**
     * Function GetUnits
     * Returns currently used units.
     */
    EDA_UNITS_T GetUnits() const
    {
        return m_units;
    }

    /**
     * Function SetStep
     * Sets the difference introduced by a single spin button click.
     * @param aStep is new step size.
     */
    void SetStep( double aStep )
    {
        assert( aStep > 0.0 );

        m_step = aStep;
    }

    /**
     * Function GetStep
     * Returns the difference introduced by a single spin button click.
     */
    double GetStep() const
    {
        return m_step;
    }

protected:
    ///> Spin up button click event handler.
    void onSpinUpEvent( wxSpinEvent& aEvent );

    ///> Spin down button click event handler.
    void onSpinDownEvent( wxSpinEvent& aEvent );

    ///> Label for the input (e.g. "Size:")
    wxStaticText*   m_inputLabel;

    ///> Text input control.
    wxTextCtrl*     m_inputValue;

    ///> Spin buttons for changing the value using mouse.
    wxSpinButton*   m_spinButton;

    ///> Label showing currently used units.
    wxStaticText*   m_unitLabel;

    ///> Currently used units.
    EDA_UNITS_T     m_units;

    ///> Step size (added/subtracted difference if spin buttons are used).
    double          m_step;

    ///> Default value (or non-specified)
    static const wxString DEFAULT_VALUE;
};

#endif /* WXUNITTEXT_H_ */
