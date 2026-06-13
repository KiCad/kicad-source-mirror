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

#ifndef KICAD_DIFF_JOB_OUTPUT_H
#define KICAD_DIFF_JOB_OUTPUT_H

#include <diff_merge/diff_scene.h>
#include <jobs/job_diff_base.h>

#include <gal/color4d.h>
#include <wx/string.h>

#include <functional>


class REPORTER;


namespace KICAD_DIFF
{

struct DOCUMENT_DIFF;


/**
 * Describes how a computed DOCUMENT_DIFF should be emitted by a diff job.
 *
 * The geometry callbacks are invoked only for the PNG/SVG formats, with the
 * per-side theme colour, so JSON/text runs pay nothing for geometry extraction.
 */
struct DIFF_EMIT_OPTIONS
{
    JOB_DIFF_BASE::OUTPUT_FORMAT format = JOB_DIFF_BASE::OUTPUT_FORMAT::JSON;
    wxString                     outputPath;   ///< empty -> stdout (JSON/text only)
    wxString                     labelA;
    wxString                     labelB;

    /// Source document type, propagated onto the scene so the PNG/SVG renderer
    /// sizes its viewport with the matching internal-unit scale. Defaults to
    /// PCB; schematic and symbol-library jobs must set SCH / SYM_LIB.
    DOC_KIND                     docKind = DOC_KIND::PCB;

    std::function<DOCUMENT_GEOMETRY( const KIGFX::COLOR4D& )> referenceGeometry;
    std::function<DOCUMENT_GEOMETRY( const KIGFX::COLOR4D& )> comparisonGeometry;
};


/**
 * Build a DIFF_EMIT_OPTIONS pre-filled from the job's format, resolved output
 * path and the supplied per-side labels. The geometry callbacks are left unset
 * for the caller to assign, since they capture local scene data.
 */
DIFF_EMIT_OPTIONS MakeEmitOptions( const JOB_DIFF_BASE& aJob, const wxString& aLabelA,
                                   const wxString& aLabelB );


/**
 * Map a computed diff onto its CLI exit code -- SUCCESS when empty, otherwise
 * ERR_RC_VIOLATIONS.
 */
int DiffExitCode( const DOCUMENT_DIFF& aResult );


/**
 * Emit a computed DOCUMENT_DIFF in the requested format.
 *
 * PNG/SVG render through the plotter (BuildScene + the geometry supplied lazily
 * by the option callbacks); JSON and text go through WriteDiffOutput. Lives in
 * the `common` library rather than `kicommon` because the plotter renderers do.
 *
 * Returns @p aDiffExitCode on success, or the matching CLI error code (after
 * reporting through @p aReporter) when a render or file write fails.
 */
int EmitDiffResult( const DOCUMENT_DIFF& aResult, const DIFF_EMIT_OPTIONS& aOptions,
                    int aDiffExitCode, REPORTER& aReporter );

} // namespace KICAD_DIFF

#endif // KICAD_DIFF_JOB_OUTPUT_H
