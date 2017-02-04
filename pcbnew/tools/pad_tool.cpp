/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 KiCad Developers, see AUTHORS.txt for contributors.
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


#include "pad_tool.h"

#include <wxPcbStruct.h>
#include <class_draw_panel_gal.h>
#include <view/view_controls.h>
#include <view/view.h>
#include <tool/tool_manager.h>

#include <class_board_item.h>
#include <class_module.h>
#include <board_commit.h>

#include <dialogs/dialog_global_pads_edition.h>

#include "common_actions.h"
#include "selection_tool.h"
#include "selection_conditions.h"
#include "edit_tool.h"

class PAD_CONTEXT_MENU : public CONTEXT_MENU
{
public:
    PAD_CONTEXT_MENU()
    {
        SetIcon( pad_xpm );
        SetTitle( _( "Pads" ) );

        Add( COMMON_ACTIONS::importPadSettings );
        Add( COMMON_ACTIONS::exportPadSettings );
        Add( COMMON_ACTIONS::pushPadSettings );
    }

protected:

    CONTEXT_MENU* create() const override
    {
        return new PAD_CONTEXT_MENU();
    }

private:

    void update() override
    {
        auto selTool = getToolManager()->GetTool<SELECTION_TOOL>();
        const SELECTION& selection = selTool->GetSelection();

        auto anyPadSel = SELECTION_CONDITIONS::HasType( PCB_PAD_T );

        auto singlePadSel = SELECTION_CONDITIONS::Count( 1 )
                                && SELECTION_CONDITIONS::OnlyType( PCB_PAD_T );
        auto emptySel = SELECTION_CONDITIONS::Count( 0 );

        // Import pads enabled when any pads selected (it applies to each one
        // individually)
        const bool canImport = ( anyPadSel )( selection );
        Enable( getMenuId( COMMON_ACTIONS::importPadSettings ), canImport );

        // Export pads item enabled only when there is a single pad selected
        // (otherwise how would we know which one to export?)
        const bool canExport = ( singlePadSel )( selection );
        Enable( getMenuId( COMMON_ACTIONS::exportPadSettings ), canExport );

        // Push pads available when nothing selected, or a single pad
        const bool canPush = ( singlePadSel || emptySel ) ( selection );
        Enable( getMenuId( COMMON_ACTIONS::pushPadSettings ), canPush );
    }
};


PAD_TOOL::PAD_TOOL() :
        PCB_TOOL( "pcbnew.PadTool" )
{
}


PAD_TOOL::~PAD_TOOL()
{}


void PAD_TOOL::Reset( RESET_REASON aReason )
{
}


bool PAD_TOOL::Init()
{
    auto contextMenu = std::make_shared<PAD_CONTEXT_MENU>();
    contextMenu->SetTool( this );

    SELECTION_TOOL* selTool = m_toolMgr->GetTool<SELECTION_TOOL>();

    if( selTool )
    {
        auto& toolMenu = selTool->GetToolMenu();
        auto& menu = toolMenu.GetMenu();

        toolMenu.AddSubMenu( contextMenu );

        // show menu when any pads selected, or nothing selected
        // (push settings works on no selection)
        auto showCond = SELECTION_CONDITIONS::HasType( PCB_PAD_T )
                        || SELECTION_CONDITIONS::Count( 0 );

        menu.AddMenu( contextMenu.get(), false, showCond );
    }

    return true;
}


/**
 * Function doExportPadSettings
 *
 * Export a given pad to the destination pad. Normally, the destination
 * would be a board reference settings master pad.
 */
static void doExportPadSettings( const D_PAD& aSrc, D_PAD& aDest )
{
    // Copy all settings. Some of them are not used, but they break anything
    aDest = aSrc;

    // The pad orientation, for historical reasons is the
    // pad rotation + parent rotation.
    // store only the pad rotation.
    aDest.SetOrientation( aSrc.GetOrientation() - aSrc.GetParent()->GetOrientation() );
}


/**
 * Function doImportPadSettings
 *
 * Import pad settings from a "reference" source to a destination.
 * Often, the reference source will be a board reference settings
 * master pad.
 */
static void doImportPadSettings( const D_PAD& aSrc, D_PAD& aDest )
{
    const auto& destParent = *aDest.GetParent();

    aDest.SetShape( aSrc.GetShape() );
    aDest.SetLayerSet( aSrc.GetLayerSet() );
    aDest.SetAttribute( aSrc.GetAttribute() );
    aDest.SetOrientation( aSrc.GetOrientation() + destParent.GetOrientation() );
    aDest.SetSize( aSrc.GetSize() );
    aDest.SetDelta( wxSize( 0, 0 ) );
    aDest.SetOffset( aSrc.GetOffset() );
    aDest.SetDrillSize( aSrc.GetDrillSize() );
    aDest.SetDrillShape( aSrc.GetDrillShape() );
    aDest.SetRoundRectRadiusRatio( aSrc.GetRoundRectRadiusRatio() );

    switch( aSrc.GetShape() )
    {
    case PAD_SHAPE_TRAPEZOID:
        aDest.SetDelta( aSrc.GetDelta() );
        break;

    case PAD_SHAPE_CIRCLE:
        // ensure size.y == size.x
        aDest.SetSize( wxSize( aDest.GetSize().x, aDest.GetSize().x ) );
        break;

    default:
        ;
    }

    switch( aSrc.GetAttribute() )
    {
    case PAD_ATTRIB_SMD:
    case PAD_ATTRIB_CONN:
        // These pads do not have hole (they are expected to be only on one
        // external copper layer)
        aDest.SetDrillSize( wxSize( 0, 0 ) );
        break;

    default:
        ;
    }
}


int PAD_TOOL::importPadSettings( const TOOL_EVENT& aEvent )
{
    auto& selTool = *m_toolMgr->GetTool<SELECTION_TOOL>();
    const auto& selection = selTool.GetSelection();

    auto& frame = *getEditFrame<PCB_EDIT_FRAME>();

    const D_PAD& masterPad = frame.GetDesignSettings().m_Pad_Master;

    BOARD_COMMIT commit( &frame );

    // for every selected pad, copy global settings
    for( auto item : selection )
    {
        if( item->Type() == PCB_PAD_T )
        {
            commit.Modify( item );

            auto& destPad = static_cast<D_PAD&>( *item );

            doImportPadSettings( masterPad, destPad );
        }
    }

    commit.Push( _( "Import Pad Settings" ) );

    m_toolMgr->RunAction( COMMON_ACTIONS::editModifiedSelection, true );
    frame.Refresh();

    return 0;
}


int PAD_TOOL::exportPadSettings( const TOOL_EVENT& aEvent )
{
    auto& selTool = *m_toolMgr->GetTool<SELECTION_TOOL>();
    const auto& selection = selTool.GetSelection();

    auto& frame = *getEditFrame<PCB_EDIT_FRAME>();

    D_PAD& masterPad = frame.GetDesignSettings().m_Pad_Master;

    // can only export from a single pad
    if( selection.Size() == 1 )
    {
        auto item = selection[0];

        if( item->Type() == PCB_PAD_T )
        {
            const auto& selPad = static_cast<const D_PAD&>( *item );

            doExportPadSettings( selPad, masterPad );
        }
    }

    return 0;
}


static void globalChangePadSettings( BOARD& board,
                                     const D_PAD& aSrcPad,
                                     BOARD_COMMIT& commit,
                                     bool aSameFootprints,
                                     bool aPadShapeFilter,
                                     bool aPadOrientFilter,
                                     bool aPadLayerFilter )
{
    const MODULE* moduleRef = aSrcPad.GetParent();

    // if there is no module, we can't make the comparisons to see which
    // pads to aply the src pad settings to
    if( moduleRef == nullptr )
    {
        wxLogDebug( "globalChangePadSettings() Error: NULL module" );
        return;
    }

    double pad_orient = aSrcPad.GetOrientation() - moduleRef->GetOrientation();

    for( const MODULE* module = board.m_Modules; module; module = module->Next() )
    {
        if( !aSameFootprints && ( module != moduleRef ) )
            continue;

        if( module->GetFPID() != moduleRef->GetFPID() )
            continue;

        for( D_PAD* pad = module->Pads();  pad;  pad = pad->Next() )
        {
            // Filters changes prohibited.
            if( aPadShapeFilter && ( pad->GetShape() != aSrcPad.GetShape() ) )
                continue;

            double currpad_orient = pad->GetOrientation() - module->GetOrientation();

            if( aPadOrientFilter && ( currpad_orient != pad_orient ) )
                continue;

            if( aPadLayerFilter && ( pad->GetLayerSet() != aSrcPad.GetLayerSet() ) )
                continue;

            if( aPadLayerFilter && ( pad->GetLayerSet() != aSrcPad.GetLayerSet() ) )
                continue;

            commit.Modify( pad );

            // copy source pad settings to this pad
            doImportPadSettings( aSrcPad, *pad );
        }
    }
}



int PAD_TOOL::pushPadSettings( const TOOL_EVENT& aEvent )
{
    auto& selTool = *m_toolMgr->GetTool<SELECTION_TOOL>();
    const auto& selection = selTool.GetSelection();

    auto& frame = *getEditFrame<PCB_EDIT_FRAME>();

    // not const - can be changed in the dialog
    D_PAD* srcPad = nullptr;

    // If nothing selected, use the master pad setting,
    // otherwise, if one pad selected, use the selected pad
    if( selection.Size() == 0 )
    {
        srcPad = &frame.GetDesignSettings().m_Pad_Master;
    }
    else if( selection.Size() == 1 )
    {
        if( selection[0]->Type() == PCB_PAD_T )
        {
            srcPad = static_cast<D_PAD*>( selection[0] );
        }
    }
    else
    {
        // multiple selected what to do?
        // maybe master->selection? same as import multiple?
    }

    // no valid selection, nothing to do
    if( !srcPad )
    {
        return 0;
    }

    MODULE* module = srcPad->GetParent();

    if( module != nullptr )
    {
        frame.SetMsgPanel( module );
    }

    int dialogRet;
    {
        DIALOG_GLOBAL_PADS_EDITION dlg( &frame, srcPad );
        dialogRet = dlg.ShowModal();
    }

    // cancel
    if( dialogRet == -1 )
    {
        return 0;
    }

    const bool edit_Same_Modules = (dialogRet == 1);

    BOARD_COMMIT commit( &frame );

    globalChangePadSettings( *getModel<BOARD>(), *srcPad, commit,
                              edit_Same_Modules,
                              DIALOG_GLOBAL_PADS_EDITION::m_Pad_Shape_Filter,
                              DIALOG_GLOBAL_PADS_EDITION::m_Pad_Orient_Filter,
                              DIALOG_GLOBAL_PADS_EDITION::m_Pad_Layer_Filter );

    commit.Push( _( "Import Pad Settings" ) );

    m_toolMgr->RunAction( COMMON_ACTIONS::editModifiedSelection, true );
    frame.Refresh();

    return 0;
}


void PAD_TOOL::SetTransitions()
{
    Go( &PAD_TOOL::importPadSettings, COMMON_ACTIONS::importPadSettings.MakeEvent() );
    Go( &PAD_TOOL::exportPadSettings, COMMON_ACTIONS::exportPadSettings.MakeEvent() );
    Go( &PAD_TOOL::pushPadSettings,   COMMON_ACTIONS::pushPadSettings.MakeEvent() );
}
