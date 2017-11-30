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
#include <class_draw_panel_gal.h>
#include <ratsnest_data.h>
#include <wxPcbStruct.h>
#include <macros.h>

#include <class_board.h>
#include <class_track.h>
#include <class_zone.h>

#include <pcbnew.h>
#include <zones.h>

#include <connectivity_data.h>
#include <board_commit.h>

#include <widgets/progress_reporter.h>
#include <zone_filler.h>


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

int PCB_EDIT_FRAME::Fill_All_Zones( wxWindow * aActiveWindow, bool aVerbose )
{
    std::vector<ZONE_CONTAINER*> toFill;

    BOARD_COMMIT commit( this );

    for( auto zone : GetBoard()->Zones() )
    {
        toFill.push_back(zone);
    }

    std::unique_ptr<WX_PROGRESS_REPORTER> progressReporter(
            new WX_PROGRESS_REPORTER( aActiveWindow, _( "Fill All Zones" ), 3 )
            );

    ZONE_FILLER filler( GetBoard() );
    filler.SetProgressReporter( progressReporter.get() );
    filler.Fill( toFill );

    return 0;
}
