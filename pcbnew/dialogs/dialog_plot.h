/**
 * @file dialog_plot.h
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <board.h>
#include <dialog_plot_base.h>
#include <pcb_plot_params.h>
#include <widgets/unit_binder.h>

// the plot dialog window name, used by wxWidgets
#define DLG_WINDOW_NAME "plot_dialog-window"

/**
 * A dialog to set the plot options and create plot files in various formats.
 */
class DIALOG_PLOT : public DIALOG_PLOT_BASE
{
public:
    DIALOG_PLOT( PCB_EDIT_FRAME* parent );

private:
    // Event called functions
    void        Plot( wxCommandEvent& event ) override;
    void        OnOutputDirectoryBrowseClicked( wxCommandEvent& event ) override;
    void        OnRightClick( wxMouseEvent& event ) override;
    void        OnPopUpLayers( wxCommandEvent& event ) override;
    void        SetPlotFormat( wxCommandEvent& event ) override;
    void        OnChangeDXFPlotMode( wxCommandEvent& event ) override;
    void        OnSetScaleOpt( wxCommandEvent& event ) override;
    void        CreateDrillFile( wxCommandEvent& event ) override;
    void        OnGerberX2Checked( wxCommandEvent& event ) override;
    void        onRunDRC( wxCommandEvent& event ) override;
    void        onBoardSetup( wxHyperlinkEvent& aEvent ) override;

    // other functions
    void        init_Dialog();      // main initialization
    void        reInitDialog();     // initialization after calling drill dialog
    void        applyPlotSettings();
    PLOT_FORMAT getPlotFormat();

    void        setPlotModeChoiceSelection( OUTLINE_MODE aPlotMode )
    {
        m_plotModeOpt->SetSelection( aPlotMode == SKETCH ? 1 : 0 );
    }

    PCB_EDIT_FRAME*     m_parent;
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
    UNIT_BINDER         m_defaultPenSize;
    UNIT_BINDER         m_trackWidthCorrection;

    wxString            m_DRCWarningTemplate;

    PCB_PLOT_PARAMS     m_plotOpts;
};
