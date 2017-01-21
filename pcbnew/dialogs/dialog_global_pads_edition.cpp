/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 KiCad Developers, see AUTHORS.txt for contributors.
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


#include <dialog_global_pads_edition.h>

#include <class_pad.h>
#include <wxPcbStruct.h>


DIALOG_GLOBAL_PADS_EDITION::DIALOG_GLOBAL_PADS_EDITION( PCB_BASE_FRAME* aParent, D_PAD* aPad ) :
    DIALOG_GLOBAL_PADS_EDITION_BASE( aParent )
{
    m_parent     = aParent;
    m_currentPad = aPad;

    // Pad filter selection.
    m_Pad_Shape_Filter_CB->SetValue( m_Pad_Shape_Filter );
    m_Pad_Layer_Filter_CB->SetValue( m_Pad_Layer_Filter );
    m_Pad_Orient_Filter_CB->SetValue( m_Pad_Orient_Filter );

    SetFocus();

    GetSizer()->Fit( this );
    Centre();
}


// Class DIALOG_GLOBAL_PADS_EDITION static variables
bool DIALOG_GLOBAL_PADS_EDITION::m_Pad_Shape_Filter  = true;
bool DIALOG_GLOBAL_PADS_EDITION::m_Pad_Layer_Filter  = true;
bool DIALOG_GLOBAL_PADS_EDITION::m_Pad_Orient_Filter = true;


void DIALOG_GLOBAL_PADS_EDITION::OnCancelClick( wxCommandEvent& event )
{
    EndModal( -1 );
}


/* Calls the Pad editor.
 */
void DIALOG_GLOBAL_PADS_EDITION::InstallPadEditor( wxCommandEvent& event )
{
    m_parent->InstallPadOptionsFrame( m_currentPad );
}


/* Update the parameters for the component being edited.
 */
void DIALOG_GLOBAL_PADS_EDITION::PadPropertiesAccept( wxCommandEvent& event )
{
    int returncode = 0;

    switch( event.GetId() )
    {
    case ID_CHANGE_ID_MODULES:
        returncode = 1;

    // Fall through

    case ID_CHANGE_CURRENT_MODULE:
        m_Pad_Shape_Filter  = m_Pad_Shape_Filter_CB->GetValue();
        m_Pad_Layer_Filter  = m_Pad_Layer_Filter_CB->GetValue();
        m_Pad_Orient_Filter = m_Pad_Orient_Filter_CB->GetValue();
        EndModal( returncode );
        break;
    }

    m_parent->OnModify();
}
