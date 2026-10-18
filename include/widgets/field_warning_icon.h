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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <kicommon.h>
#include <wx/arrstr.h>
#include <wx/generic/statbmpg.h>

class wxEvent;
class wxWindowDestroyEvent;


/**
 * A warning icon drawn inside the right edge of a field, left of its dropdown button when it has
 * one, with the warnings as its tooltip.  The icon is shown only while there is something to warn
 * about.  It marks a combo box, a choice or a text control.
 *
 * The icon is a sibling of the field, since native combo boxes cannot host child windows on every
 * port, and follows the field when it moves or changes size.  Like any child window it belongs to the
 * field's parent, which destroys it.
 */
class KICOMMON_API FIELD_WARNING_ICON : public wxGenericStaticBitmap
{
public:
    /// The most warnings listed in the tooltip, with a count of the rest
    static constexpr size_t MAX_LINES = 10;

    /// @param aField the field to mark, which needs a parent window
    explicit FIELD_WARNING_ICON( wxWindow* aField );
    ~FIELD_WARNING_ICON() override;

    /// Lists aMessages in the tooltip, once each with a count when repeated, and shows the icon, or hides it when
    /// there are none
    void SetWarnings( const wxArrayString& aMessages );

    /// Shows aMessage as the tooltip, or hides the icon when it is empty
    void SetWarning( const wxString& aMessage );

    /// Moves the icon back over the field after the field has moved or changed size
    void Reposition();

private:
    void onFieldChanged( wxEvent& aEvent );
    void onFieldDestroyed( wxWindowDestroyEvent& aEvent );

    wxWindow* m_field; ///< Null once the field has been destroyed
};
