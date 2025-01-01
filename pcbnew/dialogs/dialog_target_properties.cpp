/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <pcb_edit_frame.h>
#include <base_units.h>
#include <board_commit.h>
#include <pcb_target.h>
#include <dialog_target_properties_base.h>
#include <widgets/unit_binder.h>


class DIALOG_TARGET_PROPERTIES : public DIALOG_TARGET_PROPERTIES_BASE
{
private:
    PCB_EDIT_FRAME*   m_Parent;
    PCB_TARGET*       m_Target;

    UNIT_BINDER       m_Size;
    UNIT_BINDER       m_Thickness;

public:
    DIALOG_TARGET_PROPERTIES( PCB_EDIT_FRAME* aParent, PCB_TARGET* aTarget );
    ~DIALOG_TARGET_PROPERTIES() { }

private:
    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;
};


void PCB_EDIT_FRAME::ShowTargetOptionsDialog( PCB_TARGET* aTarget )
{
    DIALOG_TARGET_PROPERTIES dialog( this, aTarget );

    dialog.ShowModal();
}


DIALOG_TARGET_PROPERTIES::DIALOG_TARGET_PROPERTIES( PCB_EDIT_FRAME* aParent, PCB_TARGET* aTarget ) :
        DIALOG_TARGET_PROPERTIES_BASE( aParent ),
        m_Parent( aParent ),
        m_Target( aTarget ),
        m_Size( aParent, m_sizeLabel, m_sizeCtrl, m_sizeUnits ),
        m_Thickness( aParent, m_thicknessLabel, m_thicknessCtrl, m_thicknessUnits )
{
    SetupStandardButtons();

    SetInitialFocus( m_sizeCtrl );

    // Now all widgets have the size fixed, call FinishDialogSettings
    finishDialogSettings();
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
    if( !m_Size.Validate( EDA_UNIT_UTILS::Mils2IU( pcbIUScale, 1 ), INT_MAX ) )
        return false;

    BOARD_COMMIT commit( m_Parent );
    commit.Modify( m_Target );

    // Save old item in undo list, if it's not currently edited (will be later if so)
    bool pushCommit = ( m_Target->GetEditFlags() == 0 );

    if( m_Target->GetEditFlags() != 0 )         // other edit in progress (MOVE, NEW ..)
        m_Target->SetFlags( IN_EDIT );          //   set flag IN_EDIT to force
                                                //   undo/redo/abort proper operation

    m_Target->SetWidth( m_Thickness.GetIntValue() );
    m_Target->SetSize( m_Size.GetIntValue() );
    m_Target->SetShape( m_TargetShape->GetSelection() ? 1 : 0 );

    if( pushCommit )
        commit.Push( _( "Edit Alignment Target" ) );

    return true;
}


