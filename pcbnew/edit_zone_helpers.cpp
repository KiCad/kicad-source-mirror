/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * Some code comes from FreePCB.
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

#include <kiface_base.h>
#include <confirm.h>
#include <pcb_edit_frame.h>
#include <pcbnew_settings.h>
#include <board.h>
#include <board_commit.h>
#include <board_design_settings.h>
#include <tool/tool_manager.h>
#include <tool/actions.h>
#include <zone.h>
#include <zones.h>
#include <connectivity/connectivity_data.h>


void PCB_EDIT_FRAME::Edit_Zone_Params( ZONE* aZone )
{
    int           dialogResult;
    ZONE_SETTINGS zoneInfo = m_pcb->GetDesignSettings().GetDefaultZoneSettings();
    BOARD_COMMIT  commit( this );

    if( aZone->GetIsRuleArea() )
    {
        // edit a rule area on a copper layer
        zoneInfo << *aZone;
        dialogResult = InvokeRuleAreaEditor( this, &zoneInfo, GetBoard() );
    }
    else if( IsCopperLayer( aZone->GetFirstLayer() ) )
    {
        // edit a zone on a copper layer
        zoneInfo << *aZone;
        dialogResult = InvokeCopperZonesEditor( this, aZone, &zoneInfo );
    }
    else
    {
        zoneInfo << *aZone;
        dialogResult = InvokeNonCopperZonesEditor( this, &zoneInfo );
    }

    if( dialogResult == wxID_CANCEL )
        return;

    wxBusyCursor dummy;

    // Undraw old zone outlines
    for( ZONE* zone : GetBoard()->Zones() )
        GetCanvas()->GetView()->Update( zone );

    commit.Modify( aZone );

    zoneInfo.ExportSetting( *aZone );

    if( NETINFO_ITEM* net = GetBoard()->FindNet( zoneInfo.m_Netcode ) )
        aZone->SetNetCode( net->GetNetCode() );

    // restore default net properties
    zoneInfo.m_Netcode = NETINFO_LIST::ORPHANED;

    m_pcb->GetDesignSettings().SetDefaultZoneSettings( zoneInfo );

    commit.Push( _( "Edit Zone Properties" ), SKIP_CONNECTIVITY );
    rebuildConnectivity();
}


bool BOARD::TestZoneIntersection( ZONE* aZone1, ZONE* aZone2 )
{
    // see if areas are on same layer
    if( !( aZone1->GetLayerSet() & aZone2->GetLayerSet() ).any() )
        return false;

    SHAPE_POLY_SET* poly1 = aZone1->Outline();
    SHAPE_POLY_SET* poly2 = aZone2->Outline();

    // test bounding rects
    BOX2I b1 = poly1->BBox();
    BOX2I b2 = poly2->BBox();

    if( ! b1.Intersects( b2 ) )
        return false;

    // Now test for intersecting segments
    for( auto segIterator1 = poly1->IterateSegmentsWithHoles(); segIterator1; segIterator1++ )
    {
        // Build segment
        SEG firstSegment = *segIterator1;

        for( auto segIterator2 = poly2->IterateSegmentsWithHoles(); segIterator2; segIterator2++ )
        {
            // Build second segment
            SEG secondSegment = *segIterator2;

            // Check whether the two segments built collide
            if( firstSegment.Collide( secondSegment, 0 ) )
                return true;
        }
    }

    // If a contour is inside another contour, no segments intersects, but the zones
    // can be combined if a corner is inside an outline (only one corner is enough)
    for( auto iter = poly2->IterateWithHoles(); iter; iter++ )
    {
        if( poly1->Contains( *iter ) )
            return true;
    }

    for( auto iter = poly1->IterateWithHoles(); iter; iter++ )
    {
        if( poly2->Contains( *iter ) )
            return true;
    }

    return false;
}



