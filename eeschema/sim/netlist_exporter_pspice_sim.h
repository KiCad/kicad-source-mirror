/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 CERN
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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

#ifndef NETLIST_EXPORTER_PSPICE_SIM_H
#define NETLIST_EXPORTER_PSPICE_SIM_H

#include <netlist_exporters/netlist_exporter_pspice.h>

/// Special netlist exporter flavor that allows to override simulation commands
class NETLIST_EXPORTER_PSPICE_SIM : public NETLIST_EXPORTER_PSPICE
{
public:
    NETLIST_EXPORTER_PSPICE_SIM( NETLIST_OBJECT_LIST* aMasterList, PART_LIBS* aLibs,
            SEARCH_STACK* aPaths = NULL ) :
        NETLIST_EXPORTER_PSPICE( aMasterList, aLibs, aPaths )
    {
    }

    void SetSimCommand( const wxString& aCmd )
    {
        m_simCommand = aCmd;
    }

    const wxString& GetSimCommand() const
    {
        return m_simCommand;
    }

    void ClearSimCommand()
    {
        m_simCommand.Clear();
    }

    wxString GetSheetSimCommand();

protected:
    void writeDirectives( OUTPUTFORMATTER* aFormatter, unsigned aCtl ) const override;

private:
    static bool isSimCommand( const wxString& aCmd );

    ///> Overridden simulation command
    wxString m_simCommand;
};

#endif /* NETLIST_EXPORTER_PSPICE_SIM_H */
