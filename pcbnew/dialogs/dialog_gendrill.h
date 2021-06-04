/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2019 Jean_Pierre Charras <jp.charras@ujf-grenoble.fr>
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

#ifndef DIALOG_GENDRILL_H_
#define DIALOG_GENDRILL_H_

#include <gendrill_file_writer_base.h>      // for DRILL_PRECISION definition
#include <dialog_gendrill_base.h>

class DIALOG_GENDRILL : public DIALOG_GENDRILL_BASE
{
public:
    /**
     * @param aPcbEditFrame is the board edit frame.
     * @param aParent is the parent window caller ( the board edit frame or a dialog ).
     */
    DIALOG_GENDRILL( PCB_EDIT_FRAME* aPcbEditFrame, wxWindow* aParent );
    ~DIALOG_GENDRILL();

    /**
     * Update board drill/plot parameters
     */
    void             UpdateDrillParams();

private:
    void initDialog();
    void InitDisplayParams();

    // event functions
    void OnSelDrillUnitsSelected( wxCommandEvent& event ) override;
    void OnSelZerosFmtSelected( wxCommandEvent& event ) override;
    void OnGenDrillFile( wxCommandEvent& event ) override;
    void OnGenMapFile( wxCommandEvent& event ) override;
	void onFileFormatSelection( wxCommandEvent& event ) override;

    // Called when closing the dialog: Update config.
    // This is not done in Dtor, because the dtor call is often delayed and the update
    // could happen too late for the caller.
	void onCloseDlg( wxCloseEvent& event ) override
    {
        UpdateConfig();
        event.Skip();
    }

	void onQuitDlg( wxCommandEvent& event ) override
    {
        UpdateConfig();
        event.Skip();
    }

    /*
     *  Create a plain text report file giving a list of drill values and drill count
     *  for through holes, oblong holes, and for buried vias,
     *  drill values and drill count per layer pair
     */
    void OnGenReportFile( wxCommandEvent& event ) override;

    void OnOutputDirectoryBrowseClicked( wxCommandEvent& event ) override;

    // Specific functions:
    /**
     * Call the functions to create EXCELLON drill files and/or drill map files.
     *
     * When all holes are through holes, only one excellon file is created.  When there are
     * some partial holes (some blind or buried vias), one excellon file is created, for all
     * plated through holes, and one file per layer pair, which have one or more holes, excluding
     * through holes, already in the first file.  One file for all Not Plated through holes.
     */
    void GenDrillAndMapFiles( bool aGenDrill, bool aGenMap );

    void UpdatePrecisionOptions();
    void UpdateConfig();

public:
    static int       m_UnitDrillIsInch;
    static int       m_ZerosFormat;
    static bool      m_MinimalHeader;
    static bool      m_Mirror;
    static bool      m_Merge_PTH_NPTH;
    DRILL_PRECISION  m_Precision;                // Precision for drill files, in non decimal format
    wxPoint          m_FileDrillOffset;          // Drill offset: 0,0 for absolute coordinates,
                                                 // or origin of the auxiliary axis
    static bool      m_UseRouteModeForOvalHoles; // True to use a G00 route command for oval holes
                                                 // False to use a G85 canned mode for oval holes

private:
    PCB_EDIT_FRAME*  m_pcbEditFrame;
    BOARD*           m_board;
    PCB_PLOT_PARAMS  m_plotOpts;
    bool             m_drillOriginIsAuxAxis;     // Axis selection (main / auxiliary)
                                                 // for drill origin coordinates
    int              m_platedPadsHoleCount;
    int              m_notplatedPadsHoleCount;
    int              m_throughViasCount;
    int              m_microViasCount;
    int              m_blindOrBuriedViasCount;

    static int       m_mapFileType;              // format of map file: HPGL, PS ...
    static int       m_drillFileType;            // for Excellon, Gerber
};

#endif      // DIALOG_GENDRILL_H_
