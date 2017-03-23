/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2015-2017 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file viewlibs.cpp
 */

#include <fctsys.h>
#include <kiway.h>
#include <gr_basic.h>
#include <pgm_base.h>
#include <class_drawpanel.h>
#include <confirm.h>
#include <eda_doc.h>
#include <class_sch_screen.h>

#include <general.h>
#include <viewlib_frame.h>
#include <eeschema_id.h>
#include <class_library.h>
#include <dialog_helpers.h>
#include <dialog_choose_component.h>
#include <cmp_tree_model_adapter.h>


void LIB_VIEW_FRAME::OnSelectSymbol( wxCommandEvent& aEvent )
{
    wxString   dialogTitle;
    PART_LIBS* libs = Prj().SchLibs();

    // Container doing search-as-you-type.
    auto adapter( CMP_TREE_MODEL_ADAPTER::Create( libs ) );

    for( PART_LIB& lib : *libs )
    {
        adapter->AddLibrary( lib );
    }

    dialogTitle.Printf( _( "Choose Component (%d items loaded)" ),
                        adapter->GetComponentsCount() );
    DIALOG_CHOOSE_COMPONENT dlg( this, dialogTitle, adapter, m_convert, false );

    if( dlg.ShowQuasiModal() == wxID_CANCEL )
        return;

    /// @todo: The unit selection gets reset to 1 by SetSelectedComponent() so the unit
    ///        selection feature of the choose symbol dialog doesn't work.
    LIB_ALIAS* const alias = dlg.GetSelectedAlias( &m_unit );

    if( !alias || !alias->GetLib() )
        return;

    if( m_libraryName == alias->GetLib()->GetName() )
    {
        if( m_entryName != alias->GetName() )
            SetSelectedComponent( alias->GetName() );
    }
    else
    {
        m_entryName = alias->GetName();
        SetSelectedLibrary( alias->GetLib()->GetName() );
    }
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


void LIB_VIEW_FRAME::onViewSymbolDocument( wxCommandEvent& aEvent )
{
    LIB_ID id( wxEmptyString, m_entryName );
    LIB_ALIAS* entry = Prj().SchLibs()->FindLibraryAlias( id, m_libraryName );

    if( entry && !entry->GetDocFileName().IsEmpty() )
    {
        SEARCH_STACK* lib_search = Prj().SchSearchS();

        GetAssociatedDocument( this, entry->GetDocFileName(), lib_search );
    }
}


void LIB_VIEW_FRAME::onSelectSymbolBodyStyle( wxCommandEvent& aEvent )
{
    int id = aEvent.GetId();

    switch( id )
    {
    default:
    case ID_LIBVIEW_DE_MORGAN_NORMAL_BUTT:
        m_convert = 1;
        break;

    case ID_LIBVIEW_DE_MORGAN_CONVERT_BUTT:
        m_convert = 2;
        break;
    }

    m_canvas->Refresh();
}


void LIB_VIEW_FRAME::onSelectSymbolUnit( wxCommandEvent& aEvent )
{
    int ii = m_selpartBox->GetCurrentSelection();

    if( ii < 0 )
        return;

    m_unit = ii + 1;
    m_canvas->Refresh();
}


void LIB_VIEW_FRAME::OnLeftClick( wxDC* DC, const wxPoint& MousePos )
{
}


bool LIB_VIEW_FRAME::OnRightClick( const wxPoint& MousePos, wxMenu* PopMenu )
{
    return true;
}


void LIB_VIEW_FRAME::DisplayLibInfos()
{
    PART_LIBS*  libs = Prj().SchLibs();

    if( libs )
    {
        PART_LIB* lib = libs->FindLibrary( m_libraryName );

        wxString title = wxString::Format( L"Library Browser \u2014 %s",
            lib ? lib->GetFullFileName() : "no library selected" );
        SetTitle( title );
    }
}


void LIB_VIEW_FRAME::RedrawActiveWindow( wxDC* DC, bool EraseBg )
{
    LIB_ID id( wxEmptyString, m_entryName );
    LIB_ALIAS* entry = Prj().SchLibs()->FindLibraryAlias( id, m_libraryName );

    if( !entry )
        return;

    LIB_PART* part = entry->GetPart();

    if( !part )
        return;

    wxString    msg;
    wxString    tmp;

    m_canvas->DrawBackGround( DC );

    if( !entry->IsRoot() )
    {
        // Temporarily change the name field text to reflect the alias name.
        msg = entry->GetName();
        tmp = part->GetName();

        part->SetName( msg );

        if( m_unit < 1 )
            m_unit = 1;

        if( m_convert < 1 )
            m_convert = 1;
    }
    else
        msg = _( "None" );

    auto opts = PART_DRAW_OPTIONS::Default();
    opts.show_elec_type = GetShowElectricalType();
    part->Draw( m_canvas, DC, wxPoint( 0, 0 ), m_unit, m_convert, opts );

    // Redraw the cursor
    m_canvas->DrawCrossHair( DC );

    if( !tmp.IsEmpty() )
        part->SetName( tmp );

    ClearMsgPanel();
    AppendMsgPanel( _( "Part" ), part->GetName(), BLUE, 6 );
    AppendMsgPanel( _( "Alias" ), msg, RED, 6 );
    AppendMsgPanel( _( "Description" ), entry->GetDescription(), CYAN, 6 );
    AppendMsgPanel( _( "Key words" ), entry->GetKeyWords(), DARKDARKGRAY );
}
