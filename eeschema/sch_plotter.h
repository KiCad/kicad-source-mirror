/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2018 Jean-Pierre Charras jp.charras at wanadoo.fr
 * Copyright (C) 1992-2010 Lorenzo Marcantonio
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@gmail.com>
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

#ifndef SCH_PLOTTER_H
#define SCH_PLOTTER_H

#include <wx/filename.h>
#include <wx/string.h>
#include <wx/gdicmn.h>
#include <page_info.h>
#include <sch_render_settings.h>
#include <sch_sheet_path.h>
#include <plotters/plotter.h>

class SCH_EDIT_FRAME;
class PLOTTER;
class SCHEMATIC;
class SCH_SCREEN;
using KIGFX::RENDER_SETTINGS;
class PDF_PLOTTER;
class REPORTER;

enum PageFormatReq
{
    PAGE_SIZE_AUTO,
    PAGE_SIZE_A4,
    PAGE_SIZE_A
};


struct SCH_PLOT_OPTS
{
    bool                  m_plotAll;
    bool                  m_plotDrawingSheet;
    std::vector<wxString> m_plotPages;

    bool           m_plotHopOver;
    bool           m_blackAndWhite;
    int            m_pageSizeSelect;
    bool           m_useBackgroundColor;
    bool           m_PDFPropertyPopups;
    bool           m_PDFHierarchicalLinks;
    bool           m_PDFMetadata;
    wxString       m_theme;

    wxString       m_outputDirectory;
    wxString       m_outputFile;
    wxString       m_variant;

    // has meaning only with DXF plotter: set DXF units in DXF file
    DXF_UNITS      m_DXF_File_Unit;

    SCH_PLOT_OPTS() :
        m_plotAll( true ),
        m_plotDrawingSheet( true ),
        m_plotHopOver( false ),
        m_blackAndWhite( false ),
        m_pageSizeSelect( 0 ),
        m_useBackgroundColor( true ),
        m_PDFPropertyPopups( false ),
        m_PDFHierarchicalLinks( false ),
        m_PDFMetadata( false ),
        m_theme(),
        m_outputDirectory(),
        m_outputFile(),
        m_DXF_File_Unit( DXF_UNITS::INCH )
    {

    }
};


/**
 * Schematic plotting class.
 */
class SCH_PLOTTER
{
public:
    /**
     * Constructor for usage with a frame having the schematic we want to print loaded.
     */
    SCH_PLOTTER( SCH_EDIT_FRAME* aFrame );

    /**
     * Constructor for usage with a schematic that can be headless.
     */
    SCH_PLOTTER( SCHEMATIC* aSch );

    /**
     * Perform the plotting of the schematic using the given \a aPlotFormat and a\ aPlotSettings.
     *
     * @param aPlotFormat The resulting output plot format (PDF, SVG, DXF, etc)
     * @param aPlotSettings The configuration for the plotting operation
     * @param aRenderSettings Mandatory object containing render settings for lower level classes
     * @param aReporter Optional reporter to print messages to
     */
    void Plot( PLOT_FORMAT aPlotFormat, const SCH_PLOT_OPTS& aPlotOpts,
               SCH_RENDER_SETTINGS* aRenderSettings, REPORTER* aReporter = nullptr );

    /**
     * Get the last output file path, this is mainly intended for PDFs with the open after
     * plot GUI option.
     */
    wxString GetLastOutputFilePath() const { return m_lastOutputFilePath; }

protected:
    /**
     * Return the output filename for formats where the output is a single file.
     */
    wxFileName getOutputFilenameSingle( const SCH_PLOT_OPTS& aPlotOpts, REPORTER* aReporter,
                                        const wxString& ext );

    // PDF
    void createPDFFile( const SCH_PLOT_OPTS& aPlotOpts, SCH_RENDER_SETTINGS* aRenderSettings,
                        REPORTER* aReporter );
    void plotOneSheetPDF( PLOTTER* aPlotter, SCH_SCREEN* aScreen, const SCH_PLOT_OPTS& aPlotOpts );
    void setupPlotPagePDF( PLOTTER* aPlotter, SCH_SCREEN* aScreen, const SCH_PLOT_OPTS& aPlotOpts );

    // DXF
    void createDXFFiles( const SCH_PLOT_OPTS& aPlotOpts, SCH_RENDER_SETTINGS* aRenderSettings,
                         REPORTER* aReporter );
    bool plotOneSheetDXF( const wxString& aFileName, SCH_SCREEN* aScreen,
                          RENDER_SETTINGS* aRenderSettings, const VECTOR2I& aPlotOffset,
                          double aScale, const SCH_PLOT_OPTS& aPlotOpts );


    // PS
    void createPSFiles( const SCH_PLOT_OPTS& aPlotOpts, SCH_RENDER_SETTINGS* aRenderSettings,
                        REPORTER* aReporter );
    bool plotOneSheetPS( const wxString& aFileName, SCH_SCREEN* aScreen,
                         RENDER_SETTINGS* aRenderSettings, const PAGE_INFO& aPageInfo,
                         const VECTOR2I& aPlot0ffset, double aScale,
                         const SCH_PLOT_OPTS& aPlotOpts );

    // SVG
    void createSVGFiles( const SCH_PLOT_OPTS& aPlotOpts, SCH_RENDER_SETTINGS* aRenderSettings,
                         REPORTER* aReporter );
    bool plotOneSheetSVG( const wxString& aFileName, SCH_SCREEN* aScreen,
                          RENDER_SETTINGS* aRenderSettings, const SCH_PLOT_OPTS& aPlotOpts );

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
    wxFileName createPlotFileName( const SCH_PLOT_OPTS& aPlotOpts, const wxString& aPlotFileName,
                                   const wxString& aExtension, REPORTER* aReporter = nullptr );

private:
    SCHEMATIC*      m_schematic;
    COLOR_SETTINGS* m_colorSettings;
    wxString        m_lastOutputFilePath;
};

#endif
