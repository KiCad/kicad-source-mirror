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
#include <wxPcbStruct.h>
#include <dialog_helpers.h>
#include <base_units.h>
#include <gr_basic.h>

#include <class_board.h>
#include <class_mire.h>

#include <pcbnew.h>
#include <dialog_target_properties_base.h>


// Routines Locales
static void AbortMoveAndEditTarget( EDA_DRAW_PANEL* Panel, wxDC* DC );
static void ShowTargetShapeWhileMovingMouse( EDA_DRAW_PANEL* aPanel,
                                             wxDC*           aDC,
                                             const wxPoint&  aPosition,
                                             bool            aErase );

// Local variables :
static int        MireDefaultSize = Millimeter2iu( 5 );

static PCB_TARGET s_TargetCopy( NULL ); /* Used to store "old" values of the
                                         * current item parameters before
                                         * edition (used in undo/redo or
                                         * cancel operations)
                                         */

/*****************************************/
/* class TARGET_PROPERTIES_DIALOG_EDITOR */
/*****************************************/

class TARGET_PROPERTIES_DIALOG_EDITOR : public TARGET_PROPERTIES_DIALOG_EDITOR_BASE
{
private:
    PCB_EDIT_FRAME*   m_Parent;
    wxDC*             m_DC;
    PCB_TARGET*       m_Target;

public:
    TARGET_PROPERTIES_DIALOG_EDITOR( PCB_EDIT_FRAME* parent, PCB_TARGET* Mire, wxDC* DC );
    ~TARGET_PROPERTIES_DIALOG_EDITOR() { }

private:
    void OnOkClick( wxCommandEvent& event );
    void OnCancelClick( wxCommandEvent& event );
};


void PCB_EDIT_FRAME::ShowTargetOptionsDialog( PCB_TARGET* aTarget, wxDC* DC )
{
    TARGET_PROPERTIES_DIALOG_EDITOR* frame =
        new TARGET_PROPERTIES_DIALOG_EDITOR( this, aTarget, DC );

    frame->ShowModal();
    frame->Destroy();
}


TARGET_PROPERTIES_DIALOG_EDITOR::TARGET_PROPERTIES_DIALOG_EDITOR( PCB_EDIT_FRAME* parent,
                                                                  PCB_TARGET* aTarget, wxDC* DC ) :
    TARGET_PROPERTIES_DIALOG_EDITOR_BASE( parent )
{
    m_Parent = parent;
    m_DC     = DC;
    m_Target = aTarget;

    // Size:
    m_staticTextSizeUnits->SetLabel( GetUnitsLabel( g_UserUnit ) );
    m_TargetSizeCtrl->SetValue( StringFromValue( g_UserUnit, m_Target->GetSize() ) );

    // Thickness:
    m_staticTextThicknessUnits->SetLabel( GetUnitsLabel( g_UserUnit ) );
    m_TargetThicknessCtrl->SetValue( StringFromValue( g_UserUnit, m_Target->GetWidth() ) );

    // Shape
    m_TargetShape->SetSelection( m_Target->GetShape() ? 1 : 0 );

    // OK button on return key.
    SetDefaultItem( m_sdbSizerButtsOK );

    GetSizer()->Fit( this );
    GetSizer()->SetSizeHints( this );
    Centre();
}


void TARGET_PROPERTIES_DIALOG_EDITOR::OnCancelClick( wxCommandEvent& event )
{
    EndModal( -1 );
}


/* Updates the different parameters for the component being edited
 */
void TARGET_PROPERTIES_DIALOG_EDITOR::OnOkClick( wxCommandEvent& event )
{
    if( m_DC )
        m_Target->Draw( m_Parent->GetCanvas(), m_DC, GR_XOR );

    // Save old item in undo list, if is is not currently edited (will be later if so)
    if( m_Target->GetFlags() == 0 )
        m_Parent->SaveCopyInUndoList( m_Target, UR_CHANGED );

    if( m_Target->GetFlags() != 0 )         // other edition in progress (MOVE, NEW ..)
        m_Target->SetFlags( IN_EDIT );      // set flag in edit to force
                                            // undo/redo/abort proper operation

    int tmp = ValueFromString( g_UserUnit, m_TargetThicknessCtrl->GetValue() );
    m_Target->SetWidth( tmp );

    MireDefaultSize = ValueFromString( g_UserUnit, m_TargetSizeCtrl->GetValue() );
    m_Target->SetSize( MireDefaultSize );

    m_Target->SetShape( m_TargetShape->GetSelection() ? 1 : 0 );

    if( m_DC )
        m_Target->Draw( m_Parent->GetCanvas(), m_DC, ( m_Target->IsMoving() ) ? GR_XOR : GR_OR );

    m_Parent->OnModify();
    EndModal( 1 );
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
    target->SetWidth( GetDesignSettings().m_EdgeSegmentWidth );
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
