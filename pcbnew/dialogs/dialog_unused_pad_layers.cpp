/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 CERN
 * Copyright (C) 2020-2023 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <pcb_track.h>
#include <pad.h>
#include <pcb_edit_frame.h>
#include <tools/pcb_selection_tool.h>


DIALOG_UNUSED_PAD_LAYERS::DIALOG_UNUSED_PAD_LAYERS( PCB_BASE_FRAME* aParent,
                                                    const PCB_SELECTION& aItems,
                                                    COMMIT& aCommit  )
    : DIALOG_UNUSED_PAD_LAYERS_BASE( aParent ),
      m_frame( aParent ),
      m_items( aItems ),
      m_commit( aCommit )
{
    m_image->SetBitmap( KiBitmap( BITMAPS::pads_remove_unused ) );

    // Set keep Through Hole pads on external layers ON by default.
    // Because such a pad does not allow soldering/unsoldering, disable this option
    // is probably not frequent
    m_cbPreservePads->SetValue( true );

    SetupStandardButtons( { { wxID_OK,     _( "Remove Unused Layers" ) },
                            { wxID_APPLY,  _( "Restore All Layers" )   },
                            { wxID_CANCEL, _( "Cancel" )               } } );

    updateImage();

    // Now all widgets have the size fixed, call FinishDialogSettings
    finishDialogSettings();
}


void DIALOG_UNUSED_PAD_LAYERS::updateImage()
{
    if( m_cbPreservePads->IsChecked() )
        m_image->SetBitmap( KiBitmap( BITMAPS::pads_remove_unused_keep_bottom ) );
    else
        m_image->SetBitmap( KiBitmap( BITMAPS::pads_remove_unused ) );
}


void DIALOG_UNUSED_PAD_LAYERS::syncImages( wxCommandEvent& aEvent )
{
    updateImage();
}


void DIALOG_UNUSED_PAD_LAYERS::onApply( wxCommandEvent& event )
{
    updatePadsAndVias( false );
    EndModal( wxID_APPLY );
}


void DIALOG_UNUSED_PAD_LAYERS::onOK( wxCommandEvent& event )
{
    updatePadsAndVias( true );   // called only with wxID_OK
    event.Skip();
}


void  DIALOG_UNUSED_PAD_LAYERS::updatePadsAndVias( bool aRemoveLayers )
{
    if( m_cbSelectedOnly->IsChecked() )
    {
        for( EDA_ITEM* item : m_items )
        {
            m_commit.Modify( item );

            if( item->Type() == PCB_VIA_T && m_cbVias->IsChecked() )
            {
                PCB_VIA* via = static_cast<PCB_VIA*>( item );
                via->SetRemoveUnconnected( aRemoveLayers );
                via->SetKeepStartEnd( m_cbPreservePads->IsChecked() );
            }

            if( item->Type() == PCB_FOOTPRINT_T && m_cbPads->IsChecked() )
            {
                FOOTPRINT* footprint = static_cast<FOOTPRINT*>( item );

                for( PAD* pad : footprint->Pads() )
                {
                    pad->SetRemoveUnconnected( aRemoveLayers );
                    pad->SetKeepTopBottom( m_cbPreservePads->IsChecked() );
                }
            }

            if( item->Type() == PCB_PAD_T && m_cbPads->IsChecked() )
            {
                PAD* pad = static_cast<PAD*>( item );

                pad->SetRemoveUnconnected( aRemoveLayers );
                pad->SetKeepTopBottom( m_cbPreservePads->IsChecked() );
            }
        }
    }
    else
    {
        if( m_cbPads->IsChecked() )
        {
            for( FOOTPRINT* footprint : m_frame->GetBoard()->Footprints() )
            {
                m_commit.Modify( footprint );

                for( PAD* pad : footprint->Pads() )
                {
                    pad->SetRemoveUnconnected( aRemoveLayers );
                    pad->SetKeepTopBottom( m_cbPreservePads->IsChecked() );
                }
            }
        }

        if( m_cbVias->IsChecked() )
        {
            for( PCB_TRACK* item : m_frame->GetBoard()->Tracks() )
            {
                if( item->Type() != PCB_VIA_T )
                    continue;

                m_commit.Modify( item );
                PCB_VIA* via = static_cast<PCB_VIA*>( item );
                via->SetRemoveUnconnected( aRemoveLayers );
                via->SetKeepStartEnd( m_cbPreservePads->IsChecked() );
            }
        }
    }

    m_commit.Push( _( "Set Unused Pad Properties" ) );
}
