/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2018 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <confirm.h>
#include <trigo.h>
#include <macros.h>
#include <pcb_base_frame.h>
#include <pcb_edit_frame.h>
#include <footprint_edit_frame.h>
#include <pcbnew.h>
#include <class_board.h>
#include <class_module.h>
#include <class_pad.h>
#include <board_design_settings.h>
#include <dialog_push_pad_properties.h>

/*
 * Exports the current pad settings to board design settings.
 */
void PCB_BASE_FRAME::Export_Pad_Settings( D_PAD* aPad )
{
    if( aPad == NULL )
        return;

    SetMsgPanel( aPad );

    D_PAD& masterPad = GetDesignSettings().m_Pad_Master;

    masterPad.ImportSettingsFromMaster( *aPad );
}


/*
 * Imports the board design settings to aPad
 * - The position, names, and keys are not modifed.
 * The parameters are expected to be correct (i.e. settings are valid)
 */
void PCB_BASE_FRAME::Import_Pad_Settings( D_PAD* aPad, bool aDraw )
{
    if( aDraw )
    {
        aPad->SetFlags( DO_NOT_DRAW );
        GetGalCanvas()->Refresh();
        aPad->ClearFlags( DO_NOT_DRAW );
    }

    const D_PAD& mp = GetDesignSettings().m_Pad_Master;

    aPad->ImportSettingsFromMaster( mp );

    if( aDraw )
        GetGalCanvas()->Refresh();

    aPad->GetParent()->SetLastEditTime();

    OnModify();
}

/*
 * Compute the 'next' pad number for autoincrement
 * aPadName is the last pad name used
 * */
static wxString GetNextPadName( wxString aPadName )
{
    // Automatically increment the current pad number.
    int num    = 0;
    int ponder = 1;

    // Trim and extract the trailing numeric part
    while( aPadName.Len() && aPadName.Last() >= '0' && aPadName.Last() <= '9' )
    {
        num += ( aPadName.Last() - '0' ) * ponder;
        aPadName.RemoveLast();
        ponder *= 10;
    }

    num++;  // Use next number for the new pad
    aPadName << num;

    return aPadName;
}

/*
 * Add a new pad to aModule.
 */
void PCB_BASE_FRAME::AddPad( MODULE* aModule, bool draw )
{
    aModule->SetLastEditTime();

    D_PAD* pad = new D_PAD( aModule );

    // Add the new pad to end of the module pad list.
    aModule->Add( pad );

    // Update the pad properties,
    // and keep NETINFO_LIST::ORPHANED as net info
    // which is the default when nets cannot be handled.
    Import_Pad_Settings( pad, false );

    pad->SetPosition( GetCrossHairPosition() );

    // Set the relative pad position
    // ( pad position for module orient, 0, and relative to the module position)

    wxPoint pos0 = pad->GetPosition() - aModule->GetPosition();
    RotatePoint( &pos0, -aModule->GetOrientation() );
    pad->SetPos0( pos0 );

    /* NPTH pads take empty pad number (since they can't be connected),
     * other pads get incremented from the last one edited */
    wxString padName;

    if( pad->GetAttribute() != PAD_ATTRIB_HOLE_NOT_PLATED )
        padName = GetNextPadName( GetDesignSettings().m_Pad_Master.GetName() );

    pad->SetName( padName );
    GetDesignSettings().m_Pad_Master.SetName( padName );

    aModule->CalculateBoundingBox();
    SetMsgPanel( pad );

    if( draw )
        GetGalCanvas()->Refresh();
}


void PCB_BASE_FRAME::DeletePad( D_PAD* aPad, bool aQuery )
{
    if( aPad == NULL )
        return;

    MODULE* module = aPad->GetParent();
    module->SetLastEditTime();

    // aQuery = true to prompt for confirmation, false to delete silently
    if( aQuery )
    {
        wxString msg = wxString::Format( _( "Delete pad (footprint %s %s)?" ),
                                         module->GetReference(),
                                         module->GetValue() );

        if( !IsOK( this, msg ) )
            return;
    }

    GetBoard()->PadDelete( aPad );

    // Update the bounding box
    module->CalculateBoundingBox();

    GetGalCanvas()->Refresh();
    OnModify();
}
