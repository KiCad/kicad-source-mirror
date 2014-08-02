/*
 * @file zones_by_polygon_fill_functions.cpp
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 Jean-Pierre Charras <jean-pierre.charras@gipsa-lab.inpg.fr>
 * Copyright (C) 2007 KiCad Developers, see change_log.txt for contributors.
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

#include <wx/progdlg.h>

#include <fctsys.h>
#include <pgm_base.h>
#include <class_drawpanel.h>
#include <wxPcbStruct.h>
#include <macros.h>

#include <class_board.h>
#include <class_track.h>
#include <class_zone.h>

#include <pcbnew.h>
#include <zones.h>

#define FORMAT_STRING _( "Filling zone %d out of %d (net %s)..." )


/**
 * Function Delete_OldZone_Fill (obsolete)
 * Used for compatibility with old boards
 * Remove the zone filling which include the segment aZone, or the zone which have the
 * given time stamp.
 * A zone is a group of segments which have the same TimeStamp
 * @param aZone = zone segment within the zone to delete. Can be NULL
 * @param aTimestamp = Timestamp for the zone to delete, used if aZone == NULL
 */
void PCB_EDIT_FRAME::Delete_OldZone_Fill( SEGZONE* aZone, time_t aTimestamp )
{
    bool          modify  = false;
    time_t        TimeStamp;

    if( aZone == NULL )
        TimeStamp = aTimestamp;
    else
        TimeStamp = aZone->GetTimeStamp(); // Save reference time stamp (aZone will be deleted)

    SEGZONE* next;

    for( SEGZONE* zone = GetBoard()->m_Zone; zone != NULL; zone = next )
    {
        next = zone->Next();

        if( zone->GetTimeStamp() == TimeStamp )
        {
            modify = true;
            // remove item from linked list and free memory
            zone->DeleteStructure();
        }
    }

    if( modify )
    {
        OnModify();
        m_canvas->Refresh();
    }
}


int PCB_EDIT_FRAME::Fill_Zone( ZONE_CONTAINER* aZone )
{
    aZone->ClearFilledPolysList();
    aZone->UnFill();

    // Cannot fill keepout zones:
    if( aZone->GetIsKeepout() )
        return 1;

    wxString msg;

    ClearMsgPanel();

    // Shows the net
    ZONE_SETTINGS zoneInfo = GetZoneSettings();
    zoneInfo.m_NetcodeSelection = aZone->GetNetCode();
    SetZoneSettings( zoneInfo );

    msg = aZone->GetNetname();

    if( msg.IsEmpty() )
        msg = wxT( "No net" );

    AppendMsgPanel( _( "NetName" ), msg, RED );

    wxBusyCursor dummy;     // Shows an hourglass cursor (removed by its destructor)

    aZone->BuildFilledSolidAreasPolygons( GetBoard() );

    OnModify();

    return 0;
}


int PCB_EDIT_FRAME::Fill_All_Zones( wxWindow * aActiveWindow, bool aVerbose )
{
    int errorLevel = 0;
    int areaCount = GetBoard()->GetAreaCount();
    wxBusyCursor dummyCursor;
    wxString msg;
    wxProgressDialog * progressDialog = NULL;

    // Create a message with a long net name, and build a wxProgressDialog
    // with a correct size to show this long net name
    msg.Printf( FORMAT_STRING, 000, areaCount, wxT("XXXXXXXXXXXXXXXXX" ) );
    if( aActiveWindow )
        progressDialog = new wxProgressDialog( _( "Fill All Zones" ), msg,
                                     areaCount+2, aActiveWindow,
                                     wxPD_AUTO_HIDE | wxPD_CAN_ABORT );
    // Display the actual message
    if( progressDialog )
        progressDialog->Update( 0, _( "Starting zone fill..." ) );

    // Remove segment zones
    GetBoard()->m_Zone.DeleteAll();

    int ii;

    for( ii = 0; ii < areaCount; ii++ )
    {
        ZONE_CONTAINER* zoneContainer = GetBoard()->GetArea( ii );
        if( zoneContainer->GetIsKeepout() )
            continue;

        msg.Printf( FORMAT_STRING, ii + 1, areaCount, GetChars( zoneContainer->GetNetname() ) );

        if( progressDialog )
        {
            if( !progressDialog->Update( ii+1, msg ) )
                break;  // Aborted by user
        }

        errorLevel = Fill_Zone( zoneContainer );

        if( errorLevel && !aVerbose )
            break;
    }

    if( progressDialog )
        progressDialog->Update( ii+2, _( "Updating ratsnest..." ) );
    TestConnections();

    // Recalculate the active ratsnest, i.e. the unconnected links
    TestForActiveLinksInRatsnest( 0 );
    if( progressDialog )
        progressDialog->Destroy();
    return errorLevel;
}
