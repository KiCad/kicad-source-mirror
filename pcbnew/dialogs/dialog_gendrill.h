/**
 *@file dialog_gendrill.h
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2010 Jean_Pierre Charras <jp.charras@ujf-grenoble.fr>
 * Copyright (C) 1992-2010 KiCad Developers, see change_log.txt for contributors.
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

#include <dialog_gendrill_base.h>

class DIALOG_GENDRILL : public DIALOG_GENDRILL_BASE
{
public:
    DIALOG_GENDRILL( PCB_EDIT_FRAME* parent );
    ~DIALOG_GENDRILL();

    static int       m_UnitDrillIsInch;
    static int       m_ZerosFormat;
    static bool      m_MinimalHeader;
    static bool      m_Mirror;
    static bool      m_Merge_PTH_NPTH;
    static bool      m_DrillOriginIsAuxAxis; /* Axis selection (main / auxiliary)
                                              *  for drill origin coordinates */
    DRILL_PRECISION  m_Precision;           // Selected precision for drill files
    wxPoint          m_FileDrillOffset;     // Drill offset: 0,0 for absolute coordinates,
                                            // or origin of the auxiliary axis


private:
    PCB_EDIT_FRAME* m_parent;
    wxConfigBase*       m_config;
    BOARD*          m_board;
    PCB_PLOT_PARAMS m_plotOpts;

    int m_platedPadsHoleCount;
    int m_notplatedPadsHoleCount;
    int m_throughViasCount;
    int m_microViasCount;
    int m_blindOrBuriedViasCount;

    static int m_mapFileType;            // HPGL, PS ...


    void            initDialog();
    void            InitDisplayParams( void );

    // event functions
    void            OnSelDrillUnitsSelected( wxCommandEvent& event );
    void            OnSelZerosFmtSelected( wxCommandEvent& event );
    void            OnGenDrillFile( wxCommandEvent& event );
    void            OnGenMapFile( wxCommandEvent& event );

    /*
     *  Create a plain text report file giving a list of drill values and drill count
     *  for through holes, oblong holes, and for buried vias,
     *  drill values and drill count per layer pair
     */
    void            OnGenReportFile( wxCommandEvent& event );

    void            OnCancelClick( wxCommandEvent& event );
    void            OnOutputDirectoryBrowseClicked( wxCommandEvent& event );

    // Specific functions:
    void            SetParams( void );

    /**
     * Function GenDrillAndMapFiles
     * Calls the functions to create EXCELLON drill files and/or drill map files
     * >When all holes are through holes, only one excellon file is created.
     * >When there are some partial holes (some blind or buried vias),
     *  one excellon file is created, for all plated through holes,
     *  and one file per layer pair, which have one or more holes, excluding
     *  through holes, already in the first file.
     *  one file for all Not Plated through holes
     */
    void            GenDrillAndMapFiles( bool aGenDrill, bool aGenMap );

    void            GenDrillMap( const wxString  aFileName,
                                 EXCELLON_WRITER& aExcellonWriter,
                                 PlotFormat      format );

    void            UpdatePrecisionOptions();
    void            UpdateConfig();
    int             Create_Drill_File_EXCELLON( FILE*  aFile,
                                                wxPoint aOffset );
    int             Gen_Liste_Tools( std::vector<DRILL_TOOL>& buffer,
                                     bool print_header );

    /**
     * Return the selected format for coordinates, if not decimal
     */
    DRILL_PRECISION GetPrecison();
};

#endif      // DIALOG_GENDRILL_H_
