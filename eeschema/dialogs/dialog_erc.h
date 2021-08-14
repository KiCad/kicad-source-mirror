/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007 Jean-Pierre Charras, jp.charras at wanadoo.fr
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

#ifndef _DIALOG_ERC_H_
#define _DIALOG_ERC_H_

#include <wx/htmllbox.h>
#include <vector>
#include <lib_pin.h>        // For PINTYPE_COUNT definition

#include <dialog_erc_base.h>
#include <widgets/progress_reporter_base.h>
#include <erc_settings.h>


class DIALOG_ERC : public DIALOG_ERC_BASE, PROGRESS_REPORTER_BASE
{
public:
    DIALOG_ERC( SCH_EDIT_FRAME* parent );
    ~DIALOG_ERC();

    // PROGRESS_REPORTER_BASE calls
    bool updateUI() override;
    void AdvancePhase( const wxString& aMessage ) override;
    void Report( const wxString& aMessage ) override;

    void PrevMarker();
    void NextMarker();
    void ExcludeMarker();

private:
    // from DIALOG_ERC_BASE:
    void OnCloseErcDialog( wxCloseEvent& event ) override;
    void OnRunERCClick( wxCommandEvent& event ) override;
    void OnEraseDrcMarkersClick( wxCommandEvent& event ) override;
    void OnERCItemSelected( wxDataViewEvent& aEvent ) override;
    void OnERCItemDClick( wxDataViewEvent& aEvent ) override;
    void OnERCItemRClick( wxDataViewEvent& aEvent ) override;
    void OnLinkClicked( wxHtmlLinkEvent& event ) override;

    void OnSeverity( wxCommandEvent& aEvent ) override;
    void OnSaveReport( wxCommandEvent& aEvent ) override;
    void OnCancelClick( wxCommandEvent& event ) override;

    void redrawDrawPanel();

    void testErc();

    bool writeReport( const wxString& aFullFileName );

    void deleteAllMarkers( bool aIncludeExclusions );

    void syncCheckboxes();
    void updateDisplayedCounts();

private:
    SCH_EDIT_FRAME*    m_parent;

    RC_ITEMS_PROVIDER* m_markerProvider;
    RC_TREE_MODEL*     m_markerTreeModel;

    bool               m_running;
    bool               m_ercRun;

    int                m_severities;
};


#endif

// _DIALOG_ERC_H_
