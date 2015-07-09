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

#ifndef __WX_UNIT_BINDER_H_
#define __WX_UNIT_BINDER_H_

#include <common.h>
#include <wx/spinbutt.h>

class wxTextCtrl;
class wxSpinButton;
class wxStaticText;

class WX_UNIT_BINDER
{
public:

    /**
     * Constructor.
     * @param aParent is the parent window.
     * @param aTextInput is the text input widget used to edit the given value.
     * @param aUnitLabel is the units label displayed next to the text field.
     * @param aSpinButton is an optional spin button (for adjusting the input value)
     */
    WX_UNIT_BINDER( wxWindow* aParent, wxTextCtrl* aTextInput, wxStaticText* aUnitLabel, wxSpinButton* aSpinButton = NULL );

    virtual ~WX_UNIT_BINDER();

    /**
     * Function SetValue
     * Sets new value (in Internal Units) for the text field, taking care of units conversion.
     * @param aValue is the new value.
     */
    virtual void SetValue( int aValue );

    /**
     * Function GetValue
     * Returns the current value in Internal Units.
     */
    virtual int GetValue() const;

    /**
     * Function Valid
     * Returns true if the text control contains a real number.
     */
    bool Valid() const;

    /**
     * Function Enable
     * Enables/diasables the binded widgets
     */
    void Enable( bool aEnable );

protected:

    void onTextChanged( wxEvent& aEvent );

    ///> Text input control.
    wxTextCtrl*   m_textCtrl;

    ///> Label showing currently used units.
    wxStaticText* m_unitLabel;

    ///> Currently used units.
    EDA_UNITS_T   m_units;

    ///> Step size (added/subtracted difference if spin buttons are used).
    int m_step;
    int m_min;
    int m_max;

    ///> Default value (or non-specified)
    static const wxString DEFAULT_VALUE;
};

#endif /* __WX_UNIT_BINDER_H_ */
