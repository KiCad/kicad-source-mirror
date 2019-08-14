/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 CERN
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
#include <kiway.h>
#include <sch_painter.h>
#include <tool/tool_manager.h>
#include <tools/ee_actions.h>
#include <tools/lib_control.h>
#include <eeschema_id.h>
#include <lib_edit_frame.h>
#include <viewlib_frame.h>
#include <wildcards_and_files_ext.h>
#include <gestfich.h>
#include <project.h>
#include <confirm.h>


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
    
        ctxMenu.AddSeparator();
        ctxMenu.AddItem( EE_ACTIONS::newSymbol,          SELECTION_CONDITIONS::ShowAlways );
        ctxMenu.AddItem( EE_ACTIONS::editSymbol,         symbolSelectedCondition );
    
        ctxMenu.AddSeparator();
        ctxMenu.AddItem( ACTIONS::save,                  symbolSelectedCondition );
        ctxMenu.AddItem( ACTIONS::saveCopyAs,            symbolSelectedCondition );
        ctxMenu.AddItem( EE_ACTIONS::duplicateSymbol,    symbolSelectedCondition );
        ctxMenu.AddItem( EE_ACTIONS::deleteSymbol,       symbolSelectedCondition );
        ctxMenu.AddItem( ACTIONS::revert,                symbolSelectedCondition );
    
        ctxMenu.AddSeparator();
        ctxMenu.AddItem( EE_ACTIONS::cutSymbol,          symbolSelectedCondition );
        ctxMenu.AddItem( EE_ACTIONS::copySymbol,         symbolSelectedCondition );
        ctxMenu.AddItem( EE_ACTIONS::pasteSymbol,        SELECTION_CONDITIONS::ShowAlways );
    
        ctxMenu.AddSeparator();
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


int LIB_CONTROL::ToggleSyncedPinsMode( const TOOL_EVENT& aEvent )
{
    if( !m_isLibEdit )
        return 0;

    LIB_EDIT_FRAME* editFrame = getEditFrame<LIB_EDIT_FRAME>();
    editFrame->m_SyncPinEdit = !editFrame->m_SyncPinEdit;

    return 0;
}


int LIB_CONTROL::ExportView( const TOOL_EVENT& aEvent )
{
    if( !m_isLibEdit )
        return 0;

    LIB_EDIT_FRAME* editFrame = getEditFrame<LIB_EDIT_FRAME>();
    LIB_PART*       part = editFrame->GetCurPart();

    if( !part )
    {
        wxMessageBox( _( "No symbol to export" ) );
        return 0;
    }

    wxString   file_ext = wxT( "png" );
    wxString   mask = wxT( "*." ) + file_ext;
    wxFileName fn( part->GetName() );
    fn.SetExt( "png" );

    wxString projectPath = wxPathOnly( m_frame->Prj().GetProjectFullName() );

    wxFileDialog dlg( editFrame, _( "Image File Name" ), projectPath, fn.GetFullName(),
                      PngFileWildcard(), wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    if( dlg.ShowModal() == wxID_OK && !dlg.GetPath().IsEmpty() )
    {
        // calling wxYield is mandatory under Linux, after closing the file selector dialog
        // to refresh the screen before creating the PNG or JPEG image from screen
        wxYield();

        if( !SaveCanvasImageToFile( editFrame, dlg.GetPath(), wxBITMAP_TYPE_PNG ) )
        {
            wxMessageBox( wxString::Format( _( "Can't save file \"%s\"." ), dlg.GetPath() ) );
        }
    }

    return 0;
}


int LIB_CONTROL::ExportSymbolAsSVG( const TOOL_EVENT& aEvent )
{
    if( !m_isLibEdit )
        return 0;

    LIB_EDIT_FRAME* editFrame = getEditFrame<LIB_EDIT_FRAME>();
    LIB_PART*       part = editFrame->GetCurPart();

    if( !part )
    {
        wxMessageBox( _( "No symbol to export" ) );
        return 0;
    }

    wxString   file_ext = wxT( "svg" );
    wxString   mask     = wxT( "*." ) + file_ext;
    wxFileName fn( part->GetName() );
    fn.SetExt( file_ext );

    wxString pro_dir = wxPathOnly( m_frame->Prj().GetProjectFullName() );

    wxString fullFileName = EDA_FILE_SELECTOR( _( "Filename:" ), pro_dir, fn.GetFullName(),
                                               file_ext, mask, m_frame, wxFD_SAVE, true );

    if( !fullFileName.IsEmpty() )
    {
        PAGE_INFO pageSave = editFrame->GetScreen()->GetPageSettings();
        PAGE_INFO pageTemp = pageSave;

        wxSize componentSize = part->GetUnitBoundingBox( editFrame->GetUnit(),
                                                         editFrame->GetConvert() ).GetSize();

        // Add a small margin to the plot bounding box
        pageTemp.SetWidthMils(  int( componentSize.x * 1.2 ) );
        pageTemp.SetHeightMils( int( componentSize.y * 1.2 ) );

        editFrame->GetScreen()->SetPageSettings( pageTemp );
        editFrame->SVG_PlotComponent( fullFileName );
        editFrame->GetScreen()->SetPageSettings( pageSave );
    }

    return 0;
}


int LIB_CONTROL::AddSymbolToSchematic( const TOOL_EVENT& aEvent )
{
    LIB_PART* part = nullptr;
    LIB_ID    libId;
    int       unit, convert;

    if( m_isLibEdit )
    {
        LIB_EDIT_FRAME* editFrame = getEditFrame<LIB_EDIT_FRAME>();

        part = editFrame->GetCurPart();
        unit = editFrame->GetUnit();
        convert = editFrame->GetConvert();

        if( part )
            libId = part->GetLibId();
    }
    else
    {
        LIB_VIEW_FRAME* viewFrame = getEditFrame<LIB_VIEW_FRAME>();

        if( viewFrame->IsModal() )
        {
            // if we're modal then we just need to return the symbol selection; the caller is
            // already in a EE_ACTIONS::placeSymbol coroutine.
            viewFrame->FinishModal();
            return 0;
        }
        else
        {
            part = viewFrame->GetSelectedSymbol();
            libId = viewFrame->GetSelectedAlias()->GetLibId();
            unit = viewFrame->GetUnit();
            convert = viewFrame->GetConvert();
        }
    }

    if( part )
    {
        SCH_EDIT_FRAME* schframe = (SCH_EDIT_FRAME*) m_frame->Kiway().Player( FRAME_SCH, false );

        if( !schframe )      // happens when the schematic editor is not active (or closed)
        {
            DisplayErrorMessage( m_frame, _( "No schematic currently open." ) );
            return 0;
        }

        SCH_COMPONENT* comp = new SCH_COMPONENT( *part, libId,g_CurrentSheet, unit, convert );

        // Be sure the link to the corresponding LIB_PART is OK:
        comp->Resolve( *m_frame->Prj().SchSymbolLibTable() );

        if( schframe->GetAutoplaceFields() )
            comp->AutoplaceFields( /* aScreen */ nullptr, /* aManual */ false );

        schframe->Raise();
        schframe->GetToolManager()->RunAction( EE_ACTIONS::placeSymbol, true, comp );
    }

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
    Go( &LIB_CONTROL::ExportView,            EE_ACTIONS::exportSymbolView.MakeEvent() );
    Go( &LIB_CONTROL::ExportSymbolAsSVG,     EE_ACTIONS::exportSymbolAsSVG.MakeEvent() );
    Go( &LIB_CONTROL::AddSymbolToSchematic,  EE_ACTIONS::addSymbolToSchematic.MakeEvent() );

    Go( &LIB_CONTROL::OnDeMorgan,            EE_ACTIONS::showDeMorganStandard.MakeEvent() );
    Go( &LIB_CONTROL::OnDeMorgan,            EE_ACTIONS::showDeMorganAlternate.MakeEvent() );

    Go( &LIB_CONTROL::ShowElectricalTypes,   EE_ACTIONS::showElectricalTypes.MakeEvent() );
    Go( &LIB_CONTROL::ShowComponentTree,     EE_ACTIONS::showComponentTree.MakeEvent() );
    Go( &LIB_CONTROL::ToggleSyncedPinsMode,  EE_ACTIONS::toggleSyncedPinsMode.MakeEvent() );
}
