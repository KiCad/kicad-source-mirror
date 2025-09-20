/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2011 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright (C) 2009 Dick Hollenbeck, dick@softplc.com
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


#pragma once

#include <chrono>
#include <wx/htmllbox.h>
#include <rc_item.h>
#include <pcb_marker.h>
#include <board.h>
#include <dialog_drc_base.h>
#include <widgets/progress_reporter_base.h>


class BOARD_DESIGN_SETTINGS;


#define DIALOG_DRC_WINDOW_NAME wxT( "DialogDrcWindowName" )

class
DIALOG_DRC: public DIALOG_DRC_BASE, PROGRESS_REPORTER_BASE
{
public:
    /// Constructors
    DIALOG_DRC( PCB_EDIT_FRAME* aEditorFrame, wxWindow* aParent );
    ~DIALOG_DRC();

    /**
     * Called after running DRC.  It's main function is prevent showing potentially-false-0
     * counts before DRC has been run.
     */
    void SetDrcRun() { m_drcRun = true; }

    /**
     * Called after running Footprint Tests.  It's main function is to update the Footprint
     * Warnings tab title.
     */
    void SetFootprintTestsRun() { m_footprintTestsRun = true; }

    /**
     * Rebuild the contents of the violation tabs based on the current markers and severties.
     */
    void UpdateData();

    void PrevMarker();
    void NextMarker();
    void SelectMarker( const PCB_MARKER* aMarker );

    void ExcludeMarker();

private:
    int getSeverities();
    void updateDisplayedCounts();

    bool TransferDataToWindow() override;

    void OnMenu( wxCommandEvent& aEvent ) override;
    void OnCharHook( wxKeyEvent& aEvt ) override;
    void OnDRCItemSelected( wxDataViewEvent& aEvent ) override;
    void OnDRCItemDClick( wxDataViewEvent& aEvent ) override;
    void OnDRCItemRClick( wxDataViewEvent& aEvent ) override;
    void OnIgnoredItemRClick( wxListEvent& event ) override;
    void OnEditViolationSeverities( wxHyperlinkEvent& aEvent ) override;

    void OnSeverity( wxCommandEvent& aEvent ) override;
  	void OnSaveReport( wxCommandEvent& aEvent ) override;

    void OnDeleteOneClick( wxCommandEvent& aEvent ) override;
    void OnDeleteAllClick( wxCommandEvent& aEvent ) override;
    void OnRunDRCClick( wxCommandEvent& aEvent ) override;

    void OnErrorLinkClicked( wxHtmlLinkEvent& event ) override;

    // These require special handling while the DRC tests are running.
    void OnCancelClick( wxCommandEvent& aEvent ) override;
    void OnClose( wxCloseEvent& event ) override;

    // Updates data which can be modified outside the dialog.
    void OnActivateDlg( wxActivateEvent& aEvent ) override;

    void OnChangingNotebookPage( wxNotebookEvent& aEvent ) override;

    void deleteAllMarkers( bool aIncludeExclusions );
    void refreshEditor();

    // PROGRESS_REPORTER calls
    bool updateUI() override;
    void AdvancePhase( const wxString& aMessage ) override;

    BOARD_DESIGN_SETTINGS& bds() { return m_currentBoard->GetDesignSettings(); }

private:
    BOARD*             m_currentBoard;     // the board currently on test
    PCB_EDIT_FRAME*    m_frame;
    bool               m_running;
    bool               m_drcRun;
    bool               m_footprintTestsRun;

    bool               m_report_all_track_errors;
    bool               m_crossprobe;
    bool               m_scroll_on_crossprobe;

    wxString           m_markersTitleTemplate;
    wxString           m_unconnectedTitleTemplate;
    wxString           m_footprintsTitleTemplate;
    wxString           m_ignoredTitleTemplate;

    std::shared_ptr<RC_ITEMS_PROVIDER> m_markersProvider;
    std::shared_ptr<RC_ITEMS_PROVIDER> m_ratsnestProvider;
    std::shared_ptr<RC_ITEMS_PROVIDER> m_fpWarningsProvider;

    RC_TREE_MODEL*                     m_markersTreeModel;      // wx reference-counted ptr
    RC_TREE_MODEL*                     m_unconnectedTreeModel;  // wx reference-counted ptr
    RC_TREE_MODEL*                     m_fpWarningsTreeModel;   // wx reference-counted ptr

    /// Used to slow down the rate of yields in updateUi()
    std::chrono::steady_clock::time_point m_lastUpdateUi;
};

