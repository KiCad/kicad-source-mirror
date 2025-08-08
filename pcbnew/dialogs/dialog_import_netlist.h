/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#include <dialog_import_netlist_base.h>


class FOOTPRINT;
class NETLIST;


class DIALOG_IMPORT_NETLIST : public DIALOG_IMPORT_NETLIST_BASE
{
public:
    DIALOG_IMPORT_NETLIST( PCB_EDIT_FRAME* aParent, wxString& aNetlistFullFilename );
    ~DIALOG_IMPORT_NETLIST();

private:
    void onFilenameChanged( bool aLoadNetlist );

    void loadNetlist( bool aDryRun );

    // Virtual event handlers:
    void onBrowseNetlistFiles( wxCommandEvent& event ) override;
    void onImportNetlist( wxCommandEvent& event ) override;
    void onUpdatePCB( wxCommandEvent& event ) override;
    void OnFilenameKillFocus( wxFocusEvent& event ) override;
    void OnMatchChanged( wxCommandEvent& event ) override;
    void OnOptionChanged( wxCommandEvent& event ) override;

private:
    PCB_EDIT_FRAME* m_parent;
    wxString&       m_netlistPath;
    bool            m_initialized;
    bool            m_runDragCommand;
};

