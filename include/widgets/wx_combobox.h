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

#ifndef WX_COMBOBOX_H
#define WX_COMBOBOX_H

#include <map>
#include <wx/odcombo.h>

/**
 * Fix some issues with wxCombobox:
 *  - setting the value of a non-read-only combobox doesn't work on MSW
 *  - rollover highlighting in the dropdown doesn't work on OSX
 *  - separators don't work anywhere
 */
class WX_COMBOBOX : public wxOwnerDrawnComboBox
{
public:
    WX_COMBOBOX( wxWindow* aParent, int aId = wxID_ANY, const wxString& aValue = wxEmptyString,
                 const wxPoint& aPos = wxDefaultPosition, const wxSize& aSize = wxDefaultSize,
                 int n = 0, const wxString choices[] = nullptr, long style = 0 );

    virtual ~WX_COMBOBOX();

    void Append( const wxString& aText, const wxString& aMenuText = wxEmptyString );

    int GetCharHeight() const override;

protected:
    virtual void DoSetPopupControl( wxComboPopup* aPopup ) override;
    virtual void OnDrawItem( wxDC& aDC, const wxRect& aRect, int aItem, int aFlags ) const override;
    virtual wxCoord OnMeasureItem( size_t aItem ) const override;
    virtual wxCoord OnMeasureItemWidth( size_t aItem ) const override;

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
    wxString GetMenuText( int aItem ) const;

    /**
     * Get selection from either the outer (combo box) or inner (popup) list.
     */
    int GetSelectionEither( bool aInner ) const;

    /**
     * Safely set selection for either the outer (combo box) or inner (popup) list, doing nothing
     * for invalid selections.
     */
    void SetSelectionEither( bool aInner, int aSel );

private:
    std::map<int, wxString> m_menuText;
    int                     m_lastSelection;
};

#endif // WX_COMBOBOX_H
