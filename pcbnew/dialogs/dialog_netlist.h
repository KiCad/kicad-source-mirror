/**
 * @file pcbnew/dialogs/dialog_netlist.h
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2015 KiCad Developers, see change_log.txt for contributors.
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

#ifndef _DIALOG_NETLIST_H_
#define _DIALOG_NETLIST_H_

#include <dialog_netlist_base.h>


class MODULE;
class NETLIST;


class DIALOG_NETLIST : public DIALOG_NETLIST_BASE
{
private:
    PCB_EDIT_FRAME* m_parent;
    wxString&       m_netlistPath;
    wxConfigBase*   m_config;
    bool            m_initialized;
    bool            m_runDragCommand;

public:
    DIALOG_NETLIST( PCB_EDIT_FRAME* aParent, wxString& aNetlistFullFilename );
    ~DIALOG_NETLIST();

private:
    void onFilenameChanged();

    void loadNetlist( bool aDryRun );



    // Virtual event handlers:
    void OnOpenNetlistClick( wxCommandEvent& event ) override;
    void OnUpdatePCB( wxCommandEvent& event ) override;
    void OnFilenameKillFocus( wxFocusEvent& event ) override;
    void OnMatchChanged( wxCommandEvent& event ) override;
    void OnOptionChanged( wxCommandEvent& event ) override;
    void OnTestFootprintsClick( wxCommandEvent& event ) override;
    void OnCompileRatsnestClick( wxCommandEvent& event ) override;
    void OnUpdateUIValidNetlistFile( wxUpdateUIEvent& aEvent ) override;
};


#endif      // _DIALOG_NETLIST_H_
