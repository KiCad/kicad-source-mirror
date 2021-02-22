/** @file dialog_plot_schematic.h
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2018 Jean-Pierre Charras jp.charras at wanadoo.fr
 * Copyright (C) 1992-2010 Lorenzo Marcantonio
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <plotter.h>
#include <sch_screen.h>
#include <sch_edit_frame.h>
#include <dialog_plot_schematic_base.h>
#include <reporter.h>
#include <widgets/unit_binder.h>

enum PageFormatReq {
    PAGE_SIZE_AUTO,
    PAGE_SIZE_A4,
    PAGE_SIZE_A
};

enum class HPGL_PLOT_ORIGIN_AND_UNITS
{
    PLOTTER_BOT_LEFT,
    PLOTTER_CENTER,
    USER_FIT_PAGE,
    USER_FIT_CONTENT,
};

class PDF_PLOTTER;

class DIALOG_PLOT_SCHEMATIC : public DIALOG_PLOT_SCHEMATIC_BASE
{
private:
    SCH_EDIT_FRAME* m_parent;
    bool            m_configChanged; // true if a project config param has changed
    PLOT_FORMAT     m_plotFormat;
    static int      m_pageSizeSelect;       // Static to keep last option for some format
    static int      m_HPGLPaperSizeSelect;  // for HPGL format only: last selected paper size
    double          m_HPGLPenSize;          // for HPGL format only: pen size

    UNIT_BINDER     m_defaultLineWidth;
    UNIT_BINDER     m_penWidth;

public:
    // / Constructors
    DIALOG_PLOT_SCHEMATIC( SCH_EDIT_FRAME* parent );

    bool PrjConfigChanged() { return m_configChanged; } // return true if the prj config was modified
                                                        // and therefore should be saved

private:
    void OnPageSizeSelected( wxCommandEvent& event ) override;
    void OnPlotCurrent( wxCommandEvent& event ) override;
    void OnPlotAll( wxCommandEvent& event ) override;
    void OnUpdateUI( wxUpdateUIEvent& event ) override;

    void    initDlg();

    // common
    void getPlotOptions( RENDER_SETTINGS* aSettings );

    bool getModeColor()
    { return m_ModeColorOption->GetSelection() == 0; }

    void setModeColor( bool aColor )
    { m_ModeColorOption->SetSelection( aColor ? 0 : 1 ); }

    COLOR_SETTINGS* getColorSettings();

    /**
     * Set the m_outputDirectoryName variable to the selected directory from directory dialog.
     */
    void OnOutputDirectoryBrowseClicked( wxCommandEvent& event ) override;

    PLOT_FORMAT GetPlotFileFormat();

    bool getPlotDrawingSheet() { return m_plotDrawingSheet->GetValue(); }
    void setPlotDrawingSheet( bool aPlot) { m_plotDrawingSheet->SetValue( aPlot ); }

    void PlotSchematic( bool aPlotAll );

    // PDF
    void    createPDFFile( bool aPlotAll, bool aPlotDrawingSheet,
                           RENDER_SETTINGS* aRenderSettings );
    void    plotOneSheetPDF( PLOTTER* aPlotter, SCH_SCREEN* aScreen, bool aPlotDrawingSheet);
    void    setupPlotPagePDF( PLOTTER* aPlotter, SCH_SCREEN* aScreen );

    /**
    * Everything done, close the plot and restore the environment
    * @param aPlotter the plotter to close and destroy
    * @param aOldsheetpath the stored old sheet path for the current sheet before the plot started
    */
    void    restoreEnvironment( PDF_PLOTTER* aPlotter, SCH_SHEET_PATH& aOldsheetpath );

    // DXF
    void    CreateDXFFile( bool aPlotAll, bool aPlotDrawingSheet,
                           RENDER_SETTINGS* aRenderSettings );
    bool    PlotOneSheetDXF( const wxString& aFileName, SCH_SCREEN* aScreen,
                             RENDER_SETTINGS* aRenderSettings, wxPoint aPlotOffset, double aScale,
                             bool aPlotFrameRef );

    // HPGL
    HPGL_PLOT_ORIGIN_AND_UNITS GetPlotOriginAndUnits()
    {
        switch( m_plotOriginOpt->GetSelection() )
        {
        case 0:
        default: return HPGL_PLOT_ORIGIN_AND_UNITS::PLOTTER_BOT_LEFT;
        case 1: return HPGL_PLOT_ORIGIN_AND_UNITS::PLOTTER_CENTER;
        case 2: return HPGL_PLOT_ORIGIN_AND_UNITS::USER_FIT_PAGE;
        case 3: return HPGL_PLOT_ORIGIN_AND_UNITS::USER_FIT_CONTENT;
        }
    }

    void SetPlotOriginAndUnits( HPGL_PLOT_ORIGIN_AND_UNITS aOriginAndUnits )
    {
        switch( aOriginAndUnits )
        {
        case HPGL_PLOT_ORIGIN_AND_UNITS::PLOTTER_BOT_LEFT:
        default:                                           m_plotOriginOpt->SetSelection( 0 ); break;
        case HPGL_PLOT_ORIGIN_AND_UNITS::PLOTTER_CENTER:   m_plotOriginOpt->SetSelection( 1 ); break;
        case HPGL_PLOT_ORIGIN_AND_UNITS::USER_FIT_PAGE:    m_plotOriginOpt->SetSelection( 2 ); break;
        case HPGL_PLOT_ORIGIN_AND_UNITS::USER_FIT_CONTENT: m_plotOriginOpt->SetSelection( 3 ); break;
        }
    }

    void    createHPGLFile( bool aPlotAll, bool aPlotFrameRef, RENDER_SETTINGS* aRenderSettings );
    void    SetHPGLPenWidth();
    bool    Plot_1_Page_HPGL( const wxString& aFileName, SCH_SCREEN* aScreen,
                              const PAGE_INFO& aPageInfo, RENDER_SETTINGS* aRenderSettings,
                              wxPoint aPlot0ffset, double aScale, bool aPlotFrameRef,
                              HPGL_PLOT_ORIGIN_AND_UNITS aOriginAndUnits );

    // PS
    void    createPSFile( bool aPlotAll, bool aPlotFrameRef, RENDER_SETTINGS* aSettings );
    bool    plotOneSheetPS( const wxString& aFileName, SCH_SCREEN* aScreen,
                            RENDER_SETTINGS* aRenderSettings, const PAGE_INFO& aPageInfo,
                            wxPoint aPlot0ffset, double aScale, bool aPlotFrameRef );

    // SVG
    void    createSVGFile( bool aPlotAll, bool aPlotFrameRef, RENDER_SETTINGS* aSettings );
    bool    plotOneSheetSVG( const wxString& aFileName, SCH_SCREEN* aScreen,
                             RENDER_SETTINGS* aRenderSettings, bool aPlotBlackAndWhite,
                             bool aPlotFrameRef );

    /**
     * Create a file name with an absolute path name
     * @param aPlotFileName the name for the file to plot without a path
     * @param aExtension the extension for the file to plot
     * @param aReporter a point to a REPORTER object use to show messages (can be NULL)
     * @return the created file name
     * @throw IO_ERROR on file I/O errors
     */
    wxFileName createPlotFileName( const wxString& aPlotFileName, const wxString& aExtension,
                                   REPORTER* aReporter = NULL );
};
