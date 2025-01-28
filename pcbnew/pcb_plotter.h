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

#include <pcb_plot_params.h>

class BOARD;
class REPORTER;
class wxFileName;
class JOB_EXPORT_PCB_PLOT;

class PCB_PLOTTER
{
public:
    PCB_PLOTTER( BOARD* aBoard, REPORTER* aReporter, PCB_PLOT_PARAMS& aParams );

    bool Plot( const wxString& aOutputPath, const LSEQ& aLayersToPlot, const LSEQ& aCommonLayers,
               bool aUseGerberFileExtensions,
                bool aOutputPathIsSingle = false,
                std::optional<wxString> aLayerName = std::nullopt,
                std::optional<wxString> aSheetName = std::nullopt,
                std::optional<wxString> aSheetPath = std::nullopt );

    /**
     * All copper layers that are disabled are actually selected
     * This is due to wonkyness in automatically selecting copper layers
     * for plotting when adding more than two layers to a board.
     * If plot options become accessible to the layers setup dialog
     * please move this functionality there!
     * This skips a copper layer if it is actually disabled on the board.
     */
    bool copperLayerShouldBeSkipped( PCB_LAYER_ID aLayerToPlot );

    /**
     * Complete a plot filename.
     *
     * It forces the output directory, adds a suffix to the name, and sets the specified extension.
     * The suffix is usually the layer name and replaces illegal file name character in the suffix
     * with an underscore character.
     *
     * @param aFilename is the file name to initialize that contains the base filename.
     * @param aOutputDir is the path.
     * @param aSuffix is the suffix to add to the base filename.
     * @param aExtension is the file extension.
     */
    static void BuildPlotFileName( wxFileName* aFilename, const wxString& aOutputDir, const wxString& aSuffix,
                                   const wxString& aExtension );

    /**
     * Translate a JOB to PCB_PLOT_PARAMS
     */
    static void PlotJobToPlotOpts( PCB_PLOT_PARAMS& aOpts, JOB_EXPORT_PCB_PLOT* aJob,
                                   REPORTER& aReporter );

protected:
    BOARD*          m_board;
    PCB_PLOT_PARAMS m_plotOpts;
    REPORTER*       m_reporter;

private:
    /**
     * Generates a final LSEQ for plotting by removing duplicates
     */
    LSEQ getPlotSequence( PCB_LAYER_ID aLayerToPlot, LSEQ aPlotWithAllLayersSeq );

};
