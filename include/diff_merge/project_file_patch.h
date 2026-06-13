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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef KICAD_DIFF_PROJECT_FILE_PATCH_H
#define KICAD_DIFF_PROJECT_FILE_PATCH_H

#include <kicommon.h>
#include <diff_merge/diff_doc_kind.h>

#include <nlohmann/json_fwd.hpp>
#include <wx/string.h>

#include <optional>
#include <set>
#include <string>


namespace KICAD_DIFF
{

/**
 * Return the JSON pointer (RFC 6901, slash-separated) under which @p aDocProp
 * is persisted in a `.kicad_pro` project file.
 *
 * Returns an empty optional for DOC_PROP values that do NOT live in
 * `.kicad_pro` -- those go to sibling files (.kicad_dru, fp-lib-table,
 * sym-lib-table) or to the document file (.kicad_pcb / .kicad_sch).  The
 * caller is responsible for routing those separately.
 *
 * Used by the 3-way merge handler to surgically patch only the diffed
 * fields of the output `.kicad_pro`, preserving any non-diffed user
 * customisations the existing output file may carry (text variables,
 * private layouts, last-paths, etc.).  The previous full-copy approach
 * silently overwrote those fields with ancestor's values.
 */
KICOMMON_API std::optional<std::string> DocPropJsonPointer( const wxString& aDocProp,
                                                            DOC_KIND        aKind );

KICOMMON_API std::optional<std::string> DocPropJsonPointer( const wxString& aDocProp );


/**
 * Copy the JSON sub-tree located at the pointer for @p aDocProp from
 * @p aSource into @p aTarget.
 *
 * @return true iff @p aDocProp has a json-pointer mapping AND @p aSource
 *         contains the sub-tree.  @p aTarget is left untouched on false.
 *
 * This is the elemental decision-table entry that the handler iterates over.
 * Pure function (no I/O), so each DOC_PROP mapping is unit-testable in
 * isolation.
 */
KICOMMON_API bool ApplyProjectFilePatch( nlohmann::json&       aTarget,
                                         const nlohmann::json& aSource,
                                         const wxString&       aDocProp,
                                         DOC_KIND              aKind );

KICOMMON_API bool ApplyProjectFilePatch( nlohmann::json&       aTarget,
                                         const nlohmann::json& aSource,
                                         const wxString&       aDocProp );


/**
 * Higher-level orchestrator: load the existing @p aOutputProPath as JSON (or
 * start from @p aSource if the file doesn't exist), apply each DOC_PROP
 * patch from @p aSource via ApplyProjectFilePatch, and write the result
 * back to @p aOutputProPath.
 *
 * Empty @p aDocProps is a no-op and does not create @p aOutputProPath.
 *
 * @return true on success; false if the output file existed but couldn't be
 *         parsed, if any requested field is missing from @p aSource, or if
 *         the final write failed.
 *
 * This is the path the merge handler should use in place of a full
 * SaveProjectCopy when the goal is to surgically patch the diffed fields.
 */
KICOMMON_API bool ApplyProjectFilePatches( const wxString&            aOutputProPath,
                                           const nlohmann::json&      aSource,
                                           const std::set<wxString>&  aDocProps,
                                           DOC_KIND                   aKind );

KICOMMON_API bool ApplyProjectFilePatches( const wxString&            aOutputProPath,
                                           const nlohmann::json&      aSource,
                                           const std::set<wxString>&  aDocProps );

} // namespace KICAD_DIFF

#endif // KICAD_DIFF_PROJECT_FILE_PATCH_H
