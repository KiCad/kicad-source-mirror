/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <fctsys.h>
#include <sch_painter.h>
#include <tool/tool_manager.h>
#include <tools/ee_actions.h>
#include <tools/lib_control.h>
#include <eeschema_id.h>
#include <lib_edit_frame.h>
#include <viewlib_frame.h>


TOOL_ACTION EE_ACTIONS::newSymbol( "eeschema.SymbolLibraryControl.newSymbol",
        AS_GLOBAL, 0, "",
        _( "New Symbol..." ), _( "Create a new symbol" ),
        new_component_xpm );

TOOL_ACTION EE_ACTIONS::editSymbol( "eeschema.SymbolLibraryControl.editSymbol",
        AS_GLOBAL, 0, "",
        _( "Edit Symbol" ), _( "Show selected symbol on editor canvas" ),
        edit_xpm );

TOOL_ACTION EE_ACTIONS::duplicateSymbol( "eeschema.SymbolLibraryControl.duplicateSymbol",
        AS_GLOBAL, 0, "",
        _( "Duplicate Symbol" ), _( "Make a copy of the selected symbol" ),
        duplicate_xpm );

TOOL_ACTION EE_ACTIONS::deleteSymbol( "eeschema.SymbolLibraryControl.deleteSymbol",
        AS_GLOBAL, 0, "",
        _( "Delete Symbol" ), _( "Remove the selected symbol from its library" ),
        delete_xpm );

TOOL_ACTION EE_ACTIONS::cutSymbol( "eeschema.SymbolLibraryControl.cutSymbol",
        AS_GLOBAL, 0, "",
        _( "Cut Symbol" ), "",
        cut_xpm );

TOOL_ACTION EE_ACTIONS::copySymbol( "eeschema.SymbolLibraryControl.copySymbol",
        AS_GLOBAL, 0, "",
        _( "Copy Symbol" ), "",
        copy_xpm );

TOOL_ACTION EE_ACTIONS::pasteSymbol( "eeschema.SymbolLibraryControl.pasteSymbol",
        AS_GLOBAL, 0, "",
        _( "Paste Symbol" ), "",
        paste_xpm );

TOOL_ACTION EE_ACTIONS::importSymbol( "eeschema.SymbolLibraryControl.importSymbol",
        AS_GLOBAL, 0, "",
        _( "Import Symbol..." ), _( "Import a symbol to the current library" ),
        import_part_xpm );

TOOL_ACTION EE_ACTIONS::exportSymbol( "eeschema.SymbolLibraryControl.exportSymbol",
        AS_GLOBAL, 0, "",
        _( "Export Symbol..." ), _( "Export a symbol to a new library file" ),
        export_part_xpm );

TOOL_ACTION EE_ACTIONS::showElectricalTypes( "eeschema.SymbolLibraryControl.showElectricalTypes",
        AS_GLOBAL, 0, "",
        _( "Show Pin Electrical Types" ), _( "Annotate pins with their electrical types" ),
        pin_show_etype_xpm );


TOOL_ACTION EE_ACTIONS::showComponentTree( "eeschema.SymbolLibraryControl.showComponentTree",
        AS_GLOBAL, 0, "",
        _( "Show Symbol Tree" ), "",
        search_tree_xpm );


bool LIB_CONTROL::Init()
{
    m_frame = getEditFrame<SCH_BASE_FRAME>();
    m_selectionTool = m_toolMgr->GetTool<EE_SELECTION_TOOL>();
    m_isLibEdit = m_frame->IsType( FRAME_SCH_LIB_EDITOR );
    
    if( m_isLibEdit )
    {
        CONDITIONAL_MENU& ctxMenu = m_menu.GetMenu();
        LIB_EDIT_FRAME* editFrame = getEditFrame<LIB_EDIT_FRAME>();

        auto libSelectedCondition = [ editFrame ] ( const SELECTION& aSel ) {
            LIB_ID sel = editFrame->GetTreeLIBID();
            return !sel.GetLibNickname().empty() && sel.GetLibItemName().empty();
        };
        auto symbolSelectedCondition = [ editFrame ] ( const SELECTION& aSel ) {
            LIB_ID sel = editFrame->GetTreeLIBID();
            return !sel.GetLibNickname().empty() && !sel.GetLibItemName().empty();
        };
    
        ctxMenu.AddItem( ACTIONS::newLibrary,            SELECTION_CONDITIONS::ShowAlways );
        ctxMenu.AddItem( ACTIONS::addLibrary,            SELECTION_CONDITIONS::ShowAlways );
        ctxMenu.AddItem( ACTIONS::save,                  libSelectedCondition );
        ctxMenu.AddItem( ACTIONS::saveAs,                libSelectedCondition );
        ctxMenu.AddItem( ACTIONS::revert,                libSelectedCondition );
    
        ctxMenu.AddSeparator( SELECTION_CONDITIONS::ShowAlways );
        ctxMenu.AddItem( EE_ACTIONS::newSymbol,          SELECTION_CONDITIONS::ShowAlways );
        ctxMenu.AddItem( EE_ACTIONS::editSymbol,         symbolSelectedCondition );
    
        ctxMenu.AddSeparator( SELECTION_CONDITIONS::ShowAlways );
        ctxMenu.AddItem( ACTIONS::save,                  symbolSelectedCondition );
        ctxMenu.AddItem( ACTIONS::saveCopyAs,            symbolSelectedCondition );
        ctxMenu.AddItem( EE_ACTIONS::duplicateSymbol,    symbolSelectedCondition );
        ctxMenu.AddItem( EE_ACTIONS::deleteSymbol,       symbolSelectedCondition );
        ctxMenu.AddItem( ACTIONS::revert,                symbolSelectedCondition );
    
        ctxMenu.AddSeparator( SELECTION_CONDITIONS::ShowAlways );
        ctxMenu.AddItem( EE_ACTIONS::cutSymbol,          symbolSelectedCondition );
        ctxMenu.AddItem( EE_ACTIONS::copySymbol,         symbolSelectedCondition );
        ctxMenu.AddItem( EE_ACTIONS::pasteSymbol,        SELECTION_CONDITIONS::ShowAlways );
    
        ctxMenu.AddSeparator( symbolSelectedCondition );
        ctxMenu.AddItem( EE_ACTIONS::importSymbol,       SELECTION_CONDITIONS::ShowAlways );
        ctxMenu.AddItem( EE_ACTIONS::exportSymbol,       symbolSelectedCondition );
    }
    
    return true;
}


int LIB_CONTROL::AddLibrary( const TOOL_EVENT& aEvent )
{
    bool createNew = aEvent.IsAction( &ACTIONS::newLibrary );

    if( m_frame->IsType( FRAME_SCH_LIB_EDITOR ) )
        static_cast<LIB_EDIT_FRAME*>( m_frame )->AddLibraryFile( createNew );

    return 0;
}


int LIB_CONTROL::EditSymbol( const TOOL_EVENT& aEvent )
{
    if( m_frame->IsType( FRAME_SCH_LIB_EDITOR ) )
    {
        LIB_EDIT_FRAME* editFrame = static_cast<LIB_EDIT_FRAME*>( m_frame );
        int             unit = 0;
        LIB_ID          partId = editFrame->GetTreeLIBID( &unit );
        
        editFrame->LoadPart( partId.GetLibItemName(), partId.GetLibNickname(), unit );
    }

    return 0;
}


int LIB_CONTROL::AddSymbol( const TOOL_EVENT& aEvent )
{
    if( m_frame->IsType( FRAME_SCH_LIB_EDITOR ) )
    {
        LIB_EDIT_FRAME* editFrame = static_cast<LIB_EDIT_FRAME*>( m_frame );

        if( aEvent.IsAction( &EE_ACTIONS::newSymbol ) )
            editFrame->CreateNewPart();
        else if( aEvent.IsAction( &EE_ACTIONS::importSymbol ) )
            editFrame->ImportPart();
    }

    return 0;
}


int LIB_CONTROL::Save( const TOOL_EVENT& aEvt )
{
    if( m_frame->IsType( FRAME_SCH_LIB_EDITOR ) )
    {
        LIB_EDIT_FRAME* editFrame = static_cast<LIB_EDIT_FRAME*>( m_frame );

        if( aEvt.IsAction( &EE_ACTIONS::save ) )
            editFrame->Save();
        else if( aEvt.IsAction( &EE_ACTIONS::saveAs ) || aEvt.IsAction( &EE_ACTIONS::saveCopyAs ) )
            editFrame->SaveAs();
        else if( aEvt.IsAction( &EE_ACTIONS::saveAll ) )
            editFrame->SaveAll();
    }

    return 0;
}


int LIB_CONTROL::Revert( const TOOL_EVENT& aEvent )
{
    if( m_frame->IsType( FRAME_SCH_LIB_EDITOR ) )
        static_cast<LIB_EDIT_FRAME*>( m_frame )->Revert();

    return 0;
}


int LIB_CONTROL::ExportSymbol( const TOOL_EVENT& aEvent )
{
    if( m_frame->IsType( FRAME_SCH_LIB_EDITOR ) )
        static_cast<LIB_EDIT_FRAME*>( m_frame )->ExportPart();

    return 0;
}


int LIB_CONTROL::CutCopyDelete( const TOOL_EVENT& aEvt )
{
    if( m_frame->IsType( FRAME_SCH_LIB_EDITOR ) )
    {
        LIB_EDIT_FRAME* editFrame = static_cast<LIB_EDIT_FRAME*>( m_frame );
        
        if( aEvt.IsAction( &EE_ACTIONS::cutSymbol ) || aEvt.IsAction( &EE_ACTIONS::copySymbol ) )
            editFrame->CopyPartToClipboard();
        
        if( aEvt.IsAction( &EE_ACTIONS::cutSymbol ) || aEvt.IsAction( &EE_ACTIONS::deleteSymbol ) )
            editFrame->DeletePartFromLibrary();
    }

    return 0;
}


int LIB_CONTROL::DuplicateSymbol( const TOOL_EVENT& aEvent )
{
    if( m_frame->IsType( FRAME_SCH_LIB_EDITOR ) )
    {
        LIB_EDIT_FRAME* editFrame = static_cast<LIB_EDIT_FRAME*>( m_frame );
        editFrame->DuplicatePart( aEvent.IsAction( &EE_ACTIONS::pasteSymbol ) );
    }

    return 0;
}


int LIB_CONTROL::OnDeMorgan( const TOOL_EVENT& aEvent )
{
    int convert = aEvent.IsAction( &EE_ACTIONS::showDeMorganStandard ) ?
            LIB_ITEM::LIB_CONVERT::BASE : LIB_ITEM::LIB_CONVERT::DEMORGAN;

    if( m_frame->IsType( FRAME_SCH_LIB_EDITOR ) )
    {
        m_toolMgr->RunAction( ACTIONS::cancelInteractive, true );
        m_toolMgr->RunAction( EE_ACTIONS::clearSelection, true );

        LIB_EDIT_FRAME* libEditFrame = static_cast<LIB_EDIT_FRAME*>( m_frame );
        libEditFrame->SetConvert( convert );

        m_toolMgr->ResetTools( TOOL_BASE::MODEL_RELOAD );
        libEditFrame->RebuildView();
    }
    else if( m_frame->IsType( FRAME_SCH_VIEWER ) || m_frame->IsType( FRAME_SCH_VIEWER_MODAL ) )
    {
        LIB_VIEW_FRAME* libViewFrame = static_cast<LIB_VIEW_FRAME*>( m_frame );
        libViewFrame->SetUnitAndConvert( libViewFrame->GetUnit(), convert );
    }

    return 0;
}


int LIB_CONTROL::ShowComponentTree( const TOOL_EVENT& aEvent )
{
    if( m_frame->IsType( FRAME_SCH_LIB_EDITOR ) )
    {
        wxCommandEvent dummy;
        static_cast<LIB_EDIT_FRAME*>( m_frame )->OnToggleSearchTree( dummy );
    }

    return 0;
}


int LIB_CONTROL::ShowElectricalTypes( const TOOL_EVENT& aEvent )
{
    m_frame->SetShowElectricalType( !m_frame->GetShowElectricalType() );

    // Update canvas
    m_frame->GetRenderSettings()->m_ShowPinsElectricalType = m_frame->GetShowElectricalType();
    m_frame->GetCanvas()->GetView()->UpdateAllItems( KIGFX::REPAINT );
    m_frame->GetCanvas()->Refresh();

    return 0;
}


void LIB_CONTROL::setTransitions()
{
    Go( &LIB_CONTROL::AddLibrary,            ACTIONS::newLibrary.MakeEvent() );
    Go( &LIB_CONTROL::AddLibrary,            ACTIONS::addLibrary.MakeEvent() );
    Go( &LIB_CONTROL::AddSymbol,             EE_ACTIONS::newSymbol.MakeEvent() );
    Go( &LIB_CONTROL::AddSymbol,             EE_ACTIONS::importSymbol.MakeEvent() );
    Go( &LIB_CONTROL::EditSymbol,            EE_ACTIONS::editSymbol.MakeEvent() );

    Go( &LIB_CONTROL::Save,                  ACTIONS::save.MakeEvent() );
    Go( &LIB_CONTROL::Save,                  ACTIONS::saveAs.MakeEvent() );     // for libraries
    Go( &LIB_CONTROL::Save,                  ACTIONS::saveCopyAs.MakeEvent() ); // for symbols
    Go( &LIB_CONTROL::Save,                  ACTIONS::saveAll.MakeEvent() );
    Go( &LIB_CONTROL::Revert,                ACTIONS::revert.MakeEvent() );

    Go( &LIB_CONTROL::DuplicateSymbol,       EE_ACTIONS::duplicateSymbol.MakeEvent() );
    Go( &LIB_CONTROL::CutCopyDelete,         EE_ACTIONS::deleteSymbol.MakeEvent() );
    Go( &LIB_CONTROL::CutCopyDelete,         EE_ACTIONS::cutSymbol.MakeEvent() );
    Go( &LIB_CONTROL::CutCopyDelete,         EE_ACTIONS::copySymbol.MakeEvent() );
    Go( &LIB_CONTROL::DuplicateSymbol,       EE_ACTIONS::pasteSymbol.MakeEvent() );
    Go( &LIB_CONTROL::ExportSymbol,          EE_ACTIONS::exportSymbol.MakeEvent() );

    Go( &LIB_CONTROL::OnDeMorgan,            EE_ACTIONS::showDeMorganStandard.MakeEvent() );
    Go( &LIB_CONTROL::OnDeMorgan,            EE_ACTIONS::showDeMorganAlternate.MakeEvent() );

    Go( &LIB_CONTROL::ShowElectricalTypes,   EE_ACTIONS::showElectricalTypes.MakeEvent() );
    Go( &LIB_CONTROL::ShowComponentTree,     EE_ACTIONS::showComponentTree.MakeEvent() );
}
