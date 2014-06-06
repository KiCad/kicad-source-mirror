/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2011 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright (C) 2009 Dick Hollenbeck, dick@softplc.com
 * Copyright (C) 2004-2012 KiCad Developers, see change_log.txt for contributors.
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


#ifndef _DIALOG_DRC_H_
#define _DIALOG_DRC_H_

#include <wx/htmllbox.h>

#include <fctsys.h>
#include <pcbnew.h>
#include <class_drawpanel.h>
#include <wxstruct.h>
#include <drc_stuff.h>
#include <class_marker_pcb.h>
#include <class_board.h>

#include <dialog_drc_base.h>
#include <dialog_drclistbox.h>


// forward declarations
class DRCLISTBOX;
class BOARD_DESIGN_SETTINGS;

//end forward declarations

/*!
 * DrcDialog class declaration
 */

class DIALOG_DRC_CONTROL: public DIALOG_DRC_CONTROL_BASE
{
public:
    BOARD_DESIGN_SETTINGS  m_BrdSettings;

    /// Constructors
    DIALOG_DRC_CONTROL( DRC* aTester, PCB_EDIT_FRAME* parent );
    ~DIALOG_DRC_CONTROL(){};

private:
    /**
     * Function writeReport
     * outputs the MARKER items and unconnecte DRC_ITEMs with commentary to an
     * open text file.
     * @param fpOut The text file to write the report to.
     */
    void writeReport( FILE* fpOut );

    void InitValues( );

    void SetDrcParmeters( );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX
    void OnReportCheckBoxClicked( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_BROWSE_RPT_FILE
    void OnButtonBrowseRptFileClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_STARTDRC
    void OnStartdrcClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_LIST_UNCONNECTED
    void OnListUnconnectedClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_DELETE_ALL
    void OnDeleteAllClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_DELETE_ONE
    void OnDeleteOneClick( wxCommandEvent& event );

    /// wxEVT_LEFT_DCLICK event handler for ID_CLEARANCE_LIST
    void OnLeftDClickClearance( wxMouseEvent& event );

    /// wxEVT_RIGHT_UP event handler for ID_CLEARANCE_LIST
    void OnRightUpClearance( wxMouseEvent& event );

    /// wxEVT_LEFT_DCLICK event handler for ID_UNCONNECTED_LIST
    void OnLeftDClickUnconnected( wxMouseEvent& event );

    /// wxEVT_RIGHT_UP event handler for ID_UNCONNECTED_LIST
    void OnRightUpUnconnected( wxMouseEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
    void OnCancelClick( wxCommandEvent& event );

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_OK
    void OnOkClick( wxCommandEvent& event );

    void OnMarkerSelectionEvent( wxCommandEvent& event );
    void OnUnconnectedSelectionEvent( wxCommandEvent& event );
	void OnChangingMarkerList( wxNotebookEvent& event );

    void DelDRCMarkers();
    void RedrawDrawPanel();

    void OnPopupMenu( wxCommandEvent& event );

    DRC*                m_tester;
    PCB_EDIT_FRAME*     m_Parent;
};

#endif  // _DIALOG_DRC_H_

