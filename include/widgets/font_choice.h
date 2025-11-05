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

#ifndef FONT_CHOICE_H
#define FONT_CHOICE_H

#include <wx/odcombo.h>
#include <wx/fontenum.h>
#include <font/font.h>


class FONT_CHOICE : public wxOwnerDrawnComboBox
{
public:
    FONT_CHOICE( wxWindow* aParent, int aId, wxPoint aPosition, wxSize aSize, int nChoices,
                 wxString* aChoices, int aStyle );

    virtual ~FONT_CHOICE();

    void RefreshFonts();

    /**
     * Set the selection in wxChoice widget
     * @param aFont is the fotn to select
     * @param aSilentMode allow or not a wxEVT_CHOICE on selection
     * default = false: the event is sent.
     * if true, just make silently the selection
     */
    void SetFontSelection( KIFONT::FONT* aFont, bool aSilentMode = false );

    bool HaveFontSelection() const;

    KIFONT::FONT* GetFontSelection( bool aBold, bool aItalic, bool aForDrawingSheet = false ) const;

    wxString GetStringSelection() const override;

protected:
    wxCoord OnMeasureItem( size_t aItem ) const override;
    void    OnDrawItem( wxDC& aDc, const wxRect& aRect, int aItem, int aFlags ) const override;
    void    OnKeyDown( wxKeyEvent& aEvent );
    void    OnCharHook( wxKeyEvent& aEvent );
    void    OnTextCtrl( wxCommandEvent& aEvent );
    void    OnDropDown( wxCommandEvent& aEvent );
    void    OnCloseUp( wxCommandEvent& aEvent );
    void    OnSetFocus( wxFocusEvent& aEvent );
    void    OnKillFocus( wxFocusEvent& aEvent );

private:
    // Clear the wxOwnerDrawnComboBox list.
    // Same as wxOwnerDrawnComboBox::Clear(), but without issues related to Clear() (see code)
    void clearList();
    void DoAutoComplete( const wxString& aText );
    void FilterFontList( const wxString& aFilter );
    void RestoreFullFontList();
    int FindBestMatch( const wxString& aText );
    wxString FindBestPartialMatch( const wxString& aText );
    wxString GetDefaultFontName() const;

    int                m_systemFontCount;
    wxString           m_notFound;
    wxArrayString      m_fullFontList;
    bool               m_isFiltered;
    wxString           m_lastText;
    wxString           m_originalSelection;  // Store original selection when popup opens
};

#endif // FONT_CHOICE_H
