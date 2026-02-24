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

#ifndef DRC_RE_BITMAP_OVERLAY_PANEL_H
#define DRC_RE_BITMAP_OVERLAY_PANEL_H

#include <map>
#include <memory>
#include <vector>

#include <wx/bitmap.h>
#include <wx/panel.h>

#include <bitmaps.h>
#include "drc_re_content_panel_base.h"
#include "drc_re_overlay_field.h"
#include "drc_re_overlay_types.h"

class wxControl;
class wxCheckBox;
class wxPaintEvent;
class wxDPIChangedEvent;
class wxSysColourChangedEvent;
class wxFocusEvent;
class UNIT_BINDER;


/**
 * Base class for DRC rule editor panels that display input fields overlaid on a bitmap.
 *
 * This class provides infrastructure for creating constraint editor panels where input fields
 * are precisely positioned over explanatory constraint diagrams. The bitmap serves as a visual
 * guide showing what each field value represents.
 *
 * Derived classes should:
 * - Call SetBackgroundBitmap() in their constructor to set the diagram
 * - Create fields using AddField(), AddFieldWithUnits(), or AddCheckbox()
 * - Override TransferDataToWindow/FromWindow to populate/read field values
 */
class DRC_RE_BITMAP_OVERLAY_PANEL : public wxPanel,
                                    public DRC_RULE_EDITOR_CONTENT_PANEL_BASE
{
public:
    DRC_RE_BITMAP_OVERLAY_PANEL( wxWindow* aParent, wxWindowID aId = wxID_ANY );

    virtual ~DRC_RE_BITMAP_OVERLAY_PANEL();

    /**
     * Set the background bitmap for this panel.
     *
     * @param aBitmap The BITMAPS enum value identifying the constraint diagram.
     */
    void SetBackgroundBitmap( BITMAPS aBitmap );

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    /**
     * Clear error indicators from all fields.
     */
    void ClearFieldErrors();

    /**
     * Show an error indicator on the specified field.
     *
     * @param aFieldId The string identifier of the field to highlight.
     */
    void ShowFieldError( const wxString& aFieldId );

protected:
    /**
     * Create and position a field control on the bitmap overlay.
     *
     * @tparam T The wxWindow-derived control type to create.
     * @param aId String identifier for this field (used for error display and lookup).
     * @param aPosition Position specification in 1x bitmap coordinates.
     * @param aStyle Window style flags passed to the control constructor
     * @return Pointer to the created field wrapper.
     */
    template <typename T>
    DRC_RE_OVERLAY_FIELD* AddField( const wxString& aId, const DRC_RE_FIELD_POSITION& aPosition,
                                    long aStyle = 0 );

    /**
     * Create and position a field control with unit binding.
     *
     * @tparam T The wxWindow-derived control type to create.
     * @param aId String identifier for this field.
     * @param aPosition Position specification in 1x bitmap coordinates.
     * @param aUnitBinder UNIT_BINDER to associate with this field for unit conversion.
     * @param aStyle Window style flags passed to the control constructor
     * @return Pointer to the created field wrapper.
     */
    template <typename T>
    DRC_RE_OVERLAY_FIELD* AddFieldWithUnits( const wxString& aId,
                                              const DRC_RE_FIELD_POSITION& aPosition,
                                              UNIT_BINDER* aUnitBinder,
                                              long aStyle = 0 );

    /**
     * Create and position a checkbox control on the bitmap overlay.
     *
     * @param aId String identifier for this checkbox.
     * @param aPosition Position specification in 1x bitmap coordinates.
     * @return Pointer to the created field wrapper.
     */
    DRC_RE_OVERLAY_FIELD* AddCheckbox( const wxString& aId, const DRC_RE_FIELD_POSITION& aPosition );

    /**
     * Load the appropriate bitmap variant for the current theme and DPI.
     */
    void LoadBitmap();

    /**
     * Position all fields based on the current scale factor.
     */
    void PositionFields();

    /**
     * Apply transparent styling to a field control.
     *
     * @param aControl The control to style.
     */
    void SetupFieldStyling( wxControl* aControl );

private:
    void OnPaint( wxPaintEvent& aEvent );
    void OnDPIChanged( wxDPIChangedEvent& aEvent );
    void OnThemeChange( wxSysColourChangedEvent& aEvent );
    void OnFieldFocus( wxFocusEvent& aEvent );
    void OnFieldBlur( wxFocusEvent& aEvent );

    /**
     * Position a label relative to its field control based on the label position setting.
     *
     * @param aField The overlay field containing the label to position.
     */
    void PositionLabel( DRC_RE_OVERLAY_FIELD* aField );

protected:
    wxBitmap                                        m_bitmap;       ///< Current background bitmap
    BITMAPS                                         m_bitmapId;     ///< BITMAPS enum value
    wxSize                                          m_baseBitmapSize; ///< Bitmap size at 1x scale
    std::vector<std::unique_ptr<DRC_RE_OVERLAY_FIELD>> m_fields;    ///< All overlay fields
    std::map<wxString, DRC_RE_OVERLAY_FIELD*>       m_fieldIdMap;   ///< Field ID to field lookup
};


template <typename T>
DRC_RE_OVERLAY_FIELD* DRC_RE_BITMAP_OVERLAY_PANEL::AddField( const wxString& aId,
                                                             const DRC_RE_FIELD_POSITION& aPosition,
                                                             long aStyle )
{
    // Create the control
    T* control = new T( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, aStyle );

    // Create the overlay field wrapper
    auto field = std::make_unique<DRC_RE_OVERLAY_FIELD>( this, aId, control, aPosition );
    DRC_RE_OVERLAY_FIELD* fieldPtr = field.get();

    // Set up styling
    SetupFieldStyling( control );

    // Position the field
    wxPoint pos( aPosition.xStart, aPosition.yTop );
    int width = aPosition.xEnd - aPosition.xStart;
    wxSize size( width, control->GetBestSize().GetHeight() );
    control->SetPosition( pos );
    control->SetSize( size );

    // Create label if specified
    fieldPtr->CreateLabel();

    if( fieldPtr->HasLabel() )
        PositionLabel( fieldPtr );

    // Store in collections
    m_fieldIdMap[aId] = fieldPtr;
    m_fields.push_back( std::move( field ) );

    return fieldPtr;
}


template <typename T>
DRC_RE_OVERLAY_FIELD* DRC_RE_BITMAP_OVERLAY_PANEL::AddFieldWithUnits( const wxString& aId,
                                                                       const DRC_RE_FIELD_POSITION& aPosition,
                                                                       UNIT_BINDER* aUnitBinder,
                                                                       long aStyle )
{
    // Create the field using the base AddField method
    DRC_RE_OVERLAY_FIELD* field = AddField<T>( aId, aPosition, aStyle );

    // Associate the unit binder for value conversion
    field->SetUnitBinder( aUnitBinder );

    return field;
}

#endif // DRC_RE_BITMAP_OVERLAY_PANEL_H
