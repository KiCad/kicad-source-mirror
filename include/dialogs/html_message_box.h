/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2011-2014 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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

#ifndef HTML_MESSAGE_BOX_H
#define HTML_MESSAGE_BOX_H

#include <dialog_display_html_text_base.h>
#include <vector>

class wxPanel;
class wxStaticText;
class wxTextCtrl;
class wxButton;


class HTML_MESSAGE_BOX : public DIALOG_DISPLAY_HTML_TEXT_BASE
{
public:
    HTML_MESSAGE_BOX( wxWindow* aParent, const wxString& aTitle = wxEmptyString,
                      const wxPoint& aPosition = wxDefaultPosition,
                      const wxSize& aSize = wxDefaultSize );

    ~HTML_MESSAGE_BOX() override;

    /**
     * Set the dialog size, using a "logical" value.
     *
     * The physical size in pixel will depend on the display definition so a value used here
     * should be OK with any display (HDPI for instance).
     *
     * @param aWidth is a "logical" value of the dialog width.
     * @param aHeight is a "logical" value of the dialog height.
     */
    void SetDialogSizeInDU( int aWidth, int aHeight )
    {
        setSizeInDU( aWidth, aHeight );
        Center();
    }

    /**
     * Add a list of items.
     *
     * @param aList is a string containing HTML items. Items are separated by '\n'
     */
    void ListSet( const wxString& aList );

    /**
     * Add a list of items.
     *
     * @param aList is the list of HTML strings to display.
     */
    void ListSet( const wxArrayString& aList );

    void ListClear();

    /**
     * Add a message (in bold) to message list.
     */
    void MessageSet( const wxString& message );

    /**
     * Add HTML text (without any change) to message list.
     */
    void AddHTML_Text( const wxString& message );

    /**
     * Show a modeless version of the dialog (without an OK button).
     */
    void ShowModeless();

    void OnHTMLLinkClicked( wxHtmlLinkEvent& event ) override;


protected:
    void reload();

    void onThemeChanged( wxSysColourChangedEvent &aEvent );
    virtual void OnCharHook( wxKeyEvent& aEvt ) override;
    void OnSearchText( wxCommandEvent& aEvent );
    void OnNext( wxCommandEvent& aEvent );
    void OnPrev( wxCommandEvent& aEvent );
    void ShowSearchBar();
    void HideSearchBar();
    void updateSearch();

private:
    wxString  m_source;
    wxString  m_originalSource;
    wxPanel*  m_searchPanel;
    wxStaticText* m_matchCount;
    wxTextCtrl* m_searchCtrl;
    wxButton* m_prevBtn;
    wxButton* m_nextBtn;
    std::vector<size_t> m_matchPos;
    int      m_currentMatch;
};

#endif // HTML_MESSAGE_BOX_H
