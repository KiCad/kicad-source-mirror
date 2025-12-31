/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef MARGIN_OFFSET_BINDER_H
#define MARGIN_OFFSET_BINDER_H

#include <optional>
#include <base_units.h>
#include <units_provider.h>
#include <libeval/numeric_evaluator.h>
#include <wx/event.h>

class EDA_DRAW_FRAME;
class wxTextEntry;
class wxStaticText;

/**
 * A specialized binder for combined margin/ratio input fields.
 *
 * This binder handles input expressions that can contain both an absolute
 * offset value and a percentage ratio, such as:
 *   "-2mm + 1%"
 *   "0.5mm - 5%"
 *   "0.5mm"
 *   "-10%"
 *
 * The final clearance is computed as: absolute_value + (ratio * reference_size)
 */
class MARGIN_OFFSET_BINDER : public wxEvtHandler
{
public:
    /**
     * @param aParent is the parent EDA_DRAW_FRAME, used to fetch units and coordinate systems.
     * @param aLabel is the static text used to label the text input widget
     * @param aValueCtrl is the control used to edit the combined value (wxTextCtrl or wxComboBox)
     * @param aUnitLabel (optional) is the units label displayed after the text input widget
     */
    MARGIN_OFFSET_BINDER( EDA_DRAW_FRAME* aParent,
                          wxStaticText* aLabel, wxWindow* aValueCtrl, wxStaticText* aUnitLabel );

    MARGIN_OFFSET_BINDER( UNITS_PROVIDER* aUnitsProvider, wxWindow* aEventSource,
                          wxStaticText* aLabel, wxWindow* aValueCtrl, wxStaticText* aUnitLabel );

    virtual ~MARGIN_OFFSET_BINDER() override;

    /**
     * Set the absolute offset value (in Internal Units).
     * Use std::nullopt to indicate no value is set.
     */
    void SetOffsetValue( std::optional<int> aValue );

    /**
     * Set the ratio value as a fraction (e.g., -0.05 for -5%).
     * Use std::nullopt to indicate no value is set.
     */
    void SetRatioValue( std::optional<double> aRatio );

    /**
     * Get the absolute offset value (in Internal Units).
     * Returns std::nullopt if no offset was specified.
     */
    std::optional<int> GetOffsetValue() const;

    /**
     * Get the ratio value as a fraction (e.g., -0.05 for -5%).
     * Returns std::nullopt if no ratio was specified.
     */
    std::optional<double> GetRatioValue() const;

    /**
     * Return true if the control holds no value (ie: empty string).
     */
    bool IsNull() const;

    /**
     * Enable/disable the label, widget and units label.
     */
    void Enable( bool aEnable );

    /**
     * Show/hide the label, widget and units label.
     */
    void Show( bool aShow, bool aResize = false );

protected:
    void onSetFocus( wxFocusEvent& aEvent );
    void onKillFocus( wxFocusEvent& aEvent );
    void onUnitsChanged( wxCommandEvent& aEvent );

    /**
     * Parse the input string and extract offset and ratio values.
     * @return true if parsing succeeded
     */
    bool parseInput( const wxString& aInput, std::optional<int>& aOffset,
                     std::optional<double>& aRatio ) const;

    /**
     * Format the offset and ratio values into a display string.
     */
    wxString formatValue( std::optional<int> aOffset, std::optional<double> aRatio ) const;

    /**
     * Get the current text from the control.
     */
    wxString getTextValue() const;

    /**
     * Set the text in the control without triggering events.
     */
    void setTextValue( const wxString& aValue );

protected:
    wxStaticText*       m_label;
    wxWindow*           m_valueCtrl;
    wxWindow*           m_eventSource;
    wxStaticText*       m_unitLabel;      ///< Can be nullptr.

    const EDA_IU_SCALE* m_iuScale;
    EDA_UNITS           m_units;

    mutable NUMERIC_EVALUATOR m_eval;

    // Cached parsed values (updated on focus loss)
    mutable std::optional<int>    m_cachedOffset;
    mutable std::optional<double> m_cachedRatio;
    mutable bool                  m_needsParsing;
};

#endif // MARGIN_OFFSET_BINDER_H
