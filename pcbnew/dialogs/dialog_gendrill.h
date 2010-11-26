/**
 *@file dialog_gendrill.h
 */

/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 1992-2010 Jean_Pierre Charras <jp.charras@ujf-grenoble.fr>
 * Copyright (C) 1992-2010 Kicad Developers, see change_log.txt for contributors.
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

#ifndef _DIALOG_GENDRILL_H_
#define _DIALOG_GENDRILL_H_

#include "dialog_gendrill_base.h"

class DIALOG_GENDRILL : public DIALOG_GENDRILL_BASE
{
public:
    static int       m_UnitDrillIsInch;
    static int       m_ZerosFormat;
    static int       m_PrecisionFormat;
    static bool      m_MinimalHeader;
    static bool      m_Mirror;
    static bool      m_DrillOriginIsAuxAxis; /* Axis selection (main / auxiliary)
                                              *  for drill origin coordinates */
    DRILL_PRECISION  m_Precision;       // Selected precision for drill files
    wxPoint          m_FileDrillOffset; // Drill offset: 0,0 for absolute coordiantes, or auxialry axis origin

private:
    WinEDA_PcbFrame* m_Parent;
    int m_PadsHoleCount;
    int m_ThroughViasCount;
    int m_MicroViasCount;
    int m_BlindOrBuriedViasCount;

public: DIALOG_GENDRILL( WinEDA_PcbFrame* parent );
    ~DIALOG_GENDRILL();

private:

    // Initialises member variables
    void            initDialog();
    void            InitDisplayParams( void );

    // event functions
    void            OnSelDrillUnitsSelected( wxCommandEvent& event );
    void            OnSelZerosFmtSelected( wxCommandEvent& event );
    void            OnOkClick( wxCommandEvent& event );
    void            OnCancelClick( wxCommandEvent& event );

    // Specific functions:
    void            SetParams( void );
    void            GenDrillAndReportFiles();
    void            GenDrillMap( const wxString           aFileName,
                                 std::vector<HOLE_INFO>&  aHoleListBuffer,
                                 std::vector<DRILL_TOOL>& aToolListBuffer,
                                 int                      format );
    void            UpdatePrecisionOptions();
    void            UpdateConfig();
    void            GenDrillReport( const wxString aFileName );
    int             Create_Drill_File_EXCELLON( FILE*                    aFile,
                                                wxPoint                  aOffset,
                                                std::vector<HOLE_INFO>&  aHoleListBuffer,
                                                std::vector<DRILL_TOOL>& aToolListBuffer );
    int             Gen_Liste_Tools( std::vector<DRILL_TOOL>& buffer, bool print_header );

    /**
     * Return the selected format for coordinates, if not decimal
     */
    DRILL_PRECISION GetPrecison();
};

#endif      // _DIALOG_GENDRILL_H_
