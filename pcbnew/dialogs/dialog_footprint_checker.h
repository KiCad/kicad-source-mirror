/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010-2014 Jean-Pierre Charras, jean-pierre.charras at wanadoo.fr
 * Copyright (C) 1992-2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef DIALOG_FOOTPRINT_CHECKER_H
#define DIALOG_FOOTPRINT_CHECKER_H

#include <dialog_footprint_checker_base.h>
#include <rc_item.h>

class FOOTPRINT_EDIT_FRAME;


class DIALOG_FOOTPRINT_CHECKER: public DIALOG_FOOTPRINT_CHECKER_BASE
{
public:
    DIALOG_FOOTPRINT_CHECKER( FOOTPRINT_EDIT_FRAME* aParent );
    ~DIALOG_FOOTPRINT_CHECKER();

    void SetMarkersProvider( RC_ITEMS_PROVIDER* aProvider );

private:
    void syncCheckboxes();
    void updateDisplayedCounts();

    void runChecks();

    void deleteAllMarkers();
    void refreshEditor();

    void OnRunChecksClick( wxCommandEvent& aEvent ) override;
    void OnCancelClick( wxCommandEvent& aEvent ) override;
    void OnClose( wxCloseEvent& event ) override;

    void OnSeverity( wxCommandEvent& aEvent ) override;

    void OnSelectItem( wxDataViewEvent& event ) override;
    void OnLeftDClickItem( wxMouseEvent& event ) override;
    void OnDeleteOneClick( wxCommandEvent& event ) override;
    void OnDeleteAllClick( wxCommandEvent& event ) override;

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

private:
    FOOTPRINT_EDIT_FRAME* m_frame;
    bool                  m_checksRun;

    RC_TREE_MODEL*        m_markersTreeModel;
    RC_ITEMS_PROVIDER*    m_markersProvider;

    int                   m_severities;
};

#endif // DIALOG_FOOTPRINT_CHECKER_H
