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

#ifndef KICAD_DIFF_MERGE_DISPATCH_H
#define KICAD_DIFF_MERGE_DISPATCH_H

#include <kicommon.h>

#include <diff_merge/diff_doc_kind.h>

#include <wx/string.h>


class KIWAY;
class REPORTER;
class wxWindow;


namespace KICAD_DIFF
{

/**
 * Run a 3-way document/library merge by calling the owning kiface's
 * KIFACE_MERGE_DOCUMENT function export (chosen from @p aKind). This is the
 * non-job bridge used by the CLI merge subcommands and the mergetool frame.
 *
 * @return the kiface's CLI exit code, or ERR_UNKNOWN if the kiface or its
 *         export is unavailable.
 */
KICOMMON_API int DispatchMerge( KIWAY& aKiway, DOC_KIND aKind, const wxString& aAncestor,
                                const wxString& aOurs, const wxString& aTheirs,
                                const wxString& aOutput, bool aInteractive, bool aSingleFile,
                                REPORTER* aReporter );


/**
 * Open DIALOG_KICAD_DIFF on two files by calling the owning kiface's
 * KIFACE_OPEN_DIFF_DIALOG function export (chosen from @p aKind). Non-job
 * bridge used by the MR-review dialog.
 *
 * @return the kiface's exit code, or ERR_UNKNOWN if unavailable.
 */
KICOMMON_API int DispatchOpenDiffDialog( KIWAY& aKiway, DOC_KIND aKind, const wxString& aFileA,
                                         const wxString& aFileB, const wxString& aLabelA,
                                         const wxString& aLabelB, wxWindow* aParent,
                                         REPORTER* aReporter = nullptr );

} // namespace KICAD_DIFF

#endif // KICAD_DIFF_MERGE_DISPATCH_H
