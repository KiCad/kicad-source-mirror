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

/// The most bytes both sides of an inline drawing preview may hold. It loads, diffs and lays out
/// both on the UI thread, which takes about a second at this size in a release build.
constexpr unsigned long long DRAWING_PREVIEW_LIMIT = 12ULL * 1024 * 1024;

/// The most bytes of a schematic's project file the preview check reads for its top-level sheets.
/// Project files run to tens of KB, so a larger one is treated as a drawing too large to preview.
constexpr unsigned long long PROJECT_PREVIEW_LIMIT = 1024 * 1024;

/// True when the files at @p aBefore and @p aAfter together exceed DRAWING_PREVIEW_LIMIT. An empty
/// path is a side with no file. With @p aSchematic each side also counts every sheet file its
/// hierarchy reaches, from the path the loader opens, starting from all of its project's top-level
/// sheets when the file is one of them. Any doubt counts as too large: a file that cannot be sized
/// or resolved, one outside the side's materialized tree (@p aBeforeRoot or @p aAfterRoot, or the
/// file's own folder when that is empty), a sheet name with a variable to expand, an oversized
/// project, or a read past the budget. A sheet that does not exist costs nothing, as the loader opens
/// nothing for it. Reads the disk, so call it off the UI thread.
KICOMMON_API bool DrawingTooLargeToPreview( const wxString& aBefore, const wxString& aAfter, bool aSchematic,
                                            const wxString& aBeforeRoot = wxEmptyString,
                                            const wxString& aAfterRoot = wxEmptyString );

} // namespace KIGIT

#endif // GIT_TEXT_DIFF_H
