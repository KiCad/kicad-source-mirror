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


TOOL_ACTION EE_ACTIONS::showElectricalTypes( "eeschema.SymbolLibraryControl.showElectricalTypes",
        AS_GLOBAL, 0,
        _( "Show Pin Electrical Types" ), _( "Annotate pins with their electrical types" ),
        pin_show_etype_xpm );


TOOL_ACTION EE_ACTIONS::showComponentTree( "eeschema.SymbolLibraryControl.showComponentTree",
        AS_GLOBAL, 0,
        _( "Show Symbol Tree" ), "",
        search_tree_xpm );


bool LIB_CONTROL::Init()
{
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
        ctxMenu.AddItem( ID_LIBEDIT_EDIT_PART,
                         _( "Edit Symbol" ), _( "Show selected symbol on editor canvas" ),
                         edit_xpm,                       symbolSelectedCondition );
    
        ctxMenu.AddSeparator( SELECTION_CONDITIONS::ShowAlways );
        ctxMenu.AddItem( ACTIONS::save,                  symbolSelectedCondition );
        ctxMenu.AddItem( ACTIONS::saveCopyAs,            symbolSelectedCondition );
        ctxMenu.AddItem( ID_LIBEDIT_DUPLICATE_PART,
                         _( "Duplicate" ), _( "Make a copy of the selected symbol" ),
                         duplicate_xpm,                  symbolSelectedCondition );
        ctxMenu.AddItem( ID_LIBEDIT_REMOVE_PART,
                         _( "Delete" ), _( "Remove the selected symbol from the library" ),
                         delete_xpm,                     symbolSelectedCondition );
        ctxMenu.AddItem( ACTIONS::revert,                symbolSelectedCondition );
    
        ctxMenu.AddSeparator( SELECTION_CONDITIONS::ShowAlways );
        ctxMenu.AddItem( ID_LIBEDIT_CUT_PART, _( "Cut Symbol" ), "",
                         cut_xpm,                        symbolSelectedCondition );
        ctxMenu.AddItem( ID_LIBEDIT_COPY_PART, _( "Copy Symbol" ), "",
                         copy_xpm,                       symbolSelectedCondition );
        ctxMenu.AddItem( ID_LIBEDIT_PASTE_PART, _( "Paste Symbol" ), "",
                         paste_xpm,                      SELECTION_CONDITIONS::ShowAlways );
    
        ctxMenu.AddSeparator( symbolSelectedCondition );
        ctxMenu.AddItem( EE_ACTIONS::importSymbol,       SELECTION_CONDITIONS::ShowAlways );
        ctxMenu.AddItem( ID_LIBEDIT_EXPORT_PART, _( "Export Symbol..." ), "",
                         export_part_xpm,                symbolSelectedCondition );
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


int LIB_CONTROL::AddSymbol( const TOOL_EVENT& aEvent )
{
    if( m_frame->IsType( FRAME_SCH_LIB_EDITOR ) )
    {
        if( aEvent.IsAction( &EE_ACTIONS::newSymbol ) )
            static_cast<LIB_EDIT_FRAME*>( m_frame )->CreateNewPart();
        else if( aEvent.IsAction( &EE_ACTIONS::importSymbol ) )
            static_cast<LIB_EDIT_FRAME*>( m_frame )->ImportPart();
    }

    return 0;
}


int LIB_CONTROL::Save( const TOOL_EVENT& aEvent )
{
    if( m_frame->IsType( FRAME_SCH_LIB_EDITOR ) )
        static_cast<LIB_EDIT_FRAME*>( m_frame )->OnSave();

    return 0;
}


int LIB_CONTROL::SaveAs( const TOOL_EVENT& aEvent )
{
    if( m_frame->IsType( FRAME_SCH_LIB_EDITOR ) )
        static_cast<LIB_EDIT_FRAME*>( m_frame )->OnSaveAs();

    return 0;
}


int LIB_CONTROL::SaveAll( const TOOL_EVENT& aEvent )
{
    if( m_frame->IsType( FRAME_SCH_LIB_EDITOR ) )
        static_cast<LIB_EDIT_FRAME*>( m_frame )->OnSaveAll();

    return 0;
}


int LIB_CONTROL::Revert( const TOOL_EVENT& aEvent )
{
    if( m_frame->IsType( FRAME_SCH_LIB_EDITOR ) )
        static_cast<LIB_EDIT_FRAME*>( m_frame )->OnRevert();

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


int LIB_CONTROL::ShowLibraryBrowser( const TOOL_EVENT& aEvent )
{
    wxCommandEvent dummy;
    m_frame->OnOpenLibraryViewer( dummy );

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

    Go( &LIB_CONTROL::Save,                  ACTIONS::save.MakeEvent() );
    Go( &LIB_CONTROL::SaveAs,                ACTIONS::saveAs.MakeEvent() );     // for libraries
    Go( &LIB_CONTROL::SaveAs,                ACTIONS::saveCopyAs.MakeEvent() ); // for symbols
    Go( &LIB_CONTROL::SaveAll,               ACTIONS::saveAll.MakeEvent() );
    Go( &LIB_CONTROL::Revert,                ACTIONS::revert.MakeEvent() );

    Go( &LIB_CONTROL::OnDeMorgan,            EE_ACTIONS::showDeMorganStandard.MakeEvent() );
    Go( &LIB_CONTROL::OnDeMorgan,            EE_ACTIONS::showDeMorganAlternate.MakeEvent() );

    Go( &LIB_CONTROL::ShowLibraryBrowser,    ACTIONS::showSymbolBrowser.MakeEvent() );
    Go( &LIB_CONTROL::ShowElectricalTypes,   EE_ACTIONS::showElectricalTypes.MakeEvent() );
    Go( &LIB_CONTROL::ShowComponentTree,     EE_ACTIONS::showComponentTree.MakeEvent() );
}
