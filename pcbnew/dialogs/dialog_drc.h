/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2011 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright (C) 2009 Dick Hollenbeck, dick@softplc.com
 * Copyright (C) 2004-2019 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <tools/drc.h>
#include <class_marker_pcb.h>
#include <class_board.h>
#include <dialog_drc_base.h>
#include <dialog_drclistbox.h>
#include <widgets/unit_binder.h>

// forward declarations
class DRCLISTBOX;
class BOARD_DESIGN_SETTINGS;

//end forward declarations

/*!
 * DrcDialog class declaration
 */
#define DIALOG_DRC_WINDOW_NAME "DialogDrcWindowName"

class DIALOG_DRC_CONTROL: public DIALOG_DRC_CONTROL_BASE
{
public:
    BOARD_DESIGN_SETTINGS  m_BrdSettings;

    /// Constructors
    DIALOG_DRC_CONTROL( DRC* aTester, PCB_EDIT_FRAME* aEditorFrame, wxWindow* aParent );
    ~DIALOG_DRC_CONTROL();

    /**
     * Enable/disable the report file creation
     * @param aEnable = true to ask for creation
     * @param aFileName = the filename or the report file
     */
    void SetRptSettings( bool aEnable, const wxString& aFileName );

    void GetRptSettings( bool* aEnable, wxString& aFileName );

    void UpdateDisplayedCounts();


private:
    /**
     * Function writeReport
     * outputs the MARKER items and unconnecte DRC_ITEMs with commentary to an
     * open text file.
     * @param aFullFileName The text filename to write the report to.
     * @return true if OK, false on error
     */
    bool writeReport( const wxString& aFullFileName );

    /**
     * filenames can be entered by name.
     * @return a good report filename  (with .rpt extension) (a full filename)
     * from m_CreateRptCtrl
     */
    const wxString makeValidFileNameReport();

    void InitValues( );

    void DisplayDRCValues( );

    void SetDRCParameters( );

    /// @return the selection on a right click on a DRCLISTBOX
    /// return wxNOT_FOUND if no selection
    int rightUpClicSelection( DRCLISTBOX* aListBox, wxMouseEvent& event );

    /// wxEVT_COMMAND_CHECKBOX_CLICKED event handler for ID_CHECKBOX_RPT_FILE
    void OnReportCheckBoxClicked( wxCommandEvent& event ) override;

    /// wxEVT_COMMAND_TEXT_UPDATED event handler for m_RptFilenameCtrl
    void OnReportFilenameEdited( wxCommandEvent &event ) override;

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_BUTTON_BROWSE_RPT_FILE
    void OnButtonBrowseRptFileClick( wxCommandEvent& event ) override;

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_STARTDRC
    void OnStartdrcClick( wxCommandEvent& event ) override;

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_DELETE_ALL
    void OnDeleteAllClick( wxCommandEvent& event ) override;

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for ID_DELETE_ONE
    void OnDeleteOneClick( wxCommandEvent& event ) override;

    /// wxEVT_LEFT_DCLICK event handler for ID_CLEARANCE_LIST
    void OnLeftDClickClearance( wxMouseEvent& event ) override;

    /// wxEVT_LEFT_UP event handler for ID_CLEARANCE_LIST
    void OnLeftUpClearance( wxMouseEvent& event ) override;

    /// wxEVT_RIGHT_UP event handler for ID_CLEARANCE_LIST
    void OnRightUpClearance( wxMouseEvent& event ) override;

    /// wxEVT_LEFT_DCLICK event handler for ID_UNCONNECTED_LIST
    void OnLeftDClickUnconnected( wxMouseEvent& event ) override;

    /// wxEVT_LEFT_UP event handler for ID_UNCONNECTED_LIST
    void OnLeftUpUnconnected( wxMouseEvent& event ) override;

    /// wxEVT_RIGHT_UP event handler for ID_UNCONNECTED_LIST
    void OnRightUpUnconnected( wxMouseEvent& event ) override;

    /// wxEVT_LEFT_DCLICK event handler for ID_FOOTPRINTS_LIST
    void OnLeftDClickFootprints( wxMouseEvent& event ) override;

    /// wxEVT_RIGHT_UP event handler for ID_FOOTPRINTS_LIST
    void OnRightUpFootprints( wxMouseEvent& event ) override;

    /// wxEVT_COMMAND_BUTTON_CLICKED event handler for wxID_CANCEL
    void OnCancelClick( wxCommandEvent& event ) override;

    /// handler for activate event, updating data which can be modified outside the dialog
    /// (DRC parameters)
    void OnActivateDlg( wxActivateEvent& event ) override;

    void OnMarkerSelectionEvent( wxCommandEvent& event ) override;
    void OnUnconnectedSelectionEvent( wxCommandEvent& event ) override;
    void OnFootprintsSelectionEvent( wxCommandEvent& event ) override;
    void OnChangingMarkerList( wxNotebookEvent& event ) override;

    void DelDRCMarkers();
    void RedrawDrawPanel();

    /// Run the SELECTION_TOOL's disambiguation menu to highlight the two BOARD_ITEMs
    /// in the DRC_ITEM.
    void doSelectionMenu( const DRC_ITEM* aItem );

    bool focusOnItem( const DRC_ITEM* aItem );

    BOARD*              m_currentBoard;     // the board currently on test
    DRC*                m_tester;
    PCB_EDIT_FRAME*     m_brdEditor;
    wxConfigBase*       m_config;

    wxString            m_markersTitleTemplate;
    wxString            m_unconnectedTitleTemplate;
    wxString            m_footprintsTitleTemplate;

    UNIT_BINDER         m_trackMinWidth;
    UNIT_BINDER         m_viaMinSize;
    UNIT_BINDER         m_uviaMinSize;
};

#endif  // _DIALOG_DRC_H_

