/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2017 Jean-Pierre Charras, jp.charras@wanadoo.fr
 * Copyright (C) 2013 Wayne Stambaugh <stambaughw@gmail.com>
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

#include <dialogs/dialog_export_netlist_base.h>
#include <netlist.h>

class SCH_EDIT_FRAME;
class EXPORT_NETLIST_PAGE;
class JOB_EXPORT_SCH_NETLIST;

/* Dialog frame for creating netlists */
class DIALOG_EXPORT_NETLIST : public DIALOG_EXPORT_NETLIST_BASE
{
public:
    DIALOG_EXPORT_NETLIST( SCH_EDIT_FRAME* aEditFrame );
    DIALOG_EXPORT_NETLIST( SCH_EDIT_FRAME* aEditFrame, wxWindow* aParent, JOB_EXPORT_SCH_NETLIST* aJob = nullptr );
    ~DIALOG_EXPORT_NETLIST(){};

private:
    void                 InstallCustomPages();
    EXPORT_NETLIST_PAGE* AddOneCustomPage( const wxString& aTitle, const wxString& aCommandString,
                                           NETLIST_TYPE_ID aNetTypeId );
    void                 InstallPageSpice();
    void                 InstallPageSpiceModel();

    bool TransferDataFromWindow() override;

    void updateGeneratorButtons();

    // Called when changing the notebook page (and therefore the current netlist format)
    void OnNetlistTypeSelection( wxNotebookEvent& event ) override;

    /**
     * Add a new panel for a new netlist plugin.
     */
    void OnAddGenerator( wxCommandEvent& event ) override;

    /**
     * Remove a panel relative to a netlist plugin.
     */
    void OnDelGenerator( wxCommandEvent& event ) override;

    /**
     * Write the current netlist options setup in the configuration.
     */
    void WriteCurrentNetlistSetup();

    /**
     * Return the filename extension and the wildcard string for this page or a void name
     * if there is no default name.
     *
     * @param aType is the netlist type ( NET_TYPE_PCBNEW ... ).
     * @param aExt [in] is a holder for the netlist file extension.
     * @param aWildCard [in] is a holder for netlist file dialog wildcard.
     * @return true for known netlist type, false for custom formats.
     */
    bool FilenamePrms( NETLIST_TYPE_ID aType, wxString* aExt, wxString* aWildCard );

private:
    SCH_EDIT_FRAME*                   m_editFrame;
    JOB_EXPORT_SCH_NETLIST*           m_job;
    std::vector<EXPORT_NETLIST_PAGE*> m_PanelNetType;
};