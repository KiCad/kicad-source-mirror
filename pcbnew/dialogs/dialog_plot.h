/**
 * @file dialog_plot.h
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2012 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <class_board.h>
#include <dialog_plot_base.h>
#include <pcb_plot_params.h>

/**
 * Class DIALOG_PLOT
 *
 */
class DIALOG_PLOT : public DIALOG_PLOT_BASE
{
public:
    DIALOG_PLOT( PCB_EDIT_FRAME* parent );
private:
    PCB_EDIT_FRAME*     m_parent;
    BOARD*              m_board;
    wxConfigBase*       m_config;
    LSEQ                m_layerList;                // List to hold CheckListBox layer numbers
    double              m_XScaleAdjust;             // X scale factor adjust to compensate
                                                    // plotter X scaling error
    double              m_YScaleAdjust;             // X scale factor adjust to compensate
                                                    // plotter Y scaling error
    int                 m_PSWidthAdjust;            // Global width correction for exact line width
                                                    // in postscript output.
                                                    // this is a correction factor for tracks width
                                                    // when plotted
    int                 m_widthAdjustMinValue;      // Global track width limits
    int                 m_widthAdjustMaxValue;      // tracks width will be "clipped" whenever the
                                                    // m_PSWidthAdjust to these limits.

    PCB_PLOT_PARAMS     m_plotOpts;

    // Event called functions
    void        Init_Dialog();
    void        Plot( wxCommandEvent& event );
    void        OnQuit( wxCommandEvent& event );
    void        OnClose( wxCloseEvent& event );
    void        OnOutputDirectoryBrowseClicked( wxCommandEvent& event );
    void        OnRightClick( wxMouseEvent& event );
    void        OnPopUpLayers( wxCommandEvent& event );
    void        SetPlotFormat( wxCommandEvent& event );
    void        OnSetScaleOpt( wxCommandEvent& event );
    void        CreateDrillFile( wxCommandEvent& event );

    // orther functions
    void        applyPlotSettings();
    PlotFormat  getPlotFormat();

    void        setPlotModeChoiceSelection( EDA_DRAW_MODE_T aPlotMode )
    {
        m_plotModeOpt->SetSelection( aPlotMode == SKETCH ? 1 : 0 );
    }
};
