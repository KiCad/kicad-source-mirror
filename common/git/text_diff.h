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
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef GIT_TEXT_DIFF_H
#define GIT_TEXT_DIFF_H

#include <string>
#include <vector>

// wxWidgets exports std::vector<wxString> from its own DLL. Without this header MSVC
// exports a second copy from here and the link fails.
#include <wx/arrstr.h>
#include <wx/string.h>

#include <kicommon.h>

namespace KIGIT
{

enum class TEXT_DIFF_KIND
{
    CONTEXT,
    ADDED,
    REMOVED
};

struct KICOMMON_API TEXT_DIFF_LINE
{
    TEXT_DIFF_KIND kind;
    wxString       text;
};

/// Tolerates CRLF endings and content that is not valid UTF-8.
KICOMMON_API std::vector<wxString> SplitTextLines( const std::string& aContent );

KICOMMON_API bool TextLooksBinary( const std::string& aContent );

/// Every line of both sides appears once. There are no hunks and nothing is elided.
KICOMMON_API std::vector<TEXT_DIFF_LINE> BuildTextDiff( const std::vector<wxString>& aOld,
                                                        const std::vector<wxString>& aNew );

} // namespace KIGIT

#endif // GIT_TEXT_DIFF_H
