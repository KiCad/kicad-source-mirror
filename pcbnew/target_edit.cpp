/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2013 KiCad Developers, see change_log.txt for contributors.
 *
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
 * @file target_edit.cpp
 * @brief Functions to edit targets (class #PCB_TARGET).
 */

#include <fctsys.h>
#include <class_drawpanel.h>
#include <pcb_edit_frame.h>
#include <dialog_helpers.h>
#include <base_units.h>
#include <gr_basic.h>
#include <board_commit.h>

#include <class_board.h>
#include <class_pcb_target.h>

#include <pcbnew.h>
#include <dialog_target_properties_base.h>
#include <widgets/unit_binder.h>

// Routines Locales
static void AbortMoveAndEditTarget( EDA_DRAW_PANEL* Panel, wxDC* DC );
static void ShowTargetShapeWhileMovingMouse( EDA_DRAW_PANEL* aPanel,
                                             wxDC*           aDC,
                                             const wxPoint&  aPosition,
                                             bool            aErase );

// Local variables :
static int        MireDefaultSize = Millimeter2iu( 5 );

static PCB_TARGET s_TargetCopy( NULL ); // Used to store "old" values of the current item
                                        // parameters before editing for undo/redo/cancel

/**********************************/
/* class DIALOG_TARGET_PROPERTIES */
/**********************************/

class DIALOG_TARGET_PROPERTIES : public DIALOG_TARGET_PROPERTIES_BASE
{
private:
    PCB_EDIT_FRAME*   m_Parent;
    wxDC*             m_DC;
    PCB_TARGET*       m_Target;

    UNIT_BINDER       m_Size;
    UNIT_BINDER       m_Thickness;

public:
    DIALOG_TARGET_PROPERTIES( PCB_EDIT_FRAME* aParent, PCB_TARGET* aTarget, wxDC* aDC );
    ~DIALOG_TARGET_PROPERTIES() { }

private:
    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;
};


void PCB_EDIT_FRAME::ShowTargetOptionsDialog( PCB_TARGET* aTarget, wxDC* DC )
{
    DIALOG_TARGET_PROPERTIES dialog( this, aTarget, DC );

    dialog.ShowModal();
}


DIALOG_TARGET_PROPERTIES::DIALOG_TARGET_PROPERTIES( PCB_EDIT_FRAME* aParent, PCB_TARGET* aTarget,
                                                    wxDC* aDC ) :
    DIALOG_TARGET_PROPERTIES_BASE( aParent ),
    m_Parent( aParent ),
    m_DC( aDC ),
    m_Target( aTarget ),
    m_Size( aParent, m_sizeLabel, m_sizeCtrl, m_sizeUnits, true ),
    m_Thickness( aParent, m_thicknessLabel, m_thicknessCtrl, m_thicknessUnits, true )
{
    m_sdbSizerButtsOK->SetDefault();

    SetInitialFocus( m_sizeCtrl );

    // Now all widgets have the size fixed, call FinishDialogSettings
    FinishDialogSettings();
}


bool DIALOG_TARGET_PROPERTIES::TransferDataToWindow()
{
    m_Size.SetValue( m_Target->GetSize() );
    m_Thickness.SetValue( m_Target->GetWidth() );

    m_TargetShape->SetSelection( m_Target->GetShape() ? 1 : 0 );

    return true;
}


bool DIALOG_TARGET_PROPERTIES::TransferDataFromWindow()
{
    // Zero-size targets are hard to see/select.
    if( !m_Size.Validate( Mils2iu( 1 ), INT_MAX ) )
        return false;

    BOARD_COMMIT commit( m_Parent );
    commit.Modify( m_Target );

    if( m_DC )
        m_Target->Draw( m_Parent->GetCanvas(), m_DC, GR_XOR );

    // Save old item in undo list, if is is not currently edited (will be later if so)
    bool pushCommit = ( m_Target->GetFlags() == 0 );

    if( m_Target->GetFlags() != 0 )         // other edit in progress (MOVE, NEW ..)
        m_Target->SetFlags( IN_EDIT );      // set flag in edit to force
    // undo/redo/abort proper operation

    m_Target->SetWidth( m_Thickness.GetValue() );
    m_Target->SetSize( m_Size.GetValue() );
    m_Target->SetShape( m_TargetShape->GetSelection() ? 1 : 0 );

    if( m_DC )
        m_Target->Draw( m_Parent->GetCanvas(), m_DC, ( m_Target->IsMoving() ) ? GR_XOR : GR_OR );

    if( pushCommit )
        commit.Push( _( "Modified alignment target" ) );

    return true;
}


void PCB_EDIT_FRAME::DeleteTarget( PCB_TARGET* aTarget, wxDC* DC )
{
    if( aTarget == NULL )
        return;

    aTarget->Draw( m_canvas, DC, GR_XOR );
    SaveCopyInUndoList( aTarget, UR_DELETED );
    aTarget->UnLink();
}


static void AbortMoveAndEditTarget( EDA_DRAW_PANEL* Panel, wxDC* DC )
{
    BASE_SCREEN* screen  = Panel->GetScreen();
    PCB_TARGET*  target = (PCB_TARGET*) screen->GetCurItem();

    ( (PCB_EDIT_FRAME*) Panel->GetParent() )->SetCurItem( NULL );

    Panel->SetMouseCapture( NULL, NULL );

    if( target == NULL )
        return;

    target->Draw( Panel, DC, GR_XOR );

    if( target->IsNew() )     // If it is new, delete it
    {
        target->Draw( Panel, DC, GR_XOR );
        target->DeleteStructure();
        target = NULL;
    }
    else    // it is an existing item: retrieve initial values of parameters
    {
        if( ( target->GetFlags() & (IN_EDIT | IS_MOVED) ) )
        {
            target->SetPosition( s_TargetCopy.GetPosition() );
            target->SetWidth( s_TargetCopy.GetWidth() );
            target->SetSize( s_TargetCopy.GetSize() );
            target->SetShape( s_TargetCopy.GetShape() );
        }

        target->ClearFlags();
        target->Draw( Panel, DC, GR_OR );
    }
}


PCB_TARGET* PCB_EDIT_FRAME::CreateTarget( wxDC* DC )
{
    PCB_TARGET* target = new PCB_TARGET( GetBoard() );

    target->SetFlags( IS_NEW );

    GetBoard()->Add( target );

    target->SetLayer( Edge_Cuts );
    target->SetWidth( GetDesignSettings().GetLineThickness( Edge_Cuts ) );
    target->SetSize( MireDefaultSize );
    target->SetPosition( GetCrossHairPosition() );

    PlaceTarget( target, DC );

    return target;
}


void PCB_EDIT_FRAME::BeginMoveTarget( PCB_TARGET* aTarget, wxDC* DC )
{
    if( aTarget == NULL )
        return;

    s_TargetCopy      = *aTarget;
    aTarget->SetFlags( IS_MOVED );
    m_canvas->SetMouseCapture( ShowTargetShapeWhileMovingMouse, AbortMoveAndEditTarget );
    SetCurItem( aTarget );
}


void PCB_EDIT_FRAME::PlaceTarget( PCB_TARGET* aTarget, wxDC* DC )
{
    if( aTarget == NULL )
        return;

    aTarget->Draw( m_canvas, DC, GR_OR );
    m_canvas->SetMouseCapture( NULL, NULL );
    SetCurItem( NULL );
    OnModify();

    if( aTarget->IsNew() )
    {
        SaveCopyInUndoList( aTarget, UR_NEW );
        aTarget->ClearFlags();
        return;
    }

    if( aTarget->GetFlags() == IS_MOVED )
    {
        SaveCopyInUndoList( aTarget, UR_MOVED,
                            aTarget->GetPosition() - s_TargetCopy.GetPosition() );
        aTarget->ClearFlags();
        return;
    }

    if( (aTarget->GetFlags() & IN_EDIT) )
    {
        aTarget->SwapData( &s_TargetCopy );
        SaveCopyInUndoList( aTarget, UR_CHANGED );
        aTarget->SwapData( &s_TargetCopy );
    }

    aTarget->ClearFlags();
}


// Redraw the contour of the track while moving the mouse
static void ShowTargetShapeWhileMovingMouse( EDA_DRAW_PANEL* aPanel, wxDC* aDC,
                                             const wxPoint& aPosition, bool aErase )
{
    BASE_SCREEN* screen  = aPanel->GetScreen();
    PCB_TARGET*  target = (PCB_TARGET*) screen->GetCurItem();

    if( target == NULL )
        return;

    if( aErase )
        target->Draw( aPanel, aDC, GR_XOR );

    target->SetPosition( aPanel->GetParent()->GetCrossHairPosition() );

    target->Draw( aPanel, aDC, GR_XOR );
}
