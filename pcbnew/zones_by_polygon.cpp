/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2012 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2017 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <fctsys.h>
#include <kiface_i.h>
#include <confirm.h>
#include <pcb_edit_frame.h>
#include <board_commit.h>
#include <view/view.h>
#include <class_board.h>
#include <class_zone.h>
#include <pcbnew.h>
#include <zones.h>
#include <pcbnew_id.h>
#include <zones_functions_for_undo_redo.h>
#include <tools/drc.h>
#include <connectivity/connectivity_data.h>
#include <widgets/progress_reporter.h>
#include <zone_filler.h>

// Local variables
static PICKED_ITEMS_LIST s_PickedList;    // a picked list to save zones for undo/redo command
static PICKED_ITEMS_LIST s_AuxiliaryList; // a picked list to store zones that are deleted or added when combined


void PCB_EDIT_FRAME::Edit_Zone_Params( ZONE_CONTAINER* aZone )
{
    int           dialogResult;
    ZONE_SETTINGS zoneInfo = GetZoneSettings();

    BOARD_COMMIT commit( this );

    // Save initial zones configuration, for undo/redo, before adding new zone
    // note the net name and the layer can be changed, so we must save all zones
    s_AuxiliaryList.ClearListAndDeleteItems();
    s_PickedList.ClearListAndDeleteItems();
    SaveCopyOfZones( s_PickedList, GetBoard(), -1, UNDEFINED_LAYER );

    if( aZone->GetIsKeepout() )
    {
        // edit a keepout area on a copper layer
        zoneInfo << *aZone;
        dialogResult = InvokeKeepoutAreaEditor( this, &zoneInfo );
    }
    else if( IsCopperLayer( aZone->GetLayer() ) )
    {
        // edit a zone on a copper layer
        zoneInfo << *aZone;
        dialogResult = InvokeCopperZonesEditor( this, &zoneInfo );
    }
    else
    {
        zoneInfo << *aZone;
        dialogResult = InvokeNonCopperZonesEditor( this, &zoneInfo );
    }

    if( dialogResult == wxID_CANCEL )
    {
        s_AuxiliaryList.ClearListAndDeleteItems();
        s_PickedList.ClearListAndDeleteItems();
        return;
    }

    SetZoneSettings( zoneInfo );
    OnModify();

    if( dialogResult == ZONE_EXPORT_VALUES )
    {
        UpdateCopyOfZonesList( s_PickedList, s_AuxiliaryList, GetBoard() );
        commit.Stage( s_PickedList );
        commit.Push( _( "Modify zone properties" ) );
        s_PickedList.ClearItemsList(); // s_ItemsListPicker is no more owner of picked items
        return;
    }

    wxBusyCursor dummy;

    // Undraw old zone outlines
    for( int ii = 0; ii < GetBoard()->GetAreaCount(); ii++ )
    {
        ZONE_CONTAINER* edge_zone = GetBoard()->GetArea( ii );
        GetCanvas()->GetView()->Update( edge_zone );
    }

    zoneInfo.ExportSetting( *aZone );

    NETINFO_ITEM* net = GetBoard()->FindNet( zoneInfo.m_NetcodeSelection );

    if( net )   // net == NULL should not occur
        aZone->SetNetCode( net->GetNet() );

    // Combine zones if possible
    GetBoard()->OnAreaPolygonModified( &s_AuxiliaryList, aZone );

    UpdateCopyOfZonesList( s_PickedList, s_AuxiliaryList, GetBoard() );

    // refill zones with the new properties applied
    std::vector<ZONE_CONTAINER*> zones_to_refill;

    for( unsigned i = 0; i < s_PickedList.GetCount(); ++i )
    {
        ZONE_CONTAINER* zone = dyn_cast<ZONE_CONTAINER*>( s_PickedList.GetPickedItem( i ) );

        if( zone == nullptr )
        {
            wxASSERT_MSG( false, "Expected a zone after zone properties edit" );
            continue;
        }

        if( zone->IsFilled() )
            zones_to_refill.push_back( zone );
    }

    if( zones_to_refill.size() )
    {
        ZONE_FILLER filler ( GetBoard() );
        wxString title = wxString::Format( _( "Refill %d Zones" ), (int) zones_to_refill.size() );
        filler.InstallNewProgressReporter( this, title, 4 );
        filler.Fill( zones_to_refill );
    }

    commit.Stage( s_PickedList );
    commit.Push( _( "Modify zone properties" ) );
    GetBoard()->GetConnectivity()->RecalculateRatsnest();

    s_PickedList.ClearItemsList();  // s_ItemsListPicker is no longer owner of picked items
}
