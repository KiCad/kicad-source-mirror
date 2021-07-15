/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file ui_common.h
 * Functions to provide common constants and other functions to assist
 * in making a consistent UI
 */

#ifndef UI_COMMON_H
#define UI_COMMON_H

#include <wx/string.h>
#include <wx/font.h>

class wxSize;
class wxTextCtrl;
class wxTextEntry;
class wxWindow;

namespace KIUI
{

/**
 * Get the standard margin around a widget in the KiCad UI
 * @return margin in pixels
 */
int GetStdMargin();

/**
 * Return the size of @a aSingleLine of text when it is rendered in @a aWindow
 * using whatever font is currently set in that window.
 */
wxSize GetTextSize( const wxString& aSingleLine, wxWindow* aWindow );

wxFont GetMonospacedUIFont();

wxFont GetInfoFont();

/**
 * Set the minimum pixel width on a text control in order to make a text
 * string be fully visible within it.
 *
 * The current font within the text control is considered.  The text can come either from
 * the control or be given as an argument.  If the text control is larger than needed, then
 * nothing is done.
 *
 * @param aCtrl the text control to potentially make wider.
 * @param aString the text that is used in sizing the control's pixel width.
 * If NULL, then
 *   the text already within the control is used.
 * @return true if the \a aCtrl had its size changed, else false.
 */
bool EnsureTextCtrlWidth( wxTextCtrl* aCtrl, const wxString* aString = nullptr );

/**
 * Select the number (or "?") in a reference for ease of editing.
 */
void SelectReferenceNumber( wxTextEntry* aTextEntry );

/**
 * Checks if a input control has focus
 *
 * @param aFocus Control that has focus, if null, wxWidgets will be queried
 */
bool IsInputControlFocused( wxWindow* aFocus = nullptr );

/**
 * Checks if a input control has focus
 *
 * @param aFocus Control that test if editable
 *
 * @return True if control is input and editable OR control is not a input. False if control is
 *         input and not editable.
 */
bool IsInputControlEditable( wxWindow* aControl );

bool IsModalDialogFocused();

}

// Note: On windows, SEVERITY_ERROR collides with a system declaration,
// so we used RPT_SEVERITY _xxx instead of SEVERITY _xxx
enum SEVERITY {
    RPT_SEVERITY_UNDEFINED = 0x00,
    RPT_SEVERITY_INFO      = 0x01,
    RPT_SEVERITY_EXCLUSION = 0x02,
    RPT_SEVERITY_ACTION    = 0x04,
    RPT_SEVERITY_WARNING   = 0x08,
    RPT_SEVERITY_ERROR     = 0x10,
    RPT_SEVERITY_IGNORE    = 0x20
};

SEVERITY SeverityFromString( const wxString& aSeverity );

wxString SeverityToString( const SEVERITY& aSeverity );

#endif // UI_COMMON_H
