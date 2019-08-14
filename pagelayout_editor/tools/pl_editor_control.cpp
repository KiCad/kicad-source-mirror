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
#include <view/view.h>
#include <tool/tool_manager.h>
#include <tools/pl_actions.h>
#include <tools/pl_editor_control.h>
#include <tools/pl_selection_tool.h>
#include <pl_editor_frame.h>
#include <ws_painter.h>
#include <confirm.h>
#include <bitmaps.h>
#include <properties_frame.h>
#include <pl_editor_id.h>
#include <dialog_page_settings.h>

bool PL_EDITOR_CONTROL::Init()
{
    m_frame = getEditFrame<PL_EDITOR_FRAME>();
    return true;
}


void PL_EDITOR_CONTROL::Reset( RESET_REASON aReason )
{
    if( aReason == MODEL_RELOAD )
        m_frame = getEditFrame<PL_EDITOR_FRAME>();
}


int PL_EDITOR_CONTROL::New( const TOOL_EVENT& aEvent )
{
    wxCommandEvent evt( wxEVT_NULL, wxID_NEW );
    m_frame->Files_io( evt );
    return 0;
}


int PL_EDITOR_CONTROL::Open( const TOOL_EVENT& aEvent )
{
    wxCommandEvent evt( wxEVT_NULL, wxID_OPEN );
    m_frame->Files_io( evt );
    return 0;
}


int PL_EDITOR_CONTROL::Save( const TOOL_EVENT& aEvent )
{
    wxCommandEvent evt( wxEVT_NULL, wxID_SAVE );
    m_frame->Files_io( evt );
    return 0;
}


int PL_EDITOR_CONTROL::SaveAs( const TOOL_EVENT& aEvent )
{
    wxCommandEvent evt( wxEVT_NULL, wxID_SAVEAS );
    m_frame->Files_io( evt );
    return 0;
}


int PL_EDITOR_CONTROL::PageSetup( const TOOL_EVENT& aEvent )
{
    m_frame->SaveCopyInUndoList( true );

    DIALOG_PAGES_SETTINGS dlg( m_frame, wxSize( MAX_PAGE_SIZE_MILS, MAX_PAGE_SIZE_MILS ) );
    dlg.SetWksFileName( m_frame->GetCurrFileName() );
    dlg.EnableWksFileNamePicker( false );

    if( dlg.ShowModal() != wxID_OK )
    {
        // Nothing to roll back but we have to at least pop the stack
        m_frame->RollbackFromUndo();
    }
    else
    {
        m_frame->HardRedraw();
    }
    return 0;
}


int PL_EDITOR_CONTROL::Print( const TOOL_EVENT& aEvent )
{
    m_frame->ToPrinter( false );
    return 0;
}


int PL_EDITOR_CONTROL::Plot( const TOOL_EVENT& aEvent )
{
    wxMessageBox( wxT( "Not yet available" ) );
    return 0;
}


int PL_EDITOR_CONTROL::ToggleBackgroundColor( const TOOL_EVENT& aEvent )
{
    m_frame->SetDrawBgColor( m_frame->GetDrawBgColor() == WHITE ? BLACK : WHITE );
    getView()->GetPainter()->GetSettings()->SetBackgroundColor( m_frame->GetDrawBgColor() );

    m_frame->GetCanvas()->GetView()->UpdateAllLayersColor();
    m_frame->GetCanvas()->Refresh();

    return 0;
}


int PL_EDITOR_CONTROL::ShowInspector( const TOOL_EVENT& aEvent )
{
    m_frame->ShowDesignInspector();
    return 0;
}


int PL_EDITOR_CONTROL::UpdateMessagePanel( const TOOL_EVENT& aEvent )
{
    PL_SELECTION_TOOL* selTool = m_toolMgr->GetTool<PL_SELECTION_TOOL>();
    PL_SELECTION&      selection = selTool->GetSelection();

    // The Properties frame will be updated. Avoid flicker during update:
    m_frame->GetPropertiesFrame()->Freeze();

    if( selection.GetSize() == 1 )
    {
        EDA_ITEM* item = (EDA_ITEM*) selection.Front();

        MSG_PANEL_ITEMS msgItems;
        item->GetMsgPanelInfo( m_frame->GetUserUnits(), msgItems );
        m_frame->SetMsgPanel( msgItems );

        WS_DATA_ITEM* dataItem = static_cast<WS_DRAW_ITEM_BASE*>( item )->GetPeer();
        m_frame->GetPropertiesFrame()->CopyPrmsFromItemToPanel( dataItem );
    }
    else
    {
        m_frame->ClearMsgPanel();
        m_frame->GetPropertiesFrame()->CopyPrmsFromItemToPanel( nullptr );
    }

    m_frame->GetPropertiesFrame()->CopyPrmsFromGeneralToPanel();

    // The Properties frame is updated. Reenable it:
    m_frame->GetPropertiesFrame()->Thaw();

    return 0;
}


void PL_EDITOR_CONTROL::setTransitions()
{
    Go( &PL_EDITOR_CONTROL::New,                   ACTIONS::doNew.MakeEvent() );
    Go( &PL_EDITOR_CONTROL::Open,                  ACTIONS::open.MakeEvent() );
    Go( &PL_EDITOR_CONTROL::Save,                  ACTIONS::save.MakeEvent() );
    Go( &PL_EDITOR_CONTROL::SaveAs,                ACTIONS::saveAs.MakeEvent() );
    Go( &PL_EDITOR_CONTROL::Print,                 ACTIONS::print.MakeEvent() );
    Go( &PL_EDITOR_CONTROL::Plot,                  ACTIONS::plot.MakeEvent() );

    Go( &PL_EDITOR_CONTROL::PageSetup,             PL_ACTIONS::previewSettings.MakeEvent() );
    Go( &PL_EDITOR_CONTROL::ToggleBackgroundColor, PL_ACTIONS::toggleBackground.MakeEvent() );
    Go( &PL_EDITOR_CONTROL::ShowInspector,         PL_ACTIONS::showInspector.MakeEvent() );

    Go( &PL_EDITOR_CONTROL::UpdateMessagePanel,    EVENTS::SelectedEvent );
    Go( &PL_EDITOR_CONTROL::UpdateMessagePanel,    EVENTS::UnselectedEvent );
    Go( &PL_EDITOR_CONTROL::UpdateMessagePanel,    EVENTS::ClearedEvent );
    Go( &PL_EDITOR_CONTROL::UpdateMessagePanel,    EVENTS::SelectedItemsModified );
}
