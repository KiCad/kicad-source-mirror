/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 KiCad Developers, see change_log.txt for contributors.
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

#ifndef SCINTILLA_TRICKS_H
#define SCINTILLA_TRICKS_H


#include <wx/stc/stc.h>
#include <functional>

/**
 * Add cut/copy/paste, autocomplete and brace highlighting to a wxStyleTextCtrl instance.
 */
class SCINTILLA_TRICKS : public wxEvtHandler
{
public:

    SCINTILLA_TRICKS( wxStyledTextCtrl* aScintilla, const wxString& aBraces, bool aSingleLine,
                      std::function<void()> m_enterCallback = [](){ } );

    void DoAutocomplete( const wxString& aPartial, const wxArrayString& aTokens );

    void CancelAutocomplete();

protected:
    void setupStyles();

    int firstNonWhitespace( int aLine, int* aWhitespaceCount = nullptr );

    void onCharHook( wxKeyEvent& aEvent );
    void onScintillaUpdateUI( wxStyledTextEvent& aEvent );
    void onThemeChanged( wxSysColourChangedEvent &aEvent );

protected:
    wxStyledTextCtrl*     m_te;
    wxString              m_braces;
    int                   m_lastCaretPos;
    bool                  m_suppressAutocomplete;
    bool                  m_singleLine;            // Treat <return> as OK, and skip special tab
                                                   //  stop handling (including monospaced font).
    std::function<void()> m_returnCallback;        // Process <return> in singleLine, and
                                                   //  <shift> + <return> irrespective.
};

#endif  // SCINTILLA_TRICKS_H
