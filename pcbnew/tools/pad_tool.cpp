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
#include <bitmaps.h>

#include <class_board_item.h>
#include <class_module.h>
#include <board_commit.h>

#include <dialogs/dialog_global_pads_edition.h>

#include "pcb_actions.h"
#include "selection_tool.h"
#include "pcb_selection_conditions.h"
#include "edit_tool.h"

// Pad tools
TOOL_ACTION PCB_ACTIONS::copyPadSettings(
        "pcbnew.PadTool.CopyPadSettings",
        AS_GLOBAL, 0,
        _( "Copy Pad Settings" ), _( "Copy current pad's settings to the board design settings" ),
        copy_pad_settings_xpm );

TOOL_ACTION PCB_ACTIONS::applyPadSettings(
        "pcbnew.PadTool.ApplyPadSettings",
        AS_GLOBAL, 0,
        _( "Apply Pad Settings" ), _( "Copy the board design settings pad properties to the current pad" ),
        apply_pad_settings_xpm );

TOOL_ACTION PCB_ACTIONS::pushPadSettings(
        "pcbnew.PadTool.PushPadSettings",
        AS_GLOBAL, 0,
        _( "Push Pad Settings" ), _( "Copy the current pad settings to other pads" ),
        push_pad_settings_xpm );


class PAD_CONTEXT_MENU : public CONTEXT_MENU
{
public:

    using SHOW_FUNCTOR = std::function<bool()>;

    PAD_CONTEXT_MENU( bool aEditingFootprint,
                      SHOW_FUNCTOR aHaveGlobalPadSetting ):
        m_editingFootprint( aEditingFootprint ),
        m_haveGlobalPadSettings( aHaveGlobalPadSetting )
    {
        SetIcon( pad_xpm );
        SetTitle( _( "Pads" ) );

        Add( PCB_ACTIONS::copyPadSettings );
        Add( PCB_ACTIONS::applyPadSettings );
        Add( PCB_ACTIONS::pushPadSettings );

        // show modedit-specific items
        if( m_editingFootprint )
        {
            AppendSeparator();

            Add( PCB_ACTIONS::enumeratePads );
        }
    }

protected:

    CONTEXT_MENU* create() const override
    {
        return new PAD_CONTEXT_MENU( m_editingFootprint, m_haveGlobalPadSettings );
    }

private:

    struct ENABLEMENTS
    {
        bool canImport;
        bool canExport;
        bool canPush;
    };

    ENABLEMENTS getEnablements( const SELECTION& aSelection )
    {
        using S_C = SELECTION_CONDITIONS;
        ENABLEMENTS enablements;

        auto anyPadSel = S_C::HasType( PCB_PAD_T );
        auto singlePadSel = S_C::Count( 1 ) && S_C::OnlyType( PCB_PAD_T );

        // Apply pads enabled when any pads selected (it applies to each one
        // individually), plus need a valid global pad setting
        enablements.canImport = m_haveGlobalPadSettings() && ( anyPadSel )( aSelection );

        // Copy pads item enabled only when there is a single pad selected
        // (otherwise how would we know which one to copy?)
        enablements.canExport = ( singlePadSel )( aSelection );

        // Push pads available when there is a single pad to push from
        enablements.canPush = ( singlePadSel )( aSelection );

        return enablements;
    }

    void update() override
    {
        auto selTool = getToolManager()->GetTool<SELECTION_TOOL>();
        const SELECTION& selection = selTool->GetSelection();

        auto enablements = getEnablements( selection );

        Enable( getMenuId( PCB_ACTIONS::applyPadSettings ), enablements.canImport );
        Enable( getMenuId( PCB_ACTIONS::copyPadSettings ), enablements.canExport );
        Enable( getMenuId( PCB_ACTIONS::pushPadSettings ), enablements.canPush );
    }

    bool m_editingFootprint;
    SHOW_FUNCTOR m_haveGlobalPadSettings;
};


PAD_TOOL::PAD_TOOL() :
        PCB_TOOL( "pcbnew.PadTool" ), m_padCopied( false )
{
}


PAD_TOOL::~PAD_TOOL()
{}


void PAD_TOOL::Reset( RESET_REASON aReason )
{
    m_padCopied = false;
}


bool PAD_TOOL::haveFootprints()
{
    auto& board = *getModel<BOARD>();
    return board.m_Modules.GetCount() > 0;
}


bool PAD_TOOL::Init()
{
    auto contextMenu = std::make_shared<PAD_CONTEXT_MENU>( EditingModules(),
            [this]() { return m_padCopied; } );
    contextMenu->SetTool( this );

    SELECTION_TOOL* selTool = m_toolMgr->GetTool<SELECTION_TOOL>();

    if( selTool )
    {
        auto& toolMenu = selTool->GetToolMenu();
        auto& menu = toolMenu.GetMenu();

        toolMenu.AddSubMenu( contextMenu );

        SELECTION_CONDITION canShowMenuCond = [this, contextMenu] ( const SELECTION& aSel ) {
            contextMenu->UpdateAll();
            return haveFootprints() && contextMenu->HasEnabledItems();
        };

        // show menu when there is a footprint, and the menu has any items
        auto showCond = canShowMenuCond &&
                        ( SELECTION_CONDITIONS::HasType( PCB_PAD_T )
                            || SELECTION_CONDITIONS::Count( 0 ) );

        menu.AddMenu( contextMenu.get(), false, showCond );
    }

    return true;
}


int PAD_TOOL::applyPadSettings( const TOOL_EVENT& aEvent )
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

            D_PAD& destPad = static_cast<D_PAD&>( *item );
            destPad.ImportSettingsFromMaster( masterPad );
        }
    }

    commit.Push( _( "Apply Pad Settings" ) );

    m_toolMgr->RunAction( PCB_ACTIONS::selectionModified, true );
    frame.Refresh();

    return 0;
}


int PAD_TOOL::copyPadSettings( const TOOL_EVENT& aEvent )
{
    auto& selTool = *m_toolMgr->GetTool<SELECTION_TOOL>();
    const auto& selection = selTool.GetSelection();

    auto& frame = *getEditFrame<PCB_EDIT_FRAME>();

    D_PAD& masterPad = frame.GetDesignSettings().m_Pad_Master;

    // can only copy from a single pad
    if( selection.Size() == 1 )
    {
        auto item = selection[0];

        if( item->Type() == PCB_PAD_T )
        {
            const auto& selPad = static_cast<const D_PAD&>( *item );
            masterPad.ImportSettingsFromMaster( selPad );
            m_padCopied = true;
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

    // If there is no module, we can't make the comparisons to see which
    // pads to apply the source pad settings to
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

            // Apply source pad settings to this pad
            pad->ImportSettingsFromMaster( aSrcPad );
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
        // maybe master->selection? same as apply multiple?
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

    commit.Push( _( "Apply Pad Settings" ) );

    m_toolMgr->RunAction( PCB_ACTIONS::selectionModified, true );
    frame.Refresh();

    return 0;
}


void PAD_TOOL::SetTransitions()
{
    Go( &PAD_TOOL::applyPadSettings, PCB_ACTIONS::applyPadSettings.MakeEvent() );
    Go( &PAD_TOOL::copyPadSettings,  PCB_ACTIONS::copyPadSettings.MakeEvent() );
    Go( &PAD_TOOL::pushPadSettings,  PCB_ACTIONS::pushPadSettings.MakeEvent() );
}
