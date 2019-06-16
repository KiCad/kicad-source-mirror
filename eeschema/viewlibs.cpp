/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2015-2019 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <pgm_base.h>
#include <sch_draw_panel.h>
#include <sch_view.h>
#include <confirm.h>
#include <eda_doc.h>

#include <viewlib_frame.h>
#include <eeschema_id.h>
#include <class_library.h>
#include <dialog_helpers.h>
#include <dialog_choose_component.h>
#include <symbol_tree_model_adapter.h>
#include <symbol_lib_table.h>

void LIB_VIEW_FRAME::OnSelectSymbol( wxCommandEvent& aEvent )
{
    std::unique_lock<std::mutex> dialogLock( DIALOG_CHOOSE_COMPONENT::g_Mutex, std::defer_lock );

    // One CHOOSE_COMPONENT dialog at a time.  User probaby can't handle more anyway.
    if( !dialogLock.try_lock() )
        return;

    // Container doing search-as-you-type.
    SYMBOL_LIB_TABLE* libs = Prj().SchSymbolLibTable();
    auto adapterPtr( SYMBOL_TREE_MODEL_ADAPTER::Create( libs ) );
    auto adapter = static_cast<SYMBOL_TREE_MODEL_ADAPTER*>( adapterPtr.get() );

    const auto libNicknames = libs->GetLogicalLibs();
    adapter->AddLibraries( libNicknames, this );

    LIB_ALIAS *current = GetSelectedAlias();
    LIB_ID id;
    int unit = 0;

    if( current )
    {
        id = current->GetLibId();
        adapter->SetPreselectNode( id, unit );
    }

    wxString dialogTitle;
    dialogTitle.Printf( _( "Choose Symbol (%d items loaded)" ), adapter->GetItemCount() );

    DIALOG_CHOOSE_COMPONENT dlg( this, dialogTitle, adapterPtr, m_convert, false, false, false );

    if( dlg.ShowQuasiModal() == wxID_CANCEL )
        return;

    id = dlg.GetSelectedLibId( &unit );

    if( !id.IsValid() )
        return;

    SetSelectedLibrary( id.GetLibNickname() );
    SetSelectedComponent( id.GetLibItemName() );
    SetUnitAndConvert( unit, 1 );
}


void LIB_VIEW_FRAME::onSelectNextSymbol( wxCommandEvent& aEvent )
{
    wxCommandEvent evt( wxEVT_COMMAND_LISTBOX_SELECTED, ID_LIBVIEW_CMP_LIST );
    int ii = m_cmpList->GetSelection();

    // Select the next symbol or stop at the end of the list.
    if( ii != wxNOT_FOUND || ii != (int)m_cmpList->GetCount() - 1 )
        ii += 1;

    m_cmpList->SetSelection( ii );
    ProcessEvent( evt );
}


void LIB_VIEW_FRAME::onSelectPreviousSymbol( wxCommandEvent& aEvent )
{
    wxCommandEvent evt( wxEVT_COMMAND_LISTBOX_SELECTED, ID_LIBVIEW_CMP_LIST );
    int ii = m_cmpList->GetSelection();

    // Select the previous symbol or stop at the beginning of list.
    if( ii != wxNOT_FOUND && ii != 0 )
        ii -= 1;

    m_cmpList->SetSelection( ii );
    ProcessEvent( evt );
}


void LIB_VIEW_FRAME::onSelectSymbolUnit( wxCommandEvent& aEvent )
{
    int ii = m_unitChoice->GetSelection();

    if( ii < 0 )
        return;

    m_unit = ii + 1;

    updatePreviewSymbol();
}


void LIB_VIEW_FRAME::DisplayLibInfos()
{
    if( m_libList && !m_libList->IsEmpty() && !m_libraryName.IsEmpty() )
    {
        const SYMBOL_LIB_TABLE_ROW* row = Prj().SchSymbolLibTable()->FindRow( m_libraryName );

        wxString title = wxString::Format( _( "Symbol Library Browser -- %s" ),
                                           row ? row->GetFullURI() : _( "no library selected" ) );
        SetTitle( title );
    }
}

