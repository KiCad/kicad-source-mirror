/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2015 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2015 Wayne Stambaugh <stambaughw@verizon.net>
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

#include <kiface_base.h>
#include <confirm.h>
#include <gestfich.h>
#include <pcb_edit_frame.h>
#include <pcbnew_id.h>
#include <board.h>
#include <footprint.h>
#include <pad.h>
#include <zone.h>
#include <pcb_group.h>
#include <pcb_generator.h>
#include <pcb_target.h>
#include <pcb_dimension.h>
#include <pcb_textbox.h>
#include <pcb_table.h>
#include <pcb_shape.h>
#include <dialog_drc.h>
#include <connectivity/connectivity_data.h>
#include <tool/tool_manager.h>
#include <tools/pcb_actions.h>
#include <tools/drc_tool.h>
#include <dialogs/dialog_dimension_properties.h>
#include <dialogs/dialog_table_properties.h>
#include <pcb_layer_box_selector.h>

// Handles the selection of command events.
void PCB_EDIT_FRAME::Process_Special_Functions( wxCommandEvent& event )
{
    switch( event.GetId() )   // Execute command
    {
    case ID_MENU_EXPORT_FOOTPRINTS_TO_LIBRARY:
        ExportFootprintsToLibrary( false );
        break;

    case ID_MENU_EXPORT_FOOTPRINTS_TO_NEW_LIBRARY:
        ExportFootprintsToLibrary( true );
        break;

    default:
        break;
    }
}


void PCB_EDIT_FRAME::SwitchLayer( PCB_LAYER_ID layer )
{
    PCB_LAYER_ID curLayer = GetActiveLayer();
    const PCB_DISPLAY_OPTIONS& displ_opts = GetDisplayOptions();

    // Check if the specified layer matches the present layer
    if( layer == curLayer )
        return;

    // Copper layers cannot be selected unconditionally; how many of those layers are currently
    // enabled needs to be checked.
    if( IsCopperLayer( layer ) )
    {
        if( layer > GetBoard()->GetCopperLayerStackMaxId() )
            return;
    }

    // Is yet more checking required? E.g. when the layer to be selected is a non-copper layer,
    // or when switching between a copper layer and a non-copper layer, or vice-versa?

    SetActiveLayer( layer );

    if( displ_opts.m_ContrastModeDisplay != HIGH_CONTRAST_MODE::NORMAL )
        GetCanvas()->Refresh();
}


void PCB_EDIT_FRAME::OnEditItemRequest( BOARD_ITEM* aItem )
{
    switch( aItem->Type() )
    {
    case PCB_REFERENCE_IMAGE_T:
        ShowReferenceImagePropertiesDialog( aItem );
        break;

    case PCB_FIELD_T:
    case PCB_TEXT_T:
        ShowTextPropertiesDialog( static_cast<PCB_TEXT*>( aItem ) );
        break;

    case PCB_TEXTBOX_T:
        ShowTextBoxPropertiesDialog( static_cast<PCB_TEXTBOX*>( aItem ) );
        break;

    case PCB_TABLE_T:
    {
        DIALOG_TABLE_PROPERTIES dlg( this, static_cast<PCB_TABLE*>( aItem ) );

        //QuasiModal required for Scintilla auto-complete
        dlg.ShowQuasiModal();
        break;
    }

    case PCB_PAD_T:
        ShowPadPropertiesDialog( static_cast<PAD*>( aItem ) );
        break;

    case PCB_FOOTPRINT_T:
        ShowFootprintPropertiesDialog( static_cast<FOOTPRINT*>( aItem ) );
        break;

    case PCB_TARGET_T:
        ShowTargetOptionsDialog( static_cast<PCB_TARGET*>( aItem ) );
        break;

    case PCB_DIM_ALIGNED_T:
    case PCB_DIM_CENTER_T:
    case PCB_DIM_RADIAL_T:
    case PCB_DIM_ORTHOGONAL_T:
    case PCB_DIM_LEADER_T:
    {
        DIALOG_DIMENSION_PROPERTIES dlg( this, static_cast<PCB_DIMENSION_BASE*>( aItem ) );

        // TODO: why is this QuasiModal?
        dlg.ShowQuasiModal();
        break;
    }

    case PCB_SHAPE_T:
        ShowGraphicItemPropertiesDialog( static_cast<PCB_SHAPE*>( aItem ) );
        break;

    case PCB_ZONE_T:
        Edit_Zone_Params( static_cast<ZONE*>( aItem ) );
        break;

    case PCB_GROUP_T:
        m_toolManager->RunAction( ACTIONS::groupProperties,
                                  static_cast<EDA_GROUP*>( static_cast<PCB_GROUP*>( aItem ) ) );
        break;

    case PCB_GENERATOR_T:
        static_cast<PCB_GENERATOR*>( aItem )->ShowPropertiesDialog( this );
        break;

    case PCB_MARKER_T:
        m_toolManager->GetTool<DRC_TOOL>()->CrossProbe( static_cast<PCB_MARKER*>( aItem ) );
        break;

    case PCB_POINT_T:
        break;

    default:
        break;
    }
}

