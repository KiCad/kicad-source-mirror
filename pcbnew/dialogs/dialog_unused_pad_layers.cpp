/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 CERN
 * Copyright (C) 2020 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "dialog_unused_pad_layers.h"

#include <bitmaps.h>
#include <board_commit.h>
#include <footprint.h>
#include <track.h>
#include <pad.h>
#include <pcb_edit_frame.h>
#include <tools/pcb_selection_tool.h>

enum SCOPE : int
{
    SCOPE_VIAS = 0,
    SCOPE_PADS = 1
};


enum PAD_ACTION : int
{
    PAD_ACTION_REMOVE = 0,
    PAD_ACTION_RESET
};


DIALOG_UNUSED_PAD_LAYERS::DIALOG_UNUSED_PAD_LAYERS( PCB_BASE_FRAME* aParent,
                                                    const PCB_SELECTION& aItems,
                                                    COMMIT& aCommit  )
    : DIALOG_UNUSED_PAD_LAYERS_BASE( aParent ),
      m_frame( aParent ),
      m_items( aItems ),
      m_commit( aCommit )
{
    m_StdButtonsOK->SetDefault();
    m_image->SetBitmap( KiBitmap( BITMAPS::pads_remove_unused ) );

    // Set keep Through Hole pads on external layers ON by default.
    // Because such a pad does not allow soldering/unsoldering, disable this option
    // is probably not frequent
    m_cbPreservePads->SetValue( true );

    // Now all widgets have the size fixed, call FinishDialogSettings
    finishDialogSettings();
}


void DIALOG_UNUSED_PAD_LAYERS::syncImages( wxCommandEvent& aEvent )
{
    if( m_rbAction->GetSelection() == PAD_ACTION_RESET )
        m_image->SetBitmap( KiBitmap( BITMAPS::pads_reset_unused ) );
    else if( m_cbPreservePads->IsChecked() )
        m_image->SetBitmap( KiBitmap( BITMAPS::pads_remove_unused_keep_bottom ) );
    else
        m_image->SetBitmap( KiBitmap( BITMAPS::pads_remove_unused ) );
}


bool DIALOG_UNUSED_PAD_LAYERS::TransferDataFromWindow()
{
    if( m_cbSelectedOnly->IsChecked() )
    {
        for( EDA_ITEM* item : m_items )
        {
            m_commit.Modify( item );

            if( item->Type() == PCB_VIA_T && m_rbScope->GetSelection() == SCOPE_VIAS )
            {
                VIA* via = static_cast<VIA*>( item );
                via->SetRemoveUnconnected( m_rbAction->GetSelection() == PAD_ACTION_REMOVE );
                via->SetKeepTopBottom( m_cbPreservePads->IsChecked() );
            }

            if( item->Type() == PCB_FOOTPRINT_T && m_rbScope->GetSelection() == SCOPE_PADS )
            {
                FOOTPRINT* footprint = static_cast<FOOTPRINT*>( item );

                for( PAD* pad : footprint->Pads() )
                {
                    pad->SetRemoveUnconnected( m_rbAction->GetSelection() == PAD_ACTION_REMOVE );
                    pad->SetKeepTopBottom( m_cbPreservePads->IsChecked() );
                }
            }

            if( item->Type() == PCB_PAD_T && m_rbScope->GetSelection() == SCOPE_PADS )
            {
                PAD* pad = static_cast<PAD*>( item );

                pad->SetRemoveUnconnected( m_rbAction->GetSelection() == PAD_ACTION_REMOVE );
                pad->SetKeepTopBottom( m_cbPreservePads->IsChecked() );
            }
        }
    }
    else
    {
        if( m_rbScope->GetSelection() == SCOPE_PADS )
        {
            for( FOOTPRINT* footprint : m_frame->GetBoard()->Footprints() )
            {
                m_commit.Modify( footprint );

                for( PAD* pad : footprint->Pads() )
                {
                    pad->SetRemoveUnconnected( m_rbAction->GetSelection() == PAD_ACTION_REMOVE );
                    pad->SetKeepTopBottom( m_cbPreservePads->IsChecked() );
                }
            }
        }
        else
        {
            for( TRACK* item : m_frame->GetBoard()->Tracks() )
            {
                if( item->Type() != PCB_VIA_T )
                    continue;

                m_commit.Modify( item );
                VIA* via = static_cast<VIA*>( item );
                via->SetRemoveUnconnected( m_rbAction->GetSelection() == PAD_ACTION_REMOVE );
                via->SetKeepTopBottom( m_cbPreservePads->IsChecked() );
            }
        }
    }

    m_commit.Push( _( "Set Unused Pad Properties" ) );
    return true;
}
