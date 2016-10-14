/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2009 Jean-Pierre Charras, jean-pierre.charras@gipsa-lab.inpg.fr
 * Copyright (C) 1992-2009 KiCad Developers, see change_log.txt for contributors.
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

#ifndef __dialog_global_edit_tracks_and_vias__
#define __dialog_global_edit_tracks_and_vias__

#include <dialog_global_edit_tracks_and_vias_base.h>

class BOARD;

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS
///////////////////////////////////////////////////////////////////////////////
class DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS :
    public DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS_BASE
{
private:
    PCB_EDIT_FRAME* m_parent;
    BOARD* m_brd;
    int m_curr_netcode;
    int m_optionID;

public:
    DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS( PCB_EDIT_FRAME* aParent, int aNetcode );
    ~DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS() {};

private:
    // Virtual event handlers, overided here
    void OnSelectionClick( wxCommandEvent& event ) override
    {
            m_optionID = event.GetId();
    }
    void OnOkClick( wxCommandEvent& event ) override;
	void onNetSelection( wxCommandEvent& event ) override;


    void MyInit();
    void updateNetInfo();
    void buildNetsList();
};

#endif //__dialog_global_edit_tracks_and_vias__
