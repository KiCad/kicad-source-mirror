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


#include <common.h>                         // for PAGE_INFO

#include <class_board.h>
#include <netinfo.h>
#include <class_module.h>
#include <class_pad.h>
#include <class_zone.h>

#include <pcb_netlist.h>
#include <connectivity/connectivity_data.h>
#include <reporter.h>

#include <board_netlist_updater.h>

#include <pcb_edit_frame.h>


BOARD_NETLIST_UPDATER::BOARD_NETLIST_UPDATER( PCB_EDIT_FRAME* aFrame, BOARD* aBoard ) :
    m_frame( aFrame ),
    m_commit( aFrame ),
    m_board( aBoard )
{
    m_reporter = &NULL_REPORTER::GetInstance();

    m_deleteSinglePadNets = true;
    m_deleteUnusedComponents = false;
    m_isDryRun = false;
    m_replaceFootprints = true;
    m_lookupByTimestamp = false;

    m_warningCount = 0;
    m_errorCount = 0;
    m_newFootprintsCount = 0;
}


BOARD_NETLIST_UPDATER::~BOARD_NETLIST_UPDATER()
{
}


// These functions allow inspection of pad nets during dry runs by keeping a cache of
// current pad netnames indexed by pad.

void BOARD_NETLIST_UPDATER::cacheNetname( D_PAD* aPad, const wxString& aNetname )
{
    m_padNets[ aPad ] = aNetname;
}

wxString BOARD_NETLIST_UPDATER::getNetname( D_PAD* aPad )
{
    if( m_isDryRun && m_padNets.count( aPad ) )
        return m_padNets[ aPad ];
    else
        return aPad->GetNetname();
}


wxPoint BOARD_NETLIST_UPDATER::estimateComponentInsertionPosition()
{
    wxPoint bestPosition;

    if( !m_board->IsEmpty() )
    {
        // Position new components below any existing board features.
        EDA_RECT bbox = m_board->GetBoardEdgesBoundingBox();

        if( bbox.GetWidth() || bbox.GetHeight() )
        {
            bestPosition.x = bbox.Centre().x;
            bestPosition.y = bbox.GetBottom() + Millimeter2iu( 10 );
        }
    }
    else
    {
        // Position new components in the center of the page when the board is empty.
        wxSize pageSize = m_board->GetPageSettings().GetSizeIU();

        bestPosition.x = pageSize.GetWidth() / 2;
        bestPosition.y = pageSize.GetHeight() / 2;
    }

    return bestPosition;
}


MODULE* BOARD_NETLIST_UPDATER::addNewComponent( COMPONENT* aComponent )
{
    wxString msg;

    if( aComponent->GetFPID().empty() )
    {
        msg.Printf( _( "Cannot add %s (no footprint assigned)." ),
                    aComponent->GetReference(),
                    aComponent->GetFPID().Format().wx_str() );
        m_reporter->Report( msg, REPORTER::RPT_ERROR );
        ++m_errorCount;
        return nullptr;
    }

    MODULE* footprint = m_frame->LoadFootprint( aComponent->GetFPID() );

    if( footprint == nullptr )
    {
        msg.Printf( _( "Cannot add %s (footprint \"%s\" not found)." ),
                    aComponent->GetReference(),
                    aComponent->GetFPID().Format().wx_str() );
        m_reporter->Report( msg, REPORTER::RPT_ERROR );
        ++m_errorCount;
        return nullptr;
    }

    msg.Printf( _( "Add %s (footprint \"%s\")." ),
                aComponent->GetReference(),
                aComponent->GetFPID().Format().wx_str() );
    m_reporter->Report( msg, REPORTER::RPT_ACTION );

    m_newFootprintsCount++;

    if( !m_isDryRun )
    {
        footprint->SetParent( m_board );
        footprint->SetPosition( estimateComponentInsertionPosition( ) );
        footprint->SetTimeStamp( GetNewTimeStamp() );

        m_addedComponents.push_back( footprint );
        m_commit.Add( footprint );

        return footprint;
    }
    else
        delete footprint;

    return NULL;
}


MODULE* BOARD_NETLIST_UPDATER::replaceComponent( NETLIST& aNetlist, MODULE* aPcbComponent,
                                                 COMPONENT* aNewComponent )
{
    wxString msg;

    if( aNewComponent->GetFPID().empty() )
    {
        msg.Printf( _( "Cannot update %s (no footprint assigned)." ),
                    aNewComponent->GetReference(),
                    aNewComponent->GetFPID().Format().wx_str() );
        m_reporter->Report( msg, REPORTER::RPT_ERROR );
        ++m_errorCount;
        return nullptr;
    }

    MODULE* newFootprint = m_frame->LoadFootprint( aNewComponent->GetFPID() );

    if( newFootprint == nullptr )
    {
        msg.Printf( _( "Cannot update %s (footprint \"%s\" not found)." ),
                    aNewComponent->GetReference(),
                    aNewComponent->GetFPID().Format().wx_str() );
        m_reporter->Report( msg, REPORTER::RPT_ERROR );
        ++m_errorCount;
        return nullptr;
    }

    msg.Printf( _( "Change %s footprint from \"%s\" to \"%s\"."),
                aPcbComponent->GetReference(),
                aPcbComponent->GetFPID().Format().wx_str(),
                aNewComponent->GetFPID().Format().wx_str() );
    m_reporter->Report( msg, REPORTER::RPT_ACTION );

    m_newFootprintsCount++;

    if( !m_isDryRun )
    {
        m_frame->Exchange_Module( aPcbComponent, newFootprint, m_commit, true, true, true );
        return newFootprint;
    }
    else
        delete newFootprint;

    return nullptr;
}


bool BOARD_NETLIST_UPDATER::updateComponentParameters( MODULE* aPcbComponent,
                                                       COMPONENT* aNewComponent )
{
    wxString msg;

    // Create a copy only if the module has not been added during this update
    MODULE* copy = m_commit.GetStatus( aPcbComponent ) ? nullptr : (MODULE*) aPcbComponent->Clone();
    bool changed = false;

    // Test for reference designator field change.
    if( aPcbComponent->GetReference() != aNewComponent->GetReference() )
    {
        msg.Printf( _( "Change %s reference to %s." ),
                    aPcbComponent->GetReference(),
                    aNewComponent->GetReference() );
        m_reporter->Report( msg, REPORTER::RPT_ACTION );

        if ( !m_isDryRun )
        {
            changed = true;
            aPcbComponent->SetReference( aNewComponent->GetReference() );
        }
    }

    // Test for value field change.
    if( aPcbComponent->GetValue() != aNewComponent->GetValue() )
    {
        msg.Printf( _( "Change %s value from %s to %s." ),
                    aPcbComponent->GetReference(),
                    aPcbComponent->GetValue(),
                    aNewComponent->GetValue() );
        m_reporter->Report( msg, REPORTER::RPT_ACTION );

        if( !m_isDryRun )
        {
            changed = true;
            aPcbComponent->SetValue( aNewComponent->GetValue() );
        }
    }

    // Test for time stamp change.
    if( aPcbComponent->GetPath() != aNewComponent->GetTimeStamp() )
    {
        msg.Printf( _( "Change symbol path \"%s:%s\" to \"%s\"." ),
                    aPcbComponent->GetReference(),
                    aPcbComponent->GetPath(),
                    aNewComponent->GetTimeStamp() );
        m_reporter->Report( msg, REPORTER::RPT_INFO );

        if( !m_isDryRun )
        {
            changed = true;
            aPcbComponent->SetPath( aNewComponent->GetTimeStamp() );
        }
    }

    if( changed && copy )
        m_commit.Modified( aPcbComponent, copy );
    else
        delete copy;

    return true;
}


bool BOARD_NETLIST_UPDATER::updateComponentPadConnections( MODULE* aPcbComponent,
                                                           COMPONENT* aNewComponent )
{
    wxString msg;

    // Create a copy only if the module has not been added during this update
    MODULE* copy = m_commit.GetStatus( aPcbComponent ) ? nullptr : (MODULE*) aPcbComponent->Clone();
    bool changed = false;

    // At this point, the component footprint is updated.  Now update the nets.
    for( auto pad : aPcbComponent->Pads() )
    {
        COMPONENT_NET net = aNewComponent->GetNet( pad->GetName() );

        // Test if new footprint pad has no net (pads not on copper layers have no net).
        if( !net.IsValid() || !pad->IsOnCopperLayer() )
        {
            if( !pad->GetNetname().IsEmpty() )
            {
                msg.Printf( _( "Disconnect %s pin %s." ),
                            aPcbComponent->GetReference(),
                            pad->GetName() );
                m_reporter->Report( msg, REPORTER::RPT_ACTION );
            }

            if( !m_isDryRun )
            {
                changed = true;
                pad->SetNetCode( NETINFO_LIST::UNCONNECTED );
            }
            else
                cacheNetname( pad, wxEmptyString );
        }
        else                                 // New footprint pad has a net.
        {
            const wxString& netName = net.GetNetName();
            NETINFO_ITEM* netinfo = m_board->FindNet( netName );

            if( netinfo && !m_isDryRun )
                netinfo->SetIsCurrent( true );

            if( pad->GetNetname() != netName )
            {

                if( netinfo == nullptr )
                {
                    // It might be a new net that has not been added to the board yet
                    if( m_addedNets.count( netName ) )
                        netinfo = m_addedNets[ netName ];
                }

                if( netinfo == nullptr )
                {
                    netinfo = new NETINFO_ITEM( m_board, netName );

                    // It is a new net, we have to add it
                    if( !m_isDryRun )
                    {
                        changed = true;
                        m_commit.Add( netinfo );
                    }

                    m_addedNets[netName] = netinfo;
                    msg.Printf( _( "Add net %s." ), UnescapeString( netName ) );
                    m_reporter->Report( msg, REPORTER::RPT_ACTION );
                }

                if( !pad->GetNetname().IsEmpty() )
                {
                    m_oldToNewNets[ pad->GetNetname() ] = netName;

                    msg.Printf( _( "Reconnect %s pin %s from %s to %s."),
                            aPcbComponent->GetReference(),
                            pad->GetName(),
                            UnescapeString( pad->GetNetname() ),
                            UnescapeString( netName ) );
                }
                else
                {
                    msg.Printf( _( "Connect %s pin %s to %s."),
                            aPcbComponent->GetReference(),
                            pad->GetName(),
                            UnescapeString( netName ) );
                }
                m_reporter->Report( msg, REPORTER::RPT_ACTION );

                if( !m_isDryRun )
                {
                    changed = true;
                    pad->SetNet( netinfo );
                }
                else
                    cacheNetname( pad, netName );
            }
        }
    }

    if( changed && copy )
        m_commit.Modified( aPcbComponent, copy );
    else
        delete copy;

    return true;
}


void BOARD_NETLIST_UPDATER::cacheCopperZoneConnections()
{
    for( int ii = 0; ii < m_board->GetAreaCount(); ii++ )
    {
        ZONE_CONTAINER* zone = m_board->GetArea( ii );

        if( !zone->IsOnCopperLayer() || zone->GetIsKeepout() )
            continue;

        m_zoneConnectionsCache[ zone ] = m_board->GetConnectivity()->GetConnectedPads( zone );
    }
}


bool BOARD_NETLIST_UPDATER::updateCopperZoneNets( NETLIST& aNetlist )
{
    wxString msg;
    std::set<wxString> netlistNetnames;

    for( int ii = 0; ii < (int) aNetlist.GetCount(); ii++ )
    {
        const COMPONENT* component = aNetlist.GetComponent( ii );
        for( unsigned jj = 0; jj < component->GetNetCount(); jj++ )
        {
            const COMPONENT_NET& net = component->GetNet( jj );
            netlistNetnames.insert( net.GetNetName() );
        }
    }

    for( auto via : m_board->Tracks() )
    {
        if( via->Type() != PCB_VIA_T )
            continue;

        if( netlistNetnames.count( via->GetNetname() ) == 0 )
        {
            wxString updatedNetname = wxEmptyString;

            // Take via name from name change map if it didn't match to a new pad
            // (this is useful for stitching vias that don't connect to tracks)
            if( m_oldToNewNets.count( via->GetNetname() ) )
            {
                updatedNetname = m_oldToNewNets[via->GetNetname()];
            }

            if( !updatedNetname.IsEmpty() )
            {
                msg.Printf( _( "Reconnect via from %s to %s." ),
                        UnescapeString( via->GetNetname() ), UnescapeString( updatedNetname ) );
                m_reporter->Report( msg, REPORTER::RPT_ACTION );

                if( !m_isDryRun )
                {
                    NETINFO_ITEM* netinfo = m_board->FindNet( updatedNetname );

                    if( !netinfo )
                        netinfo = m_addedNets[updatedNetname];

                    if( netinfo )
                    {
                        m_commit.Modify( via );
                        via->SetNet( netinfo );
                    }
                }
            }
            else
            {
                msg.Printf( _( "Via connected to unknown net (%s)." ),
                        UnescapeString( via->GetNetname() ) );
                m_reporter->Report( msg, REPORTER::RPT_WARNING );
                ++m_warningCount;
            }
        }
    }

    // Test copper zones to detect "dead" nets (nets without any pad):
    for( int i = 0; i < m_board->GetAreaCount(); i++ )
    {
        ZONE_CONTAINER* zone = m_board->GetArea( i );

        if( !zone->IsOnCopperLayer() || zone->GetIsKeepout() )
            continue;

        if( netlistNetnames.count( zone->GetNetname() ) == 0 )
        {
            // Look for a pad in the zone's connected-pad-cache which has been updated to
            // a new net and use that. While this won't always be the right net, the dead
            // net is guaranteed to be wrong.
            wxString updatedNetname = wxEmptyString;

            for( D_PAD* pad : m_zoneConnectionsCache[ zone ] )
            {
                if( getNetname( pad ) != zone->GetNetname() )
                {
                    updatedNetname = getNetname( pad );
                    break;
                }
            }

            // Take zone name from name change map if it didn't match to a new pad
            // (this is useful for zones on internal layers)
            if( updatedNetname.IsEmpty() && m_oldToNewNets.count( zone->GetNetname() ) )
            {
                updatedNetname = m_oldToNewNets[ zone->GetNetname() ];
            }

            if( !updatedNetname.IsEmpty() )
            {
                msg.Printf( _( "Reconnect copper zone from %s to %s." ),
                            UnescapeString( zone->GetNetname() ),
                            UnescapeString( updatedNetname ) );
                m_reporter->Report( msg, REPORTER::RPT_ACTION );

                if( !m_isDryRun )
                {
                    NETINFO_ITEM* netinfo = m_board->FindNet( updatedNetname );

                    if( !netinfo )
                        netinfo = m_addedNets[ updatedNetname ];

                    if( netinfo )
                    {
                        m_commit.Modify( zone );
                        zone->SetNet( netinfo );
                    }
                }
            }
            else
            {
                msg.Printf( _( "Copper zone (%s) has no pads connected." ),
                            UnescapeString( zone->GetNetname() ) );
                m_reporter->Report( msg, REPORTER::RPT_WARNING );
                ++m_warningCount;
            }
        }
    }

    return true;
}


bool BOARD_NETLIST_UPDATER::deleteUnusedComponents( NETLIST& aNetlist )
{
    wxString msg;
    const COMPONENT* component;

    for( auto module : m_board->Modules() )
    {

        if( m_lookupByTimestamp )
            component = aNetlist.GetComponentByTimeStamp( module->GetPath() );
        else
            component = aNetlist.GetComponentByReference( module->GetReference() );

        if( component == NULL )
        {
            if( module->IsLocked() )
            {
                msg.Printf( _( "Cannot remove unused footprint %s (locked)." ), module->GetReference() );
                m_reporter->Report( msg, REPORTER::RPT_WARNING );
                continue;
            }

            msg.Printf( _( "Remove unused footprint %s." ), module->GetReference() );
            m_reporter->Report( msg, REPORTER::RPT_ACTION );

            if( !m_isDryRun )
                m_commit.Remove( module );
        }
    }

    return true;
}


bool BOARD_NETLIST_UPDATER::deleteSinglePadNets()
{
    int         count = 0;
    wxString    netname;
    wxString    msg;
    D_PAD*      previouspad = NULL;

    // We need the pad list for next tests.

    m_board->BuildListOfNets();

    std::vector<D_PAD*> padlist = m_board->GetPads();

    // Sort pads by netlist name
    std::sort( padlist.begin(), padlist.end(),
        [ this ]( D_PAD* a, D_PAD* b ) -> bool { return getNetname( a ) < getNetname( b ); } );

    for( D_PAD* pad : padlist )
    {
        if( getNetname( pad ).IsEmpty() )
            continue;

        if( netname != getNetname( pad ) )  // End of net
        {
            if( previouspad && count == 1 )
            {
                // First, see if we have a copper zone attached to this pad.
                // If so, this is not really a single pad net

                for( ZONE_CONTAINER* zone : m_board->Zones() )
                {
                    if( !zone->IsOnCopperLayer() )
                        continue;

                    if( zone->GetIsKeepout() )
                        continue;

                    if( zone->GetNetname() == getNetname( previouspad ) )
                    {
                        count++;
                        break;
                    }
                }

                if( count == 1 )    // Really one pad, and nothing else
                {
                    msg.Printf( _( "Remove single pad net %s." ),
                                UnescapeString( getNetname( previouspad ) ) );
                    m_reporter->Report( msg, REPORTER::RPT_ACTION );

                    if( !m_isDryRun )
                        previouspad->SetNetCode( NETINFO_LIST::UNCONNECTED );
                    else
                        cacheNetname( previouspad, wxEmptyString );
                }
            }

            netname = getNetname( pad );
            count = 1;
        }
        else
        {
            count++;
        }

        previouspad = pad;
    }

    // Examine last pad
    if( count == 1 )
    {
        if( !m_isDryRun )
            previouspad->SetNetCode( NETINFO_LIST::UNCONNECTED );
        else
            cacheNetname( previouspad, wxEmptyString );
    }

    return true;
}


bool BOARD_NETLIST_UPDATER::testConnectivity( NETLIST& aNetlist )
{
    // Verify that board contains all pads in netlist: if it doesn't then footprints are
    // wrong or missing.
    // Note that we use references to find the footprints as they're already updated by this
    // point (whether by-reference or by-timestamp).

    wxString msg;
    wxString padname;

    for( int i = 0; i < (int) aNetlist.GetCount(); i++ )
    {
        const COMPONENT* component = aNetlist.GetComponent( i );
        MODULE* footprint = m_board->FindModuleByReference( component->GetReference() );

        if( footprint == NULL )    // It can be missing in partial designs
            continue;

        // Explore all pins/pads in component
        for( unsigned jj = 0; jj < component->GetNetCount(); jj++ )
        {
            const COMPONENT_NET& net = component->GetNet( jj );
            padname = net.GetPinName();

            if( footprint->FindPadByName( padname ) )
                continue;   // OK, pad found

            // not found: bad footprint, report error
            msg.Printf( _( "%s pad %s not found in %s." ),
                        component->GetReference(),
                        padname,
                        footprint->GetFPID().Format().wx_str() );
            m_reporter->Report( msg, REPORTER::RPT_ERROR );
            ++m_errorCount;
        }
    }

    return true;
}


bool BOARD_NETLIST_UPDATER::UpdateNetlist( NETLIST& aNetlist )
{
    wxString msg;
    m_errorCount = 0;
    m_warningCount = 0;
    m_newFootprintsCount = 0;
    MODULE* lastPreexistingFootprint = m_board->Modules().empty() ? NULL : m_board->Modules().back();

    cacheCopperZoneConnections();

    if( !m_isDryRun )
    {
        m_board->SetStatus( 0 );

        // Mark all nets (except <no net>) as stale; we'll update those to current that
        // we find in the netlist
        for( NETINFO_ITEM* net : m_board->GetNetInfo() )
            net->SetIsCurrent( net->GetNet() == 0 );
    }

    for( unsigned i = 0; i < aNetlist.GetCount(); i++ )
    {
        COMPONENT* component = aNetlist.GetComponent( i );
        int        matchCount = 0;
        MODULE*    tmp;

        msg.Printf( _( "Processing component \"%s:%s:%s\"." ),
                    component->GetReference(),
                    component->GetTimeStamp(),
                    component->GetFPID().Format().wx_str() );
        m_reporter->Report( msg, REPORTER::RPT_INFO );

        for( auto footprint : m_board->Modules() )
        {
            bool     match = false;

            if( footprint )
            {
                if( m_lookupByTimestamp )
                    match = footprint->GetPath() == component->GetTimeStamp();
                else
                    match = footprint->GetReference().CmpNoCase( component->GetReference() ) == 0;
            }

            if( match )
            {
                tmp = footprint;

                if( m_replaceFootprints && component->GetFPID() != footprint->GetFPID() )
                    tmp = replaceComponent( aNetlist, footprint, component );

                if( tmp )
                {
                    updateComponentParameters( tmp, component );
                    updateComponentPadConnections( tmp, component );
                }

                matchCount++;
            }

            if( footprint == lastPreexistingFootprint )
            {
                // No sense going through the newly-created footprints: end of loop
                break;
            }
        }

        if( matchCount == 0 )
        {
            tmp = addNewComponent( component );

            if( tmp )
            {
                updateComponentParameters( tmp, component );
                updateComponentPadConnections( tmp, component );
            }
        }
        else if( matchCount > 1 )
        {
            msg.Printf( _( "Multiple footprints found for \"%s\"." ),
                        component->GetReference() );
            m_reporter->Report( msg, REPORTER::RPT_ERROR );
        }
    }

    updateCopperZoneNets( aNetlist );

    if( m_deleteUnusedComponents )
        deleteUnusedComponents( aNetlist );

    if( !m_isDryRun )
    {
        m_commit.Push( _( "Update netlist" ) );
        m_board->GetConnectivity()->Build( m_board );
        testConnectivity( aNetlist );

        // Now the connectivity data is rebuilt, we can delete single pads nets
        if( m_deleteSinglePadNets )
            deleteSinglePadNets();
    }
    else if( m_deleteSinglePadNets && !m_newFootprintsCount )
        // We can delete single net pads in dry run mode only if no new footprints
        // are added, because these new footprints are not actually added to the board
        // and the current pad list is wrong in this case.
        deleteSinglePadNets();

    if( m_isDryRun )
    {
        for( auto it : m_addedNets )
            delete it.second;

        m_addedNets.clear();
    }

    // Update the ratsnest
    m_reporter->ReportTail( wxT( "" ), REPORTER::RPT_ACTION );
    m_reporter->ReportTail( wxT( "" ), REPORTER::RPT_ACTION );

    msg.Printf( _( "Total warnings: %d, errors: %d." ), m_warningCount, m_errorCount );
    m_reporter->ReportTail( msg, REPORTER::RPT_ACTION );

    if( m_errorCount )
    {
        m_reporter->ReportTail( _( "Errors occurred during the netlist update. Unless you fix them "
                                   "your board will not be consistent with the schematics." ),
                                REPORTER::RPT_ERROR );
        return false;
    }

    m_reporter->ReportTail( _( "Netlist update successful!" ), REPORTER::RPT_ACTION );
    return true;
}
