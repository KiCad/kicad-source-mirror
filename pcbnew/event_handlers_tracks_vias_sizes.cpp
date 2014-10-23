/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009-2014 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2012 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file event_handlers_tracks_vias_sizes.cpp
 * @brief Handlers for popup and toolbars events relative to the tracks and vias sizes.
 */


#include <fctsys.h>
#include <class_drawpanel.h>
#include <confirm.h>
#include <wxPcbStruct.h>
#include <dialog_helpers.h>

#include <pcbnew_id.h>
#include <pcbnew.h>

#include <class_board.h>
#include <class_module.h>


/**
 * Function Tracks_and_Vias_Size_Event
 * Event handler for tracks and vias size selection (and some options)
 * relative to toolbars and popup events
 */
void PCB_EDIT_FRAME::Tracks_and_Vias_Size_Event( wxCommandEvent& event )
{
    int ii;
    int id = event.GetId();

/* Note: none of these events require aborting the current command (if any)
 * (like move, edit or block command)
 * so we do not test for a current command in progress and call
 *  m_canvas->m_endMouseCaptureCallback( m_canvas, &dc );
 */
    switch( id )
    {
    case ID_AUX_TOOLBAR_PCB_SELECT_AUTO_WIDTH:
        GetDesignSettings().m_UseConnectedTrackWidth =
            not GetDesignSettings().m_UseConnectedTrackWidth;
        break;

    case ID_POPUP_PCB_SELECT_USE_NETCLASS_VALUES:
        GetDesignSettings().m_UseConnectedTrackWidth = false;
        GetDesignSettings().SetTrackWidthIndex( 0 );
        GetDesignSettings().SetViaSizeIndex( 0 );
        break;

    case ID_POPUP_PCB_SELECT_AUTO_WIDTH:
        m_canvas->MoveCursorToCrossHair();
        GetDesignSettings().m_UseConnectedTrackWidth = true;
        break;

    case ID_POPUP_PCB_SELECT_WIDTH1:      // this is the default Netclass selection
    case ID_POPUP_PCB_SELECT_WIDTH2:      // this is a custom value selection
    case ID_POPUP_PCB_SELECT_WIDTH3:
    case ID_POPUP_PCB_SELECT_WIDTH4:
    case ID_POPUP_PCB_SELECT_WIDTH5:
    case ID_POPUP_PCB_SELECT_WIDTH6:
    case ID_POPUP_PCB_SELECT_WIDTH7:
    case ID_POPUP_PCB_SELECT_WIDTH8:
    case ID_POPUP_PCB_SELECT_WIDTH9:
    case ID_POPUP_PCB_SELECT_WIDTH10:
    case ID_POPUP_PCB_SELECT_WIDTH11:
    case ID_POPUP_PCB_SELECT_WIDTH12:
    case ID_POPUP_PCB_SELECT_WIDTH13:
    case ID_POPUP_PCB_SELECT_WIDTH14:
    case ID_POPUP_PCB_SELECT_WIDTH15:
    case ID_POPUP_PCB_SELECT_WIDTH16:
        m_canvas->MoveCursorToCrossHair();
        GetDesignSettings().m_UseConnectedTrackWidth = false;
        ii = id - ID_POPUP_PCB_SELECT_WIDTH1;
        GetDesignSettings().SetTrackWidthIndex( ii );
        break;

    case ID_POPUP_PCB_SELECT_VIASIZE1:   // this is the default Netclass selection
    case ID_POPUP_PCB_SELECT_VIASIZE2:   // this is a custom value selection
    case ID_POPUP_PCB_SELECT_VIASIZE3:
    case ID_POPUP_PCB_SELECT_VIASIZE4:
    case ID_POPUP_PCB_SELECT_VIASIZE5:
    case ID_POPUP_PCB_SELECT_VIASIZE6:
    case ID_POPUP_PCB_SELECT_VIASIZE7:
    case ID_POPUP_PCB_SELECT_VIASIZE8:
    case ID_POPUP_PCB_SELECT_VIASIZE9:
    case ID_POPUP_PCB_SELECT_VIASIZE10:
    case ID_POPUP_PCB_SELECT_VIASIZE11:
    case ID_POPUP_PCB_SELECT_VIASIZE12:
    case ID_POPUP_PCB_SELECT_VIASIZE13:
    case ID_POPUP_PCB_SELECT_VIASIZE14:
    case ID_POPUP_PCB_SELECT_VIASIZE15:
    case ID_POPUP_PCB_SELECT_VIASIZE16:
        // select the new current value for via size (via diameter)
        m_canvas->MoveCursorToCrossHair();
        ii = id - ID_POPUP_PCB_SELECT_VIASIZE1;
        GetDesignSettings().SetViaSizeIndex( ii );
        break;

    case ID_AUX_TOOLBAR_PCB_TRACK_WIDTH:
        ii = m_SelTrackWidthBox->GetCurrentSelection();
        GetDesignSettings().SetTrackWidthIndex( ii );
        break;

    case ID_AUX_TOOLBAR_PCB_VIA_SIZE:
        ii = m_SelViaSizeBox->GetCurrentSelection();
        GetDesignSettings().SetViaSizeIndex( ii );
        break;

    default:
        wxMessageBox( wxT( "PCB_EDIT_FRAME::Tracks_and_Vias_Size_Event() error") );
        break;
    }

    // Refresh track in progress, if any, by forcing a mouse event,
    // to call the current function attached to the mouse
    /*if( m_canvas->IsMouseCaptured() )
    {
        wxMouseEvent event(wxEVT_MOTION);
        wxPostEvent( m_canvas, event );
    }*/
    //+hp
    //Refresh canvas, that we can see changes instantly. I use this because it dont,t throw  mouse up-left corner.
    m_canvas->Refresh();
}
