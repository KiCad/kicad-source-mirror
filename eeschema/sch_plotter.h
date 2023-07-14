/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2018 Jean-Pierre Charras jp.charras at wanadoo.fr
 * Copyright (C) 1992-2010 Lorenzo Marcantonio
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 1992-2022 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef SCH_PLOTTER_H
#define SCH_PLOTTER_H

#include <wx/string.h>
#include <wx/gdicmn.h>
#include <page_info.h>
#include <render_settings.h>
#include <sch_sheet_path.h>
#include <plotters/plotter.h>

class SCH_EDIT_FRAME;
class PLOTTER;
class SCHEMATIC;
class SCH_SCREEN;
using KIGFX::RENDER_SETTINGS;
class PDF_PLOTTER;
class REPORTER;

enum class HPGL_PLOT_ORIGIN_AND_UNITS
{
    PLOTTER_BOT_LEFT,
    PLOTTER_CENTER,
    USER_FIT_PAGE,
    USER_FIT_CONTENT,
};


enum PageFormatReq
{
    PAGE_SIZE_AUTO,
    PAGE_SIZE_A4,
    PAGE_SIZE_A
};


enum class HPGL_PAGE_SIZE
{
    DEFAULT = 0,
    SIZE_A5,
    SIZE_A4,
    SIZE_A3,
    SIZE_A2,
    SIZE_A1,
    SIZE_A0,
    SIZE_A,
    SIZE_B,
    SIZE_C,
    SIZE_D,
    SIZE_E,
};


struct SCH_PLOT_SETTINGS
{
    bool         m_plotAll;
    bool         m_plotDrawingSheet;
    bool         m_blackAndWhite;
    int          m_pageSizeSelect;
    bool         m_useBackgroundColor;
    double       m_HPGLPenSize; // for HPGL format only: pen size
    HPGL_PAGE_SIZE m_HPGLPaperSizeSelect;
    wxString     m_theme;

    wxString m_outputDirectory;
    wxString m_outputFile;

    HPGL_PLOT_ORIGIN_AND_UNITS m_HPGLPlotOrigin;

    SCH_PLOT_SETTINGS() :
        m_plotAll( true ),
        m_plotDrawingSheet( true ),
        m_blackAndWhite( false ),
        m_pageSizeSelect( 0 ),
        m_useBackgroundColor( true ),
        m_HPGLPenSize( 1.0 ),
        m_HPGLPaperSizeSelect( HPGL_PAGE_SIZE::DEFAULT ),
        m_theme(),
        m_outputDirectory(),
        m_outputFile(),
        m_HPGLPlotOrigin( HPGL_PLOT_ORIGIN_AND_UNITS::USER_FIT_CONTENT )
    {

    }
};


/**
 * Schematic plotting class
 */
class SCH_PLOTTER
{
public:
    /**
     * Constructor for usage with a frame having the schematic we want to print loaded
     */
    SCH_PLOTTER( SCH_EDIT_FRAME* aFrame );

    /**
     * Constructor for usage with a schematic that can be headless
     */
    SCH_PLOTTER( SCHEMATIC* aSch );

    /**
     * Perform the plotting of the schematic using the given aPlotFormat and aPlotSettings
     *
     * @param aPlotFormat The resulting output plot format (PDF, SVG, DXF, etc)
     * @param aPlotSettings The configuration for the plotting operation
     * @param aRenderSettings Mandatory object containing render settings for lower level classes
     * @param aReporter Optional reporter to print messages to
     */
    void Plot( PLOT_FORMAT aPlotFormat, const SCH_PLOT_SETTINGS& aPlotSettings,
               RENDER_SETTINGS* aRenderSettings, REPORTER* aReporter = nullptr );

    /**
     * Get the last output file path, this is mainly intended for PDFs with the open after plot GUI option
     */
    wxString GetLastOutputFilePath() const { return m_lastOutputFilePath; }

protected:
    /**
     * Returns the output filename for formats where the output is a single file
     */
    wxFileName getOutputFilenameSingle( const SCH_PLOT_SETTINGS& aPlotSettings, REPORTER* aReporter,
                                        const wxString& ext );

    // PDF
    void createPDFFile( const SCH_PLOT_SETTINGS& aPlotSettings, RENDER_SETTINGS* aRenderSettings,
                        REPORTER* aReporter );
    void plotOneSheetPDF( PLOTTER* aPlotter, SCH_SCREEN* aScreen,
                          const SCH_PLOT_SETTINGS& aPlotSettings );
    void setupPlotPagePDF( PLOTTER* aPlotter, SCH_SCREEN* aScreen,
                           const SCH_PLOT_SETTINGS& aPlotSettings );

    // DXF
    void createDXFFiles( const SCH_PLOT_SETTINGS& aPlotSettings, RENDER_SETTINGS* aRenderSettings,
                         REPORTER* aReporter );
    bool plotOneSheetDXF( const wxString& aFileName, SCH_SCREEN* aScreen,
                          RENDER_SETTINGS* aRenderSettings, const VECTOR2I& aPlotOffset,
                          double aScale, const SCH_PLOT_SETTINGS& aPlotSettings );


    // HPGL
    void createHPGLFiles( const SCH_PLOT_SETTINGS& aPlotSettings, RENDER_SETTINGS* aRenderSettings,
                          REPORTER* aReporter );
    bool plotOneSheetHpgl( const wxString& aFileName, SCH_SCREEN* aScreen,
                           const PAGE_INFO& aPageInfo, RENDER_SETTINGS* aRenderSettings,
                           const VECTOR2I& aPlot0ffset, double aScale,
                           const SCH_PLOT_SETTINGS&   aPlotSettings );

    // PS
    void createPSFiles( const SCH_PLOT_SETTINGS& aPlotSettings, RENDER_SETTINGS* aRenderSettings,
                        REPORTER* aReporter );
    bool plotOneSheetPS( const wxString& aFileName, SCH_SCREEN* aScreen,
                         RENDER_SETTINGS* aRenderSettings, const PAGE_INFO& aPageInfo,
                         const VECTOR2I& aPlot0ffset, double aScale,
                         const SCH_PLOT_SETTINGS& aPlotSettings );

    // SVG
    void createSVGFiles( const SCH_PLOT_SETTINGS& aPlotSettings, RENDER_SETTINGS* aRenderSettings,
                         REPORTER* aReporter );
    bool plotOneSheetSVG( const wxString& aFileName, SCH_SCREEN* aScreen,
                          RENDER_SETTINGS*         aRenderSettings,
                          const SCH_PLOT_SETTINGS& aPlotSettings );

    /**
     * Everything done, close the plot and restore the environment.
     *
     * @param aPlotter the plotter to close and destroy (can be null if no current active plotter)
     * @param aOldsheetpath the stored old sheet path for the current sheet before the plot started
     */
    void restoreEnvironment( PDF_PLOTTER* aPlotter, SCH_SHEET_PATH& aOldsheetpath );


    /**
     * Create a file name with an absolute path name.
     *
     * @param aPlotFileName the name for the file to plot without a path.
     * @param aExtension the extension for the file to plot.
     * @param aReporter a point to a REPORTER object use to show messages (can be NULL).
     * @return the created file name.
     * @throw IO_ERROR on file I/O errors.
     */
    wxFileName createPlotFileName( const SCH_PLOT_SETTINGS& aPlotSettings,
                                   const wxString& aPlotFileName, const wxString& aExtension,
                                   REPORTER* aReporter = nullptr );

private:
    SCHEMATIC*      m_schematic;
    COLOR_SETTINGS* m_colorSettings;
    wxString        m_lastOutputFilePath;
};

#endif