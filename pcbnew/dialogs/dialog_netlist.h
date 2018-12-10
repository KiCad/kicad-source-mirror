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
    wxConfigBase*   m_config;
    bool            m_initialized;
    bool            m_runDragCommand;

public:
    DIALOG_NETLIST( PCB_EDIT_FRAME* aParent, const wxString & aNetlistFullFilename );
    ~DIALOG_NETLIST();

private:
    void onFilenameChanged();

    /**
     * Function verifyFootprints
     * compares the netlist to the board and builds a list of duplicate, missing, and
     * extra footprints.
     *
     * @param aNetlistFilename the netlist filename.
     * @param aCmpFilename the component link filename.
     * @param aDuplicate the list of duplicate modules to populate
     * @param aMissing the list of missing module references and values to populate. For
     *                 each missing item, the first string is the reference designator and
     *                 the second is the value.
     * @param aNotInNetlist is the list of component footprint found in the netlist but not on
     *                      the board.
     * @return true if no errors occurred while reading the netlist. Otherwise false.
     */
    bool verifyFootprints( const wxString&         aNetlistFilename,
                           const wxString&         aCmpFilename,
                           std::vector< MODULE* >& aDuplicate,
                           wxArrayString&          aMissing,
                           std::vector< MODULE* >& aNotInNetlist );

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
