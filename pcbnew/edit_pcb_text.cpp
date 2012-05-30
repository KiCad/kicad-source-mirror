/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
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
 * @file edit_pcb_text.cpp
 * @brief Editimg of text on copper and technical layers (TEXTE_PCB class)
 */

#include <fctsys.h>
#include <gr_basic.h>
#include <class_drawpanel.h>
#include <pcbnew.h>
#include <wxPcbStruct.h>
#include <macros.h>

#include <class_board.h>
#include <class_pcb_text.h>

#include <protos.h>


static void Move_Texte_Pcb( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aPosition,
                            bool aErase );
static void Abort_Edit_Pcb_Text( EDA_DRAW_PANEL* Panel, wxDC* DC );


static TEXTE_PCB s_TextCopy( (BOARD_ITEM*) NULL ); /* copy of the edited text
                                                    * (used to undo/redo/abort
                                                    * a complex edition command
                                                    */


/*
 * Abort current text edit progress.
 *
 * If a text is selected, its initial coord are regenerated
 */
void Abort_Edit_Pcb_Text( EDA_DRAW_PANEL* Panel, wxDC* DC )
{
    TEXTE_PCB* TextePcb = (TEXTE_PCB*) Panel->GetScreen()->GetCurItem();
    ( (PCB_EDIT_FRAME*) Panel->GetParent() )->SetCurItem( NULL );

    Panel->SetMouseCapture( NULL, NULL );

    if( TextePcb == NULL )  // Should not occur
        return;

    TextePcb->Draw( Panel, DC, GR_XOR );

    if( TextePcb->IsNew() )  // If new: remove it
    {
        TextePcb->DeleteStructure();
        return;
    }


    SwapData(TextePcb, &s_TextCopy);
    TextePcb->ClearFlags();
    TextePcb->Draw( Panel, DC, GR_OR );
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
        SaveCopyInUndoList( TextePcb, UR_MOVED, TextePcb->m_Pos - s_TextCopy.m_Pos );
    }
    else
    {
        // Restore initial params
        SwapData( TextePcb, &s_TextCopy);
        // Prepare undo command
        SaveCopyInUndoList( TextePcb, UR_CHANGED );
        SwapData( TextePcb, &s_TextCopy);
        // Restore current params
    }

    TextePcb->ClearFlags();
}


/* Initialize parameters to move a pcb text
 */
void PCB_EDIT_FRAME::StartMoveTextePcb( TEXTE_PCB* TextePcb, wxDC* DC )
{
    if( TextePcb == NULL )
        return;

    // if it is an existing item: prepare a copy to undo/abort command
    if( !TextePcb->IsNew() )
        s_TextCopy.Copy( TextePcb );

    TextePcb->Draw( m_canvas, DC, GR_XOR );
    TextePcb->SetFlags( IS_MOVED );
    TextePcb->DisplayInfo( this );

    GetScreen()->SetCrossHairPosition( TextePcb->GetPosition() );
    m_canvas->MoveCursorToCrossHair();

    m_canvas->SetMouseCapture( Move_Texte_Pcb, Abort_Edit_Pcb_Text );
    SetCurItem( TextePcb );
    m_canvas->CallMouseCapture( DC, wxDefaultPosition, false );
}


/* Move  PCB text following the cursor. */
static void Move_Texte_Pcb( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aPosition,
                            bool aErase )
{
    TEXTE_PCB* TextePcb = (TEXTE_PCB*) aPanel->GetScreen()->GetCurItem();

    if( TextePcb == NULL )
        return;

    if( aErase )
        TextePcb->Draw( aPanel, aDC, GR_XOR );

    TextePcb->m_Pos = aPanel->GetScreen()->GetCrossHairPosition();

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


TEXTE_PCB* PCB_EDIT_FRAME::Create_Texte_Pcb( wxDC* DC )
{
    TEXTE_PCB* TextePcb;

    TextePcb = new TEXTE_PCB( GetBoard() );

    /* Add text to the board item list. */
    GetBoard()->Add( TextePcb );

    /* Update text properties. */
    TextePcb->SetFlags( IS_NEW );
    TextePcb->SetLayer( ( (PCB_SCREEN*) GetScreen() )->m_Active_Layer );
    TextePcb->m_Mirror = false;

    if( TextePcb->GetLayer() == LAYER_N_BACK )
        TextePcb->m_Mirror = true;

    TextePcb->m_Size  = GetBoard()->GetDesignSettings().m_PcbTextSize;
    TextePcb->m_Pos   = GetScreen()->GetCrossHairPosition();
    TextePcb->m_Thickness = GetBoard()->GetDesignSettings().m_PcbTextWidth;

    InstallTextPCBOptionsFrame( TextePcb, DC );

    if( TextePcb->m_Text.IsEmpty() )
    {
        TextePcb->DeleteStructure();
        TextePcb = NULL;
    }
    else
    {
        StartMoveTextePcb( TextePcb, DC );
    }

    return TextePcb;
}


void PCB_EDIT_FRAME::Rotate_Texte_Pcb( TEXTE_PCB* TextePcb, wxDC* DC )
{
    int angle    = 900;

    if( TextePcb == NULL )
        return;

    /* Erase previous text. */
    TextePcb->Draw( m_canvas, DC, GR_XOR );

    TextePcb->m_Orient += angle;
    NORMALIZE_ANGLE_POS( TextePcb->m_Orient );

    /* Redraw text in new position. */
    TextePcb->Draw( m_canvas, DC, GR_XOR );
    TextePcb->DisplayInfo( this );

    if( TextePcb->GetFlags() == 0 )    // i.e. not edited, or moved
        SaveCopyInUndoList( TextePcb, UR_ROTATED, TextePcb->GetPosition() );
    else                 // set flag edit, to show it was a complex command
        TextePcb->SetFlags( IN_EDIT );

    OnModify();
}


void PCB_EDIT_FRAME::FlipTextePcb( TEXTE_PCB* aTextePcb, wxDC* aDC )
{
    if( aTextePcb == NULL )
        return;

    aTextePcb->Draw( m_canvas, aDC, GR_XOR );

    aTextePcb->Flip( aTextePcb->GetPosition() );

    aTextePcb->Draw( m_canvas, aDC, GR_XOR );
    aTextePcb->DisplayInfo( this );

    if( aTextePcb->GetFlags() == 0 )    // i.e. not edited, or moved
        SaveCopyInUndoList( aTextePcb, UR_FLIPPED, aTextePcb->GetPosition() );
    else                 // set flag edit, to show it was a complex command
        aTextePcb->SetFlags( IN_EDIT );

    OnModify();
}
