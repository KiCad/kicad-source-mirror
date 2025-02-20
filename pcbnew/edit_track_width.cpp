/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007-2016 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

void PCB_EDIT_FRAME::SetTrackSegmentWidth( PCB_TRACK* aItem, PICKED_ITEMS_LIST* aItemsListPicker,
                                           bool aUseDesignRules )
{
    PCB_VIA* via = dynamic_cast<PCB_VIA*>( aItem );
    int      new_width = -1;
    int      new_drill = -1;

    if( aUseDesignRules )
    {
        MINOPTMAX<int> constraint = aItem->GetWidthConstraint();

        if( constraint.HasOpt() )
            new_width = constraint.Opt();
        else if( constraint.Min() > 0 )
            new_width = constraint.Min();

        if( via )
        {
            constraint = via->GetDrillConstraint();

            if( constraint.HasOpt() )
                new_drill = constraint.Opt();
            else if( constraint.Min() > 0 )
                new_drill = constraint.Min();
        }
    }
    else if( via && via->GetViaType() == VIATYPE::MICROVIA )
    {
        new_width = aItem->GetEffectiveNetClass()->GetuViaDiameter();
        new_drill = aItem->GetEffectiveNetClass()->GetuViaDrill();
    }
    else if( via )
    {
        new_width = GetDesignSettings().GetCurrentViaSize();
        new_drill = GetDesignSettings().GetCurrentViaDrill();
    }
    else
    {
        new_width = GetDesignSettings().GetCurrentTrackWidth();
    }

    if( new_width <= 0 )
        new_width = aItem->GetWidth();

    if( via && new_drill <= 0 )
        new_drill = via->GetDrillValue();

    if( aItem->GetWidth() != new_width || ( via && via->GetDrillValue() != new_drill ) )
    {
        ITEM_PICKER picker( nullptr, aItem, UNDO_REDO::CHANGED );
        picker.SetLink( aItem->Clone() );
        aItemsListPicker->PushItem( picker );

        aItem->SetWidth( new_width );

        if( via && new_drill > 0 )
            via->SetDrill( new_drill );
    }
}


void PCB_EDIT_FRAME::Tracks_and_Vias_Size_Event( wxCommandEvent& event )
{
    int ii;
    int id = event.GetId();

    switch( id )
    {
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
            m_SelTrackWidthBox->SetSelection( GetDesignSettings().GetTrackWidthIndex() );
        }
        else if( ii == int( m_SelTrackWidthBox->GetCount() - 1 ) )
        {
            m_SelTrackWidthBox->SetSelection( GetDesignSettings().GetTrackWidthIndex() );
            ShowBoardSetupDialog( _( "Pre-defined Sizes" ) );
        }
        else
        {
            GetDesignSettings().SetTrackWidthIndex( ii );
            GetDesignSettings().m_TempOverrideTrackWidth = true;
        }

        // Needed on Windows because the canvas loses focus after clicking on m_SelTrackWidthBox:
        GetCanvas()->SetFocus();

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

        // Needed on Windows because the canvas loses focus after clicking on m_SelViaSizeBox:
        GetCanvas()->SetFocus();

        break;

    default:
        break;
    }

    m_toolManager->RunAction( PCB_ACTIONS::trackViaSizeChanged );
}
