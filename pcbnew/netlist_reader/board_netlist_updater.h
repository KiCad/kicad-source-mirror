/**
 * @file board_netlist_updater.h
 * @brief BOARD_NETLIST_UPDATER class definition
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2015 CERN
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@verizon.net>
 *
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

#ifndef __BOARD_NETLIST_UPDATER_H
#define __BOARD_NETLIST_UPDATER_H

class BOARD;
class REPORTER;
class NETLIST;
class COMPONENT;
class FOOTPRINT;
class PCB_EDIT_FRAME;

#include <board_commit.h>

/**
 * Update the #BOARD with a new netlist.
 *
 * The changes are made to the board are as follows they are not disabled in the status
 * settings in the #NETLIST:
 * - If a new component is found in the #NETLIST and not in the #BOARD, it is added
 *   to the #BOARD.
 * - If a the component in the #NETLIST is already on the #BOARD, then one or more of the
 *   following actions can occur:
 *   + If the footprint name in the #NETLIST does not match the footprint name on the
 *     #BOARD, the footprint on the #BOARD is replaced with the footprint specified in
 *     the #NETLIST and the proper parameters are copied from the existing footprint.
 *   + If the reference designator in the #NETLIST does not match the reference designator
 *     on the #BOARD, the reference designator is updated from the #NETLIST.
 *   + If the value field in the #NETLIST does not match the value field on the #BOARD,
 *     the value field is updated from the #NETLIST.
 *   + If the time stamp in the #NETLIST does not match the time stamp  on the #BOARD,
 *     the time stamp is updated from the #NETLIST.
 * - After each footprint is added or update as described above, each footprint pad net
 *   name is compared and updated to the value defined in the #NETLIST.
 * - After all of the footprints have been added, updated, and net names and
 *    pin function properly set, any extra unlock footprints are removed
 *    from the #BOARD.
 *
 */
class BOARD_NETLIST_UPDATER
{
public:
    BOARD_NETLIST_UPDATER( PCB_EDIT_FRAME* aFrame, BOARD* aBoard );
    ~BOARD_NETLIST_UPDATER();

    /**
     * Update the board's components according to the new netlist.
     *
     * See #BOARD_NETLIST_UPDATER class description for the details of the process.
     *
     * @param aNetlist the new netlist
     * @return true if process was completed successfully
     */
    bool UpdateNetlist( NETLIST& aNetlist );

    ///< Set the reporter object.
    void SetReporter( REPORTER* aReporter )
    {
        m_reporter = aReporter;
    }

    ///< Enable "delete single pad nets" option.
    void SetDeleteSinglePadNets( bool aEnabled )
    {
        m_deleteSinglePadNets = aEnabled;
    }

    ///< Enable warning option if a connectable pad is not found in netlist
    ///< connectable = pad with a name and on a copper layer.
    void SetWarnPadNoNetInNetlist( bool aEnabled )
    {
        m_warnForNoNetPads = aEnabled;
    }

    ///< Enable dry run mode (just report, no changes to PCB).
    void SetIsDryRun( bool aEnabled )
    {
        m_isDryRun = aEnabled;
    }

    ///< Enable replacing footprints with new ones.
    void SetReplaceFootprints( bool aEnabled )
    {
        m_replaceFootprints = aEnabled;
    }

    ///< Enable removing unused components.
    void SetDeleteUnusedComponents( bool aEnabled )
    {
        m_deleteUnusedComponents = aEnabled;
    }

    ///< Enable component lookup by timestamp instead of reference.
    void SetLookupByTimestamp( bool aEnabled )
    {
        m_lookupByTimestamp = aEnabled;
    }

    std::vector<FOOTPRINT*> GetAddedComponents() const
    {
        return m_addedComponents;
    }

private:
    void cacheNetname( PAD* aPad, const wxString& aNetname );
    wxString getNetname( PAD* aPad );

    void cachePinFunction( PAD* aPad, const wxString& aPinFunction );
    wxString getPinFunction( PAD* aPad );

    wxPoint estimateComponentInsertionPosition();

    FOOTPRINT* addNewComponent( COMPONENT* aComponent );

    FOOTPRINT* replaceComponent( NETLIST& aNetlist, FOOTPRINT* aPcbComponent,
                                 COMPONENT* aNewComponent );

    bool updateFootprintParameters( FOOTPRINT* aPcbFootprint, COMPONENT* aNetlistComponent );

    bool updateComponentPadConnections( FOOTPRINT* aFootprint, COMPONENT* aNewComponent );

    void cacheCopperZoneConnections();

    bool updateCopperZoneNets( NETLIST& aNetlist );

    bool deleteUnusedComponents( NETLIST& aNetlist );

    bool deleteSinglePadNets();

    bool testConnectivity( NETLIST& aNetlist, std::map<COMPONENT*, FOOTPRINT*>& aFootprintMap );

    PCB_EDIT_FRAME* m_frame;
    BOARD_COMMIT    m_commit;
    BOARD*          m_board;
    REPORTER*       m_reporter;

    std::map<ZONE*, std::vector<PAD*>> m_zoneConnectionsCache;
    std::map<wxString, wxString>       m_oldToNewNets;
    std::map<PAD*, wxString>           m_padNets;
    std::map<PAD*, wxString>           m_padPinFunctions;
    std::vector<FOOTPRINT*>            m_addedComponents;
    std::map<wxString, NETINFO_ITEM*>  m_addedNets;

    bool m_deleteSinglePadNets;
    bool m_deleteUnusedComponents;
    bool m_isDryRun;
    bool m_replaceFootprints;
    bool m_lookupByTimestamp;
    bool m_warnForNoNetPads;    // Warn if a connectable pad has no net in netlist (not found).

    int m_warningCount;
    int m_errorCount;
    int m_newFootprintsCount;   // the count of new footprints
                                // either really new or replaced by new fp.
};

#endif
