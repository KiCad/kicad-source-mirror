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


#ifndef __DIALOG_PLOT_SCHEMATIC__
#define __DIALOG_PLOT_SCHEMATIC__

#include <plotters/plotter.h>
#include <dialog_plot_schematic_base.h>
#include <widgets/unit_binder.h>
#include <sch_plotter.h>

class PDF_PLOTTER;
class SCH_EDIT_FRAME;
class SCH_SCREEN;
class SCH_SHEET_PATH;

class JOB_EXPORT_SCH_PLOT;

class DIALOG_PLOT_SCHEMATIC : public DIALOG_PLOT_SCHEMATIC_BASE
{
public:
    DIALOG_PLOT_SCHEMATIC( SCH_EDIT_FRAME* aEditFrame );
    DIALOG_PLOT_SCHEMATIC( SCH_EDIT_FRAME* aEditFrame, wxWindow* aParent,
                           JOB_EXPORT_SCH_PLOT* aJob = nullptr );

    /**
     * Return true if the project configuration was modified.
     */
    bool PrjConfigChanged() { return m_configChanged; }

private:
    void OnPageSizeSelected( wxCommandEvent& event ) override;
    void OnPlotCurrent( wxCommandEvent& event ) override;
    void OnPlotAll( wxCommandEvent& event ) override;
    void OnUpdateUI( wxUpdateUIEvent& event ) override;

    void initDlg();

    // common
    void getPlotOptions( RENDER_SETTINGS* aSettings );

    bool getModeColor() { return m_ModeColorOption->GetSelection() == 0; }

    void setModeColor( bool aColor ) { m_ModeColorOption->SetSelection( aColor ? 0 : 1 ); }

    COLOR_SETTINGS* getColorSettings();

    /**
     * Set the m_outputDirectoryName variable to the selected directory from directory dialog.
     */
    void onOutputDirectoryBrowseClicked( wxCommandEvent& event ) override;

    PLOT_FORMAT GetPlotFileFormat();

    bool getPlotDrawingSheet() { return m_plotDrawingSheet->GetValue(); }
    void setPlotDrawingSheet( bool aPlot) { m_plotDrawingSheet->SetValue( aPlot ); }

    bool getOpenFileAfterPlot() { return m_openFileAfterPlot->GetValue(); }
    void setOpenFileAfterPlot( bool aOpen ) { m_openFileAfterPlot->SetValue( aOpen ); }

    void plotSchematic( bool aPlotAll );

    /**
     * Determine the best absolute path to plot files given the contents of the path
     * edit control.
     *
     * - If the path edit control results in an absolute path, use it as is.
     * - If the path edit control is not an absolute path and the project file is valid, use
         the project root path to normalize the contents of the path edit control.
     * - If the path edit control is not an absolute path and the project file does not exist
     *   and the screen file name is valid, use the screen file name path.
     * - If the path edit control is not an absolute path and the project file does not exist
     *   and the screen file name is empty, user the user's documents folder.
     * - Fall back to the user's document path if any of the above conditions do not result
     *   in a valid absolute path.
     *
     * @return a valid path to write the plot files.
     */
    wxString getOutputPath();

private:
    SCH_EDIT_FRAME*       m_editFrame;
    bool                  m_configChanged;        // true if a project config param has changed
    PLOT_FORMAT           m_plotFormat;
    static int            m_pageSizeSelect;       // Static to keep last option for some format
    UNIT_BINDER           m_defaultLineWidth;
    JOB_EXPORT_SCH_PLOT*  m_job;
};

#endif    // __DIALOG_PLOT_SCHEMATIC__
