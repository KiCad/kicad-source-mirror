/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
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

#ifndef MDW_COMPARE_DOC_H
#define MDW_COMPARE_DOC_H

#include <diff_merge/diff_doc_kind.h>
#include <kicommon.h>

#include <wx/string.h>

#include <string>


/// What the diff viewer is given for one item, its document kind and the file name both sides are saved under
struct MDW_COMPARE_DOC
{
    KICAD_DIFF::DOC_KIND kind = KICAD_DIFF::DOC_KIND::UNKNOWN;
    wxString             fileName;
};


/**
 * The document aItemId compares as. A part's local file says it, as a library member's id such as
 * passives.kicad_sym#C is no file name. An item with no local file is named by its id.
 */
KICOMMON_API MDW_COMPARE_DOC MdwCompareDocument( const wxString& aItemId, const wxString& aLocalPath );

/// aLibraryText cut down to the symbol aMember, or empty when it holds none of that name
KICOMMON_API std::string MdwSymbolMemberText( const std::string& aLibraryText, const wxString& aMember );

#endif // MDW_COMPARE_DOC_H
