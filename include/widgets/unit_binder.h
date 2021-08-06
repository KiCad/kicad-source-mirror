/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014-2015 CERN
 * Copyright (C) 2020-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <eda_units.h>
#include <base_units.h>
#include <origin_transforms.h>
#include <libeval/numeric_evaluator.h>
#include <wx/event.h>

class EDA_DRAW_FRAME;
class wxTextEntry;
class wxSpinButton;
class wxStaticText;


class UNIT_BINDER : public wxEvtHandler
{
public:

    /**
     * @param aParent is the parent EDA_DRAW_FRAME.
     * @param aLabel is the static text used to label the text input widget (note: the label
     *               text, trimmed of its colon, will also be used in error messages)
     * @param aValueCtrl is the control used to edit or display the given value (wxTextCtrl,
     *               wxComboBox, wxStaticText, etc.).
     * @param aUnitLabel is the units label displayed after the text input widget
     * Can be nullptr.
     * @param aAllowEval indicates \a aTextInput's content should be eval'ed before storing
     */
    UNIT_BINDER( EDA_DRAW_FRAME* aParent,
                 wxStaticText* aLabel, wxWindow* aValueCtrl, wxStaticText* aUnitLabel,
                 bool aAllowEval = true );

    ~UNIT_BINDER() override;

    /**
     * Normally not needed (as the UNIT_BINDER inherits from the parent frame), but can be
     * used to set to DEGREES for angular controls.
     */
    virtual void SetUnits( EDA_UNITS aUnits );

    virtual void SetNegativeZero() { m_negativeZero = true; }

    /**
     * Normally not needed, but can be used to set the precision when using
     * internal units that are floats (not integers) like DEGREES or PERCENT.
     * Not used for integer values in IU
     * @param aLength is the number of digits for mantissa (0 = no truncation)
     * must be <= 6
     */
    virtual void SetPrecision( int aLength );

    /**
     * Used to override the datatype of the displayed property (default is DISTANCE)
     * @param aDataType is the datatype to use for the units text display
     */
    void SetDataType( EDA_DATA_TYPE aDataType );

    /**
     * Set new value (in Internal Units) for the text field, taking care of units conversion.
     */
    virtual void SetValue( int aValue );

    void SetValue( const wxString& aValue );

    /**
     * Set new value (in Internal Units) for the text field, taking care of units conversion.
     *
     * The value will be truncated according to the precision set by SetPrecision() (if not <= 0).
     */
    virtual void SetDoubleValue( double aValue );

    /**
     * Set new value (in Internal Units) for the text field, taking care of units conversion
     * WITHOUT triggering the update routine.
     */
    virtual void ChangeValue( int aValue );

    void ChangeValue( const wxString& aValue );

    /**
     * Set new value (in Internal Units) for the text field, taking care of units conversion
     * WITHOUT triggering the update routine.
     *
     * The value will be truncated according to the precision set by SetPrecision() (if not <= 0).
     */
    virtual void ChangeDoubleValue( double aValue );

    /**
     * Return the current value in Internal Units.
     */
    virtual long long int GetValue();

    /**
     * Return the current value in Internal Units.
     *
     * The returned value will be truncated according to the precision set by
     * SetPrecision() (if not <= 0)
     */
    virtual double GetDoubleValue();

    /**
     * Return true if the control holds the indeterminate value (for instance, if it
     * represents a multiple selection of differing values).
     */
    bool IsIndeterminate() const;

    /**
     * Return the pre-evaluated text (or the current text if evaluation is not supported).
     * Used primarily to remember values between dialog invocations.
     */
    wxString GetOriginalText() const;

    /**
     * Validate the control against the given range, informing the user of any errors found.
     *
     * @param aMin a minimum value for validation
     * @param aMax a maximum value for validation
     * @param aUnits the units of the min/max parameters (use UNSCALED for internal units)
     * @return false on error.
     */
    virtual bool Validate( double aMin, double aMax, EDA_UNITS aUnits = EDA_UNITS::UNSCALED );

    void SetLabel( const wxString& aLabel );

    /**
     * Enable/disable the label, widget and units label.
     */
    void Enable( bool aEnable );

    /**
     * Show/hide the label, widget and units label.
     *
     * @param aShow called for the Show() routine in wx
     * @param aResize if true, the element will be sized to 0 on hide and -1 on show
     */
    void Show( bool aShow, bool aResize = false );

    /**
     * Get the origin transforms coordinate type
     *
     * @returns the origin transforms coordinate type
     */
    ORIGIN_TRANSFORMS::COORD_TYPES_T GetCoordType() const
    {
        return m_coordType;
    }

    /**
     * Set the current origin transform mode
     */
    void SetCoordType( ORIGIN_TRANSFORMS::COORD_TYPES_T aCoordType )
    {
        m_coordType = aCoordType;
    }

protected:

    void onSetFocus( wxFocusEvent& aEvent );
    void onKillFocus( wxFocusEvent& aEvent );
    void delayedFocusHandler( wxCommandEvent& aEvent );

    void onUnitsChanged( wxCommandEvent& aEvent );

    /**
     * When m_precision > 0 truncate the value aValue to show only
     * m_precision digits in mantissa.
     * used in GetDoubleValue to return a rounded value.
     * Mainly for units set to DEGREES.
     *
     * @param aValue is the value to modify.
     * @param aValueUsesUserUnits must be set to true if aValue is a user value,
     * and set to false if aValue is a internal unit value.
     * @return the "rounded" value.
     */
    double setPrecision( double aValue, bool aValueUsesUserUnits );

    EDA_DRAW_FRAME*   m_frame;

    ///< The bound widgets
    wxStaticText*     m_label;
    wxWindow*         m_valueCtrl;
    wxStaticText*     m_unitLabel;      ///< Can be nullptr

    ///< Currently used units.
    EDA_UNITS         m_units;
    bool              m_negativeZero;   ///< Indicates "-0" should be displayed for 0.
    EDA_DATA_TYPE     m_dataType;
    int               m_precision;      ///< 0 to 6

    wxString          m_errorMessage;

    NUMERIC_EVALUATOR m_eval;
    bool              m_allowEval;
    bool              m_needsEval;

    long              m_selStart;       ///< Selection start and end of the original text
    long              m_selEnd;

    /// A reference to an ORIGIN_TRANSFORMS object
    ORIGIN_TRANSFORMS&               m_originTransforms;

    /// Type of coordinate for display origin transforms
    ORIGIN_TRANSFORMS::COORD_TYPES_T m_coordType;
};

#endif /* __UNIT_BINDER_H_ */
