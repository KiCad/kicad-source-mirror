/*
 * @file zones_by_polygon_fill_functions.cpp
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 Jean-Pierre Charras jp.charras at wanadoo.fr
 * Copyright (C) 2018 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <pcb_edit_frame.h>
#include <macros.h>

#include <tool/tool_manager.h>
#include <tools/pcb_actions.h>

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
void PCB_EDIT_FRAME::Delete_OldZone_Fill( SEGZONE* aZone, timestamp_t aTimestamp )
{
    bool          modify  = false;
    timestamp_t   TimeStamp;

    if( aZone == NULL )
        TimeStamp = aTimestamp;
    else
        TimeStamp = aZone->GetTimeStamp(); // Save reference time stamp (aZone will be deleted)

    // SEGZONE is a deprecated item, only used for compatibility with very old boards
    SEGZONE* next;

    for( SEGZONE* zone = GetBoard()->m_SegZoneDeprecated; zone != NULL; zone = next )
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


int PCB_EDIT_FRAME::Fill_All_Zones( wxWindow* aActiveWindow )
{
    auto toolMgr = GetToolManager();
    wxCHECK( toolMgr, 1 );
    toolMgr->RunAction( PCB_ACTIONS::zoneFillAll, true );
    return 0;
}


void PCB_EDIT_FRAME::Check_All_Zones( wxWindow* aActiveWindow )
{
    if( !m_ZoneFillsDirty )
        return;

    std::vector<ZONE_CONTAINER*> toFill;

    for( auto zone : GetBoard()->Zones() )
        toFill.push_back(zone);

    BOARD_COMMIT commit( this );

    std::unique_ptr<WX_PROGRESS_REPORTER> progressReporter(
            new WX_PROGRESS_REPORTER( aActiveWindow, _( "Checking Zones" ), 4 ) );

    ZONE_FILLER filler( GetBoard(), &commit, aActiveWindow );
    filler.SetProgressReporter( progressReporter.get() );

    if( filler.Fill( toFill, true ) )
    {
        m_ZoneFillsDirty = false;

        if( IsGalCanvasActive() && GetGalCanvas() )
            GetGalCanvas()->ForceRefresh();

        GetCanvas()->Refresh();
    }
}
