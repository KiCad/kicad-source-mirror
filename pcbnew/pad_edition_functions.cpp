/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2011 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file pad_edition_functions.cpp
 */

#include <fctsys.h>
#include <class_drawpanel.h>
#include <confirm.h>
#include <trigo.h>
#include <macros.h>
#include <wxBasePcbFrame.h>

#include <pcbnew.h>
#include <class_board.h>
#include <class_module.h>
#include <class_pad.h>
#include <class_board_design_settings.h>

/* Exports the current pad settings to board design settings.
 */
void PCB_BASE_FRAME::Export_Pad_Settings( D_PAD* aPad )
{
    if( aPad == NULL )
        return;

    SetMsgPanel( aPad );

    D_PAD& mp = GetDesignSettings().m_Pad_Master;

    mp.SetShape( aPad->GetShape() );
    mp.SetAttribute( aPad->GetAttribute() );
    mp.SetLayerSet( aPad->GetLayerSet() );

    mp.SetOrientation( aPad->GetOrientation() - aPad->GetParent()->GetOrientation() );

    mp.SetSize( aPad->GetSize() );
    mp.SetDelta( aPad->GetDelta() );

    mp.SetOffset( aPad->GetOffset() );
    mp.SetDrillSize( aPad->GetDrillSize() );
    mp.SetDrillShape( aPad->GetDrillShape() );
}


/* Imports the board design settings to aPad
 * - The position, names, and keys are not modifed.
 */
void PCB_BASE_FRAME::Import_Pad_Settings( D_PAD* aPad, bool aDraw )
{
    if( aDraw )
    {
        aPad->SetFlags( DO_NOT_DRAW );
        m_canvas->RefreshDrawingRect( aPad->GetBoundingBox() );
        aPad->ClearFlags( DO_NOT_DRAW );
    }

    D_PAD& mp = GetDesignSettings().m_Pad_Master;

    aPad->SetShape( mp.GetShape() );
    aPad->SetLayerSet( mp.GetLayerSet() );
    aPad->SetAttribute( mp.GetAttribute() );
    aPad->SetOrientation( mp.GetOrientation() + aPad->GetParent()->GetOrientation() );
    aPad->SetSize( mp.GetSize() );
    aPad->SetDelta( wxSize( 0, 0 ) );
    aPad->SetOffset( mp.GetOffset() );
    aPad->SetDrillSize( mp.GetDrillSize() );
    aPad->SetDrillShape( mp.GetDrillShape() );

    switch( mp.GetShape() )
    {
    case PAD_SHAPE_TRAPEZOID:
        aPad->SetDelta( mp.GetDelta() );
        break;

    case PAD_SHAPE_CIRCLE:
        // ensure size.y == size.x
        aPad->SetSize( wxSize( aPad->GetSize().x, aPad->GetSize().x ) );
        break;

    default:
        ;
    }

    switch( mp.GetAttribute() )
    {
    case PAD_ATTRIB_SMD:
    case PAD_ATTRIB_CONN:
        aPad->SetDrillSize( wxSize( 0, 0 ) );
        aPad->SetOffset( wxPoint( 0, 0 ) );
        break;
    default:
        ;
    }

    if( aDraw )
        m_canvas->RefreshDrawingRect( aPad->GetBoundingBox() );

    aPad->GetParent()->SetLastEditTime();

    OnModify();
}

/** Compute the 'next' pad number for autoincrement
 * aPadName is the last pad name used */
static wxString GetNextPadName( wxString aPadName )
{
    // Automatically increment the current pad number.
    int num    = 0;
    int ponder = 1;

    // Trim and extract the trailing numeric part
    while( aPadName.Len()
            && aPadName.Last() >= '0'
            && aPadName.Last() <= '9' )
    {
        num += ( aPadName.Last() - '0' ) * ponder;
        aPadName.RemoveLast();
        ponder *= 10;
    }

    num++;  // Use next number for the new pad
    aPadName << num;

    return aPadName;
}

/* Add a new pad to aModule.
 */
void PCB_BASE_FRAME::AddPad( MODULE* aModule, bool draw )
{
    m_Pcb->m_Status_Pcb     = 0;
    aModule->SetLastEditTime();

    D_PAD* pad = new D_PAD( aModule );

    // Add the new pad to end of the module pad list.
    aModule->Pads().PushBack( pad );

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
    {
        padName = GetNextPadName( GetDesignSettings()
                .m_Pad_Master.GetPadName() );
    }

    pad->SetPadName( padName );
    GetDesignSettings().m_Pad_Master.SetPadName( padName );

    aModule->CalculateBoundingBox();
    SetMsgPanel( pad );

    if( draw )
        m_canvas->RefreshDrawingRect( aModule->GetBoundingBox() );
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
        wxString msg;
        msg.Printf( _( "Delete Pad (footprint %s %s) ?" ),
                    GetChars( module->GetReference() ),
                    GetChars( module->GetValue() ) );

        if( !IsOK( this, msg ) )
            return;
    }

    // Stores the initial bounding box to refresh the old area
    EDA_RECT bbox = module->GetBoundingBox();

    m_Pcb->m_Status_Pcb = 0;

    GetBoard()->PadDelete( aPad );

    // Update the bounding box
    module->CalculateBoundingBox();

    // Refresh the modified screen area, using the initial bounding box
    // which is perhaps larger than the new bounding box
    m_canvas->RefreshDrawingRect( bbox );

    OnModify();
}


// Rotate selected pad 90 degrees.
void PCB_BASE_FRAME::RotatePad( D_PAD* aPad, wxDC* DC )
{
    if( aPad == NULL )
        return;

    MODULE* module = aPad->GetParent();

    module->SetLastEditTime();

    OnModify();

    if( DC )
        module->Draw( m_canvas, DC, GR_XOR );

    wxSize  sz = aPad->GetSize();
    std::swap( sz.x, sz.y );
    aPad->SetSize( sz );

    sz = aPad->GetDrillSize();
    std::swap( sz.x, sz.y );
    aPad->SetDrillSize( sz );

    wxPoint pt = aPad->GetOffset();
    std::swap( pt.x, pt.y );
    aPad->SetOffset( pt );

    aPad->SetOffset( wxPoint( aPad->GetOffset().x, -aPad->GetOffset().y ) );

    sz = aPad->GetDelta();
    std::swap( sz.x, sz.y );
    sz.x = -sz.x;
    aPad->SetDelta( sz );

    module->CalculateBoundingBox();
    SetMsgPanel( aPad );

    if( DC )
        module->Draw( m_canvas, DC, GR_OR );
}
