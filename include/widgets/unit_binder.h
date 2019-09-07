/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014-2015 CERN
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

#ifndef __UNIT_BINDER_H_
#define __UNIT_BINDER_H_

#include <common.h>
#include <base_units.h>
#include <base_struct.h>
#include <libeval/numeric_evaluator.h>


class wxTextEntry;
class wxSpinButton;
class wxStaticText;


class UNIT_BINDER : public wxEvtHandler
{
public:

    /**
     * Constructor.
     * @param aParent is the parent EDA_DRAW_FRAME.
     * @param aLabel is the static text used to label the text input widget (note: the label
     *               text, trimmed of its colon, will also be used in error messages)
     * @param aValue is the control used to edit or display the given value (wxTextCtrl,
     *               wxComboBox, wxStaticText, etc.).
     * @param aUnitLabel is the units label displayed after the text input widget
     * @param aUseMils specifies the use of mils for imperial units (instead of inches)
     * @param aAllowEval indicates \a aTextInput's content should be eval'ed before storing
     */
    UNIT_BINDER( EDA_DRAW_FRAME* aParent,
                 wxStaticText* aLabel, wxWindow* aValue, wxStaticText* aUnitLabel,
                 bool aUseMils = false, bool aAllowEval = true );

    /**
     * Function SetUnits
     * Normally not needed (as the UNIT_BINDER inherits from the parent frame), but can be
     * used to set to DEGREES for angular controls.
     */
    virtual void SetUnits( EDA_UNITS_T aUnits, bool aUseMils = false );

    /**
     * Function SetValue
     * Sets new value (in Internal Units) for the text field, taking care of units conversion.
     * @param aValue is the new value.
     */
    virtual void SetValue( int aValue );

    void SetValue( wxString aValue );

    /**
     * Function ChangeValue
     * Changes the value (in Internal Units) for the text field, taking care of units conversion
     * but does not trigger the update routine
     * @param aValue is the new value.
     */
    virtual void ChangeValue( int aValue );

    void ChangeValue( wxString aValue );

    /**
     * Function GetValue
     * Returns the current value in Internal Units.
     */
    virtual long long int GetValue();

    /**
     * Function IsIndeterminate
     * Returns true if the control holds the indeterminate value (for instance, if it
     * represents a multiple selection of differing values).
     */
    bool IsIndeterminate() const;

    /**
     * Function Validate
     * Validates the control against the given range, informing the user of any errors found.
     *
     * @param aMin a minimum value (in internal units) for validation
     * @param aMax a maximum value (in internal units) for validation
     * @return false on error.
     */
    virtual bool Validate( long long int aMin, long long int aMax, bool setFocusOnError = true );

    void SetLabel( const wxString& aLabel );

    /**
     * Function Enable
     * Enables/diasables the label, widget and units label.
     */
    void Enable( bool aEnable );

    /**
     * Function Show
     * Shows/hides the label, widget and units label.
     *
     * @param aShow called for the Show() routine in wx
     * @param aResize if true, the element will be sized to 0 on hide and -1 on show
     */
    void Show( bool aShow, bool aResize = false );

protected:

    void onSetFocus( wxFocusEvent& aEvent );
    void onKillFocus( wxFocusEvent& aEvent );
    void delayedFocusHandler( wxCommandEvent& aEvent );

    ///> The bound widgets
    wxStaticText*     m_label;
    wxWindow*         m_value;
    wxStaticText*     m_unitLabel;

    ///> Currently used units.
    EDA_UNITS_T       m_units;
    bool              m_useMils;

    ///> Validation support.
    wxString          m_errorMessage;

    ///> Evaluator
    NUMERIC_EVALUATOR m_eval;
    bool              m_allowEval;
    bool              m_needsEval;

    ///> Selection start and end of the original text
    long              m_selStart;
    long              m_selEnd;
};

#endif /* __UNIT_BINDER_H_ */
