/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007-2016 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <board_design_settings.h>
#include <pcb_edit_frame.h>
#include <pcbnew_id.h>
#include <pcb_track.h>
#include <tools/pcb_actions.h>
#include <tool/tool_manager.h>
#include <wx/choice.h>

void PCB_EDIT_FRAME::SetTrackSegmentWidth( PCB_TRACK*         aTrackItem,
                                           PICKED_ITEMS_LIST* aItemsListPicker,
                                           bool               aUseNetclassValue )
{
    int initial_width;
    int new_width;
    int initial_drill = -1;
    int new_drill = -1;

    initial_width = aTrackItem->GetWidth();

    if( aUseNetclassValue )
        new_width = aTrackItem->GetNetClass()->GetTrackWidth();
    else
        new_width = GetDesignSettings().GetCurrentTrackWidth();

    if( aTrackItem->Type() == PCB_VIA_T )
    {
        const PCB_VIA *via = static_cast<const PCB_VIA*>( aTrackItem );

        // Get the drill value, regardless it is default or specific
        initial_drill = via->GetDrillValue();

        if( aUseNetclassValue || via->GetViaType() == VIATYPE::MICROVIA )
        {
            new_width = aTrackItem->GetNetClass()->GetViaDiameter();
            new_drill = aTrackItem->GetNetClass()->GetViaDrill();
        }
        else
        {
            new_width = GetDesignSettings().GetCurrentViaSize();
            new_drill = GetDesignSettings().GetCurrentViaDrill();
        }

        // Old versions set a drill value <= 0, when the default netclass it used
        // but it could be better to set the drill value to the actual value
        // to avoid issues for existing vias, if the default drill value is modified
        // in the netclass, and not in current vias.
        if( via->GetDrill() <= 0 )      // means default netclass drill value used
        {
            initial_drill  = -1;        // Force drill vias re-initialization
        }
    }

    if( initial_width != new_width || initial_drill != new_drill )
    {
        OnModify();

        if( aItemsListPicker )
        {
            aTrackItem->SetWidth( initial_width );
            ITEM_PICKER picker( nullptr, aTrackItem, UNDO_REDO::CHANGED );
            picker.SetLink( aTrackItem->Clone() );
            aItemsListPicker->PushItem( picker );
            aTrackItem->SetWidth( new_width );

            if( aTrackItem->Type() == PCB_VIA_T )
            {
                // Set new drill value. Note: currently microvias have only a default drill value
                PCB_VIA *via = static_cast<PCB_VIA*>( aTrackItem );

                if( new_drill > 0 )
                    via->SetDrill( new_drill );
                else
                    via->SetDrillDefault();
            }
        }
    }
}


void PCB_EDIT_FRAME::Tracks_and_Vias_Size_Event( wxCommandEvent& event )
{
    int ii;
    int id = event.GetId();

    switch( id )
    {
    case ID_AUX_TOOLBAR_PCB_SELECT_AUTO_WIDTH:
    {
        if( GetDesignSettings().UseCustomTrackViaSize() )
        {
            GetDesignSettings().UseCustomTrackViaSize( false );
            GetDesignSettings().m_UseConnectedTrackWidth = true;
        }
        else
        {
            GetDesignSettings().m_UseConnectedTrackWidth =
                    not GetDesignSettings().m_UseConnectedTrackWidth;
        }

        break;
    }

    case ID_POPUP_PCB_SELECT_USE_NETCLASS_VALUES:
        GetDesignSettings().m_UseConnectedTrackWidth = false;
        GetDesignSettings().SetTrackWidthIndex( 0 );
        GetDesignSettings().SetViaSizeIndex( 0 );
        break;

    case ID_POPUP_PCB_SELECT_AUTO_WIDTH:
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
        ii = id - ID_POPUP_PCB_SELECT_VIASIZE1;
        GetDesignSettings().SetViaSizeIndex( ii );
        break;

    case ID_AUX_TOOLBAR_PCB_TRACK_WIDTH:
        ii = m_SelTrackWidthBox->GetSelection();

        if( ii == int( m_SelTrackWidthBox->GetCount() - 2 ) )
        {
            // this is the separator
        }
        else if( ii == int( m_SelTrackWidthBox->GetCount() - 1 ) )
        {
            m_SelTrackWidthBox->SetSelection( GetDesignSettings().GetTrackWidthIndex() );
            ShowBoardSetupDialog( _( "Pre-defined Sizes" ) );
        }
        else
        {
            GetDesignSettings().SetTrackWidthIndex( ii );
        }

        break;

    case ID_AUX_TOOLBAR_PCB_VIA_SIZE:
        ii = m_SelViaSizeBox->GetSelection();

        if( ii == int( m_SelViaSizeBox->GetCount() - 2 ) )
        {
            // this is the separator
            m_SelViaSizeBox->SetSelection( GetDesignSettings().GetViaSizeIndex() );
        }
        else if( ii == int( m_SelViaSizeBox->GetCount() - 1 ) )
        {
            m_SelViaSizeBox->SetSelection( GetDesignSettings().GetViaSizeIndex() );
            ShowBoardSetupDialog( _( "Pre-defined Sizes" ) );
        }
        else
        {
            GetDesignSettings().SetViaSizeIndex( ii );
        }

        break;

    default:
        break;
    }

    m_toolManager->RunAction( PCB_ACTIONS::trackViaSizeChanged, true );
}
