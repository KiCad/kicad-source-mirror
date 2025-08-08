/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010-2014 Jean-Pierre Charras, jean-pierre.charras at wanadoo.fr
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

#include <dialog_cleanup_tracks_and_vias_base.h>
#include <cleanup_item.h>


class PCB_EDIT_FRAME;
class WX_TEXT_CTRL_REPORTER;


class DIALOG_CLEANUP_TRACKS_AND_VIAS: public DIALOG_CLEANUP_TRACKS_AND_VIAS_BASE
{
public:
    DIALOG_CLEANUP_TRACKS_AND_VIAS( PCB_EDIT_FRAME* parent );
    ~DIALOG_CLEANUP_TRACKS_AND_VIAS();

private:
    void doCleanup( bool aDryRun );
    void setupOKButtonLabel();

    void OnCheckBox( wxCommandEvent& aEvent ) override;
    void OnNetFilterSelect( wxCommandEvent& aEvent );
    void OnNetclassFilterSelect( wxCommandEvent& aEvent ) override;
    void OnLayerFilterSelect( wxCommandEvent& aEvent ) override;
    void OnSelectItem( wxDataViewEvent& event ) override;
    void OnLeftDClickItem( wxMouseEvent& event ) override;

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

private:
    PCB_EDIT_FRAME*  m_parentFrame;
    BOARD*           m_brd;
    RC_TREE_MODEL*   m_changesTreeModel;
    bool             m_firstRun;

    std::vector<std::shared_ptr<CLEANUP_ITEM>> m_items;
    WX_TEXT_CTRL_REPORTER*                     m_reporter;
};
