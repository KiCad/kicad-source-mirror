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

#include <diff_merge/diff_job_output.h>

#include <diff_merge/kicad_diff_types.h>
#include <diff_merge/diff_renderer_plotter.h>

#include <cli/exit_codes.h>
#include <eda_units.h>
#include <reporter.h>

#include <wx/translation.h>

#include <string>


namespace KICAD_DIFF
{

DIFF_EMIT_OPTIONS MakeEmitOptions( const JOB_DIFF_BASE& aJob, const wxString& aLabelA,
                                   const wxString& aLabelB )
{
    DIFF_EMIT_OPTIONS opts;
    opts.format     = aJob.m_format;
    opts.outputPath = aJob.GetFullOutputPath( nullptr );
    opts.labelA     = aLabelA;
    opts.labelB     = aLabelB;

    return opts;
}


int DiffExitCode( const DOCUMENT_DIFF& aResult )
{
    return aResult.Empty() ? CLI::EXIT_CODES::SUCCESS : CLI::EXIT_CODES::ERR_RC_VIOLATIONS;
}


int EmitDiffResult( const DOCUMENT_DIFF& aResult, const DIFF_EMIT_OPTIONS& aOptions,
                    int aDiffExitCode, REPORTER& aReporter )
{
    using FORMAT = JOB_DIFF_BASE::OUTPUT_FORMAT;

    // PNG / SVG render through the plotter and write straight to the output
    // path, bypassing the JSON/text string path.
    if( aOptions.format == FORMAT::PNG || aOptions.format == FORMAT::SVG )
    {
        DIFF_COLOR_THEME theme;
        DIFF_SCENE       scene = BuildScene( aResult, theme );
        scene.docKind          = aOptions.docKind;

        if( aOptions.referenceGeometry )
            scene.referenceGeometry = aOptions.referenceGeometry( theme.reference );

        if( aOptions.comparisonGeometry )
            scene.comparisonGeometry = aOptions.comparisonGeometry( theme.comparison );

        PLOTTER_RENDER_OPTIONS opts;
        opts.theme = theme;

        bool ok = ( aOptions.format == FORMAT::PNG )
                          ? RenderSceneToPng( scene, aOptions.outputPath, opts )
                          : RenderSceneToSvg( scene, aOptions.outputPath, opts );

        if( !ok )
        {
            aReporter.Report( _( "Failed to render diff image\n" ), RPT_SEVERITY_ERROR );
            return CLI::EXIT_CODES::ERR_INVALID_OUTPUT_CONFLICT;
        }

        return aDiffExitCode;
    }

    // Text output renders distance/coordinate/angle deltas in user units. Units
    // are not yet configurable on the diff jobs, so default to mm; the IU scale
    // tracks the source document kind (PCB vs schematic) already threaded here.
    std::string output =
            ( aOptions.format == FORMAT::JSON )
                    ? aResult.ToJson().dump( 2 )
                    : FormatDiffAsText( aResult, aOptions.labelA, aOptions.labelB, EDA_UNITS::MM,
                                        IuScaleForDocKind( aOptions.docKind ) );

    if( !WriteDiffOutput( output, aOptions.outputPath ) )
        return CLI::EXIT_CODES::ERR_INVALID_OUTPUT_CONFLICT;

    return aDiffExitCode;
}

} // namespace KICAD_DIFF
