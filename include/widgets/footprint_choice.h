/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef FOOTPRINT_CHOICE_H
#define FOOTPRINT_CHOICE_H

#include <wx/odcombo.h>

/**
 * Customized combo box for footprint selection. This provides the following features:
 *
 * - library name is greyed out for readability when lib:footprint format is found in
 *   the item text
 * - empty items are displayed as nonselectable separators
 *
 * Multiple separators in a row is undefined behavior; it is likely to result in errors
 * such as the ability to select separators. Separators ARE valid at the top and bottom.
 *
 * For any items containing footprints, the "lib:footprint" name should be attached to
 * the item as a wxStringClientData.
 */
class FOOTPRINT_CHOICE : public wxOwnerDrawnComboBox
{
public:
    FOOTPRINT_CHOICE( wxWindow* aParent, int aId );

    virtual ~FOOTPRINT_CHOICE();

protected:
    virtual void DoSetPopupControl( wxComboPopup* aPopup ) override;
    virtual void OnDrawItem( wxDC& aDC, const wxRect& aRect, int aItem, int aFlags ) const override;
    virtual wxCoord OnMeasureItem( size_t aItem ) const override;
    virtual wxCoord OnMeasureItemWidth( size_t aItem ) const override;

    /**
     * Draw a fragment of text, then return the next x coordinate to continue drawing.
     */
    static wxCoord DrawTextFragment( wxDC& aDC, wxCoord x, wxCoord y, const wxString& aText );

    /// Veto a mouseover event if in the separator
    void TryVetoMouse( wxMouseEvent& aEvent );

    /**
     * Veto a select event for the separator
     *
     * @param aEvent - the wxCommandEvent caller
     * @param aInner - true if event was called for the inner list (ie the popup)
     */
    void TryVetoSelect( wxCommandEvent& aEvent, bool aInner );

    /**
     * Safely get a string for an item, returning wxEmptyString if the item doesn't exist.
     */
    wxString SafeGetString( int aItem ) const;

    /**
     * Get selection from either the outer (combo box) or inner (popup) list.
     */
    int GetSelectionEither( bool aInner ) const;

    /**
     * Safely set selection for either the outer (combo box) or inner (popup) list, doing nothing
     * for invalid selections.
     */
    void SetSelectionEither( bool aInner, int aSel );

    static wxColour m_grey;

private:
    int m_last_selection;
};

#endif // FOOTPRINT_CHOICE_H
