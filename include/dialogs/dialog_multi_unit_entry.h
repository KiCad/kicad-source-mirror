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

#pragma once

#include <memory>
#include <variant>

#include <dialog_shim.h>

class EDA_DRAW_FRAME;
class UNIT_BINDER;

/**
 * A dialog like @ref WX_UNIT_ENTRY_DIALOG, but with multiple entries.
 *
 * You can give a list of entries, each with a label and a default value.
 * The control type will be chosen based on the type of the default value.
 */
class WX_MULTI_ENTRY_DIALOG : public DIALOG_SHIM
{
public:
    struct UNIT_BOUND
    {
        long long int m_default;
    };

    struct CHECKBOX
    {
        bool m_default;
    };

    /**
     * The type of the entry value
     *
     *  - long long int: for a unit-bound value
     *  - bool:          for a checkbox
     */
    using TYPE = std::variant<UNIT_BOUND, CHECKBOX>;

    struct ENTRY
    {
        wxString m_label;
        TYPE     m_value;
        wxString m_tooltip;
    };

    /**
     * Corresponding result type for each entry type
     */
    using RESULT = std::variant<long long int, bool>;

    /**
     * Create a multi-entry dialog
     *
     * @param aParent  The parent frame
     * @param aCaption The dialog caption (title)
     * @param aEntries The list of entries
     */
    WX_MULTI_ENTRY_DIALOG( EDA_DRAW_FRAME* aParent, const wxString& aCaption, std::vector<ENTRY> aEntries );

    /**
     * Returns the values in the order they were added.
     *
     * The type of the values will be the same as the default value type
     * for the ENTRY that defined this value.
     */
    std::vector<RESULT> GetValues() const;

private:
    std::vector<ENTRY> m_entries;

    std::vector<wxWindow*>                    m_controls;
    std::vector<std::unique_ptr<UNIT_BINDER>> m_unit_binders;
};
