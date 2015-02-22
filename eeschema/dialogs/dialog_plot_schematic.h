/** @file dialog_plot_schematic.cpp
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2014 Jean-Pierre Charras <jp.charras at wanadoo.fr
 * Copyright (C) 1992-2010 Lorenzo Marcantonio
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@verizon.net>
 *
 * Copyright (C) 1992-2012 KiCad Developers, see change_log.txt for contributors.
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

#include <fctsys.h>
#include <plot_common.h>
#include <class_sch_screen.h>
#include <schframe.h>
#include <dialog_plot_schematic_base.h>
#include <reporter.h>


enum PageFormatReq {
    PAGE_SIZE_AUTO,
    PAGE_SIZE_A4,
    PAGE_SIZE_A
};


class DIALOG_PLOT_SCHEMATIC : public DIALOG_PLOT_SCHEMATIC_BASE
{
private:
    SCH_EDIT_FRAME* m_parent;
    wxConfigBase*   m_config;
    bool            m_configChanged;        // true if a project config param has changed
    static int      m_pageSizeSelect;       // Static to keep last option for some format:
                                            // Static to keep last option:
                                            // use default size or force A or A4 size
    int             m_HPGLPaperSizeSelect;  // for HPGL format only: last selected paper size
    double          m_HPGLPenSize;          // for HPGL format only: pen size

public:
    // / Constructors
    DIALOG_PLOT_SCHEMATIC( SCH_EDIT_FRAME* parent );

    bool PrjConfigChanged() { return m_configChanged; } // return true if the prj config was modified
                                                        // and therefore should be saved

private:
    void OnPlotFormatSelection( wxCommandEvent& event );
    void OnButtonPlotCurrentClick( wxCommandEvent& event );
    void OnButtonPlotAllClick( wxCommandEvent& event );
    void OnButtonCancelClick( wxCommandEvent& event );

    void    initDlg();

    // common
    void getPlotOptions();

    bool getModeColor()
    { return m_ModeColorOption->GetSelection() == 0; }

    void setModeColor( bool aColor )
    { m_ModeColorOption->SetSelection( aColor ? 0 : 1 ); }

    /**
     * Set the m_outputDirectoryName variable to the selected directory from directory dialog.
     */
    void OnOutputDirectoryBrowseClicked( wxCommandEvent& event );

    PlotFormat GetPlotFileFormat();

    bool getPlotFrameRef() { return m_PlotFrameRefOpt->GetValue(); }
    void setPlotFrameRef( bool aPlot) {m_PlotFrameRefOpt->SetValue( aPlot ); }

    void PlotSchematic( bool aPlotAll );

    // PDF
    void    createPDFFile( bool aPlotAll, bool aPlotFrameRef );
    void    plotOneSheetPDF( PLOTTER* aPlotter, SCH_SCREEN* aScreen, bool aPlotFrameRef);
    void    setupPlotPagePDF( PLOTTER* aPlotter, SCH_SCREEN* aScreen );

    /**
    * Everything done, close the plot and restore the environment
    * @param aPlotter the plotter to close and destroy
    * @param aOldsheetpath the stored old sheet path for the current sheet before the plot started
    * @param aMsg the message which is print to the message box
    */
    void    restoreEnvironment( PDF_PLOTTER* aPlotter, SCH_SHEET_PATH  aOldsheetpath,
                                const wxString& aMsg );

    // DXF
    void    CreateDXFFile( bool aPlotAll, bool aPlotFrameRef );
    bool    PlotOneSheetDXF( const wxString& aFileName, SCH_SCREEN* aScreen,
                             wxPoint aPlot0ffset, double aScale, bool aPlotFrameRef );

    // HPGL
    bool    GetPlotOriginCenter()
    {
        return m_plotOriginOpt->GetSelection() == 1;
    }

    void    SetPlotOriginCenter( bool aCenter )
    {
        m_plotOriginOpt->SetSelection( aCenter ? 1 : 0 );
    }

    void    createHPGLFile( bool aPlotAll, bool aPlotFrameRef );
    void    SetHPGLPenWidth();
    bool    Plot_1_Page_HPGL( const wxString& aFileName, SCH_SCREEN* aScreen,
                              const PAGE_INFO& aPageInfo,
                              wxPoint aPlot0ffset, double aScale, bool aPlotFrameRef );

    // PS
    void    createPSFile( bool aPlotAll, bool aPlotFrameRef );
    bool    plotOneSheetPS( const wxString& aFileName, SCH_SCREEN* aScreen,
                            const PAGE_INFO& aPageInfo,
                            wxPoint aPlot0ffset, double aScale, bool aPlotFrameRef );

    // SVG
    void    createSVGFile( bool aPlotAll, bool aPlotFrameRef );

    /**
     * Create a file name with an absolute path name
     * @param aOutputDirectoryName the diretory name to plot,
     *      this can be a relative name of the current project diretory or an absolute directory name.
     * @param aPlotFileName the name for the file to plot without a path
     * @param aExtension the extension for the file to plot
     * @param aReporter a point to a REPORTER object use to show messages (can be NULL)
     * @return the created file name
     * @throw IO_ERROR on file I/O errors
     */
    wxFileName createPlotFileName( wxTextCtrl* aOutputDirectoryName,
                                   wxString& aPlotFileName,
                                   wxString& aExtension, REPORTER* aReporter = NULL );

public:
    // This function is static because it is called by libedit
    // outside a dialog. This is the reason we need aFrame as parameter
    static bool plotOneSheetSVG( EDA_DRAW_FRAME* aFrame, const wxString& aFileName,
                                 SCH_SCREEN* aScreen,
                                 bool aPlotBlackAndWhite, bool aPlotFrameRef );
};
