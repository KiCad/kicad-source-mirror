/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef DRC_RE_OVERLAY_FIELD_H
#define DRC_RE_OVERLAY_FIELD_H

#include <functional>
#include <wx/string.h>

#include "drc_re_overlay_types.h"

class wxWindow;
class wxControl;
class wxStaticBitmap;
class wxStaticText;
class UNIT_BINDER;


/**
 * Wraps a wxControl positioned over a bitmap overlay panel.
 *
 * Manages the control's position within the overlay, data binding via lambda functions,
 * optional UNIT_BINDER integration for unit conversion, and error icon display for
 * validation feedback.
 */
class DRC_RE_OVERLAY_FIELD
{
public:
    using Getter = std::function<double()>;
    using Setter = std::function<void( double )>;

    /**
     * Construct an overlay field wrapping an existing control.
     *
     * @param aParent Parent window (typically the overlay panel)
     * @param aFieldId Unique identifier for this field, used for error mapping
     * @param aControl The wxControl to wrap and position
     * @param aPosition Position specification in 1x bitmap coordinates
     */
    DRC_RE_OVERLAY_FIELD( wxWindow* aParent, const wxString& aFieldId, wxControl* aControl,
                          const DRC_RE_FIELD_POSITION& aPosition );

    ~DRC_RE_OVERLAY_FIELD();

    /**
     * @return The wrapped wxControl.
     */
    wxControl* GetControl() const { return m_control; }

    /**
     * @return The unique field identifier string.
     */
    const wxString& GetFieldId() const { return m_fieldId; }

    /**
     * @return The field's position specification in 1x bitmap coordinates.
     */
    const DRC_RE_FIELD_POSITION& GetPosition() const { return m_position; }

    /**
     * Transfer data from the model to the control.
     *
     * Uses the getter lambda if set. If a UNIT_BINDER is associated, the value is
     * passed through SetDoubleValue for unit conversion.
     *
     * @return true on success, false if getter throws or is not set
     */
    bool TransferToWindow();

    /**
     * Transfer data from the control to the model.
     *
     * Reads the control value (via UNIT_BINDER if associated) and calls the setter lambda.
     *
     * @return true on success, false if setter throws or is not set
     */
    bool TransferFromWindow();

    /**
     * Set the getter lambda for reading model data.
     *
     * @param aGetter Function returning the current model value
     */
    void SetGetter( Getter aGetter ) { m_getter = std::move( aGetter ); }

    /**
     * Set the setter lambda for writing model data.
     *
     * @param aSetter Function accepting the new value from the control
     */
    void SetSetter( Setter aSetter ) { m_setter = std::move( aSetter ); }

    /**
     * Associate a UNIT_BINDER for unit conversion.
     *
     * When set, TransferToWindow and TransferFromWindow use the UNIT_BINDER's
     * SetDoubleValue and GetDoubleValue methods for proper unit handling.
     *
     * @param aBinder The UNIT_BINDER to associate (ownership remains with caller)
     */
    void SetUnitBinder( UNIT_BINDER* aBinder ) { m_unitBinder = aBinder; }

    /**
     * @return The associated UNIT_BINDER, or nullptr if none.
     */
    UNIT_BINDER* GetUnitBinder() const { return m_unitBinder; }

    /**
     * Show or hide the error indicator icon adjacent to this field.
     *
     * @param aShow true to display the error icon, false to hide it
     */
    void ShowError( bool aShow );

    /**
     * @return true if the error icon is currently visible.
     */
    bool IsShowingError() const { return m_showingError; }

    /**
     * @return The label control, or nullptr if no label was created.
     */
    wxStaticText* GetLabel() const { return m_label; }

    /**
     * @return true if this field has a label.
     */
    bool HasLabel() const { return m_label != nullptr; }

    /**
     * Create and associate a label with this field.
     * The label is positioned according to the LABEL_POSITION in the field position struct.
     */
    void CreateLabel();

private:
    void createErrorIcon();

    wxWindow*            m_parent;
    wxString             m_fieldId;
    wxControl*           m_control;
    DRC_RE_FIELD_POSITION m_position;

    Getter               m_getter;
    Setter               m_setter;
    UNIT_BINDER*         m_unitBinder;

    wxStaticBitmap*      m_errorIcon;
    bool                 m_showingError;

    wxStaticText*        m_label;
};

#endif // DRC_RE_OVERLAY_FIELD_H
