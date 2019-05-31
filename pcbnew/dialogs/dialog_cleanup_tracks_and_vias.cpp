/**
 * @file dialog_cleaning_options.cpp
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2019 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <wx/wx.h>

#include <dialog_cleanup_tracks_and_vias.h>
#include <wx_html_report_panel.h>
#include <board_commit.h>
#include <tools/pcb_actions.h>
#include <pcb_edit_frame.h>
#include <tracks_cleaner.h>
#include <reporter.h>
#include <class_drawpanel.h>
#include <tool/tool_manager.h>
#include <collectors.h>
#include "dialog_drclistbox.h"

// Static members of DIALOG_CLEANUP_TRACKS_AND_VIAS
bool DIALOG_CLEANUP_TRACKS_AND_VIAS::m_cleanVias = true;
bool DIALOG_CLEANUP_TRACKS_AND_VIAS::m_mergeSegments = true;
bool DIALOG_CLEANUP_TRACKS_AND_VIAS::m_deleteUnconnectedSegm = true;
bool DIALOG_CLEANUP_TRACKS_AND_VIAS::m_deleteShortCircuits = true;


DIALOG_CLEANUP_TRACKS_AND_VIAS::DIALOG_CLEANUP_TRACKS_AND_VIAS( PCB_EDIT_FRAME* aParentFrame ):
        DIALOG_CLEANUP_TRACKS_AND_VIAS_BASE( aParentFrame ),
    m_parentFrame( aParentFrame )
{
    m_cleanViasOpt->SetValue( m_cleanVias );
    m_mergeSegmOpt->SetValue( m_mergeSegments );
    m_deleteUnconnectedOpt->SetValue( m_deleteUnconnectedSegm );
    m_cleanShortCircuitOpt->SetValue( m_deleteShortCircuits );

    // We use a sdbSizer to get platform-dependent ordering of the action buttons, but
    // that requires us to correct the button labels here.
    m_sdbSizerOK->SetLabel( _( "Update PCB" ) );

    m_sdbSizerOK->SetDefault();
    GetSizer()->SetSizeHints(this);
    Centre();
}


DIALOG_CLEANUP_TRACKS_AND_VIAS::~DIALOG_CLEANUP_TRACKS_AND_VIAS()
{
    m_cleanVias = m_cleanViasOpt->GetValue();
    m_mergeSegments = m_mergeSegmOpt->GetValue();
    m_deleteUnconnectedSegm = m_deleteUnconnectedOpt->GetValue();
    m_deleteShortCircuits = m_cleanShortCircuitOpt->GetValue();

    for( DRC_ITEM* item : m_items )
        delete item;
}


void DIALOG_CLEANUP_TRACKS_AND_VIAS::OnCheckBox( wxCommandEvent& anEvent )
{
    doCleanup( true );
}


bool DIALOG_CLEANUP_TRACKS_AND_VIAS::TransferDataToWindow()
{
    doCleanup( true );

    return true;
}


bool DIALOG_CLEANUP_TRACKS_AND_VIAS::TransferDataFromWindow()
{
    doCleanup( false );

    return true;
}


void DIALOG_CLEANUP_TRACKS_AND_VIAS::doCleanup( bool aDryRun )
{
    for( DRC_ITEM* item : m_items )
        delete item;

    m_items.clear();

    wxBusyCursor busy;
    BOARD_COMMIT commit( m_parentFrame );
    TRACKS_CLEANER cleaner( m_parentFrame->GetUserUnits(), m_parentFrame->GetBoard(), commit );

    if( !aDryRun )
    {
        // Clear current selection list to avoid selection of deleted items
        m_parentFrame->GetToolManager()->RunAction( PCB_ACTIONS::selectionClear, true );
    }

    // Old model has to be refreshed, GAL normally does not keep updating it
    m_parentFrame->Compile_Ratsnest( false );

    bool modified = cleaner.CleanupBoard( aDryRun, &m_items,
                                          m_cleanShortCircuitOpt->GetValue(),
                                          m_cleanViasOpt->GetValue(),
                                          m_mergeSegmOpt->GetValue(),
                                          m_deleteUnconnectedOpt->GetValue() );

    if( aDryRun )
    {
        m_ItemsListBox->SetList( GetUserUnits(), new DRC_LIST_GENERIC( &m_items ) );
    }
    else if( modified )
    {
        // Clear undo and redo lists to avoid inconsistencies between lists
        commit.Push( _( "Board cleanup" ) );
        m_parentFrame->GetGalCanvas()->Refresh( true );
    }
}


void DIALOG_CLEANUP_TRACKS_AND_VIAS::OnSelectItem( wxCommandEvent& event )
{
    int selection = event.GetSelection();

    if( selection != wxNOT_FOUND )
    {
        // Find the selected DRC_ITEM in the listbox, position cursor there.
        const DRC_ITEM* item = m_ItemsListBox->GetItem( selection );

        if( item )
        {
            m_parentFrame->FocusOnLocation( item->GetPointA(), false, true );
            WINDOW_THAWER thawer( m_parentFrame );
            m_parentFrame->GetGalCanvas()->Refresh();
        }
    }

    event.Skip();
}


void DIALOG_CLEANUP_TRACKS_AND_VIAS::OnLeftDClickItem( wxMouseEvent& event )
{
    event.Skip();

    int selection = m_ItemsListBox->GetSelection();

    if( selection != wxNOT_FOUND )
    {
        // Find the selected DRC_ITEM in the listbox, position cursor there.
        // Then hide the dialog.
        const DRC_ITEM* item = m_ItemsListBox->GetItem( selection );
        if( item )
        {
            m_parentFrame->FocusOnLocation( item->GetPointA(), true, true );

            if( !IsModal() )
                Show( false );
        }
    }
}


void DIALOG_CLEANUP_TRACKS_AND_VIAS::OnRightUpItem( wxMouseEvent& event )
{
    // popup menu to go to either of the items listed in the DRC_ITEM.

    int selection = m_ItemsListBox->GetSelection();

    if( selection != wxNOT_FOUND )
    {
        // popup menu to go to either of the items listed in the DRC_ITEM.
        const DRC_ITEM* item = m_ItemsListBox->GetItem( selection );
        GENERAL_COLLECTOR items;

        items.Append( item->GetMainItem( m_parentFrame->GetBoard() ) );

        if( item->HasSecondItem() )
            items.Append( item->GetAuxiliaryItem( m_parentFrame->GetBoard() ) );

        WINDOW_THAWER thawer( m_parentFrame );
        m_parentFrame->GetToolManager()->RunAction( PCB_ACTIONS::selectionMenu, true, &items );
        m_parentFrame->GetGalCanvas()->Refresh();
    }
}


