/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 1992-2012 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file edit_pcb_text.cpp
 * @brief Editing of text on copper and technical layers (TEXTE_PCB class)
 */

#include <fctsys.h>
#include <gr_basic.h>
#include <class_drawpanel.h>
#include <pcbnew.h>
#include <wxPcbStruct.h>
#include <macros.h>

#include <class_board.h>
#include <class_pcb_text.h>
#include <class_board_item.h>


static void Move_Texte_Pcb( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aPosition,
                            bool aErase );
static void Abort_Edit_Pcb_Text( EDA_DRAW_PANEL* Panel, wxDC* DC );


static TEXTE_PCB s_TextCopy( (BOARD_ITEM*) NULL ); /* copy of the edited text
                                                    * (used to undo/redo/abort
                                                    * a complex edition command
                                                    */


/*
 * Abort current text edit progress.
 * If a text is selected, its initial coord are regenerated
 */
void Abort_Edit_Pcb_Text( EDA_DRAW_PANEL* Panel, wxDC* DC )
{
    TEXTE_PCB* TextePcb = (TEXTE_PCB*) Panel->GetScreen()->GetCurItem();
    ( (PCB_EDIT_FRAME*) Panel->GetParent() )->SetCurItem( NULL );

    Panel->SetMouseCapture( NULL, NULL );

    if( TextePcb == NULL )  // Should not occur
        return;

#ifndef USE_WX_OVERLAY
    TextePcb->Draw( Panel, DC, GR_XOR );
#endif

    if( TextePcb->IsNew() )  // If new: remove it
    {
        TextePcb->DeleteStructure();
        return;
    }


    TextePcb->SwapData( &s_TextCopy );
    TextePcb->ClearFlags();
#ifndef USE_WX_OVERLAY
    TextePcb->Draw( Panel, DC, GR_OR );
#else
    Panel->Refresh();
#endif
}


/*
 *  Place the current text being moving
 */
void PCB_EDIT_FRAME::Place_Texte_Pcb( TEXTE_PCB* TextePcb, wxDC* DC )
{
    m_canvas->SetMouseCapture( NULL, NULL );
    SetCurItem( NULL );

    if( TextePcb == NULL )
        return;

    TextePcb->Draw( m_canvas, DC, GR_OR );
    OnModify();

    if( TextePcb->IsNew() )  // If new: prepare undo command
    {
        SaveCopyInUndoList( TextePcb, UR_NEW );
        TextePcb->ClearFlags();
        return;
    }

    if( TextePcb->IsMoving() ) // If moved only
    {
        SaveCopyInUndoList( TextePcb, UR_MOVED,
                            TextePcb->GetTextPosition() - s_TextCopy.GetTextPosition() );
    }
    else
    {
        // Restore initial params
        TextePcb->SwapData( &s_TextCopy );
        // Prepare undo command
        SaveCopyInUndoList( TextePcb, UR_CHANGED );
        // Restore current params
        TextePcb->SwapData( &s_TextCopy );
    }

    TextePcb->ClearFlags();
#ifdef USE_WX_OVERLAY
    m_canvas->Refresh();
#endif
}


void PCB_EDIT_FRAME::StartMoveTextePcb( TEXTE_PCB* aTextePcb, wxDC* aDC, bool aErase )
{
    if( aTextePcb == NULL )
        return;

    // if it is an existing item: prepare a copy to undo/abort command
    if( !aTextePcb->IsNew() )
        s_TextCopy.Copy( aTextePcb );

    aTextePcb->SetFlags( IS_MOVED );
    SetMsgPanel( aTextePcb );

#ifdef USE_WX_OVERLAY
    m_canvas->Refresh();
#endif

    SetCrossHairPosition( aTextePcb->GetTextPosition() );
    m_canvas->MoveCursorToCrossHair();

    m_canvas->SetMouseCapture( Move_Texte_Pcb, Abort_Edit_Pcb_Text );
    SetCurItem( aTextePcb );
    m_canvas->CallMouseCapture( aDC, wxDefaultPosition, aErase );
}


// Move  PCB text following the cursor.
static void Move_Texte_Pcb( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aPosition,
                            bool aErase )
{
    TEXTE_PCB* TextePcb = (TEXTE_PCB*) aPanel->GetScreen()->GetCurItem();

    if( TextePcb == NULL )
        return;

    if( aErase )
        TextePcb->Draw( aPanel, aDC, GR_XOR );

    TextePcb->SetTextPosition( aPanel->GetParent()->GetCrossHairPosition() );

    TextePcb->Draw( aPanel, aDC, GR_XOR );
}


void PCB_EDIT_FRAME::Delete_Texte_Pcb( TEXTE_PCB* TextePcb, wxDC* DC )
{
    if( TextePcb == NULL )
        return;

    TextePcb->Draw( m_canvas, DC, GR_XOR );

    SaveCopyInUndoList( TextePcb, UR_DELETED );
    TextePcb->UnLink();
    m_canvas->SetMouseCapture( NULL, NULL );
    SetCurItem( NULL );
}


TEXTE_PCB* PCB_EDIT_FRAME::CreateTextePcb( wxDC* aDC, TEXTE_PCB* aText )
{
    TEXTE_PCB* textePcb = new TEXTE_PCB( GetBoard() );

    if( aText )
    {
        textePcb->Copy( aText );
        GetBoard()->Add( textePcb );
        textePcb->SetFlags( IS_NEW );
        if( aDC )
            StartMoveTextePcb( textePcb, aDC, false ); // Don't erase aText when copying
    }
    else
    {
        GetBoard()->Add( textePcb );
        textePcb->SetFlags( IS_NEW );

        LAYER_ID layer = ( (PCB_SCREEN*) GetScreen() )->m_Active_Layer;

        textePcb->SetLayer( layer );

        // Set the mirrored option for layers on the BACK side of the board
        if( layer == B_Cu || layer == B_SilkS ||
            layer == B_Paste || layer == B_Mask ||
            layer == B_Adhes
            )
            textePcb->SetMirrored( true );

        textePcb->SetSize( GetBoard()->GetDesignSettings().m_PcbTextSize );
        textePcb->SetTextPosition( GetCrossHairPosition() );
        textePcb->SetThickness( GetBoard()->GetDesignSettings().m_PcbTextWidth );

        InstallTextPCBOptionsFrame( textePcb, aDC );

        if( textePcb->GetText().IsEmpty() )
        {
            textePcb->DeleteStructure();
            textePcb = NULL;
        }
        else if( aDC )
        {
            StartMoveTextePcb( textePcb, aDC );
        }
    }

    return textePcb;
}


void PCB_EDIT_FRAME::Rotate_Texte_Pcb( TEXTE_PCB* TextePcb, wxDC* DC )
{
    int angle    = 900;

    if( TextePcb == NULL )
        return;

    // Erase previous text:
    TextePcb->Draw( m_canvas, DC, GR_XOR );

    TextePcb->SetOrientation( TextePcb->GetOrientation() + angle );

    // Redraw text in new position:
    TextePcb->Draw( m_canvas, DC, GR_XOR );
    SetMsgPanel( TextePcb );

    if( TextePcb->GetFlags() == 0 )    // i.e. not edited, or moved
        SaveCopyInUndoList( TextePcb, UR_ROTATED, TextePcb->GetTextPosition() );
    else                 // set flag edit, to show it was a complex command
        TextePcb->SetFlags( IN_EDIT );

    OnModify();
#ifdef USE_WX_OVERLAY
    m_canvas->Refresh();
#endif
}


void PCB_EDIT_FRAME::FlipTextePcb( TEXTE_PCB* aTextePcb, wxDC* aDC )
{
    if( aTextePcb == NULL )
        return;

    aTextePcb->Draw( m_canvas, aDC, GR_XOR );

    aTextePcb->Flip( aTextePcb->GetTextPosition() );

    aTextePcb->Draw( m_canvas, aDC, GR_XOR );
    SetMsgPanel( aTextePcb );

    if( aTextePcb->GetFlags() == 0 )    // i.e. not edited, or moved
        SaveCopyInUndoList( aTextePcb, UR_FLIPPED, aTextePcb->GetTextPosition() );
    else                 // set edit flag, for the current command
        aTextePcb->SetFlags( IN_EDIT );

    OnModify();
#ifdef USE_WX_OVERLAY
    m_canvas->Refresh();
#endif
}
