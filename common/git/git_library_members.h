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


/**
 * @file git_library_members.h
 * @brief Which symbols a change to a symbol library file touches.
 *
 * Committing a library file publishes every symbol in it, so a commit names them first.
 */

#ifndef GIT_LIBRARY_MEMBERS_H
#define GIT_LIBRARY_MEMBERS_H

#include <kicommon.h>

#include <wx/string.h>

#include <functional>
#include <string>
#include <vector>

struct git_repository;

namespace KICAD_DIFF
{
struct DOCUMENT_DIFF;
struct ITEM_CHANGE;
}


namespace KIGIT
{

/// Names the symbols two versions of a symbol library file's text differ in, safe off the GUI thread.
/// False when either side does not parse.
using LIBRARY_DIFF = std::function<bool( const std::string& aBefore, const std::string& aAfter,
                                         KICAD_DIFF::DOCUMENT_DIFF* aDiff )>;

/// LIBRARY_DIFF for a library held in several files, each read in its own format
using LIBRARY_FILES_DIFF =
        std::function<bool( const std::vector<std::string>& aBefore, const std::vector<std::string>& aAfter,
                            KICAD_DIFF::DOCUMENT_DIFF* aDiff )>;

/// The symbols the working copy of aPath, a symbol library relative to aRepo, changes against HEAD.
/// Empty when aDiff fails.
KICOMMON_API std::vector<KICAD_DIFF::ITEM_CHANGE> ChangedSymbolsInWorkdir( git_repository* aRepo,
                                                                          const std::string& aPath,
                                                                          const LIBRARY_DIFF& aDiff );

/// The changed symbols' names for a message, such as "LM358, TL072 and 4001", or "LM358 and 6 more" past five.
KICOMMON_API wxString MemberList( const std::vector<KICAD_DIFF::ITEM_CHANGE>& aChanges );

} // namespace KIGIT

#endif // GIT_LIBRARY_MEMBERS_H
