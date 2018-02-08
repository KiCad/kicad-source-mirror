/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009-2018 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2018 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file globaleditpad.cpp
 */

#include <fctsys.h>
#include <common.h>
#include <class_drawpanel.h>
#include <confirm.h>
#include <pcb_edit_frame.h>
#include <footprint_edit_frame.h>

#include <class_board.h>
#include <class_module.h>

#include <pcbnew.h>
#include <dialog_global_pads_edition.h>

/*
 * PCB_EDIT_FRAME::Function DlgGlobalChange_PadSettings
 * Function to change pad caracteristics for the given footprint
 * or alls footprints which look like the given footprint
 * Options are set by the opened dialog.
 * aPad is the pattern. The given footprint is the parent of this pad
 * aRedraw: if true: redraws the footprint
 */
void PCB_EDIT_FRAME::DlgGlobalChange_PadSettings( D_PAD* aPad, bool aRedraw )
{
    int     diag;

    if( aPad == NULL )
        aPad = &GetDesignSettings().m_Pad_Master;

    MODULE* module = aPad->GetParent();

    if( module == NULL )
    {
        DisplayError( this, wxT( "Global_Import_Pad_Settings() Error: NULL module" ) );
        return;
    }

    SetMsgPanel( module );

    {
        DIALOG_GLOBAL_PADS_EDITION dlg( this, aPad );

        diag = dlg.ShowModal();
    }

    if( diag == -1 )
        return;

    bool edit_Same_Modules = false;
    if( diag == 1 )
        edit_Same_Modules = true;

    GlobalChange_PadSettings( aPad,edit_Same_Modules,
                              DIALOG_GLOBAL_PADS_EDITION::m_Pad_Shape_Filter,
                              DIALOG_GLOBAL_PADS_EDITION::m_Pad_Orient_Filter,
                              DIALOG_GLOBAL_PADS_EDITION::m_Pad_Layer_Filter,
                              aRedraw, true );
}

/*
 * FOOTPRINT_EDIT_FRAME::Function DlgGlobalChange_PadSettings
 * Function to change pad caracteristics for the given footprint
 * or alls footprints which look like the given footprint
 * Options are set by the opened dialog.
 * aPad is the pattern. The given footprint is the parent of this pad
 */
void FOOTPRINT_EDIT_FRAME::DlgGlobalChange_PadSettings( D_PAD* aPad )
{
    int     diag;

    if( aPad == NULL )
        aPad = &GetDesignSettings().m_Pad_Master;

    MODULE* module = aPad->GetParent();

    if( module == NULL )
    {
        DisplayError( this, wxT( "Global_Import_Pad_Settings() Error: NULL module" ) );
        return;
    }

    SetMsgPanel( module );

    {
        DIALOG_GLOBAL_PADS_EDITION dlg( this, aPad );

        dlg.m_buttonIdModules->Enable( false );

        diag = dlg.ShowModal();
    }

    if( diag == -1 )
        return;

    bool edit_Same_Modules = false;
    if( diag == 1 )
        edit_Same_Modules = true;

    GlobalChange_PadSettings( aPad, edit_Same_Modules,
                              DIALOG_GLOBAL_PADS_EDITION::m_Pad_Shape_Filter,
                              DIALOG_GLOBAL_PADS_EDITION::m_Pad_Orient_Filter,
                              DIALOG_GLOBAL_PADS_EDITION::m_Pad_Layer_Filter,
                              true, false );
}

/*
 * Function GlobalChange_PadSettings
 * Function to change pad caracteristics for the given footprint
 * or alls footprints which look like the given footprint
 * aPad is the pattern. The given footprint is the parent of this pad
 * aSameFootprints: if true, make changes on all identical footprints
 * aPadShapeFilter: if true, make changes only on pads having the same shape as aPad
 * aPadOrientFilter: if true, make changes only on pads having the same orientation as aPad
 * aPadLayerFilter: if true, make changes only on pads having the same layers as aPad
 * aRedraw: if true: redraws the footprint
 * aSaveForUndo: if true: create an entry in the Undo/Redo list
 *        (usually: true in Schematic editor, false in Module editor)
 */
void PCB_BASE_FRAME::GlobalChange_PadSettings( D_PAD* aPad,
                                               bool aSameFootprints,
                                               bool aPadShapeFilter,
                                               bool aPadOrientFilter,
                                               bool aPadLayerFilter,
                                               bool aRedraw, bool aSaveForUndo )
{
    if( aPad == NULL )
        aPad = &GetDesignSettings().m_Pad_Master;

    MODULE* module = aPad->GetParent();

    if( module == NULL )
    {
        DisplayError( this, wxT( "Global_Import_Pad_Settings() Error: NULL module" ) );
        return;
    }

    // Search and copy the name of library reference.
    MODULE* Module_Ref = module;
    double pad_orient = aPad->GetOrientation() - Module_Ref->GetOrientation();

    // Prepare an undo list:
    if( aSaveForUndo )
    {
        PICKED_ITEMS_LIST itemsList;

        for( module = m_Pcb->m_Modules;  module;  module = module->Next() )
        {
            if( !aSameFootprints && (module != Module_Ref) )
                continue;

            if( module->GetFPID() != Module_Ref->GetFPID() )
                continue;

            bool   saveMe = false;

            for( D_PAD* pad = module->PadsList(); pad; pad = pad->Next() )
            {
                // Filters changes prohibited.
                if( aPadShapeFilter && ( pad->GetShape() != aPad->GetShape() ) )
                    continue;

                double currpad_orient = pad->GetOrientation() - module->GetOrientation();

                if( aPadOrientFilter && ( currpad_orient != pad_orient ) )
                    continue;

                if( aPadLayerFilter  &&  pad->GetLayerSet() != aPad->GetLayerSet() )
                    continue;

                // Do not copy a pad into it, it can create issue, and this is useless
                if( pad == aPad )
                    continue;

                saveMe = true;
            }

            if( saveMe )
            {
                ITEM_PICKER itemWrapper( module, UR_CHANGED );

                itemsList.PushItem( itemWrapper );
            }
        }

        SaveCopyInUndoList( itemsList, UR_CHANGED );
    }

    // Update the current module and same others modules if requested.
    for( module = m_Pcb->m_Modules;  module;  module = module->Next() )
    {
        if( !aSameFootprints && (module != Module_Ref) )
            continue;

        if( module->GetFPID() != Module_Ref->GetFPID() )
            continue;

        // Erase module on screen
        if( aRedraw )
        {
            module->SetFlags( DO_NOT_DRAW );
            m_canvas->RefreshDrawingRect( module->GetBoundingBox() );
            module->ClearFlags( DO_NOT_DRAW );
        }

        for( D_PAD* pad = module->PadsList();  pad;  pad = pad->Next() )
        {
            // Filters changes prohibited.
            if( aPadShapeFilter && ( pad->GetShape() != aPad->GetShape() ) )
                continue;

            if( aPadOrientFilter &&
                (pad->GetOrientation() - module->GetOrientation()) != pad_orient )
                continue;

            if( aPadLayerFilter )
            {
                if( pad->GetLayerSet() != aPad->GetLayerSet() )
                    continue;
            }

            // Do not copy a pad into it, it can create issue in custom pads
            // and primitive list.
            if( pad == aPad )
                continue;

            // Copy physical characteristics:
            pad->ImportSettingsFromMaster( *aPad );

            // copy also local mask margins:
            pad->SetLocalSolderMaskMargin( aPad->GetLocalSolderMaskMargin() );
            pad->SetLocalSolderPasteMargin( aPad->GetLocalSolderPasteMargin() );
            pad->SetLocalSolderPasteMarginRatio( aPad->GetLocalSolderPasteMarginRatio() );
        }

        module->CalculateBoundingBox();

        if( aRedraw )
            m_canvas->RefreshDrawingRect( module->GetBoundingBox() );
    }

    OnModify();
}
