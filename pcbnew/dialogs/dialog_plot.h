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

#include <board.h>
#include <dialog_plot_base.h>
#include <pcb_plot_params.h>
#include <widgets/unit_binder.h>

// the plot dialog window name, used by wxWidgets
#define DLG_WINDOW_NAME wxT( "plot_dialog-window" )

class wxRearrangeList;
class wxBitmapButton;

class JOB_EXPORT_PCB_PLOT;

/**
 * A dialog to set the plot options and create plot files in various formats.
 */
class DIALOG_PLOT : public DIALOG_PLOT_BASE
{
public:
    DIALOG_PLOT( PCB_EDIT_FRAME* aEditFrame );
    DIALOG_PLOT( PCB_EDIT_FRAME* aEditFrame, wxWindow* aParent, JOB_EXPORT_PCB_PLOT* aJob = nullptr );

    virtual ~DIALOG_PLOT();

    bool TransferDataToWindow() override;

private:
    // Event called functions
    void Plot( wxCommandEvent& event ) override;
    void onOutputDirectoryBrowseClicked( wxCommandEvent& event ) override;
    void OnRightClickLayers( wxMouseEvent& event );
    void OnRightClickAllLayers( wxMouseEvent& event );
    void SetPlotFormat( wxCommandEvent& event ) override;
    void OnChangeDXFPlotMode( wxCommandEvent& event ) override;
    void CreateDrillFile( wxCommandEvent& event ) override;
    void OnGerberX2Checked( wxCommandEvent& event ) override;
    void onRunDRC( wxCommandEvent& event ) override;
    void onOpenOutputDirectory( wxCommandEvent& event ) override;
    void onBoardSetup( wxHyperlinkEvent& aEvent ) override;

    void onPlotAllListMoveUp( wxCommandEvent& aEvent );
    void onPlotAllListMoveDown( wxCommandEvent& aEvent );

    void onDNPCheckbox( wxCommandEvent& event ) override;
    void onSketchPads( wxCommandEvent& event ) override;
    void onPDFColorChoice( wxCommandEvent& event ) override;

    // other functions
    void reInitDialog();     // initialization after calling drill dialog
    void applyPlotSettings();
    PLOT_FORMAT getPlotFormat();

    void arrangeAllLayersList( const LSEQ& aSeq );
    void transferPlotParamsToJob();
    void updatePdfColorOptions();

private:
    PCB_EDIT_FRAME*     m_editFrame;
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
    UNIT_BINDER         m_trackWidthCorrection;

    wxString            m_DRCWarningTemplate;

    PCB_PLOT_PARAMS     m_plotOpts;

    wxRearrangeList*    m_plotAllLayersList;

    STD_BITMAP_BUTTON*  m_bpMoveUp;
    STD_BITMAP_BUTTON*  m_bpMoveDown;

    JOB_EXPORT_PCB_PLOT* m_job;

    /// The plot on all layers ordering the last time the dialog was opened.
    static LSEQ         s_lastAllLayersOrder;
};
