/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include "pcb_netlist_utils.h"

#include <board.h>
#include <board_design_settings.h>
#include <footprint.h>
#include <footprint_library_adapter.h>
#include <lib_id.h>
#include <netlist_reader/netlist_reader.h>
#include <netlist_reader/pcb_netlist.h>
#include <project_pcb.h>
#include <reporter.h>


FOOTPRINT* LoadFootprintFromProject( BOARD* aBoard, const LIB_ID& aFootprintId, bool aKeepUuid )
{
    FOOTPRINT* footprint = nullptr;

    try
    {
        FOOTPRINT_LIBRARY_ADAPTER* adapter = PROJECT_PCB::FootprintLibAdapter( nullptr /* ignored */ );
        footprint = adapter->LoadFootprintWithOptionalNickname( aFootprintId, aKeepUuid );
    }
    catch( const IO_ERROR& )
    {
    }

    if( footprint )
    {
        // If the footprint is found, clear all net info to be sure there are no broken links to
        // any netinfo list (should be not needed, but it can be edited from the footprint editor )
        footprint->ClearAllNets();

        if( aBoard && !aBoard->IsFootprintHolder() )
        {
            BOARD_DESIGN_SETTINGS& bds = aBoard->GetDesignSettings();

            footprint->ApplyDefaultSettings( *aBoard, bds.m_StyleFPFields, bds.m_StyleFPText,
                                             bds.m_StyleFPShapes, bds.m_StyleFPDimensions,
                                             bds.m_StyleFPBarcodes );
        }
    }

    return footprint;
}


void LoadNetlistFootprints( BOARD* aBoard, NETLIST& aNetlist, REPORTER& aReporter )
{
    wxString   msg;
    LIB_ID     lastFPID;
    COMPONENT* component;
    FOOTPRINT* footprint = nullptr;
    FOOTPRINT* fpOnBoard = nullptr;

    FOOTPRINT_LIBRARY_ADAPTER* adapter = PROJECT_PCB::FootprintLibAdapter( nullptr /* ignored */ );

    if( aNetlist.IsEmpty() || adapter->Rows().empty() )
        return;

    // Make sure the libraries are loaded before we start loading the footprints.
    adapter->AsyncLoad();
    adapter->BlockUntilLoaded();

    aNetlist.SortByFPID();

    for( unsigned ii = 0; ii < aNetlist.GetCount(); ii++ )
    {
        component = aNetlist.GetComponent( ii );

        if( !component->GetFPID().GetLibItemName().size() )
        {
            msg.Printf( _( "No footprint defined for symbol %s." ), component->GetReference() );
            aReporter.Report( msg, RPT_SEVERITY_ERROR );
            continue;
        }

        if( aNetlist.IsFindByTimeStamp() )
        {
            for( const KIID& uuid : component->GetKIIDs() )
            {
                KIID_PATH path = component->GetPath();
                path.push_back( uuid );

                if( ( fpOnBoard = aBoard->FindFootprintByPath( path ) ) != nullptr )
                    break;
            }
        }
        else
        {
            fpOnBoard = aBoard->FindFootprintByReference( component->GetReference() );
        }

        bool footprintMisMatch = false;

        if( fpOnBoard )
        {
            if( component->GetFPID().IsLegacy() )
            {
                footprintMisMatch = fpOnBoard->GetFPID().GetLibItemName()
                                    != component->GetFPID().GetLibItemName();
            }
            else
            {
                footprintMisMatch = fpOnBoard->GetFPID() != component->GetFPID();
            }
        }

        if( footprintMisMatch && !aNetlist.GetReplaceFootprints() )
        {
            msg.Printf( _( "Footprint of %s changed: board footprint '%s', netlist footprint '%s'." ),
                        component->GetReference(),
                        fpOnBoard->GetFPID().Format().wx_str(),
                        component->GetFPID().Format().wx_str() );
            aReporter.Report( msg, RPT_SEVERITY_WARNING );
            continue;
        }

        if( !aNetlist.GetReplaceFootprints() )
            footprintMisMatch = false;

        if( fpOnBoard && !footprintMisMatch )
            continue;

        if( component->GetFPID() != lastFPID )
        {
            footprint = nullptr;

            if( !component->GetFPID().GetLibItemName().size() )
            {
                msg.Printf( _( "%s footprint ID '%s' is not valid." ),
                            component->GetReference(),
                            component->GetFPID().Format().wx_str() );
                aReporter.Report( msg, RPT_SEVERITY_ERROR );
                continue;
            }

            footprint = LoadFootprintFromProject( aBoard, component->GetFPID() );

            if( footprint )
            {
                lastFPID = component->GetFPID();
            }
            else
            {
                msg.Printf( _( "%s footprint '%s' not found in any libraries in the footprint "
                               "library table." ),
                            component->GetReference(),
                            component->GetFPID().GetLibItemName().wx_str() );
                aReporter.Report( msg, RPT_SEVERITY_ERROR );
                continue;
            }
        }
        else
        {
            if( !footprint )
                continue;

            footprint = new FOOTPRINT( *footprint );
            footprint->ResetUuidDirect();
        }

        if( footprint )
            component->SetFootprint( footprint );
    }
}
